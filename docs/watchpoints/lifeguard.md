# Lifeguard
### Motivation
Virtual memory serves a critical role in securing modern computers.
Manipulating VM configuration, however, is very expensive (costing up to and 
including a TLB flush, a `dsb`, and an `isb`), which essentially limits VM to 
only being used for long lasting configurations as attempting to rapidly change
the configuration would incur a massive performance penalty.
Rapid protection changes, however, can be very desirable for improving security
as it allows the implementation of more fine grained, context sensitive 
protection.
Faster memory protection changes could enable the construction of efficient
software enclaves, fast shadow stacks for implementing ROP/JOP protection, as
well as more esoteric features like accelerating user space context switching by
allowing multiple processes to inhabit the same page table and merely changing
the protections such that each process is still isolated from all of its peers.
In this way, accelerating virtual memory permission changes, in any form, is
highly valuable.

### Mechanism
To implement fast memory permission changes, we take advantage of ARMv8A's
implementation of memory watchpoints.
Watchpoints allow us to take a synchronous debug exception whenever issuing a 
load or store (configurable, one or both) against a memory region. 
Notably, watchpoints may be disabled/re-enabled without *any synchronization* 
as writes to `PSTATE.D`, which controls the masking of debug exceptions, is
guaranteed to occur in program order.
Further, a watchpoint may be reconfigured to cover a separate memory range using
only a single `isb` which, while non-free, *pales* in comparison to the 
performance cost of a traditional memory protection change which requires a 
synchronized page table modification, shooting down TLBs across all cores, and a
DSB+ISB pair before the operation may be considered complete.
When taking these factors together, we can thus treat watchpoints as 
configurable memory restrictions which may be used to apply an *additional* 
layer restrictions on top of memory in a way that is free to toggle on/off and 
cheap to fully reconfigure.

Watchpoints, however, were not intended to be used for memory protection nor do
they offer an ideal implementation of such a mechanism.
One critical limitation of watchpoints is that each watchpoint is limited to
covering approximately 2GBs of *contiguous*, aligned virtual memory.
This, when paired with the fact that most production ARMv8 CPUs only have four
watchpoints, means that the maximum theoretical total protection range for 
watchpoints is 8GBs of virtual memory.
Although this is more than enough range for implementing smaller features like
shadow stacks and enclaves, this poses a significant issue for the idea of
co-locating multiple processes in the same page table because not only does it
restrict the total VM space to no more than 8GBs but it in fact forces a *much 
lower cap* as watchpoints may only cover contiguous, size aligned chunks of
memory.

While we can't easily cope with the range limitation, we can somewhat cope with
the requirement of contiguous ranges to enable multiple processes to cohabit a
single page table through a technique I've chosen to name "lifeguarding".
This technique, for `n` watchpoints, assigns `n - 1` watchpoints the task of 
protecting a contiguous, page aligned pool of `2GB * (n - 1)` VA space.
The single remaining watchpoint is referred to as the "guard".
For some global natural number constant `p`, each process which lives on a given
page table is assigned an aligned 2^p byte sized chunk of VA from the pool.
When the kernel wishes to context switch to any given process homed in a page
table, it adjusts both the watchpoint assigned to that section of the pool and 
the guard such that the pool watchpoint covers the lower region up to the 2^p 
byte chunk and the guard watchpoint covers everything above the chunk such that
the entire pool except the desired chunk remains protected by watchpoints.
While adjusting watchpoints typically requires an `isb`, since we are performing
a context switch in the kernel and an `eret` instruction is is implicitly
synchronizing, we can omit the `isb` and perform a full context switch **with no 
performance penalty** beyond the cost of taking an exception into the kernel.

While lifeguaring is an effective and efficient solution which allows multiple
processes to share a page table, it does however still have unavoidable 
restrictions due to the limited size and number of watchpoints.
Since we can only use `n - 1` watchpoints to construct a pool, implementing this
scheme on current hardware limits us to a mere 6GB of VA to share across all
processes in a page table. 
This either forces us to place rather austere memory limitations on the 
processes in these fast context switching tables or it requires that we house
only a handful of processes within a table.
To put some numbers to this challenge, if we use a very restrictive chunk sizes 
of 32MB, we can fit 192 processes in a table. 
If we offer more breathing room and give each process 512MB of VA, we can fit
just 12 processes per table.
Despite this, an operating system can make a number of intelligent choices both 
about chunk size (i.e. using multiple sets of page tables holding multiple 
processes + gang scheduling, migrating processes from smaller chunked page 
tables to large chunks when they use too much memory, making it an opt-in system
where a process can self-restrict in order to gain more favorable scheduler 
priority) and can make optimizations to better use the VA pool (i.e. only place 
data in the pool and treat all executable code as read-only, public data).
With proper workarounds, lifeguard makes fast page table cohabitation a 
practical and highly performant solution to a very common, high overhead problem
that all other existing operating systems face today.

### Notes
* The name of this component is a pun based on pools and guards since we use watchpoints to construct VA pools and then use a floating guard to enable pieces of a pool to be snapped on/off. The (life)guard makes the shared pool safe for everyone :)
* This same mechanism could be used to do low-overhead user-space context switches!
	* We wouldn't have code isolation, but this isn't so much a real issue since code should be considered "non-private" anyways
	* A context switch could be handled in a syscall which spills and reconfigures the watchpoints
	* We'll probably run into trouble with the VA being much smaller than what we can cover with a watchpoint (`1<<31`, or \~2GB each...)
		* Could use it as a "light weight user space kernel"
	* Micro/macro processes
		* Pool-and-guard method allows for 6GB of VA per table (3 pools and 1 floating guard)
		* Each processes is allocated a seed amount of space from the fast context switch pool (say, 32MB for 192 micro processes per table) and if they exceed that limit they are ejected from the fast context switch table and placed into their own table which has a full, standard VA space
			* While this breaks BSS (maybe?), we can keep all executable code out somewhere else and so the VA pools are going to be pure data
		* Scheduler
			* Gets a bit complicated fast. We want to prioritize threads which do not trigger a table swap.
			* A good scheduler could allow us to have multiple fast tables where we only end up paying for contex switches when we cross tables
	* If we don't support migration, can store the lifeguard tables in TTBR1 (which works nicely since we can put SIG there) and then unmap TTBR0 via TCR_EL1 to allow execution of micro processes at any time with zero context switching
		* Maybe make micro-processes an opt in thing, like you accept that you are VM restricted but you get scheduler priority since you are cheaper (might be good for realtime processes? )
		* Can we mess with T0SZ, actually? So that when T0SZ is a specific size the micro processes are in the fault region? or something to that effect
			* If we place them in all TTBR0s near 0x000, we can keep the guard
			on while executing normal processes (prevent others from tampering
			with micro processes) and adjust T0SZ when executing micro processes
			to prevent micros from messing with macros. We can't pull this trick
			with T1SZ since that would move the entire kernel up or down since
			index 0 in TTBR1 always is at the first valid address after T1SZ
			whereas (I think) T0SZ truncates from the end of the table 
* **I don't think we've actually solved the alignment issue -- the guard and the matching pool watchpoint may (?) not always be able to exclude only a specific region**
	* This is kind of a big problem. We don't have base and bound, we only have 
	masking
	* Our best bet is to pivot to drop the lifeguard design (which requires too 
	much dexterity) and instead use all four watchpoints to define a \~8GB
	VA region. Each process gets 1GB of VA space by adjusting which half of the 
	pool is guarded. This works because we can either guard at 2GB with 2GB 
	alignment (cover the entire pool) or 1GB with 1GB alignment (cover either 
	top or bottom of pool).
		* This does somewhat simplify the linkers-and-loaders issue since 1GB
		will let us just load the entire binary in the VA without needing weird
		relocations and all the issues of PC relative access to BSS.