# Shadow Memory (SM)
### Concept
We can take advantage of ARM's `LDTR` and `STTR` instructions in conjunction with
conditional watchpoints to create memory that is only accessible in the kernel
via specific instructions.

We do this through the following procedure:

1. The kernel defines some contiguous memory region as a shadow region
2. On kernel entry, a conditional read/write watchpoint is placed over the region 
which traps only on accesses in EL1. 
are not trapped by the watchpoint.
3. On kernel exit, access is flipped so that all accesses trap. User accesses to
the region are treated as segfaults

These two features, when taken together, result in memory that is not accessible
by standard load/store instructions *regardless of EL* and is instead *only*
accessible via LDTR/STTR when executed in EL1.

#### Application

When taken in alongside a feature like [SIG](system_integrity_guard.md), this
allows for the creation of an ultra-low/zero overhead in-kernel enclave. While
this enclave design has a somewhat weak security guarantee (all integrity is lost
upon kernel ROP since an attacker can abuse LDTR/STTR instructions in the kernel
image), it does provide a powerful mitigation against attackers with only kernel
R/W. This weak guarantee, however, is still very useful in a number of ways:

1. On CPUs which do not support ARMv8.3-A's Pointer Authentication feature,
the enclave can be used as a secure shadow stack/indirect pointer cache since
SM provides memory which is secure under kernel R/W. By modifying the compiler
to read/write return addresses from a shadow stack, we can ensure that the stashed
link addresses have not been tampered with. Further, by modifying kernel code to
explicitly locate all indirect instruction pointers in SM, we can ensure that R/W
cannot be used to hijack code execution via other means. A correct and complete
implementation of this feature could be used to offer low-overhead, deterministic
protection against control-flow hijacking without the use of specialized hardware
features.
2. With control-flow integrity (either due to PAC or the aforementioned scheme),
SM enclaves may be used to segment the kernel into a smaller "trusted" codebase 
(i.e. kernel core, sandboxing, privilege/credential management, etc.) from its 
much larger "less trusted" codebase (i.e. random drivers) to prevent bugs in the 
larger blob of code from tampering with more security critical components. This
could then be used to render certain kernel R/Ws useless unless an attacker is
able to discover another exploit to convert their generic R/W into a trusted R/W.
In either case, this enacts a new privilege boundary which raises to the cost of
attack.