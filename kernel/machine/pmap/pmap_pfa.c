#include "pmap_pfa.h"
#include "machine/synchronization/synchs.h"
#include "lib/ctype.h"
#include "lib/string.h"
#include "lib/list.h"
#include "lib/stdio.h"

/*
~* PMAP_PFA *~
This file contains the code for two tightly coupled features. Namely, the Page
Frame Allocator (PFA) and the Metadata Store (MDS). Although we'll discuss both 
of these features in detail, both revolve around the general topic of managing
and tracking the allocation of physical memory for the entire system.

** The Page Frame Allocator **
The PFA is the sole kernel agent responsible for managing physical memory
allocations on the system. If you need a DRAM page, you need to ask the PFA to
give you one.

The PFA is implemented as a buddy allocator [1] with `BUDDY_LEVELS` levels. The
PFA has two data structures which work together to make allocations possible.
The buddy_lists field of the PFA holds a series of linked lists of all free
pages on a given level. The buddy_bitmaps field of the PFA holds a series of
bitmaps which indicate the free state of any given managed physical page. These
two structures allow for constant time allocation of any chunk size less than or
equal to PAGE_SIZE<<(BUDDY_LEVELS - 1).

** The Metadata Store **
One of the kernels goals is to provide strong memory corruption. A key part of
achieving this is through detailed accounting of what data is held where so as
to enable software defined memory policies. The MDS is a sidecar datastructure
in the PFA which holds metadata concerning both the use (i.e. is the page being
using for holding program text, page tables, etc.) as well as page state (is the
page currently being paged out?). It is implemented as a large contiguous array
of metadata entries. 

[1] https://en.wikipedia.org/wiki/Buddy_memory_allocation

*/

#define BUDDY_LEVELS    (6)     /* Max level: 4K<<{BUDDY_LEVELS - 1}, or 128K */

#define PFA_LOCK(pfa)   (synchs_lock_acquire(&pfa->lock))
#define PFA_UNLOCK(pfa)   (synchs_lock_release(&pfa->lock))

/**
 * In the buddy_bitmap, indicates that a given page is allocated (not free)
 */
#define BUDDY_BIT_ALLCOATED (0x0)

/**
 * In the buddy_bitmap, indicates a given page is free (ready to allocate)
 */
#define BUDDY_BIT_FREE      (0x1)

struct pmap_pfa {
    struct synchs_lock lock;

    /** 
     * The page ID of the first managed page is the base. All MDS entries
     * are indexed relative to this base at zero
     */
    page_id_t page_base;

    /**
     * The number of pages managed by the PFA and its MDS.
     */
    page_id_t page_count;

    /**
     * The buddy lists for each level. Index 0 holds level 0 which contains 
     * page ranges of size 4K. Each level up doubles the range size.
     */
    struct list buddy_lists[BUDDY_LEVELS];

    /**
     * Sidecar data structure to support the PFA's buddy list data structure.
     * If a page is 1 in this bitmap, it is free.
     */
    uint64_t *buddy_bitmaps[BUDDY_LEVELS];

    /** 
     * Each page has a single metadata struct allocated for it.
     * This data structure, referred to as ``MDS'' is used to enforce page
     * policy.
     */
    struct pmap_page_metadata *metadata;
};

/**
 * The free entry struct is stored at the start of any free series of contiguous
 * pages that are managed by the PFA 
 */
typedef struct pmap_pfa_free_entry {
    /** 
     * The list element for this page. This will be one of the lists in
     * `pfa->buddy_lists`
     */
    struct list_elem elem;
} * pmap_pfa_free_entry_t;

/** The singleton PFA for the kernel */
static struct pmap_pfa *pfa = NULL;

/** Get the number of pages in an entry at a buddy level */
static inline unsigned int
buddy_level_page_count(unsigned int level) {
    return 1 << level;
}

/** Get the page number for a physical address */
static inline page_id_t
pa_to_page_id(phys_addr_t page) {
    return page >> PAGE_SHIFT;
}

/** Get the minimum number of pages to represent SIZE bytes */
static inline page_id_t
size_to_page_count(size_t size) {
    return (size + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
}

/** Get the maximum buddy level a page could exist on by its alignment */
static inline unsigned int
max_buddy_level_for_page_alignment(page_id_t page_id) {
    /*
    The idea here is that we can exploit alignment to calculate the max
    level a page could be a root entry for. This is because root entries are
    aligned to PAGE_SIZE << level_i and thus IDs are aligned to level_i aligned
    */
    unsigned int id_align = __builtin_ctz(page_id);
    if (page_id == 0 || id_align >= BUDDY_LEVELS) {
        return BUDDY_LEVELS - 1;
    }
    return id_align;
}

/** 
 * Get the lowest buddy level that can hold SIZE bytes without leaving any 
 * extra bytes (i.e. for three pages, the max is level 1 (two pages) since level
 * 2 (four pages) has an extra 1 page)
 */
static inline unsigned int
min_buddy_level_for_size_no_overflow(size_t size) {
    unsigned int level = 0;
    size_t value = PAGE_SIZE;

    while (level < BUDDY_LEVELS - 1 && value < size) {
        level++;
        value <<= 1;
    }
    
    if (level > 0 && value > size) {
        /* we overflowed, get rid of the extra level */
        level -= 1;
    }

    return level; 
}
/** Get the lowest buddy level which contains entries of at least size bytes */
static inline unsigned int
min_buddy_level_for_size(size_t size) {
    if (size <= PAGE_SIZE) {
        return 0;
    } else {
        /*
        The nearest level is the nearest _power_ (i.e. exponent) such that
        2^{level} >= size_to_page_count(size).
        Count leading zeros (clzl) is a trick to efficiently get the next power
        */
        /* We need to count the bits, but c sizeof does not go to bits */
        STATIC_ASSERT(sizeof(unsigned long long) == sizeof(uint64_t));
        unsigned int level = 64 - __builtin_clzl(size_to_page_count(size));

        return MIN(BUDDY_LEVELS - 1, level);
    }
}

/**
 * Get the free list entry for a given free page by its page ID
 */
static inline pmap_pfa_free_entry_t
get_pfa_free_entry_for_page(page_id_t page) {
    ASSERT(page - pfa->page_base < pfa->page_count);

    return (pmap_pfa_free_entry_t)(pmap_pa_to_kva(page << PAGE_SHIFT));
}

static inline void
mds_get_metadata_locked(page_id_t page, pmap_page_metadata_s *metadata) {
    ASSERT(page - pfa->page_base < pfa->page_count);
    *metadata = pfa->metadata[page];
}

static inline page_id_t
get_buddy_page_id_for_page(page_id_t page, unsigned int level) {
    page_id_t buddy_id = 0;
    buddy_id = page >> level;

    if (buddy_id % 2 == 0) {
        /* Even buddy IDs are the root, so the buddy of this page is at +1 */
        return page + buddy_level_page_count(level);
    } else {
        /* Odd buddy IDs are the pair entries, so the buddy/root is at -1 */
        return page - buddy_level_page_count(level);
    }
}

static inline page_id_t
get_root_buddy_page_id_for_page(page_id_t page, unsigned int level) {
    return page & ~((1 << (level + 1)) - 1);
}

static inline size_t
buddy_bitmap_required_bytes_for_level(page_id_t page_count, 
                                      unsigned int level_i) {
    /* Each bitmap entry holds 64 bits per uint64_t */
    size_t pages_per_slot = 64 << level_i;
    size_t slots = ROUND_UP(page_count, pages_per_slot) / pages_per_slot;
    return slots * sizeof(uint64_t);
}

static inline size_t
buddy_bitmap_required_bytes(page_id_t page_count) {
    size_t bytes = 0;

    for (unsigned int level_i = 0; level_i < BUDDY_LEVELS; level_i++) {
        bytes += buddy_bitmap_required_bytes_for_level(page_count, level_i);
    }

    return bytes;
}

static inline size_t
buddy_bitmap_page_index(page_id_t page, unsigned int level) {
    page_id_t debased_page = page - pfa->page_base;
    /* 
    Each bin is 64 bits, log2(64) = 6, so to get the bin we shift by 
    6 + level 
    */
    return debased_page >> (6 + level);
}

static inline size_t
buddy_bitmap_page_offset(page_id_t page, unsigned int level) {
    page_id_t debased_page = page - pfa->page_base;
    /*
    Each bin is 64 bits, log2(64) = 6, so get the 6 bit index
    */
    return (debased_page >> level) & ((1 << 6) - 1);
}

static inline void
buddy_bitmap_set_bit_locked(page_id_t page, unsigned int level, uint8_t value) {
    size_t idx = buddy_bitmap_page_index(page, level);
    size_t offset = buddy_bitmap_page_offset(page, level);
    
    uint64_t bin_value = pfa->buddy_bitmaps[level][idx];
    bin_value &= ~(1LLU << offset);
    bin_value |= ((uint64_t)value << offset);
    pfa->buddy_bitmaps[level][idx] = bin_value;
}

static inline uint8_t
buddy_bitmap_get_bit_locked(page_id_t page, unsigned int level) {
    size_t idx = buddy_bitmap_page_index(page, level);
    size_t offset = buddy_bitmap_page_offset(page, level);
    uint64_t bin_value = pfa->buddy_bitmaps[level][idx];

    return (bin_value >> offset) & 1;
}

/** Apply METADATA to all page IDs in range [base, base + page_count) */
static void
apply_metadata_range_locked(page_id_t base, size_t page_count, 
                        pmap_page_metadata_s *metadata) {
    /* When the size is 1, we can just use memset */
    STATIC_ASSERT(sizeof(*metadata) == 1);
    /* Sanity bounds check */
    ASSERT(base - pfa->page_base + page_count <= pfa->page_count);

    uint8_t m8;
    memcpy(&m8, metadata, sizeof(*metadata)); 
    memset(pfa->metadata + base - pfa->page_base, m8, page_count);
}

/**
 * Inserts freed pages in range [BASE, BASE+PAGE_COUNT) into the free lists and
 * buddy bitmaps. Note: this function DOES NOT merge and MUST NOT be used for
 * freeing pages.
 */
static pmap_pfa_free_entry_t
buddy_insert_range_freed_locked(page_id_t base, page_id_t page_count) {
    page_id_t free_i;
    page_id_t limit;
    pmap_pfa_free_entry_t fe = NULL;

    limit = base + page_count;
    for (free_i = base; free_i < limit;) {
        unsigned int level = 0;

        /*
        Calculate the largest valid level for a given page
        We're able to do this in a bit of a faster, hacky way because we know
        that the entire contiguous range is free. Thus, the only two issues we 
        need to consider are 1) what is the alignment of the page and 2) if we
        take the max level'd alignment (i.e. 256K after the given page), is that
        still in bounds?
        In the expression below, we take the largest possible contiguous chunk
        but cap it by the amount of remaining memory so we don't have chunks
        extending out off into memory we don't manage
        */
        level = MIN(
            max_buddy_level_for_page_alignment(free_i),
            min_buddy_level_for_size_no_overflow((limit - free_i) << PAGE_SHIFT)
        );

        ASSERT(level >= 0 && level < BUDDY_LEVELS);
        ASSERT(1 << level <= limit - free_i);
        fe = (void *)pmap_pa_to_kva(free_i << PAGE_SHIFT);
        list_push_front(&pfa->buddy_lists[level], &fe->elem);
        buddy_bitmap_set_bit_locked(free_i, level, BUDDY_BIT_FREE);

        free_i += buddy_level_page_count(level);
    }
    return fe;
}

void
pmap_pfa_init(phys_addr_t ram_base, 
              phys_addr_t ram_size,
              phys_addr_t kernel_text_base,
              phys_addr_t kernel_text_size,
              phys_addr_t kernel_data_base,
              phys_addr_t kernel_data_size,
              phys_addr_t bootstrap_pa_reserved) {
    page_id_t page_base = ram_base >> PAGE_SHIFT;
    page_id_t page_count = size_to_page_count(ram_size - ram_base);
    /* The size of the primary PFA structure, uint64_t aligned */
    size_t pfa_size = ROUND_UP(sizeof(struct pmap_pfa), sizeof(uint64_t));
    /* The size of the buddy bitmap, uint64_t aligned */
    size_t bitmap_size = buddy_bitmap_required_bytes(page_count);
    /* Metadata store structure, byte aligned */
    size_t mds_size = sizeof(struct pmap_page_metadata) * page_count;

    /* Calculate the number of pages for the structure and metadata array */
    size_t required_bytes = ROUND_UP(
        pfa_size + bitmap_size + mds_size,
        PAGE_SIZE
    );

    pfa = (struct pmap_pfa *)(pmap_pa_to_kva(bootstrap_pa_reserved));
    bootstrap_pa_reserved += required_bytes;

    /* Init the PFA */
    synchs_lock_init(&pfa->lock);
    pfa->page_base = page_base;
    pfa->page_count = page_count;

    /* 
    The metadata and bitmap are allocated after the PFA in memory, calculate
    their locations and store them for simplicity
    */  
    pfa->metadata = 
        (struct pmap_page_metadata *)((vm_addr_t)(pfa) + bitmap_size);

    /* 
    We don't init the metadata as there is no "free" state. It is only valid for
    allocated pages
    */

    /* Init buddy infrastructure */
    vm_addr_t bitmap_addr = (vm_addr_t)(pfa) + pfa_size;
    for (unsigned int level_i = 0; level_i < BUDDY_LEVELS; level_i++) {
        list_init(&pfa->buddy_lists[level_i]);
        pfa->buddy_bitmaps[level_i] = (uint64_t *)(bitmap_addr);

        bitmap_addr += buddy_bitmap_required_bytes_for_level(page_count, 
                                                             level_i);
    }

    /* 
    We initially 0 fill the entire bitmap to mark everything as allocated.
    We will later free real free regions. This catches weird edge cases of extra
    slots in our bitmap which map to non-existant pages since we "allocate" them
    now and never make them available.
    */
    STATIC_ASSERT(BUDDY_BIT_ALLCOATED == 0);
    memset(pfa->buddy_bitmaps[0], 0, bitmap_size);

    /* 
    Construct the buddy lists for all unreserved memory
    */
    buddy_insert_range_freed_locked(
        pa_to_page_id(bootstrap_pa_reserved),
        size_to_page_count(ram_size - ram_base - bootstrap_pa_reserved)
    );

    pmap_page_metadata_s m;
    memset(&m, 0x00, sizeof(m));

    /* Apply metadata to reserved data ranges */
    m.page_type = PMAP_PAGE_TYPE_KERNEL_DATA;
    /* reserved data */
    apply_metadata_range_locked(
        /* base page */ ram_base, 
        size_to_page_count(bootstrap_pa_reserved - ram_base), 
        &m
    );

    /* BSS/rodata */
    apply_metadata_range_locked(
        pa_to_page_id(kernel_data_base),
        size_to_page_count(kernel_data_size),
        &m
    );

    /* Apply metadata to kernel text */
    m.page_type = PMAP_PAGE_TYPE_KERNEL_TEXT;
    apply_metadata_range_locked(
        pa_to_page_id(kernel_text_base), 
        size_to_page_count(kernel_text_size), 
        &m
    );

    printf(
        "[*] pmap_pfa: Created PFA (used %zu pages, bitmap=%zu, MDS=%zu)\n", 
        required_bytes >> PAGE_SHIFT, bitmap_size, mds_size
    );

}

/**
 * Attempts to allocate SIZE bytes of contiguous pages.
 * If no valid allocation can be made, returns PHYS_ADDR_INVALID.
 * Returns in constant time wrt size, linear wrt the number of buddy levels.
 * 
 * NOTE: This function CANNOT service requests of 
 * SIZE > PAGE_SIZE << (BUDDY_LEVELS - 1). It is INVALID and UNDEFINED to 
 * TODO: Support larger allocations via another function
 */ 
static phys_addr_t
pmap_pfa_alloc_contig_small_locked(size_t size, 
                                     pmap_page_metadata_s *metadata) {
    phys_addr_t allocated_element = PHYS_ADDR_INVALID;
    unsigned int level_i = 0;
    pmap_pfa_free_entry_t allocated_entry = NULL;
    page_id_t page_count = 0;
    
    page_count = size_to_page_count(size);

    /*
    Since we do not support allocations larger than the max buddy list size, we
    never need to scan for multiple contiguous entries at the same level. Thus,
    the general approach is that we find the minimum level that can satisfy the
    request and begin searching there. If there are no entries there, we jump 
    up a level and continue searching. If an entry that is large enough is 
    found, we check split it down to size and finalize the allocation.
    */

    /* Find a valid, minimal entry */
    for (level_i = min_buddy_level_for_size(size); level_i < BUDDY_LEVELS; 
            level_i++) {
            struct list *buddy_list = NULL;

            buddy_list = pfa->buddy_lists + level_i;
        
            if (list_empty(buddy_list)) {
                /* If the list is empty, it cannot satisfy the request */
                continue;
            }

            /* 
            We found a list with at least one element on it, grab the first free
            chunk off the buddy list.
            */
            allocated_entry = list_entry(
                list_front(buddy_list), 
                struct pmap_pfa_free_entry, 
                elem
            );

            allocated_element = pmap_physmap_kva_to_pa(
                (vm_addr_t)allocated_entry
            );
            break;
    }

    if (!allocated_entry) {
        /* We do not have the requested memory, OOM event */
        return PHYS_ADDR_INVALID;
    }

    /* level_i holds the level we allocated from */

    /* Remove it from whatever free list it's on, allocate it in the bitmap */
    list_remove(&allocated_entry->elem);
    buddy_bitmap_set_bit_locked(
        /* page ID */ pa_to_page_id(allocated_element), 
        /* level   */ level_i, 
        /* bit     */ BUDDY_BIT_ALLCOATED
    );

    /* 
    Free any space of this block that we aren't using 
    Note that we do not need to join blocks as we are freeing a previously
    allocated, full block (and thus if there was anything to join it
    would have already been joined before).
    */
    buddy_insert_range_freed_locked(
        pa_to_page_id(allocated_element) + page_count,
        buddy_level_page_count(level_i) - page_count
    );

    apply_metadata_range_locked(
        /* base */ pa_to_page_id(allocated_element),
        /* page_count */ page_count,
        metadata
    );

    return allocated_element;
}

phys_addr_t
pmap_pfa_alloc_contig(size_t size, pmap_page_metadata_s *metadata) {
    phys_addr_t allocation = PHYS_ADDR_INVALID;
    if (size > (PAGE_SIZE << (BUDDY_LEVELS - 1))) {
        /*
        We don't currently support large allocations, this would require
        scanning the top level buddy and is sort of a pain to implement. 
        Under the decree of "YAGNI", large allocations are not supported yet.
        XXX: This is also necessary (at all levels) to support non-aligned
        but contiguous allocations 
        TODO: Add large allocation support
        */
        return PHYS_ADDR_INVALID;
    }


    PFA_LOCK(pfa);
    allocation = pmap_pfa_alloc_contig_small_locked(size, metadata);
    PFA_UNLOCK(pfa);

    return allocation;
}

void
pmap_pfa_mds_get_metadata(page_id_t page, pmap_page_metadata_s *metadata) {
    PFA_LOCK(pfa);
    mds_get_metadata_locked(page, metadata);
    PFA_UNLOCK(pfa);
}

void
pmap_pfa_mds_require_range_type(page_id_t page, page_id_t count,
                                pmap_page_type_e type) {
    ASSERT(page - pfa->page_base + count < pfa->page_count);

    PFA_LOCK(pfa);
    for (page_id_t i = 0; i < count; i++) {
        pmap_page_metadata_s metadata;
        page_id_t page_i = page - pfa->page_base + i;
        mds_get_metadata_locked(page, &metadata);

        if (metadata.page_type != type) {
            panic(
                "Incorrect page type on page %d (was %d but expected %d)",
                page_i, metadata.page_type, type
            );
        }
    }
    PFA_UNLOCK(pfa);
}

/**
 * Frees pages in range [BASE, BASE+PAGE_COUNT) by both modifying the free list
 * and the buddy bitmaps. This performs all necessary merging.
 */
static void
buddy_free_pages_locked(page_id_t base, page_id_t page_count) {
    page_id_t i = 0;
    for (; i < page_count; i++) {
        pmap_pfa_free_entry_t fe = NULL;
        unsigned int level_i = 0;
        page_id_t page_i = 0;

        page_i = base + i;
        /* merge up, -1 since we never merge on top level */
        for (; level_i < BUDDY_LEVELS - 1; level_i++) {
            page_id_t buddy_i = 0;

            buddy_i = get_buddy_page_id_for_page(page_i, level_i);
            if (buddy_bitmap_get_bit_locked(buddy_i, level_i)
                == BUDDY_BIT_ALLCOATED) {
                /* Our buddy is not free. Our journey ends here. */
                break;
            }
            pmap_pfa_free_entry_t buddy_fe = NULL;
            /* 
            Our buddy is free! 
            Since we have not marked ourselves as free yet, we only need to
            free our buddy before continuing (since we are implicitly free).
            */
            buddy_bitmap_set_bit_locked(buddy_i, level_i, BUDDY_BIT_ALLCOATED);

            buddy_fe = get_pfa_free_entry_for_page(buddy_i);
            list_remove(&buddy_fe->elem);

            /* continue with the root */
            page_i = get_root_buddy_page_id_for_page(page_i, level_i);
        }

        /*
        Finalize our free for this page.
        level_i holds the level we bailed on and is thus where we should insert
        the free record for page_i
        */

        fe = get_pfa_free_entry_for_page(page_i);
        list_push_front(pfa->buddy_lists + level_i, &fe->elem);
        buddy_bitmap_set_bit_locked(page_i, level_i, BUDDY_BIT_FREE);
    }
}

void
pmap_pfa_free_contig(phys_addr_t addr, size_t size) {
    page_id_t page_base = 0;
    page_id_t page_count = 0;

    page_base = pa_to_page_id(addr);
    page_count = size_to_page_count(size);

    PFA_LOCK(pfa);
    
    /* free pages */
    buddy_free_pages_locked(page_base, page_count);

    /* we do not update MDS as the data is invalid when the pages are free */

    PFA_UNLOCK(pfa);
}

#if (CONFIG_DEBUG || CONFIG_TESTING)
void
pmap_pfa_dump_state(void) {
    for (unsigned int level_i = 0; level_i < BUDDY_LEVELS; level_i++) {
        struct list_elem *e = NULL;
        struct list *buddy_list = NULL;
        size_t size = 0;

        buddy_list = pfa->buddy_lists + level_i;
        size = list_size(buddy_list);
        printf(
            "Level %d -- free count = %lu\n",
            level_i, size
        );

        if (size < 100) {
            for (e = list_begin(buddy_list); 
                e != list_end (buddy_list); e = list_next(e)) {
                pmap_pfa_free_entry_t fe = list_entry(e, 
                    struct pmap_pfa_free_entry, elem);
                printf(
                    "\t%d\n", 
                    pa_to_page_id(pmap_physmap_kva_to_pa((phys_addr_t)fe))
                );
            }
        }
        
    }
}

pmap_pfa_free_entry_t
pmap_pfa_contains(unsigned int level, page_id_t page) {
    struct list *buddy_list = NULL;
    buddy_list = pfa->buddy_lists + level;
    for (struct list_elem *e = list_begin(buddy_list);  
            e != list_end (buddy_list); e = list_next(e)) {
        pmap_pfa_free_entry_t fe1 = 
            list_entry(e, struct pmap_pfa_free_entry, elem);
        page_id_t page1 = pa_to_page_id(
            pmap_physmap_kva_to_pa((phys_addr_t)fe1)
        );

        if (page1 == page) {
            return fe1;
        }
    }
    return NULL;
}

void 
pmap_pfa_get_state(size_t *level_buffer, size_t count) {
    REQUIRE(count >= BUDDY_LEVELS);

    PFA_LOCK(pfa);
    for (unsigned int level_i = 0; level_i < BUDDY_LEVELS; level_i++) {
        struct list *buddy_list = NULL;
        buddy_list = pfa->buddy_lists + level_i;
        level_buffer[level_i] = list_size(buddy_list);
    }

    PFA_UNLOCK(pfa);
}

#endif /* CONFIG_DEBUG || CONFIG_TESTING */
