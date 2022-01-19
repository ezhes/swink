#include "pmap_pfa.h"
#include "machine/synchronization/synchs.h"
#include "lib/ctype.h"
#include "lib/string.h"
#include "lib/list.h"

#include "lib/stdio.h"

#define BUDDY_LEVELS    (4)     /* Max level is 4K^{BUDDY_LEVELS}, or 256K */

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
    
    /** The level this entry corresponds to */
    unsigned int buddy_level;
} * pmap_pfa_free_entry_t;

/** The singleton PFA for the kernel */
static struct pmap_pfa *pfa = NULL;

/** Get the number of pages in an entry at a buddy level */
static inline unsigned int
buddy_level_page_count(unsigned int level) {
    return 1 << level;
}

static inline page_id_t
pa_to_page_id(phys_addr_t page) {
    return page >> PAGE_SHIFT;
}

static inline unsigned int
max_buddy_level_for_page(page_id_t page_id) {
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
min_buddy_level_for_size(size_t size) {
    if (size <= PAGE_SIZE) {
        return 0;
    } else {
        unsigned int ctz = __builtin_ctzl(size);
        return MIN(BUDDY_LEVELS - 1, 1 << (1 + ctz));
    }
}

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
            max_buddy_level_for_page(page_id),
            min_buddy_level_for_size(ram_base + ram_size - free_i)
        );

        ASSERT(level >= 0 && level < BUDDY_LEVELS);
        pmap_pfa_free_entry_t fe = (void *)pmap_pa_to_kva(free_i);
        fe->buddy_level = level;
        list_push_front(&pfa->buddy_lists[level], &fe->elem);

        free_i += buddy_level_page_count(level) * PAGE_SIZE;
    }

    /* Apply metadata to reserved data ranges */
    pmap_page_metadata_s m;
    m.page_type = PMAP_PAGE_TYPE_DATA;
    m.is_purgeable = 0;
    apply_metadata_range_unlocked(0, kernel_text_base, &m);
    apply_metadata_range_unlocked(
        kernel_text_base + kernel_text_size, 
        bootstrap_pa_reserved - (kernel_text_base + kernel_text_size), 
        &m
    );

    /* Apply metadata to kernel text */
    m.page_type = PMAP_PAGE_TYPE_TEXT;
    apply_metadata_range_unlocked(kernel_text_base, kernel_text_size, &m);

    printf(
        "[*] pmap_pfa: Created PFA (used %zu pages)\n", 
        required_bytes >> PAGE_SHIFT
    );
}
