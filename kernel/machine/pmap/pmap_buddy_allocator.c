#include "pmap_buddy_allocator.h"
#include "machine/synchronization/synchs.h"
#include "core/vm/vm_asm.h"
#include "lib/ctype.h"
#include "lib/stdio.h"

/*
pmap_buddy_allocator.c // January 15, 2022
These routines are responsible for implementing a variant of the buddy allocator

It implements power of two chunks size ranging from 4K to 256K using bitmaps.
*/

#define BUDDY_LEVELS    (4) /* Max level is 4K^{BUDDY_LEVELS}, or 256K */

struct pmap_buddy_allocator {
    struct synchs_lock lock;

    /** 
     * The byte offset into `buddy_bitmap` at which the given (zero-indexed)
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

static struct pmap_buddy_allocator pb_allocator_s;
#define pb_allocator (&pb_allocator_s)

void pmap_buddy_allocator_init(phys_addr_t ram_base, 
                                phys_addr_t ram_size,
                                phys_addr_t bootstrap_pa_reserved) {
    synchs_lock_init(&pb_allocator->lock);
    pb_allocator->ram_base = ram_base;
    pb_allocator->ram_size = ram_size;

    uint32_t total_required_bytes = 0;
    for (unsigned int level_i = 0; level_i < BUDDY_LEVELS; level_i++) {
        /* buddy level (zero-index) + 1 is the number of pages in a pairing */
        uint32_t bits_for_level = (ram_size >> (PAGE_SHIFT + (level_i + 1)));
        /* We 8-byte align for ease of bitscan operations */
        uint32_t bytes_for_level = ROUND_UP(bits_for_level, 8) / 8;

        pb_allocator->bitmap_offsets[level_i] = total_required_bytes;
        pb_allocator->bitmap_bit_count[level_i] = bits_for_level;
        total_required_bytes += bytes_for_level;

        printf("Level %u needs %u bytes\n", level_i, bytes_for_level);
    }
    printf("total bytes = %u\n", total_required_bytes);
}