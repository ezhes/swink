#include "synchs.h"

/*
[synchs_semaphore]
Basic spin semaphore supporting V and D operations
*/

void synchs_semaphore_init(struct synchs_semaphore *semaphore, uint32_t value) {
    semaphore->value = value;
}

void synchs_semaphore_down(struct synchs_semaphore *semaphore) {
    while (true) {
        uint32_t new_value;
        uint32_t value;
        
        value =__atomic_load_n(&semaphore->value, __ATOMIC_ACQUIRE);
        if (value == 0) {
            continue;
        }

        new_value = value - 1;

        if (!__atomic_compare_exchange(
            &semaphore->value, 
            &value, 
            &new_value, 
            true /* weak -- we can tolerate spurious failures, & acquire-release */,
            __ATOMIC_RELEASE /* success_memorder */,
            __ATOMIC_ACQUIRE /* failure_memorder */)) {
                /* CAS failed, try again */
                continue;
        }

        /* Successfully downed! */
        break;
    }
}

void synchs_semaphore_up(struct synchs_semaphore *semaphore) {
    __atomic_add_fetch(&semaphore->value, 1, __ATOMIC_RELEASE);
}

/*
[synchs_lock]
Basic spin lock. Implemented as a binary semaphore
*/

void synchs_lock_init(struct synchs_lock *lock) {
    synchs_semaphore_init(&lock->semaphore, 1);
}

void synchs_lock_acquire(struct synchs_lock *lock) {
    synchs_semaphore_down(&lock->semaphore);
}

void synchs_lock_release(struct synchs_lock *lock) {
    synchs_semaphore_up(&lock->semaphore);
}
