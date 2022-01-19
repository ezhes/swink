#include "pmap_buddy_allocator.h"
#include "machine/synchronization/synchs.h"
#include "core/vm/vm_asm.h"
#include "lib/ctype.h"
#include "lib/string.h"
#include "lib/stdio.h"
#include "lib/types.h"

/*
pmap_buddy_allocator.c // January 15, 2022
These routines are responsible for implementing a variant of the buddy allocator

It implements power of two chunks size ranging from 4K to 256K using bitmaps.
*/

#define BUDDY_LEVELS    (4)     /* Max level is 4K^{BUDDY_LEVELS}, or 256K */
#define BITS_PER_BIN    (64)    /* We use 64-bit bins */ 

struct pmap_buddy_allocator {
    struct synchs_lock lock;

    /** 
     * The index offset into `buddy_bitmap` at which the given (zero-indexed)
     * buddy bitmap starts
     */
    uint32_t bitmap_offsets[BUDDY_LEVELS];

    /**
     * The number of bits at each (zero-indexed) buddy level
     */
    uint32_t bitmap_bit_count[BUDDY_LEVELS];

    uint64_t ram_base;
    uint64_t ram_size; 

    /** Bitmap buffer -- levels are defined by bitmap_offsets and  */
    uint64_t bitmap[0];
};

static void
acquire_range(phys_addr_t base, size_t page_count) {

}

static inline uint64_t
get_buddy64(pmap_buddy_allocator_t ba, unsigned int level_i, uint32_t buddy_i) {
    ASSERT(buddy_i < ba->bitmap_bit_count[level_i]);
    return ba->bitmap[ba->bitmap_offsets[level_i] + buddy_i / BITS_PER_BIN];
}

static inline uint64_t
buddy_bin_mask(uint32_t buddy_i) {
    return 1LLU << (buddy_i % BITS_PER_BIN);
}

static inline bool
get_buddy(pmap_buddy_allocator_t ba, unsigned int level_i, uint32_t buddy_i) {
    return get_buddy64(ba, level_i, buddy_i) & buddy_bin_mask(buddy_i);
}

static inline void
set_buddy(pmap_buddy_allocator_t ba, unsigned int level_i, uint32_t buddy_i,
          bool set) {
    if (set) {
        printf("set level %u, buddy %u\n", level_i, buddy_i);
        ba->bitmap[ba->bitmap_offsets[level_i] + buddy_i / BITS_PER_BIN]
            |= buddy_bin_mask(buddy_i);
    } else {
        printf("clr level %u, buddy %u\n", level_i, buddy_i);
        ba->bitmap[ba->bitmap_offsets[level_i] + buddy_i / BITS_PER_BIN]
            &= ~buddy_bin_mask(buddy_i);
    }
}

static void
split_buddy(pmap_buddy_allocator_t ba, unsigned int level_i, uint32_t buddy_i) {
    printf("split level %u, buddy %u\n", level_i, buddy_i);
    ASSERT(level_i != 0);
    ASSERT(get_buddy(ba, level_i, buddy_i));

    set_buddy(ba, level_i, buddy_i, /* set */ false);
    set_buddy(ba, level_i - 1, buddy_i * 2, /* set */ true);
    set_buddy(ba, level_i - 1, buddy_i * 2 + 1, /* set */ true);
}

static void
merge_buddy(pmap_buddy_allocator_t ba, unsigned int level_i, uint32_t buddy_i) {
    ASSERT(level_i != 0);
    ASSERT(!get_buddy(ba, level_i, buddy_i));

    set_buddy(ba, level_i, buddy_i, /* set */ true);
    set_buddy(ba, level_i - 1, buddy_i * 2, /* set */ false);
    set_buddy(ba, level_i - 1, buddy_i * 2 + 1, /* set */ false);
}

static inline uint32_t
buddy_for_phys_addr(unsigned int level_i, phys_addr_t page) {
    return (page >> (PAGE_SHIFT + level_i));
}

static void
reserve_page(pmap_buddy_allocator_t ba, phys_addr_t page) {
    printf("reserving %llx\n", page);
    for (unsigned int level_i = BUDDY_LEVELS - 1; level_i > 0; level_i--) {
        uint32_t buddy_i = buddy_for_phys_addr(level_i, page);
        if (get_buddy(ba, level_i, buddy_i)) {
            split_buddy(ba, level_i, buddy_i);
        }
    }
    
    uint32_t buddy_i = buddy_for_phys_addr(0, page);
    ASSERT(get_buddy(ba, 0 /* level_i */, buddy_i));
    set_buddy(ba, 0 /* level_i */, buddy_i, false /* set */);
}

pmap_buddy_allocator_t 
pmap_buddy_allocator_init(phys_addr_t ram_base, 
                          phys_addr_t ram_size,
                          phys_addr_t bootstrap_pa_reserved) {
    /* Place the buddy allocator at the top of the free physical memory */
    pmap_buddy_allocator_t ba = (void *)pmap_pa_to_kva(bootstrap_pa_reserved);

    synchs_lock_init(&ba->lock);
    ba->ram_base = ram_base;
    ba->ram_size = ram_size;

    uint32_t bitmap_required_bytes = 0;
    for (unsigned int level_i = 0; level_i < BUDDY_LEVELS; level_i++) {
        /* buddy level (zero-index) + 1 is the number of pages in a pairing */
        uint32_t bits_for_level = (ram_size >> (PAGE_SHIFT + (level_i + 1)));
        /* We 8-byte align for ease of bitscan operations */
        uint32_t bytes_for_level = ROUND_UP(bits_for_level, 8) / 8;

        ba->bitmap_offsets[level_i] = bitmap_required_bytes / sizeof(uint64_t);
        ba->bitmap_bit_count[level_i] = bits_for_level;
        /* If we're the top level, mark all as free. Otherwise, mark as used */
        memset(
            ba->bitmap + bitmap_required_bytes / sizeof(uint64_t),
            level_i + 1 == BUDDY_LEVELS ? 0xff : 0x00,
            bits_for_level / 8
        );

        bitmap_required_bytes += bytes_for_level;

        printf("Level %u needs %u bytes\n", level_i, bytes_for_level);
    }

    uint32_t total_required_bytes = ROUND_UP(
        bitmap_required_bytes + sizeof(struct pmap_buddy_allocator),
        PAGE_SIZE
    );

    for (phys_addr_t page_i = 0; 
            page_i < bootstrap_pa_reserved + total_required_bytes;
            page_i += PAGE_SIZE) {
        reserve_page(ba, page_i);
    }



    printf("total bytes = %u\n", total_required_bytes);



    return NULL;
}