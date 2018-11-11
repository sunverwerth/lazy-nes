#include "ppu.h"
#include "bus.h"
#include "cpu.h"
#include "pak.h"
#include "mem.h"

#include <cstdlib>
#include <cstdint>

extern bool g_debug_read;

constexpr uint32_t rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	return (b)+(g << 8) + (r << 16) + (a << 24);
}

unsigned long ppu_colors[64] = {
	rgba(124,124,124,255),
	rgba(0,0,252,255),
	rgba(0,0,188,255),
	rgba(68,40,188,255),
	rgba(148,0,132,255),
	rgba(168,0,32,255),
	rgba(168,16,0,255),
	rgba(136,20,0,255),
	rgba(80,48,0,255),
	rgba(0,120,0,255),
	rgba(0,104,0,255),
	rgba(0,88,0,255),
	rgba(0,64,88,255),
	rgba(0,0,0,255),
	rgba(0,0,0,255),
	rgba(0,0,0,255),
	rgba(188,188,188,255),
	rgba(0,120,248,255),
	rgba(0,88,248,255),
	rgba(104,68,252,255),
	rgba(216,0,204,255),
	rgba(228,0,88,255),
	rgba(248,56,0,255),
	rgba(228,92,16,255),
	rgba(172,124,0,255),
	rgba(0,184,0,255),
	rgba(0,168,0,255),
	rgba(0,168,68,255),
	rgba(0,136,136,255),
	rgba(0,0,0,255),
	rgba(0,0,0,255),
	rgba(0,0,0,255),
	rgba(248,248,248,255),
	rgba(60,188,252,255),
	rgba(104,136,252,255),
	rgba(152,120,248,255),
	rgba(248,120,248,255),
	rgba(248,88,152,255),
	rgba(248,120,88,255),
	rgba(252,160,68,255),
	rgba(248,184,0,255),
	rgba(184,248,24,255),
	rgba(88,216,84,255),
	rgba(88,248,152,255),
	rgba(0,232,216,255),
	rgba(120,120,120,255),
	rgba(0,0,0,255),
	rgba(0,0,0,255),
	rgba(252,252,252,255),
	rgba(164,228,252,255),
	rgba(184,184,248,255),
	rgba(216,184,248,255),
	rgba(248,184,248,255),
	rgba(248,164,192,255),
	rgba(240,208,176,255),
	rgba(252,224,168,255),
	rgba(248,216,120,255),
	rgba(216,248,120,255),
	rgba(184,248,184,255),
	rgba(184,248,216,255),
	rgba(0,252,252,255),
	rgba(248,216,248,255),
	rgba(0,0,0,255),
	rgba(0,0,0,255)
};

const int ppu_frame_width = 256; // 341
const int ppu_frame_height = 240; // 262
bool ppu_vsync = false;
uint32_t ppu_frame_buffer[ppu_frame_width * ppu_frame_height];
int ppu_tick_count;
int ppu_mode = 1;

unsigned char ppu_ram[2048];
unsigned char ppu_pal[32];

unsigned int ppu_current_line;
unsigned int ppu_current_pixel;

const int PPU_REG_CTRL = 0;
const int PPU_REG_MASK = 1;
const int PPU_REG_STATUS = 2;
const int PPU_REG_OAM_ADDR = 3;
const int PPU_REG_OAM_DATA = 4;
const int PPU_REG_SCROLL = 5;
const int PPU_REG_ADDR = 6;
const int PPU_REG_DATA = 7;

struct SPRITE {
	unsigned char yPos;
	union {
		unsigned char index;
		struct {
			unsigned char bigsprite_bank : 1;
			unsigned char bigsprite_index : 7;
		};
	};
	struct {
		unsigned char palette : 2;
		unsigned char unimplemented : 3;
		unsigned char priority : 1;
		unsigned char flipx : 1;
		unsigned char flipy : 1;
	} attributes;
	unsigned char xPos;
};

SPRITE ppu_oam[64];


union {
	struct {
		unsigned char nametableBase : 2;
		unsigned char addrInc : 1;
		unsigned char sprTblAddr : 1;
		unsigned char bgrTblAddr : 1;
		unsigned char spriteSize : 1;
		unsigned char mstSlv : 1;
		unsigned char nmiGen : 1;
	};
	unsigned char bits;
} PPUCTRL;

union {
	struct {
		unsigned char grayscale : 1;
		unsigned char bgleft8 : 1;
		unsigned char sprleft8 : 1;
		unsigned char bgEnable : 1;
		unsigned char sprEnable : 1;
		unsigned char emRed : 1;
		unsigned char emGreen : 1;
		unsigned char emBlue : 1;
	};
	unsigned char bits;
} PPUMASK;

union {
	struct {
		unsigned char lastdata : 5;
		unsigned char spriteOverflow : 1;
		unsigned char sprite0Hit : 1;
		unsigned char vblank : 1;
	};
	unsigned char bits;
} PPUSTATUS;

unsigned char OAMADDR;

struct {
	unsigned char x;
	unsigned char y;
} PPUSCROLL;
int PPUSCROLL_latch = 0;

unsigned short PPUADDR;
int PPUADDR_latch = 0;

static void setPixel(int x, int y, unsigned long v) {
	if (x < 0 || x >= ppu_frame_width) return;
	if (y < 0 || y >= ppu_frame_height) return;
	ppu_frame_buffer[y * ppu_frame_width + x] = v;
}

void ppu_init() {
	for (int y = 0; y < ppu_frame_height; ++y) {
		for (int x = 0; x < ppu_frame_width; ++x) {
			setPixel(x, y, rgba(x, 255 - y, 255 - x, 255));
		}
	}
}

void ppu_internal_write(unsigned short address, unsigned char data) {
	if (address < 0x2000) {
		pak_write(address, data);
	}
	else if (address < 0x3f00) {
		ppu_ram[(address - 0x2000) & 0x7ff] = data;
	}
	else {
		if (address == 0x3f10) address = 0x3f00;
		ppu_pal[(address - 0x3f00) & 0x1f] = data;
	}
}

unsigned char ppu_internal_read(unsigned short address) {
	if (address < 0x2000) {
		return pak_read(address);
	}
	else if (address < 0x3f00) {
		return ppu_ram[(address - 0x2000) & 0x7ff];
	}
	else {
		if (address == 0x3f10) address = 0x3f00;
		return ppu_pal[(address - 0x3f00) & 0x1f];
	}
	return 0;
}

void writeBlock(int _x, int _y, unsigned long col) {
	for (int y = _y; y < _y + 8; ++y) {
		for (int x = _x; x < _x + 8; ++x) {
			setPixel(x, y, col);
		}
	}
}

void writeChr(int _x, int _y, unsigned short addr, int pal) {
	for (int y = 0; y < 8 && _y + y < ppu_frame_height; ++y) {
		auto lo = ppu_internal_read(addr + y);
		auto hi = ppu_internal_read(addr + y + 8);
		for (int x = 7; x >= 0 && _x + x < ppu_frame_width; --x) {
			auto v = ((hi & 1) << 1) + (lo & 1);
			setPixel(_x + x, _y + y, v ? ppu_colors[ppu_pal[pal * 4 + v] & 0x3f] : ppu_colors[ppu_pal[0] & 0x3f]);
			//setPixel(_x + x, _y + y, ppu_colors[ppu_pal[0] & 0x3f]);
			lo >>= 1;
			hi >>= 1;
		}
	}
}

void writeSpr(const SPRITE& sprite) {
	if (PPUCTRL.spriteSize) {
		unsigned short base = 0x1000 * sprite.bigsprite_bank + sprite.bigsprite_index * 2 * 16;
		
		int yStart = sprite.attributes.flipy ? sprite.yPos + 15 + 1 : sprite.yPos + 1;
		int yEnd = sprite.attributes.flipy ? yStart - 8 : yStart + 8;
		int yMove = sprite.attributes.flipy ? -1 : 1;
		int xStart = !sprite.attributes.flipx ? sprite.xPos + 7 : sprite.xPos;
		int xEnd = !sprite.attributes.flipx ? xStart - 8 : xStart + 8;
		int xMove = !sprite.attributes.flipx ? -1 : 1;

		for (int y = yStart; y != yEnd; y += yMove) {
			auto lo = ppu_internal_read(base);
			auto hi = ppu_internal_read(base + 8);
			base++;
			for (int x = xStart; x != xEnd; x += xMove) {
				auto v = ((hi & 1) << 1) + (lo & 1);
				if (v) {
					setPixel(x, y, ppu_colors[ppu_pal[16 + sprite.attributes.palette * 4 + v] & 0x3f]);
				}
				lo >>= 1;
				hi >>= 1;
			}
		}
		
		base += 8;
		
		yStart += sprite.attributes.flipy ? -8 : 8;
		yEnd += sprite.attributes.flipy ? -8 : 8;

		for (int y = yStart; y != yEnd; y += yMove) {
			auto lo = ppu_internal_read(base);
			auto hi = ppu_internal_read(base + 8);
			base++;
			for (int x = xStart; x != xEnd; x += xMove) {
				auto v = ((hi & 1) << 1) + (lo & 1);
				if (v) {
					setPixel(x, y,ppu_colors[ppu_pal[16 + sprite.attributes.palette * 4 + v] & 0x3f]);
				}
				lo >>= 1;
				hi >>= 1;
			}
		}
	}
	else {
		unsigned short base = 0x1000 * PPUCTRL.sprTblAddr + sprite.index * 16;
		
		int yStart = sprite.attributes.flipy ? sprite.yPos + 7 + 1 : sprite.yPos + 1;
		int yEnd = sprite.attributes.flipy ? yStart - 8 : yStart + 8;
		int yMove = sprite.attributes.flipy ? -1 : 1;
		int xStart = !sprite.attributes.flipx ? sprite.xPos + 7 : sprite.xPos;
		int xEnd = !sprite.attributes.flipx ? xStart - 8 : xStart + 8;
		int xMove = !sprite.attributes.flipx ? -1 : 1;

		for (int y = yStart; y != yEnd; y += yMove) {
			auto lo = ppu_internal_read(base);
			auto hi = ppu_internal_read(base + 8);
			base++;
			for (int x = xStart; x != xEnd; x += xMove) {
				auto v = ((hi & 1) << 1) + (lo & 1);
				if (v) {
					setPixel(x, y, ppu_colors[ppu_pal[16 + sprite.attributes.palette * 4 + v] & 0x3f]);
				}
				lo >>= 1;
				hi >>= 1;
			}
		}
	}
}

void ppu_write(unsigned short address, unsigned char data) {
	//TODO
	auto reg = address & 0x7;
	PPUSTATUS.lastdata = data & 0x1f;
	switch (reg) {
	case PPU_REG_CTRL:
		PPUCTRL.bits = data;
		break;
	case PPU_REG_MASK:
		PPUMASK.bits = data;
		break;
	case PPU_REG_OAM_ADDR:
		OAMADDR = data;
		break;
	case PPU_REG_OAM_DATA:
		((unsigned char*)ppu_oam)[OAMADDR++] = data;
		break;
	case PPU_REG_SCROLL:
		if (PPUSCROLL_latch == 0) {
			PPUSCROLL.x = data;
			PPUSCROLL_latch = 1;
		}
		else {
			//PPUSCROLL.y = data;
			PPUSCROLL_latch = 0;
		}
		break;
	case PPU_REG_ADDR:
		if (PPUSCROLL_latch == 0) {
			PPUADDR = (PPUADDR & 0x00ff) | (data << 8);
			PPUSCROLL_latch = 1;
		}
		else {
			PPUADDR = (PPUADDR & 0xff00) | data;
			PPUSCROLL_latch = 0;
		}
		break;
	case PPU_REG_DATA:
		ppu_internal_write(PPUADDR, data);
		PPUADDR += PPUCTRL.addrInc ? 32 : 1;
		break;
	case PPU_REG_STATUS:
		// read only
		break;
	}
}

unsigned char regdata;
unsigned char ppu_read(unsigned short address) {
	//TODO
	auto reg = address & 0x7;
	unsigned char data = 0;
	switch (reg) {
	case PPU_REG_STATUS:
		data = PPUSTATUS.bits;
		if (g_debug_read) return data;
		PPUADDR_latch = 0;
		PPUSCROLL_latch = 0;
		PPUSTATUS.vblank = 0;
		break;
	case PPU_REG_OAM_DATA:
		data = ((unsigned char*)ppu_oam)[OAMADDR];
		break;
	case PPU_REG_DATA:
		data = regdata;
		regdata = ppu_internal_read(PPUADDR);
		if (g_debug_read) return data;
		PPUADDR += PPUCTRL.addrInc ? 32 : 1;
		break;
	case PPU_REG_CTRL:
	case PPU_REG_MASK:
	case PPU_REG_OAM_ADDR:
	case PPU_REG_SCROLL:
	case PPU_REG_ADDR:
		// write only
		return 0;
		break;
	}
	
	return data;
}

void ppu_tick() {
	// TODO
	ppu_tick_count++;

	ppu_current_pixel++;
	if (ppu_current_pixel == 341) {
		ppu_current_pixel = 0;
		ppu_current_line++;
	}

	if (ppu_current_line == 262) {
		ppu_current_line = 0;
		ppu_current_pixel = 0;
	}

	// Sprite0 hit
	if (ppu_current_line == ppu_oam[0].yPos && ppu_current_pixel == ppu_oam[0].xPos) {
		PPUSTATUS.sprite0Hit = 1;
	}

	// trigger scanline counter
	if (ppu_current_pixel == 0 && ppu_current_line < 241) pak_scanline();

	if (ppu_current_pixel == 0 && ppu_current_line < ppu_frame_height && (ppu_current_line % 8) == 0) {
		if (PPUMASK.bgEnable) {
			int iy = ppu_current_line / 8;
			for (int ix = PPUSCROLL.x / 8; ix < 32; ++ix) {
				auto chr = ppu_internal_read(0x2000 + 0x400 * PPUCTRL.nametableBase + iy * 32 + ix);
				auto attr = ppu_internal_read(0x2000 + 0x400 * PPUCTRL.nametableBase + 0x03c0 + (iy / 4) * 8 + (ix / 4));
				char pal = 0;
				if ((ix % 4) > 1) {
					if ((iy % 4) > 1) {
						pal = attr >> 6;
					}
					else {
						pal = (attr >> 2) & 0x3;
					}
				}
				else {
					if ((iy % 4) > 1) {
						pal = (attr >> 4) & 0x3;
					}
					else {
						pal = attr & 0x3;
					}
				}
				writeChr(ix * 8 - PPUSCROLL.x, iy * 8 - PPUSCROLL.y, 0x1000 * PPUCTRL.bgrTblAddr + chr * 16, pal);
			}
			for (int ix = 0; ix < PPUSCROLL.x / 8; ++ix) {
				auto chr = ppu_internal_read(0x2800 + 0x400 * PPUCTRL.nametableBase + iy * 32 + ix);
				auto attr = ppu_internal_read(0x2800 + 0x400 * PPUCTRL.nametableBase + 0x03c0 + (iy / 4) * 8 + (ix / 4));
				char pal = 0;
				if ((ix % 4) > 1) {
					if ((iy % 4) > 1) {
						pal = attr >> 6;
					}
					else {
						pal = (attr >> 2) & 0x3;
					}
				}
				else {
					if ((iy % 4) > 1) {
						pal = (attr >> 4) & 0x3;
					}
					else {
						pal = attr & 0x3;
					}
				}
				writeChr((ix + 32) * 8 - PPUSCROLL.x, iy * 8 - PPUSCROLL.y, 0x1000 * PPUCTRL.bgrTblAddr + chr * 16, pal);
			}
		}
	}

	ppu_vsync = false;
	if (ppu_current_line == ppu_frame_height && ppu_current_pixel == 1) {
		if (PPUMASK.sprEnable) {
			for (int i = 0; i < 64; ++i) {
				if (ppu_oam[i].yPos >= ppu_frame_height) continue;
				writeSpr(ppu_oam[i]);
			}
		}

		switch (ppu_mode) {
			case 3:
			{
				for (int y = 0; y < 16; ++y) {
					for (int x = 0; x < 32; ++x) {
						writeChr(x * 8, y * 8, (y * 32 + x) * 16, 0);
						writeChr(x * 8, y * 8, (y * 32 + x) * 16, 0);
					}
				}
				extern unsigned char irq_latch;
				for (int x = 0; x < ppu_frame_width; x+=2) {
					setPixel(x, irq_latch, rgba(255, 0, 255, 255));
				}
				break;
			}
		}

		PPUSTATUS.vblank = 1;

		ppu_vsync = true;
		if (PPUCTRL.nmiGen) {
			cpu_nmi = true;
		}
	}
	else if (ppu_current_line == 261 && ppu_current_pixel == 1) {
		PPUSTATUS.vblank = 0;
		PPUSTATUS.sprite0Hit = 0;

		for (int i = 0; i < ppu_frame_width * ppu_frame_height; ++i) {
			ppu_frame_buffer[i] = 0;
		}
	}
}