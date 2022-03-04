#include "pmap_pfa.h"
#include "machine/synchronization/synchs.h"
#include "lib/ctype.h"
#include "lib/string.h"
#include "lib/list.h"

#include "lib/stdio.h"

#define BUDDY_LEVELS    (5)     /* Max level is 4K<<{BUDDY_LEVELS}, or 128K */

#define PFA_LOCK(pfa)   (synchs_lock_acquire(&pfa->lock))
#define PFA_UNLOCK(pfa)   (synchs_lock_release(&pfa->lock))

/** Represents a page number */
typedef uint32_t page_id_t;

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
     * Each page has a single metadata struct allocated for it. These are stored
     * contiguously hanging off the end of this struct.
     */
    struct pmap_page_metadata metadata[0];
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
    
    /** 
     * The level this entry corresponds to.
     * This information is used to speed up page free operations since it 
     * allows constant time merging by avoiding the need to traverse all linked
     * lists to determine which free list this page is on.
     */
    unsigned int buddy_level;
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

static inline unsigned int
size_to_page_count(size_t size) {
    return ROUND_UP(size, PAGE_SIZE) >> PAGE_SHIFT;
}

/** Get the lowest buddy level which contains entries of at least size bytes */
static inline unsigned int
min_buddy_level_for_size(size_t size) {
    if (size <= PAGE_SIZE) {
        return 0;
    } else {
        unsigned int ctz = __builtin_ctzl(size_to_page_count(size));
        return MIN(BUDDY_LEVELS - 1, 1 << (1 + ctz));
    }
}

/** Apply METADATA to all page IDs in range [base, base + page_count) */
static void
apply_metadata_range_unlocked(page_id_t base, size_t page_count, 
                        pmap_page_metadata_s *metadata) {
    /* When the size is 1, we can just use memset */
    STATIC_ASSERT(sizeof(*metadata) == 1);

    uint8_t m8;
    memcpy(&m8, metadata, sizeof(*metadata));
    memset(&pfa->metadata + base - pfa->page_base, m8, page_count);
}

void
pmap_pfa_init(phys_addr_t ram_base, 
              phys_addr_t ram_size,
              phys_addr_t kernel_text_base,
              phys_addr_t kernel_text_size,
              phys_addr_t bootstrap_pa_reserved) {
    page_id_t page_base = ram_base >> PAGE_SHIFT;
    page_id_t page_count = ROUND_UP(ram_size - ram_base, PAGE_SIZE) 
                            >> PAGE_SHIFT;
    
    /* Calculate the number of pages for the structure and metadata array */
    size_t required_bytes = ROUND_UP(
        sizeof(struct pmap_pfa) 
                + sizeof(struct pmap_page_metadata) 
                * page_count,
        PAGE_SIZE
    );

    pfa = (struct pmap_pfa *)(pmap_pa_to_kva(bootstrap_pa_reserved));
    bootstrap_pa_reserved += required_bytes;

    /* Init the PFA */
    synchs_lock_init(&pfa->lock);
    pfa->page_base = page_base;
    pfa->page_count = page_count;

    for (unsigned int level_i = 0; level_i < BUDDY_LEVELS; level_i++) {
        list_init(&pfa->buddy_lists[level_i]);
    }

    /* 
    We zero the metadata region, which causes all MDS entries to be marked
    as free initially.
    */
    STATIC_ASSERT(PMAP_PAGE_TYPE_FREE == 0x00);
    memset(&pfa->metadata, 0, sizeof(struct pmap_page_metadata) * page_count);

    /* 
    Construct the buddy lists for all unreserved memory
    */
    phys_addr_t free_i = bootstrap_pa_reserved;
    while (free_i < ram_base + ram_size) {
        page_id_t page_id = pa_to_page_id(free_i);

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
        unsigned level = MIN(
            max_buddy_level_for_page_alignment(page_id),
            min_buddy_level_for_size(ram_base + ram_size - free_i)
        );

        ASSERT(level >= 0 && level < BUDDY_LEVELS);
        pmap_pfa_free_entry_t fe = (void *)pmap_pa_to_kva(free_i);
        fe->buddy_level = level;
        list_push_front(&pfa->buddy_lists[level], &fe->elem);

        free_i += buddy_level_page_count(level) * PAGE_SIZE;
    }

    pmap_page_metadata_s m;
    memset(&m, 0x00, sizeof(m));

    /* Apply metadata to reserved data ranges */
    m.page_type = PMAP_PAGE_TYPE_KERNEL_DATA;
    apply_metadata_range_unlocked(0, kernel_text_base, &m);
    apply_metadata_range_unlocked(
        kernel_text_base + kernel_text_size, 
        bootstrap_pa_reserved - (kernel_text_base + kernel_text_size), 
        &m
    );

    /* Apply metadata to kernel text */
    m.page_type = PMAP_PAGE_TYPE_KERNEL_TEXT;
    apply_metadata_range_unlocked(kernel_text_base, kernel_text_size, &m);

    printf(
        "[*] pmap_pfa: Created PFA (used %zu pages)\n", 
        required_bytes >> PAGE_SHIFT
    );
}

/**
 * Split an entry E into two subelements of half the size of E
 * Returns the second element of the split. The first element will be E.
 * PFA lock must be held.
 */
static pmap_pfa_free_entry_t
free_entry_split(pmap_pfa_free_entry_t e) {
    unsigned int sub_level = 0;
    size_t sub_level_page_count = 0;
    pmap_pfa_free_entry_t new_child = NULL;


    /* cannot split a level zero page, it is an atom */
    ASSERT(e->buddy_level > 0);

    sub_level = e->buddy_level - 1;
    sub_level_page_count = buddy_level_page_count(sub_level);

    /* Remove E from the super list since we're splitting it */
    list_remove(&e->elem);
    
    /* The split entry lives sub_level_page_count pages away from E */
    new_child = (pmap_pfa_free_entry_t)(((uint8_t *)e) 
                    + ((sub_level_page_count * PAGE_SIZE) << PAGE_SHIFT));

    /* update the two children for the lower levels */
    e->buddy_level = sub_level;
    new_child->buddy_level = sub_level;
    
    /* Register the two new pages on the lower level */
    list_push_front(pfa->buddy_lists + sub_level, &e->elem);
    list_push_front(pfa->buddy_lists + sub_level, &new_child->elem);

    return new_child;
}

/**
 * Splits a free entry E until it is the smallest granule that can hold SIZE
 * bytes
 */
static void
free_entry_split_to_size(pmap_pfa_free_entry_t e, size_t size) {
    unsigned int level_i;

    for (level_i = e->buddy_level; 
            level_i > min_buddy_level_for_size(size);
            level_i--) {
        (void)free_entry_split(e);
    }
}
/**
 * Attempts to allocate SIZE bytes of contiguous pages.
 * If no valid allocation can be made, returns PHYS_ADDR_INVALID.
 * Returns in constant time wrt size, linear wrt the number of buddy levels.
 * 
 * NOTE: This function CANNOT service requests of 
 * SIZE > PAGE_SIZE << BUDDY_LEVELS. It is INVALID and UNDEFINED to 
 * TODO: Support larger allocations via another function
 */ 
static phys_addr_t
pmap_pfa_alloc_contig_small_unlocked(size_t size, 
                                     pmap_page_metadata_s *metadata) {
    phys_addr_t allocated_element = PHYS_ADDR_INVALID;
    unsigned int level_i = 0;
    pmap_pfa_free_entry_t allocated_entry = NULL;

    /*
    Since we do not support allocations larger than the max buddy list size, we
    never need to scan for multiple contiguous entries at the same level. Thus,
    the general approach is that we find the minimum level that can satisfy the
    request and begin searching there. If there are no entries there, we jump 
    up a level and continue searching. If an entry that is large enough is 
    found, we check split it down to size and finalize the allocation.
    */

    PFA_LOCK(pfa);

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
            Note: we don't remove it yet as free_entry_split assumes it is on
            its appropriate list.
            */
            allocated_entry = list_entry(
                list_front(buddy_list), 
                struct pmap_pfa_free_entry, 
                elem
            );

            break;
    }

    if (!allocated_entry) {
        /* We do not have the requested memory, OOM event */
        return PHYS_ADDR_INVALID;
    }

    /* Pare our entry down to the minimal size */
    free_entry_split_to_size(allocated_entry, size);

    /* Remove it from whatever free list it's on */
    list_remove(&allocated_entry->elem);

    allocated_element = pmap_physmap_kva_to_pa((vm_addr_t)allocated_entry);

    apply_metadata_range_unlocked(
        /* base */ pa_to_page_id(allocated_element),
        /* page_count */ size_to_page_count(size),
        metadata
    );

    return allocated_element;
}


phys_addr_t
pmap_pfa_alloc_contig(size_t size, pmap_page_metadata_s *metadata) {
    phys_addr_t allocation = PHYS_ADDR_INVALID;
    if (size > (PAGE_SIZE << BUDDY_LEVELS)) {
        /*
        We don't currently support large allocations, this would require
        scanning the top level buddy and is sort of a pain to implement. 
        Under the decree of "YAGNI", large allocations are not supported yet.
        TODO: Add large allocation support
        */
        return PHYS_ADDR_INVALID;
    }


    PFA_LOCK(pfa);
    allocation = pmap_pfa_alloc_contig_small_unlocked(size, metadata);
    PFA_UNLOCK(pfa);

    return allocation;
}

void
pmap_pfa_free(phys_addr_t page) {

}
