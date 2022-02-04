# Paranoia
Conceptually, is it possible to build a (somewhat) functional guarded piece of
software that distrusts memory such that an attacker with arbitrary read/write
(but no code execution) cannot modify any of the secure environment's state?

This is an interesting problem because modern SoCs have an enormous number of
system agents other than the CPU, and so attempting to do any sort of practical
secure computation is nearly impossible without extensive hardware intervention
(such as by putting an IOMMU in front of each agent and meticulously configuring
them such that the CPU has a slice of DRAM all to itself). To solve this without
adding new hardware, we borrow the idea of hardware page encryption and instead
opt to implement a more complex and non-transparent software memory
*authentication* scheme which is able to detect the following changes performed
by any system agent:

1. General corruption (no adversary may create a block of memory with a valid
MAC).
2. Re-ordering resistance (no adversary may swap blocks of existing valid memory
between each other)
3. Rollback resistance (once a block of memory has been written, all previous 
generations of that memory are invalid).

With these three building blocks, data may be dumped to DRAM and validated to
ensure that it is the exact same data that we last put there. If another system
agent attempts to modify it, the manipulation can be detected and the system can
be panic-ed to prevent tainted data from influencing system state.

This can be implemented in a number of ways, however the most attractive 
technique is to make use of ARMv8.3-A's Pointer Authentication Code feature and
its `pacga` instruction which allows for extremely fast, hardware backed
cryptographic data authentication. While this feature can be emulated in
software, doing so severely limits the practical application since many signing
operations take thousands of cycles.

##### Performance hacks
General idea dumping ground:
* We can treat the NEON registers as a 512 byte SRAM block and use it to cache
authenticated data. This will let us "fault in" blocks of data and allow write
back for blocks, which enables much faster accesses to memory.
* Since PAC is only (effectively) an HMAC, we need to implement swapping and
rollback protection in software.
    * The issue of swapping is easily solved by simply appending the memory
    address to the block. This makes the hash dependent on the location and 
    causes the same data to generate a different hash if it is at different
    locations.
    * Rollback is somewhat more difficult. To prevent any single block from
    being rolled back, we effectively need a small secure hash that we can store
    outside of DRAM (i.e. in a register). By constructing a Merkle tree out of
    the block hashes, we can do just this. By choosing a suitable block size, we
    have an n-ary Merkle tree which allows modifications to only require
    rehashing the path up to the root which, while slow, should be at least
    somewhat palatable. By modifying the block address mix in to contain a type
    bit (i.e. low bit is zero if the block is a data block and one if the block
    holds Merkle tree elements) to prevent the second-preimage attack. 