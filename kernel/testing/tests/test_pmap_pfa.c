#include "test_utils.h"
#include "machine/pmap/pmap_pfa.h"
#include "lib/list.h"

extern void pmap_pfa_dump_state(void);
extern void pmap_pfa_get_state(size_t *level_buffer, size_t count);

#define BUDDY_LEVELS (6)
static size_t pfa_original_state[BUDDY_LEVELS];
static pmap_page_metadata_s pfa_metadata_m;


static int setup(void) {
    /* 
    Capture the initial PFA state so that we can check that we got back to where
    we expect later
    */
    pmap_pfa_get_state(pfa_original_state, COUNT_OF(pfa_original_state));

    memset(&pfa_metadata_m, 0x00, sizeof(pfa_metadata_m));
    pfa_metadata_m.page_type = PMAP_PAGE_TYPE_KERNEL_DATA;

    return 0;
}

static int simple_sweep(void) {
    size_t temp_state[BUDDY_LEVELS];
    phys_addr_t addr;

    for (unsigned int pg_count = 1; pg_count < 32; pg_count++) {
        addr = pmap_pfa_alloc_contig(PAGE_SIZE * pg_count, &pfa_metadata_m);
        pmap_pfa_free_contig(addr, PAGE_SIZE * pg_count);

        pmap_pfa_get_state(temp_state, COUNT_OF(temp_state));
        if (memcmp(pfa_original_state, temp_state, sizeof(temp_state))) {
            pmap_pfa_dump_state();
            return -1;
        }
    }
    return 0;
}

static int multi_sweep(void) {
    size_t temp_state[BUDDY_LEVELS];
    phys_addr_t addrs[32];

    for (unsigned int pg_count = 1; pg_count < 32; pg_count++) {
        addrs[pg_count - 1] = 
            pmap_pfa_alloc_contig(PAGE_SIZE * pg_count, &pfa_metadata_m);
    }

    for (unsigned int pg_count = 31; pg_count >= 1; pg_count--) {
        pmap_pfa_free_contig(addrs[pg_count - 1], PAGE_SIZE * pg_count);
    }

    for (unsigned int pg_count = 1; pg_count < 32; pg_count++) {
        addrs[pg_count - 1] = 
            pmap_pfa_alloc_contig(PAGE_SIZE * pg_count, &pfa_metadata_m);
    }

    for (unsigned int pg_count = 1; pg_count < 32; pg_count++) {
        pmap_pfa_free_contig(addrs[pg_count - 1], PAGE_SIZE * pg_count);
    }

    for (unsigned int pg_count = 31; pg_count >= 1; pg_count--) {
        addrs[pg_count - 1] = 
            pmap_pfa_alloc_contig(PAGE_SIZE * pg_count, &pfa_metadata_m);
    }

    for (unsigned int pg_count = 1; pg_count < 32; pg_count++) {
        pmap_pfa_free_contig(addrs[pg_count - 1], PAGE_SIZE * pg_count);
    }


    pmap_pfa_get_state(temp_state, COUNT_OF(temp_state));
    if (memcmp(pfa_original_state, temp_state, sizeof(temp_state))) {
        pmap_pfa_dump_state();
        return -1;
    }

    return 0;
}

#define OOM_SWEEP_MAGIC (0xAAAAAAAACAFECAFE)
typedef struct oom_sweep_page {
    uint64_t magic;
    struct list_elem elem;
} * oom_sweep_page_t;

static int oom_sweep(void) {
    static int oom_sweep_run_cnt = 0;

    /*
    Tests:
    1. That the allocator never allocates the same page twice (we place a magic
    value on the page which we assume will never show up randomly on a dealloc'd
    page, which we diversify by oom_sweep_run_cnt
    2. That we can allocate all memory and later free it correctly
    */
    phys_addr_t addr = PHYS_ADDR_INVALID;
    size_t temp_state[BUDDY_LEVELS];
    struct list l;
   
    list_init(&l);
    oom_sweep_run_cnt += 1;

    while ((addr = pmap_pfa_alloc_contig(PAGE_SIZE, &pfa_metadata_m)) 
            != PHYS_ADDR_INVALID) {
        oom_sweep_page_t osp = (oom_sweep_page_t)pmap_pa_to_kva(addr);
        if (osp->magic == OOM_SWEEP_MAGIC + oom_sweep_run_cnt) {
            /* We got the same page back?? */
            return -1;
        }
        osp->magic = OOM_SWEEP_MAGIC + oom_sweep_run_cnt;
        list_push_front(&l, &osp->elem);
    }

    /* We've allocated all memory, now let's free it! */
    for (struct list_elem *e = list_begin(&l);  e != list_end (&l);) {
        /* Perform list ops before freeing the page to avoid UaF */
        struct list_elem *e_next = list_next(e);
        list_remove(e);

        oom_sweep_page_t osp = list_entry(e, struct oom_sweep_page, elem);
        addr = pmap_physmap_kva_to_pa((vm_addr_t)osp);
        pmap_pfa_free_contig(addr, PAGE_SIZE);

        e = e_next;
    }

    pmap_pfa_get_state(temp_state, COUNT_OF(temp_state));
    if (memcmp(pfa_original_state, temp_state, sizeof(temp_state))) {
        pmap_pfa_dump_state();
        return -1;
    }

   return 0;
}

static struct test_case cases[] = {
    TEST_CASE(simple_sweep),
    TEST_CASE(multi_sweep),
    TEST_CASE(oom_sweep),
};

struct test_suite test_pmap_pfa = {
    .name = "pmap_pfa",
    .setup_function = setup,
    .teardown_function = NULL,
    .cases = cases,
    .cases_count = COUNT_OF(cases)
};