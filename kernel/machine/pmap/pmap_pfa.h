#ifndef PMAP_PFA_H
#define PMAP_PFA_H
#include "lib/types.h"
#include "pmap.h"

typedef enum pmap_page_type {
    PMAP_PAGE_TYPE_FREE             = 0x00,
    PMAP_PAGE_TYPE_DATA             = 0x01,
    PMAP_PAGE_TYPE_TEXT             = 0x02,
} pmap_page_type_e;

typedef struct pmap_page_metadata {
    /** 
     * The use/state of this page. Internally typed as pmap_page_type_e
     * If page_type = PMAP_PAGE_TYPE_FREE, all other metadata is considered 
     * unpredictable.
     */
    unsigned char page_type     : 2;
    
    /** Can this page be swapped if needed? */
    unsigned char is_purgeable  : 1;

    /** reserved bits -- use for pageing later */
    unsigned char padding       : 5;
} pmap_page_metadata_s;


/** Represents a page number */
typedef uint32_t page_id_t;

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


#endif /* PMAP_PFA_H */
