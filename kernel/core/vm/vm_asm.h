#ifndef VM_ASM_H
#define VM_ASM_H

/** We only support 4K pages */
#define PAGE_SHIFT              (12)
#define PAGE_SIZE               (1 << PAGE_SHIFT)
#define PAGE_ENTRY_COUNT        (PAGE_SIZE / 8)
#define VM_KERNEL_BASE_ADDRESS  (0xffffff8000000000)

#endif /* VM_ASM_H */