#include <memory>
#include "vic.h"
#include "cia.h"

Cia::Cia(Vic* vic) : vic(vic) {
}

Cia::~Cia() {
}

#include <Windows.h>
extern int cpu_irq;

void Cia::tick() {

	// CIA 1
	registers[0][0] = 0xff;
	registers[0][1] = 0xff;
	
	// timer interrupt
}

char Cia::read(unsigned short address) {
	char wrapped = address & 0xf;
	
	if (address & 0x100) {
		// second cia
		return registers[1][wrapped];
	}
	else {
		// first cia
		if (wrapped == 13) {
			registers[0][13] = 0;
			extern int cpu_irq;
			cpu_irq &= ~2;
		}
		return registers[0][wrapped];
	}
}

void Cia::write(unsigned short address, char data) {
	char wrapped = address & 0xf;
	if (address & 0x100) {
		// second cia
		registers[1][wrapped] = data;
		if (wrapped == 0) {
			vic->setBank(~data & 3);
		}
	}
	else {
		// first cia
		registers[0][wrapped] = data;
	}
}

Cia* cia = 0;