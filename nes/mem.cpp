#include "mem.h"

unsigned char mem_ram[2048];

char mem_read(unsigned short address) {
	return mem_ram[address];
}

void mem_write(unsigned short address, char data) {
	mem_ram[address] = data;
}