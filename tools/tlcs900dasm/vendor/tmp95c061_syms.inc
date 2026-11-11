// license:BSD-3-Clause
// Extracted from MAME src/devices/cpu/tlcs900/tmp95c061.cpp (commit 3e07282c98e918575a1db55e29017ec8e421d166)
static std::pair<u16, char const *> const tmp95c061_syms[] = {
	{ 0x01, "P1" }, { 0x04, "P1CR" }, { 0x06, "P2" }, { 0x09, "P2FC" },
	{ 0x0d, "P5" }, { 0x10, "P5CR" }, { 0x11, "P5FC" },
	{ 0x12, "P6" }, { 0x13, "P7" }, { 0x15, "P6FC" }, { 0x16, "P7CR" }, { 0x17, "P7FC" },
	{ 0x18, "P8" }, { 0x19, "P9" }, { 0x1a, "P8CR" }, { 0x1b, "P8FC" },
	{ 0x1e, "PA" }, { 0x1f, "PB" },
	{ 0x20, "TRUN" },
	{ 0x22, "TREG0" }, { 0x23, "TREG1" }, { 0x24, "T01MOD" }, { 0x25, "TFFCR" },
	{ 0x26, "TREG2" }, { 0x27, "TREG3" }, { 0x28, "T23MOD" }, { 0x29, "TRDC" },
	{ 0x2c, "PACR" }, { 0x2d, "PAFC" }, { 0x2e, "PBCR" }, { 0x2f, "PBFC" },
	{ 0x30, "TREG4L" }, { 0x31, "TREG4H" }, { 0x32, "TREG5L" }, { 0x33, "TREG5H" },
	{ 0x34, "CAP1L" }, { 0x35, "CAP1H" }, { 0x36, "CAP2L" }, { 0x37, "CAP2H" },
	{ 0x38, "T4MOD" }, { 0x39, "T4FFCR" }, { 0x3a, "T45CR" },
	{ 0x3c, "MSAR0" }, { 0x3d, "MAMR0" }, { 0x3e, "MSAR1" }, { 0x3f, "MAMR1" },
	{ 0x40, "TREG6L" }, { 0x41, "TREG6H" }, { 0x42, "TREG7L" }, { 0x43, "TREG7H" },
	{ 0x44, "CAP3L" }, { 0x45, "CAP3H" }, { 0x46, "CAP4L" }, { 0x47, "CAP4H" },
	{ 0x48, "T5MOD" }, { 0x49, "T5FFCR" },
	{ 0x4c, "PG0REG" }, { 0x4d, "PG1REG" }, { 0x4e, "PG01CR" },
	{ 0x50, "SC0BUF" }, { 0x51, "SC0CR" }, { 0x52, "SC0MOD" }, { 0x53, "BR0CR" },
	{ 0x54, "SC1BUF" }, { 0x55, "SC1CR" }, { 0x56, "SC1MOD" }, { 0x57, "BR1CR" },
	{ 0x58, "ODE" },
	{ 0x5a, "DREFCR" }, { 0x5b, "DMEMCR" },
	{ 0x5c, "MSAR2" }, { 0x5d, "MAMR2" }, { 0x5e, "MSAR3" }, { 0x5f, "MAMR3" },
	{ 0x60, "ADREG0L" }, { 0x61, "ADREG0H" }, { 0x62, "ADREG1L" }, { 0x63, "ADREG1H" },
	{ 0x64, "ADREG2L" }, { 0x65, "ADREG2H" }, { 0x66, "ADREG3L" }, { 0x67, "ADREG3H" },
	{ 0x68, "B0CS" }, { 0x69, "B1CS" }, { 0x6a, "B2CS" }, { 0x6b, "B3CS" },
	{ 0x6c, "BEXCS" }, { 0x6d, "ADMOD" }, { 0x6e, "WDMOD" },
	{ 0x6f, "WDCR" },
	{ 0x70, "INTE0AD" }, { 0x71, "INTE45" },
	{ 0x72, "INTE67" }, { 0x73, "INTET10" }, { 0x74, "INTET32" },
	{ 0x75, "INTET54" }, { 0x76, "INTET76" }, { 0x77, "INTES0" },
	{ 0x78, "INTES1" }, { 0x79, "INTETC01" }, { 0x7a, "INTETC23" },
	{ 0x7b, "IIMC" },
	{ 0x7c, "DMA0V" }, { 0x7d, "DMA1V" }, { 0x7e, "DMA2V" }, { 0x7f, "DMA3V" }
};

static tlcs900_disassembler::cr_sym const tmp95c061_cr_syms[] = {
	{ 8,  0x22, "DMAM0" }, { 8,  0x26, "DMAM1" }, { 8,  0x2a, "DMAM2" }, { 8,  0x2e, "DMAM3" },
	{ 16, 0x20, "DMAC0" }, { 16, 0x24, "DMAC1" }, { 16, 0x28, "DMAC2" }, { 16, 0x2c, "DMAC3" },
	{ 32, 0x00, "DMAS0" }, { 32, 0x04, "DMAS1" }, { 32, 0x08, "DMAS2" }, { 32, 0x0c, "DMAS3" },
	{ 32, 0x10, "DMAD0" }, { 32, 0x14, "DMAD1" }, { 32, 0x18, "DMAD2" }, { 32, 0x1c, "DMAD3" },
};
