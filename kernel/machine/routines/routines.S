.section ".text"
.global _routines_core_idle
_routines_core_idle:
wfe
b _routines_core_idle