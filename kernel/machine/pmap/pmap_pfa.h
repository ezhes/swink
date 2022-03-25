#ifndef PMAP_PFA_H
#define PMAP_PFA_H
#include "lib/types.h"
#include "pmap.h"

/** Represents a page number */
typedef uint32_t page_id_t;

typedef enum pmap_page_type {
    /*
    Note: There is no type for _FREE. The PFA is the sole source of truth for
    physical frame free state.
    */
    PMAP_PAGE_TYPE_KERNEL_DATA      = 0x00,
    PMAP_PAGE_TYPE_KERNEL_TEXT      = 0x01,
    PMAP_PAGE_TYPE_PAGE_TABLE       = 0x02,
} pmap_page_type_e;

typedef struct pmap_page_metadata {
    /** 
     * The use/state of this page. Internally typed as pmap_page_type_e
     * If page_type = PMAP_PAGE_TYPE_FREE, all other metadata is considered 
     * unpredictable.
     */
    unsigned char page_type     : 2;

    /** reserved bits */
    unsigned char padding       : 6;
} pmap_page_metadata_s;

/**
 * Initialize the page-frame allocator with a managed range of [ram_base, 
 * ram_base + ram_size). 
 * Pre-allocates all memory in the range [0, bootstrap_pa_reserved) as wired,
 * kernel data unless it is contained in the range [kernel_text_base, 
 * kernel_text_base + kernel_text_size), in which case it is marked as kernel
 * text.
 */
void
pmap_pfa_init(phys_addr_t ram_base, 
              phys_addr_t ram_size,
              phys_addr_t kernel_text_base,
              phys_addr_t kernel_text_size,
              phys_addr_t kernel_data_base,
              phys_addr_t kernel_data_size,
              phys_addr_t bootstrap_pa_reserved);

/**
 * Requests SIZE bytes of contiguous, aligned memory from the allocator. 
 * If an allocation can be made, METADATA is applied to those pages and the
 * address at the start of the allocation is returned.
 * If no such allocation can be made, returns PHYS_ADDR_INVALID
 * 
 * If the allocation is small (SIZE < (PAGE_SIZE << (BUDDY_LEVELS - 1))), this
 * function is constant time. If the allocation is large, this function may take
 * linear time with respects to the size of system memory.
 */
phys_addr_t
pmap_pfa_alloc_contig(size_t size, pmap_page_metadata_s *metadata);

/**
 * Frees physical pages starting at ADDR and ranging to ADDR + SIZE
 */
void
pmap_pfa_free_contig(phys_addr_t addr, size_t size);

/**
 * Get the metadata for a single page
 */
void
pmap_pfa_mds_get_metadata(page_id_t page, pmap_page_metadata_s *metadata);


/**
 * Checks that all pages in range [page, page + count) are of type TYPE
 * If the check fails, the a panic is triggered.
 */
void
pmap_pfa_mds_require_range_type(page_id_t page, page_id_t count,
                                pmap_page_type_e type);

#endif /* PMAP_PFA_H */
