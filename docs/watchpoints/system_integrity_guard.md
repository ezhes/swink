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

### Mechanism
To pull this off in a performant manner on commodity hardware, we take advantage
of the previously discussed [Lifeguard](/docs/watchpoints/lifeguard.md) technique 
to construct a fast, in-kernel data enclave which guards any relevant SIG state,
page tables, and any security critical kernel data structures against
modification except while executing in the SIG.
Since Swink only intends to implement a single enclave in the kernel, disabling
debug exceptions is a suitable alternative to allowing access to the SIG data
enclave (rather than the full lifeguard mechanism as discussed elsewhere).
This allows access to the SIG enclave to be toggled without any debug register
manipulation or `isb` instructions since modifications to `PSTATE.D` occur in
program order without additional synchronization.
This is a very powerful shortcut and it allows extremely efficient protection to
the data that the SIG is intended to protect while still providing strong 
security guarantees.

This shortcut, however, comes with a small wrinkle: namely, debug exceptions
must be very carefully controlled since any place in the kernel where debug
exceptions are masked has full control over the page tables and could be used to 
attack the SIG.
Since there is no reason the kernel would need to manually disable debug 
exceptions as part of its normal operation, we can handily avoid half of the
problem by simply ensuring that the only the SIG entry trampoline contains 
instructions which mask debug exceptions.
The other half of the problem occurs when the hardware disables debug exceptions
for us is, such as whenever an exception is taken.
This is a somewhat more difficult problem to resolve as turning debug exceptions
back on (and thereby re-protecting the SIG) may lead to an exception loop while
delaying too long may leave the SIG vulnerable to being attacked by interrupt
handler code.

To resolve this, we do something a bit odd: we just embrace the fact that
taking an exception rockets us into SIG mode.
This decision is motivated by two key factors.
Firstly, it's simpler!
Securely spilling enough registers so that we can exit the SIG while 
simultaneously ensuring that we don't immediately trigger another debug
exception and loop is complicated.
This complexity can even become a security hazard as it adds an additional, 
somewhat unexpected attack surface to the SIG where an adversary could attempt
to trick the kernel into corrupting SIG data structures as it attempts to exit
during an exception.
By choosing to not do this and accepting that we are running in the SIG, we can
avoid this complexity and the risk it brings.
Secondly, and perhaps most importantly, it turns out that protecting some or all
of the exception spill data is critical to ensuring SIG integrity.
This is because if an adversary is able to tamper with the spill state, they
may be able to compromise the SIG by corrupting the SPSR value (which includes 
the debug exception mask bit!) or by performing a slew of [interrupt storm 
based attacks](https://www.youtube.com/watch?v=7zCBOFxATFs) while a thread is 
executing in the SIG to bypass ROP guards inside SIG code.
Additionally, securing exception spill is a critical part of implementing a 
kernel CFI mitigation using [Shadow](/docs/watchpoints/shadow.md) and so these
changes benefit future Swink platform features.

### Notes
Watchpoints don't impact code execution (i.e. SIG code is always executable in the kernel) but we can do a few things:

1. Unmap bootstrap code (i.e. things that touch `TTBR1_EL1`)
2. Allow slow path map-use-unmap (or just permission swap) to make less used operations usable. This is suitable for high risk (hard to revert without side effects) operations
3. Guard after all other fast-path, low risk operations with a ROP guard which checks that this CPU is in the guard such that even if they manage to change the value of the MSR, we can revert and panic. This can be used for stuff like TTBR0_EL1.

Implementation gotchas:
- [ ] Debug exceptions are disabled on exception to prevent unsafe nesting. We need to somehow spill enough to safely to re-enable debug exceptions before handling the exception 
- [ ] The exception mask can be modified through the exception return handler. We need to ensure that either 1) exception return is fully guarded by SIG or 2) that the exception exit handler never returns without debug exceptions enabled except when returning to SIG (which in effect means storing exception frames dumped while in the SIG back into SIG protected memory so as to ensure SIG state can't be forged) 