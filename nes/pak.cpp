#include "pak.h"
#include "cpu.h"

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cassert>

unsigned char* pak_prg_rom = nullptr;
unsigned char pak_prg_rom_num_banks;
int pak_prg_rom_size = 0;

unsigned char* pak_chr_rom = nullptr;
unsigned char pak_chr_rom_num;
int pak_chr_rom_size = 0;

unsigned char* pak_prg_ram = nullptr;
unsigned char pak_prg_ram_num;
int pak_prg_ram_size = 0;

unsigned char* pak_trainer = nullptr;

unsigned char pak_name[256];
unsigned char pak_mapper = 0;

const int PAK_FLAGS6_MIRROR          = 1 << 0;
const int PAK_FLAGS6_HAS_RAM         = 1 << 1;
const int PAK_FLAGS6_HAS_TRAINER     = 1 << 2;
const int PAK_FLAGS6_HAS_FOUR_SCREEN = 1 << 3;

const int PAK_FLAGS7_VS_UNISYSTEM = 1 << 0;
const int PAK_FLAGS7_PLAYCHOICE   = 1 << 1;
const int PAK_FLAGS7_NES2         = 1 << 3;

const int PAK_FLAGS9_PAL = 1 << 0;

void pak_load(const char* filename) {
	// TODO
	auto file = fopen(filename, "rb");
	if (!file) {
		std::cerr << "ROM not found.\n";
		exit(1);
	}

	fseek(file, 0, SEEK_END);
	auto length = ftell(file);
	fseek(file, 0, SEEK_SET);

	// HEADER
	char magic[4];
	unsigned char flags6;
	unsigned char flags7;
	unsigned char prgRamNum;
	unsigned char flags9;
	unsigned char flags10;
	unsigned char zero[5];

	fread(&magic, 1, 4, file);
	fread(&pak_prg_rom_num_banks, 1, 1, file);
	fread(&pak_chr_rom_num, 1, 1, file);
	fread(&flags6, 1, 1, file);
	fread(&flags7, 1, 1, file);
	fread(&prgRamNum, 1, 1, file);
	fread(&flags9, 1, 1, file);
	fread(&flags10, 1, 1, file);
	fread(&zero, 1, 5, file);

	pak_prg_rom_num_banks *= 2;

	// TRAINER
	if (flags6 & PAK_FLAGS6_HAS_TRAINER) {
		pak_trainer = new unsigned char[512];
		fread(&pak_trainer, 1, 512, file);
	}

	pak_prg_rom_size = 8192 * pak_prg_rom_num_banks;
	pak_prg_rom = new unsigned char[pak_prg_rom_size];
	fread(pak_prg_rom, 1, pak_prg_rom_size, file);

	pak_chr_rom_size = 8192 * pak_chr_rom_num;
	pak_chr_rom = new unsigned char[pak_chr_rom_size];
	fread(pak_chr_rom, 1, pak_chr_rom_size, file);

	pak_prg_ram_size = 8192 * (pak_prg_ram_num ? pak_prg_ram_num : 1);
	pak_prg_ram = new unsigned char[pak_prg_ram_size];

	auto sofar = ftell(file);

	if (length - sofar >= 8192) {
		fseek(file, 8192, SEEK_CUR);
	}
	
	sofar = ftell(file);
	if (length - sofar >= 256) {
		fread(pak_name, 1, 256, file);
	}

	fclose(file);

	pak_mapper = ((flags6 & 0xf0) >> 4 )+ (flags7 & 0xf0);
}

unsigned char bank_select;
int r[8];
unsigned char irq_latch;
bool irq_enable;
bool irq_reload;
int pak_scanline_counter;

void pak_scanline() {
	if (pak_mapper == 4) {
		if (pak_scanline_counter == 0 || irq_reload) {
			//if (irq_enable && pak_scanline_counter == 0) cpu_irq = true;
			if (irq_reload) {
				pak_scanline_counter = irq_latch;
				irq_reload = false;
			}
		}
		else {
			pak_scanline_counter--;
		}
	}
}

unsigned char pak_read(unsigned short address) {
	// TODO: different Mappers
	if (!pak_chr_rom_size || !pak_prg_rom_size) return 0;

	if (pak_mapper == 4) {
		if (address < 0x6000) { // CHR ROM
			if (address <= 0x03ff) {
				int bank = (bank_select & 0x80) ? r[2] : r[0];
				return pak_chr_rom[bank * 1024 + address];
			}
			else if (address >= 0x0400 && address <= 0x07ff) {
				int bank = (bank_select & 0x80) ? r[3] : r[0];
				int offset = (bank_select & 0x80) ? 0 : 1024;
				return pak_chr_rom[bank * 1024 + offset + address - 0x0400];
			}
			else if (address >= 0x0800 && address <= 0x0bff) {
				int bank = (bank_select & 0x80) ? r[4] : r[1];
				return pak_chr_rom[bank * 1024 + address - 0x0800];
			}
			else if (address >= 0x0c00 && address <= 0x0fff) {
				int bank = (bank_select & 0x80) ? r[5] : r[1];
				int offset = (bank_select & 0x80) ? 0 : 1024;
				return pak_chr_rom[bank * 1024 + offset + address - 0x0c00];
			}
			else if (address >= 0x1000 && address <= 0x13ff) {
				int bank = (bank_select & 0x80) ? r[0] : r[2];
				return pak_chr_rom[bank * 1024 + address - 0x1000];
			}
			else if (address >= 0x1400 && address <= 0x17ff) {
				int bank = (bank_select & 0x80) ? r[0] : r[3];
				int offset = (bank_select & 0x80) ? 1024 : 0;
				return pak_chr_rom[bank * 1024 + offset + address - 0x1400];
			}
			else if (address >= 0x1800 && address <= 0x1bff) {
				int bank = (bank_select & 0x80) ? r[1] : r[4];
				return pak_chr_rom[bank * 1024 + address - 0x1800];
			}
			else if (address >= 0x1c00 && address <= 0x1fff) {
				int bank = (bank_select & 0x80) ? r[1] : r[5];
				int offset = (bank_select & 0x80) ? 1024 : 0;
				return pak_chr_rom[bank * 1024 + offset + address - 0x1c00];
			}
			else {
				return 0;
			}
		}
		else if (address >= 0x6000 && address <= 0x7FFF) { // 8 KB PRG RAM bank(optional)
			return pak_prg_ram[address - 0x6000];
		}
		else if (address >= 0x8000 && address <= 0x9FFF) {
			int base = (bank_select & 0x40) ? pak_prg_rom_size - 8192 * 2 : r[6] * 8192;
			return pak_prg_rom[base + (address - 0x8000)];
		}
		else if (address >= 0xA000 && address <= 0xBFFF) {
			int base = r[7] * 8192;
			return pak_prg_rom[base + (address - 0xa000)];
		}
		else if (address >= 0xC000 && address <= 0xDFFF) {
			int base = (bank_select & 0x40) ? r[6] * 8192 : pak_prg_rom_size - 8192 * 2;
			return pak_prg_rom[base + (address - 0xc000)];
		}
		else if (address >= 0xE000 && address <= 0xFFFF) {// 8 KB PRG ROM bank, fixed to the last bank
			int base = pak_prg_rom_size - 8192;
			return pak_prg_rom[base + (address - 0xe000)];
		}
	}
	else {
		// PRG ROM
		if (address < 0x8000) {
			return pak_chr_rom[address % pak_chr_rom_size];
		}
		else {
			return pak_prg_rom[(address - 0x8000) % pak_prg_rom_size];
		}
	}

	return 0;
}

void pak_write(unsigned short address, unsigned char data) {
	if (pak_mapper == 4) {
		if (address >= 0x6000 && address <= 0x7FFF) { // 8 KB PRG RAM bank(optional)
			pak_prg_ram[address - 0x6000] = data;
		}
		else if (address >= 0x8000 && address <= 0x9fff) {
			// bank select and bank data
			if (address & 1) {
				int reg = bank_select & 0x7;
				if (reg == 6 || reg == 7) data &= ~0xc0;
				else if (reg == 0 || reg == 1) data &= ~0x01;
				r[reg] = data;
			}
			else {
				bank_select = data;
			}
		}
		if (address >= 0xa000 && address <= 0xbffe) {
			int i = 0;
			// mirroring and ram
		}
		if (address >= 0xc000 && address <= 0xdfff) {
			// irq latch and reload
			if (address & 1) {
				pak_scanline_counter = 0;
				irq_reload = true;
			}
			else {
				irq_latch = data;
			}
		}
		if (address >= 0xe000 && address <= 0xffff) {
			// irq toggle
			if (address & 1) {
				irq_enable = true;
			}
			else {
				irq_enable = false;
				cpu_irq = false;
			}
		}
	}
}

