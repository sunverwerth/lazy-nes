#ifndef CPU_H
#define CPU_H

extern bool cpu_irq;
extern bool cpu_nmi;
extern unsigned short cpu_pc;

void cpu_tick();
void cpu_init();

#endif