#ifndef SYNCH_H
#define SYNCH_H
#include "lib/types.h"

/*
synch[s]_xxxx are all [S]pin type syncronization primitives
*/

/** Spin semaphore */
struct synchs_semaphore {
    uint32_t value;
};

/** Initializes a sempahore with a given value `value` */
void synchs_semaphore_init(struct synchs_semaphore *semaphore, uint32_t value);
/** Acquires a slot on a semaphore or spins until it can */
void synchs_semaphore_down(struct synchs_semaphore *semaphore);
/** Releases a slot on a semaphore */
void synchs_semaphore_up(struct synchs_semaphore *semaphore);

/** Spin lock */
struct synchs_lock {
    struct synchs_semaphore semaphore;
};

/** Initializes a lock */
void synchs_lock_init(struct synchs_lock *lock);
/** Acquires a lock or spins until the lock can be acquired */
void synchs_lock_acquire(struct synchs_lock *lock);
/** Releases a lock */
void synchs_lock_release(struct synchs_lock *lock);

#endif /* SYNCH_H */