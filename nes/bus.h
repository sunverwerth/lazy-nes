#ifndef BUS_H
#define BUS_H

extern unsigned short bus_last_read;
extern unsigned short bus_last_write;

unsigned char bus_read(unsigned short address);
void bus_write(unsigned short address, unsigned char data);

#endif
