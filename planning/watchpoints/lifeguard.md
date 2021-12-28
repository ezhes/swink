# Lifeguard
The name of this component is a pun based on pools and guards since we use watchpoints to construct VA pools and then use a floating guard to enable pieces of a pool to be snapped on/off. The (life)guard makes the shared pool safe for everyone :)

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
			* Gets a bit complicated fast. We want to prioritze threads which do not trigger a table swap.
			* A good scheduler could allow us to have multiple fast tables where we only end up paying for contex switches when we cross tables