# System Integrity Guard (SIG)
SIG is a fast, in-kernel secure execution environment which protects system integrity by ensuring that critical operating system configuration (virtual memory, control registers, etc.) are modified only in accordance with system policy. This guarantee is intended to survive even under kernel ROP/JOP

### Threat Model
1. An attacker CAN read/write arbitrary kernel memory
2. An attacker CAN execute sections of preexisting instructions in any order (i.e. standard ROP/JOP but no ability to single-step instructions)
3. An attacker CANNOT subvert the virtual memory system (i.e. disabling the MMU, changing translation configuration, etc.)
4. An attacker CANNOT arbitrarily modify page tables (either to modify existing mappings or to introduce new executable kernel code)
5. An attacker CANNOT execute arbitrary single CPU instructions in an arbitrary order (i.e. single step ROP)

### Strategy
To pull this off in a performant manner on commodity hardware, we take advantage of ARMv8A's watchpoint feature. Watchpoints allow us to take an asynchronous debug exception whenever issuing a load or store (configurable, one or both!) against a memory region. Notably watchpoints may be disabled/re-enabled without *any synchronization* as writes to `PSTATE.D` occur in program order. This means that we can use watchpoints to rapidly flip access to a memory region on or off without any costly virtual memory operations (such as modifying the page permissions, clearing the TLB, and issuing a DSB+ISB pair) simply be carefully managing debug interrupts.

We use watchpoints to implement a kernel memory region which is accessible only to the SIG subsystem by creating a write watchpoint which covers the memory region. We then ensure that debug exceptions are enabled everywhere in the kernel except for while executing trusted SIG code. This ensure that any writes to the SIG region while we are not in the SIG trigger an exception. Since non-SIG code attempting to write to the SIG constitutes abnormal behavior (and would indicate a possible kernel attack), we may simply panic the system. This scheme can be used to shield critical OS data structures (such as page tables!) from modification while not in the trusted SIG environment, which provides a large part of the SIG's guarantees.

```
This doesn't impact code execution (or doest it?), but we can do some things
1) unmap bootstrap code (i.e. things that touch TTBR1_EL1)
2) allow slow path map-use-unmap (or just permission swap) to make less used operations usable. This is suitable for high risk (hard to revert without side effects) operations
3) guard after all other fast-path, low risk operations with a ROP guard which checks that this CPU is in the guard such that even if they manage to change the value of the MSR, we can revert and panic. This can be used for stuff like TTBR0_EL1.
``` 
