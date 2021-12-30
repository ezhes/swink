# System Integrity Guard (SIG)
SIG is a fast, in-kernel secure execution environment which protects system integrity by ensuring that critical operating system configuration (virtual memory, control registers, etc.) are modified only in accordance with system policy. This guarantee is intended to survive even under kernel ROP/JOP

### Threat Model
1. An attacker CAN read/write arbitrary kernel memory
2. An attacker CAN execute whole blocks of preexisting instructions in any order (i.e. standard ROP/JOP but no ability to single-step instructions)
4. An attacker CANNOT execute arbitrary single CPU instructions in an arbitrary order (i.e. single stepping instructions)
3. An attacker CANNOT subvert the virtual memory system 
    * This is not a desired requirement but due to limitations of commodity ARMv8A SoCs (namely, severely lacking/absolutely zero SMMUs/IOMMUs), we cannot reasonably guard against attacks by other system agents since an adversary could program the DMA engine or any other number of peripherals which have unobstructed access to physical memory (the GPU, on-board radios, other non-AP processors, etc.). On better hardware platforms in which all other system agents sit behind SMMUs, this aspect of the model may be waived by allowing SIG to manage and apply its policy to the SMMUs. A relevant conference talk discussing this issue is [Timothy Roscoe's "It's time for Operating Systems to rediscover Hardware"](https://www.youtube.com/watch?v=36myc8wQhLo) in which he discusses this specific problem and the idea that the application processor is no longer the only, let alone most powerful, agent in a compute package. 

This threat model approximates an adversary which has exploited some kernel vulnerability and has achieved kernel code execution through ROP.
This is very common in real-world attack scenarios in which either a user program exploits the kernel or a remote adversary directly exploits the kernel.

### Guarantees
Under this thread model, SIG guarantees the following:

1. Critical system configuration registers (including but not limited to `TTBRn_EL1`, `SCTLR_EL1`, `SCR_EL1`, debug registers, etc.) cannot be arbitrarily modified 
2. Page tables cannot be modified except through approved, policy enforcing SIG virtual memory management APIs. Key policies that will be enforced by SIG include:
    1. No new kernel code may be mapped
    2. Physical pages containing kernel code may never be re-mapped elsewhere (especially with different permissions), meaning that existing kernel code cannot be modified
3. SIG policy and enforcement rules cannot be modified except through SIG APIs 

### Strategy
To pull this off in a performant manner on commodity hardware, we take advantage of ARMv8A's implementation of memory watchpoints. Watchpoints allow us to take an asynchronous debug exception whenever issuing a load or store (configurable, one or both!) against a memory region. Notably watchpoints may be disabled/re-enabled without *any synchronization* as writes to `PSTATE.D` occur in program order. This means that we can use watchpoints to rapidly flip access to a memory region on or off without any costly virtual memory operations (such as modifying the page permissions, clearing the TLB, and issuing a DSB+ISB pair) simply be carefully managing debug interrupts.

We use watchpoints to implement a kernel memory region which is accessible only to the SIG subsystem by creating a write watchpoint which covers the memory region. We then ensure that debug exceptions are enabled everywhere in the kernel except for while executing trusted SIG code. This ensure that any writes to the SIG region while we are not in the SIG trigger an exception. Since non-SIG code attempting to write to the SIG constitutes abnormal behavior (and would indicate a possible kernel attack), we may simply panic the system. This scheme can be used to shield critical OS data structures (such as page tables!) from modification while not in the trusted SIG environment, which provides a large part of the SIG's guarantees.


### Notes
Watchpoints don't impact code execution (i.e. SIG code is always executable in the kernel) but we can do a few things:

1. Unmap bootstrap code (i.e. things that touch `TTBR1_EL1`)
2. Allow slow path map-use-unmap (or just permission swap) to make less used operations usable. This is suitable for high risk (hard to revert without side effects) operations
3. Guard after all other fast-path, low risk operations with a ROP guard which checks that this CPU is in the guard such that even if they manage to change the value of the MSR, we can revert and panic. This can be used for stuff like TTBR0_EL1.

Implementation gotchas:

- [ ] The exception mask can be modified through the exception return handler. We need to ensure that either 1) exception return is fully guarded by SIG or 2) that the exception exit handler never returns without debug exceptions enabled except when returning to SIG (which in effect means storing exception frames dumped while in the SIG back into SIG protected memory so as to ensure SIG state can't be forged) 