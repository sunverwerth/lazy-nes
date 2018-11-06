#include "vic.h"
#include "cia.h"

char* ram = 0;
char* vram = 0;
char* kernal = 0;
char* basic = 0;
char* charset = 0;
char* ioread = 0;	
char* iowrite = 0;	

bool enable_kernal = true;
bool enable_io = true;
bool enable_chargen = false;
bool enable_basic = true;

char mem_read(unsigned short address) {

	if (enable_kernal && address >= 0xe000) { // kernal rom
		return kernal[address - 0xe000];
	}
	else if (enable_io && address >= 0xd000) { // io area
		if (address >= 0xdc00 && address < 0xde00) {
			return cia->read(address - 0xdc00);
		}
		else if (address < 0xd400) {
			return vic->read(address - 0xd000);
		}
		else {
			return ioread[address - 0xd000];
		}
	}
	else if (enable_chargen && address >= 0xd000) { // character rom
		return charset[address - 0xd000];
	}
	else if (enable_basic && address >= 0xa000 && address < 0xc000) { // basic rom
		return basic[address - 0xa000];
	}
	else { // normal ram
		return ram[address];
	}
}

void mem_write(unsigned short address, char data) {

	if (enable_io && address >= 0xd000 && address < 0xe000) { // io area
		if (address >= 0xdc00 && address < 0xde00) {
			cia->write(address - 0xdc00, data);
		}
		else if (address < 0xd400) {
			vic->write(address - 0xd000, data);
		}
		else {
			iowrite[address - 0xd000] = data;
		}
	}
	else { // normal ram
		ram[address] = data;
	}

	if (address == 1) {
		// cpu port
		if ((data & 7) == 0 || (data & 7) == 4)  {
			enable_kernal = enable_basic = enable_io = enable_chargen = false;
		}
		else if ((data & 3) == 1)  {
			enable_kernal = enable_basic = false;
			enable_io = data & 4;
			enable_chargen = !enable_io;
		}
		else if ((data & 3) == 2)  {
			enable_basic = false;
			enable_kernal = true;
			enable_io = data & 4;
			enable_chargen = !enable_io;
		}
		else if ((data & 3) == 3)  {
			enable_kernal = enable_basic = true;
			enable_io = data & 4;
			enable_chargen = !enable_io;
		}
	}
}