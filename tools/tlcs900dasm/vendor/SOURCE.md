Vendored from MAME (https://github.com/mamedev/mame), BSD-3-Clause,
commit 3e07282c98e918575a1db55e29017ec8e421d166:

- dasm900.cpp, dasm900.h: src/devices/cpu/tlcs900/
  Local patch (marked "local patch"): 8-bit immediate operands (O_I8) are printed as hex;
  upstream passes them through address(), so e.g. `ld (0x6f),0x4e` printed as `ld (WDCR),PG01CR`.
- tmp95c061_syms.inc: SFR/control-register symbol tables from src/devices/cpu/tlcs900/tmp95c061.cpp

Copyright holders: Wilbert Pol, Felipe Sanches and MAME contributors.
