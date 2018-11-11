
#ifndef APU_H
#define APU_H

extern unsigned char apu_input_up;
extern unsigned char apu_input_down;
extern unsigned char apu_input_left;
extern unsigned char apu_input_right;
extern unsigned char apu_input_a;
extern unsigned char apu_input_b;
extern unsigned char apu_input_select;
extern unsigned char apu_input_start;

unsigned char apu_read(unsigned short address);
void apu_write(unsigned short address, unsigned char data);
void apu_tick();
void apu_gen_audio(void* buffer, int numSamples);

extern bool input_up;
extern bool input_down;
extern bool input_left;
extern bool input_right;
extern bool input_a;
extern bool input_b;
extern bool input_select;
extern bool input_start;

#endif