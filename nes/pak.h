#ifndef PAK_H
#define CART_H

extern unsigned char* pak_chr_rom;
extern int pak_chr_rom_size;

void pak_scanline();
void pak_load(const char* filename);
unsigned char pak_read(unsigned short address);
void pak_write(unsigned short address, unsigned char data);

#endif
