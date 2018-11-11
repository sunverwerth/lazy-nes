#ifndef PPU_H
#define PPU_H

#include <cstdint>

extern const int ppu_frame_width;
extern const int ppu_frame_height;
extern bool ppu_vsync;
extern uint32_t ppu_frame_buffer[];
extern int ppu_mode;

void ppu_init();
void ppu_write(unsigned short address, unsigned char data);
unsigned char ppu_read(unsigned short address);
void ppu_tick();

#endif
