# Swink(OS)
(rhymes with Kinkos)

Swink is an experimental operating system which explores novel (ab)uses of architectural features to enable better performance and security.
Although I always strive for production quality code, this is chiefly a hobby/research kernel and is certainly not a production ready.

### Features
- [x] Basic serial I/O
- [x] Exceptions/interrupts
- [x] Power management
- [ ] Virtual memory
- [ ] Fast memory protection management through [Lifeguard](/planning/lifeguard.md)
- [ ] Page table and system configuration protection under kernel ROP through [System Integrity Guard](/planning/system_integrity_guard.md)
- [ ] Kernel threads
- [ ] User processes
- [ ] Fast user space context switching through [Lifeguard](/planning/lifeguard.md)
- [ ] Multi-core support

### Hardware support
Due to the diversity of ARM CPUs and the load-bearing uses of certain architectural features, Swink *requires* a CPU which supports ARMv8A (or newer).
This is due to a combination of Swink being a 64-bit only OS and because Swink requires certain debug hardware like watchpoints for normal execution.
These CPUs, however, are not exceptionally difficult to acquire and are found in many popular single board computers.
I have tested and developed drivers for Swink on the following platforms:

#### Raspberry Pi 3/Zero 2W (BCM2837)
##### Supported drivers
* Mini UART
* Power management 
