#include "bus.h"
#include "mem.h"
#include "ppu.h"
#include "apu.h"
#include "pak.h"

/*
$0000-$07FF 	$0800 	2KB internal RAM
$0800-$0FFF 	$0800 	Mirrors of $0000-$07FF
$1000-$17FF 	$0800
$1800-$1FFF 	$0800
$2000-$2007 	$0008 	NES PPU registers
$2008-$3FFF 	$1FF8 	Mirrors of $2000-2007 (repeats every 8 bytes)
$4000-$4017 	$0018 	NES APU and I/O registers
$4018-$401F 	$0008 	APU and I/O functionality that is normally disabled. See CPU Test Mode.
$4020-$FFFF 	$BFE0 	Cartridge space: PRG ROM, PRG RAM, and mapper registers (See Note)
*/

unsigned short bus_last_read;
unsigned short bus_last_write;
extern bool g_debug_read;

unsigned char bus_read(unsigned short address) {
	if (!g_debug_read) bus_last_read = address;

	if (address < 0x2000) {
		return mem_read(address & 0x07ff);
	}
	else if (address < 0x4000) {
		return ppu_read(address);
	}
	else if (address < 0x4018) {
		return apu_read(address);
	}
	else if (address < 0x4020) {
		// disabled
		return 0;
	}
	else {
		return pak_read(address);
	}
}

void bus_write(unsigned short address, unsigned char data) {
	bus_last_write = address;
	
	if (address < 0x2000) {
		mem_write(address & 0x07ff, data);
	}
	else if (address < 0x4000) {
		ppu_write(address, data);
	}
	else if (address < 0x4018) {
		apu_write(address, data);
	}
	else if (address < 0x4020) {
		// disabled
	}
	else {
		pak_write(address, data);
	}
}