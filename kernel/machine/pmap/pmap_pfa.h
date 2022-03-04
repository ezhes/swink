#ifndef PMAP_PFA_H
#define PMAP_PFA_H
#include "lib/types.h"
#include "pmap.h"

typedef enum pmap_page_type {
    PMAP_PAGE_TYPE_FREE             = 0x00,
    PMAP_PAGE_TYPE_KERNEL_DATA      = 0x01,
    PMAP_PAGE_TYPE_KERNEL_TEXT      = 0x02,
    PMAP_PAGE_TYPE_PAGE_TABLE       = 0x03,
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
              phys_addr_t bootstrap_pa_reserved);

/**
 * Request a single page from the PFA and marks it as TYPE in the MDS. If no 
 * page could be allocated, returns PHYS_ADDR_INVALID. 
 */
phys_addr_t
pmap_pfa_alloc(pmap_page_type_e type);

/**
 * Requests SIZE bytes of contiguous, aligned memory from the allocator. 
 * If an allocation can be made, METADATA is applied to those pages and the
 * address at the start of the allocation is returned.
 * If no such allocation can be made, returns PHYS_ADDR_INVALID
 * 
 * If the allocation is small (SIZE < (PAGE_SIZE << BUDDY_LEVELS)), this
 * function is constant time. If the allocation is large, this function may take
 * linear time with respects to the size of system memory.
 */
phys_addr_t
pmap_pfa_alloc_contig(size_t size, pmap_page_metadata_s *metadata);

/**
 * Frees a single page
 */
void
pmap_pfa_free(phys_addr_t page);


#endif /* PMAP_PFA_H */
