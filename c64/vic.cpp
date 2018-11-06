#include "vic.h"
#include <Windows.h>
#include <conio.h>

Vic::Vic() : x(0), y(0), interruptLine(0), backgroundColor(0), borderColor(0), gfxBase(0), charBase(0) {
	registers[0x001a] = 0;
	registers[0x0018] = 4;
}

Vic::~Vic() {
}

void Vic::tick() {
	
	extern char* ram;
	extern char* iowrite;
	extern char* charset;

	gfxBase = 0x0400;
	colorBase = 0x800;

	if ((y % 8) == 0 && x == 0) {
		// badline, fetch chars and colors
		int line = y / 8;
		for(int c=0; c<40; c++) {
			lineChars[c] = ram[gfxBase+line*40+c];
			lineColors[c] = iowrite[colorBase+line*40+c];
		}
	}

	backgroundColor = registers[0x21];
	for(int i=0; i<8; i++) {
		frameBuffer[((199-y)*320+x)] = charset[lineChars[x/8]*8+(y%8)] & (128 >> i) ? vic_colors[lineColors[x/8] & 0xf] : vic_colors[backgroundColor & 0xf];
		++x;
	}

	if (x == 320) {
		x = 0;
		y++;
		if (y == 200) {
			y = 0;
		}
	}

	if (y == interruptLine && x == 0 && (registers[0x001a] & 1)) {
		extern int cpu_irq;
		cpu_irq |= 1;
	}

	// write current raster line
	registers[0x0011] = (registers[0x0011] & 0x7f) | ((y & 0x1ff) >> 1);
	registers[0x0012] = y & 0xff;
}

char Vic::read(unsigned short address) {
	char wrapped = address % 64;
	return registers[wrapped];
}

void Vic::write(unsigned short address, char data) {
	char wrapped = address % 64;

	if (wrapped == 0x11) {
		interruptLine = (interruptLine & 0xfeff) | ((data & 0x80) << 1);
	}
	else if (wrapped == 0x12) {
		interruptLine = (interruptLine & 0xff00) | data;
	}
	else {
		registers[wrapped] = data;
	}
}

void Vic::setBank(char bank) {
	memBase = (bank & 3) << 14;
}

const unsigned int* Vic::getFrameBuffer() const {
	return frameBuffer;
}

int Vic::getFrameWidth() const {
	return 320;
}

int Vic::getFrameHeight() const {
	return 200;
}

bool Vic::frameStarted() const {
	return x == 0 && y == 0;
}


const unsigned int Vic::vic_colors[] = {
	0x00000000,
	0x00FCFCFC,
	0x008B1F00,
	0x0065CDA8,
	0x00A73B9F,
	0x004FB317,
	0x001B0D93,
	0x00F3EB5B,
	0x00A34700,
	0x003F1C00,
	0x00CB7B6F,
	0x00454444,
	0x00838383,
	0x0097FF97,
	0x004F93D3,
	0x00BBBBBB
};

Vic* vic = 0;