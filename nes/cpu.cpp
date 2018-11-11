#include "bus.h"

#include <iostream>

extern bool g_stop;

unsigned char cpu_x = 0;
unsigned char cpu_y = 0;
unsigned char cpu_acc = 0;
unsigned char cpu_stack = 0;
unsigned short cpu_pc = 0;
bool cpu_irq = false;
bool cpu_nmi = false;

unsigned char cpu_op = 0;
unsigned char cpu_op_args[2] = {0};

#define CPU_STAT_CARRY 1
#define CPU_STAT_ZERO 2
#define CPU_STAT_IRQ_DISABLE 4
#define CPU_STAT_DEC 8
#define CPU_STAT_BREAK 16
#define CPU_STAT_UNUSED 32
#define CPU_STAT_OVERFLOW 64
#define CPU_STAT_NEGATIVE 128
unsigned char cpu_p;

#define SETZERO(v) if (!(v)) { cpu_p |= CPU_STAT_ZERO; } else { cpu_p &= ~CPU_STAT_ZERO; }
#define SETNEGATIVE(v) if ((v) & 0x80) { cpu_p |= CPU_STAT_NEGATIVE; } else { cpu_p &= ~CPU_STAT_NEGATIVE; }
#define SETCARRYIF(v) if (v) { cpu_p |= CPU_STAT_CARRY; } else { cpu_p &= ~CPU_STAT_CARRY; }
#define SETOVERFLOW(v) if (v) { cpu_p |= CPU_STAT_OVERFLOW; } else { cpu_p &= ~CPU_STAT_OVERFLOW; }
#define SETZN(v) SETZERO(v); SETNEGATIVE(v);

#define CPU_CYCLE_FETCH_OP 1
#define CPU_CYCLE_FETCH_ARG_SINGLE 2
#define CPU_CYCLE_FETCH_ARG_DOUBLE_1 3
#define CPU_CYCLE_FETCH_ARG_DOUBLE_2 4
#define CPU_CYCLE_EXECUTE_OP 5
unsigned char cpu_cycle = CPU_CYCLE_FETCH_OP;

#define RELATIVE(v) ((char)(v))

void cpu_prepare_op();
void cpu_execute_op();

void cpu_adc();
void cpu_and();
void cpu_asl();
void cpu_bcc();
void cpu_bcs();
void cpu_beq();
void cpu_bit();
void cpu_bmi();
void cpu_bne();
void cpu_bpl();
void cpu_brk();
void cpu_bvc();
void cpu_bvs();

void cpu_clc();
void cpu_cld();
void cpu_cli();
void cpu_clv();
void cpu_cmp();
void cpu_cpx();
void cpu_cpy();

void cpu_dec();
void cpu_dex();
void cpu_dey();

void cpu_eor();

void cpu_inc();
void cpu_inx();
void cpu_iny();

void cpu_jmp();
void cpu_jsr();

void cpu_lda();
void cpu_ldx();
void cpu_ldy();
void cpu_lsr();
void cpu_nop();
void cpu_ora();

void cpu_pha();
void cpu_php();
void cpu_pla();
void cpu_plp();

void cpu_rol();
void cpu_ror();
void cpu_rti();
void cpu_rts();

void cpu_scb();
void cpu_sec();
void cpu_sed();
void cpu_sei();
void cpu_sta();
void cpu_stx();
void cpu_sty();

void cpu_tax();
void cpu_tay();
void cpu_tsx();
void cpu_txa();
void cpu_txs();
void cpu_tya();

/**
 * Illegal opcodes
 */
void cpu_kil();
void cpu_slo();
void cpu_rla();
void cpu_sre();
void cpu_rra();
void cpu_sax();
void cpu_lax();
void cpu_dcp();
void cpu_isc();
void cpu_anc();
void cpu_alr();
void cpu_arr();
void cpu_xaa();
void cpu_axs();
void cpu_sbc();
void cpu_ahx();
void cpu_shy();
void cpu_shx();
void cpu_tas();
void cpu_las();

void cpu_stack_push(unsigned char);
unsigned char cpu_stack_pop();

char cpu_op_width[] = {
/*     x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF */	
/*0x*/ 1, 2, 0, 0, 0, 2, 2, 0, 1, 2, 1, 0, 0, 3, 3, 0,
/*1x*/ 2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0,
/*2x*/ 3, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
/*3x*/ 2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0,
/*4x*/ 1, 2, 0, 0, 0, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
/*5x*/ 2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0,
/*6x*/ 1, 2, 0, 0, 0, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
/*7x*/ 2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0,
/*8x*/ 0, 2, 0, 0, 2, 2, 2, 0, 1, 0, 1, 0, 3, 3, 3, 0,
/*9x*/ 2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 0, 3, 0, 0,
/*Ax*/ 2, 2, 2, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
/*Bx*/ 2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
/*Cx*/ 2, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
/*Dx*/ 2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0,
/*Ex*/ 2, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
/*Fx*/ 2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0,
};

typedef void(*t_cpu_opcode)();

t_cpu_opcode cpu_opcode_vectors[] = {
/*      x0       x1       x2       x3       x4       x5       x6       x7       x8       x9       xA       xB       xC       xD       xE       xF   */	
/*0x*/cpu_brk, cpu_ora, cpu_kil, cpu_kil, cpu_kil, cpu_ora, cpu_asl, cpu_kil, cpu_php, cpu_ora, cpu_asl, cpu_kil, cpu_kil, cpu_ora, cpu_asl, cpu_kil,
/*1x*/cpu_bpl, cpu_ora, cpu_kil, cpu_kil, cpu_kil, cpu_ora, cpu_asl, cpu_kil, cpu_clc, cpu_ora, cpu_kil, cpu_kil, cpu_kil, cpu_ora, cpu_asl, cpu_kil,
/*2x*/cpu_jsr, cpu_and, cpu_kil, cpu_kil, cpu_bit, cpu_and, cpu_rol, cpu_kil, cpu_plp, cpu_and, cpu_rol, cpu_kil, cpu_bit, cpu_and, cpu_rol, cpu_kil,
/*3x*/cpu_bmi, cpu_and, cpu_kil, cpu_kil, cpu_kil, cpu_and, cpu_rol, cpu_kil, cpu_sec, cpu_and, cpu_kil, cpu_kil, cpu_kil, cpu_and, cpu_rol, cpu_kil,
/*4x*/cpu_rti, cpu_eor, cpu_kil, cpu_kil, cpu_kil, cpu_eor, cpu_lsr, cpu_kil, cpu_pha, cpu_eor, cpu_lsr, cpu_kil, cpu_jmp, cpu_eor, cpu_lsr, cpu_kil,
/*5x*/cpu_bvc, cpu_eor, cpu_kil, cpu_kil, cpu_kil, cpu_eor, cpu_lsr, cpu_kil, cpu_cli, cpu_eor, cpu_kil, cpu_kil, cpu_kil, cpu_eor, cpu_lsr, cpu_kil,
/*6x*/cpu_rts, cpu_adc, cpu_kil, cpu_kil, cpu_kil, cpu_adc, cpu_ror, cpu_kil, cpu_pla, cpu_adc, cpu_ror, cpu_kil, cpu_jmp, cpu_adc, cpu_ror, cpu_kil,
/*7x*/cpu_bvs, cpu_adc, cpu_kil, cpu_kil, cpu_kil, cpu_adc, cpu_ror, cpu_kil, cpu_sei, cpu_adc, cpu_kil, cpu_kil, cpu_kil, cpu_adc, cpu_ror, cpu_kil,
/*8x*/cpu_kil, cpu_sta, cpu_kil, cpu_kil, cpu_sty, cpu_sta, cpu_stx, cpu_kil, cpu_dey, cpu_kil, cpu_txa, cpu_kil, cpu_sty, cpu_sta, cpu_stx, cpu_kil,
/*9x*/cpu_bcc, cpu_sta, cpu_kil, cpu_kil, cpu_sty, cpu_sta, cpu_stx, cpu_kil, cpu_tya, cpu_sta, cpu_txs, cpu_kil, cpu_kil, cpu_sta, cpu_kil, cpu_kil,
/*Ax*/cpu_ldy, cpu_lda, cpu_ldx, cpu_kil, cpu_ldy, cpu_lda, cpu_ldx, cpu_kil, cpu_tay, cpu_lda, cpu_tax, cpu_kil, cpu_ldy, cpu_lda, cpu_ldx, cpu_kil,
/*Bx*/cpu_bcs, cpu_lda, cpu_kil, cpu_kil, cpu_ldy, cpu_lda, cpu_ldx, cpu_kil, cpu_clv, cpu_lda, cpu_tsx, cpu_kil, cpu_ldy, cpu_lda, cpu_ldx, cpu_kil,
/*Cx*/cpu_cpy, cpu_cmp, cpu_kil, cpu_kil, cpu_cpy, cpu_cmp, cpu_dec, cpu_kil, cpu_iny, cpu_cmp, cpu_dex, cpu_kil, cpu_cpy, cpu_cmp, cpu_dec, cpu_kil,
/*Dx*/cpu_bne, cpu_cmp, cpu_kil, cpu_kil, cpu_kil, cpu_cmp, cpu_dec, cpu_kil, cpu_cld, cpu_cmp, cpu_kil, cpu_kil, cpu_kil, cpu_cmp, cpu_dec, cpu_kil,
/*Ex*/cpu_cpx, cpu_sbc, cpu_kil, cpu_kil, cpu_cpx, cpu_sbc, cpu_inc, cpu_kil, cpu_inx, cpu_sbc, cpu_nop, cpu_kil, cpu_cpx, cpu_sbc, cpu_inc, cpu_kil,
/*Fx*/cpu_beq, cpu_sbc, cpu_kil, cpu_kil, cpu_kil, cpu_sbc, cpu_inc, cpu_kil, cpu_sed, cpu_sbc, cpu_kil, cpu_kil, cpu_kil, cpu_sbc, cpu_inc, cpu_kil,
};

unsigned int cpu_cycle_count;
void cpu_init() {
	cpu_cycle_count = 0;
	// read program counter
	unsigned char lo = bus_read(0xFFFC);
	unsigned char hi = bus_read(0xFFFD);
	cpu_pc = lo + (hi<<8);
	cpu_cycle = CPU_CYCLE_FETCH_OP;
	cpu_p |= CPU_STAT_IRQ_DISABLE;
	cpu_p &= ~CPU_STAT_DEC;
}

void cpu_tick() {
	cpu_cycle_count++;
	switch(cpu_cycle) {
	case CPU_CYCLE_FETCH_OP:
		if (cpu_nmi) {
			cpu_nmi = false;

			cpu_p &= ~CPU_STAT_BREAK;

			cpu_stack_push((cpu_pc >> 8) & 0xff);
			cpu_stack_push(cpu_pc & 0xFF);
			cpu_stack_push(cpu_p);

			cpu_p |= CPU_STAT_IRQ_DISABLE;

			cpu_pc = bus_read(0xFFFA) + (bus_read(0xFFFB) << 8);
		}
		else if (cpu_irq && !(cpu_p & CPU_STAT_IRQ_DISABLE)) {
			cpu_p &= ~CPU_STAT_BREAK;
			
			cpu_stack_push((cpu_pc >> 8) & 0xff);
			cpu_stack_push(cpu_pc & 0xFF);
			cpu_stack_push(cpu_p);
			
			cpu_p |= CPU_STAT_IRQ_DISABLE;

			cpu_pc = bus_read(0xFFFE) + (bus_read(0xFFFF) << 8);
		}
		else {
			cpu_op = bus_read(cpu_pc++);
			cpu_prepare_op();
		}
		break;
	case CPU_CYCLE_FETCH_ARG_SINGLE:
		cpu_op_args[0] = bus_read(cpu_pc++);
		cpu_cycle = CPU_CYCLE_EXECUTE_OP;
		break;
	case CPU_CYCLE_FETCH_ARG_DOUBLE_1:
		cpu_op_args[0] = bus_read(cpu_pc++);
		cpu_cycle = CPU_CYCLE_FETCH_ARG_DOUBLE_2;
		break;
	case CPU_CYCLE_FETCH_ARG_DOUBLE_2:
		cpu_op_args[1] = bus_read(cpu_pc++);
		cpu_cycle = CPU_CYCLE_EXECUTE_OP;
		break;
	case CPU_CYCLE_EXECUTE_OP:
		cpu_execute_op();
		cpu_cycle = CPU_CYCLE_FETCH_OP;
		break;
	}
}

void cpu_prepare_op() {
	switch(cpu_op_width[cpu_op]) {
	case 1:
		cpu_cycle = CPU_CYCLE_EXECUTE_OP;
		break;
	case 2:
		cpu_cycle = CPU_CYCLE_FETCH_ARG_SINGLE;
		break;
	case 3:
		cpu_cycle = CPU_CYCLE_FETCH_ARG_DOUBLE_1;
		break;
	default:
		// KILL OP
		std::cerr << "Invalid OP $" << std::hex << (int)cpu_op << " at location $" << cpu_pc << "\n";
		cpu_kil();
		break;
	}
}

void cpu_execute_op() {
	t_cpu_opcode handler = cpu_opcode_vectors[cpu_op];
	handler();
}

/**
 * Read modes
 */
unsigned char cpu_read_zp() {
	return bus_read(cpu_op_args[0]);
}

unsigned char cpu_read_zpx() {
	return bus_read((cpu_op_args[0] + cpu_x) & 0xff);
}

unsigned char cpu_read_zpy() {
	return bus_read((cpu_op_args[0] + cpu_y) & 0xff);
}

unsigned char cpu_read_izx() {
	unsigned char lo = bus_read((cpu_op_args[0] + cpu_x) & 0xff);
	unsigned char hi = bus_read((cpu_op_args[0] + cpu_x + 1) & 0xff);
	return bus_read(lo + (hi << 8));
}

unsigned char cpu_read_izy() {
	unsigned char lo = bus_read(cpu_op_args[0]);
	unsigned char hi = bus_read((cpu_op_args[0] + 1) & 0xff);
	return bus_read(lo + (hi << 8) + cpu_y);
}

unsigned char cpu_read_abs() {
	return bus_read(cpu_op_args[0] + (cpu_op_args[1] << 8));
}

unsigned char cpu_read_abx() {
	return bus_read(cpu_op_args[0] + (cpu_op_args[1] << 8) + cpu_x);
}

unsigned char cpu_read_aby() {
	return bus_read(cpu_op_args[0] + (cpu_op_args[1] << 8) + cpu_y);
}

unsigned short cpu_read_ind() {
	auto lo = cpu_op_args[0];
	auto hi = cpu_op_args[1];
	// indirect jmp 6502/6510 bug (feature?) lo byte wraps around
	unsigned short addrlo = lo + (hi << 8);
	unsigned short addrhi = ((lo + 1) & 0xff) + (hi << 8);
	return bus_read(addrlo) + (bus_read(addrhi) << 8);
}

/**
 * Write modes
 */
void cpu_write_zp(unsigned char data) {
	bus_write(cpu_op_args[0], data);
}

void cpu_write_zpx(unsigned char data) {
	bus_write((cpu_op_args[0] + cpu_x) & 0xff, data);
}

void cpu_write_zpy(unsigned char data) {
	bus_write((cpu_op_args[0] + cpu_y) & 0xff, data);
}

void cpu_write_izx(unsigned char data) {
	unsigned char lo = bus_read((cpu_op_args[0] + cpu_x) & 0xff);
	unsigned char hi = bus_read((cpu_op_args[0] + cpu_x + 1) & 0xff);
	bus_write(lo + (hi << 8), data);
}

void cpu_write_izy(unsigned char data) {
	unsigned char lo = bus_read(cpu_op_args[0]);
	unsigned char hi = bus_read((cpu_op_args[0] + 1) & 0xff);
	bus_write(lo + (hi << 8) + cpu_y, data);
}

void cpu_write_abs(unsigned char data) {
	bus_write(cpu_op_args[0] + (cpu_op_args[1] << 8), data);
}

void cpu_write_abx(unsigned char data) {
	bus_write(cpu_op_args[0] + (cpu_op_args[1] << 8) + cpu_x, data);
}

void cpu_write_aby(unsigned char data) {
	bus_write(cpu_op_args[0] + (cpu_op_args[1] << 8) + cpu_y, data);
}

/**
 * Stack operations
 */
void cpu_stack_push(unsigned char data) {
	bus_write(0x0100 | cpu_stack, data);
	--cpu_stack;
}

unsigned char cpu_stack_pop() {
	++cpu_stack;
	return bus_read(0x0100 | cpu_stack);
}

/**
 * 6510 Opcodes
 */
void cpu_adc() {
	unsigned char arg;
	switch (cpu_op) {
		case 0x69: arg = cpu_op_args[0]; break;
		case 0x65: arg = cpu_read_zp(); break;
		case 0x75: arg = cpu_read_zpx(); break;
		case 0x6D: arg = cpu_read_abs(); break;
		case 0x7D: arg = cpu_read_abx(); break;
		case 0x79: arg = cpu_read_aby(); break;
		case 0x61: arg = cpu_read_izx(); break;
		case 0x71: arg = cpu_read_izy(); break;
		default: cpu_kil();
	}

	unsigned result = cpu_acc + arg + (cpu_p & CPU_STAT_CARRY ? 1 : 0);

	SETCARRYIF(result & 0x100);
	SETOVERFLOW(((cpu_acc ^ result) & (arg ^ result) & 0x80) != 0);
	cpu_acc = result & 0xff;
	SETZN(cpu_acc);
}

void cpu_and() {
	switch (cpu_op) {
		case 0x29: cpu_acc &= cpu_op_args[0]; break;
		case 0x25: cpu_acc &= cpu_read_zp(); break;
		case 0x35: cpu_acc &= cpu_read_zpx(); break;
		case 0x2D: cpu_acc &= cpu_read_abs(); break;
		case 0x3D: cpu_acc &= cpu_read_abx(); break;
		case 0x39: cpu_acc &= cpu_read_aby(); break;
		case 0x21: cpu_acc &= cpu_read_izx(); break;
		case 0x31: cpu_acc &= cpu_read_izy(); break;
		default: cpu_kil();
	}

	SETZN(cpu_acc);
}

void cpu_asl() {
	unsigned char arg;
	if (cpu_op == 0x0A) {
		arg = cpu_acc;
	}
	else if (cpu_op == 0x06) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0x16) {
		arg = cpu_read_zpx();
	}
	else if (cpu_op == 0x0E) {
		arg = cpu_read_abs();
	}
	else if (cpu_op == 0x1E) {
		arg = cpu_read_abx();
	}
	else cpu_kil();

	if (arg & 0x80) {
		cpu_p |= CPU_STAT_CARRY;
	}
	else {
		cpu_p &= ~CPU_STAT_CARRY;
	}

	arg <<= 1;

	SETZN(arg);

	if (cpu_op == 0x0A) {
		cpu_acc = arg;
	}
	else if (cpu_op == 0x06) {
		cpu_write_zp(arg);
	}
	else if (cpu_op == 0x16) {
		cpu_write_zpx(arg);
	}
	else if (cpu_op == 0x0E) {
		cpu_write_abs(arg);
	}
	else if (cpu_op == 0x1E) {
		cpu_write_abx(arg);
	}
	else cpu_kil();
}

void cpu_bcc() {
	if (!(cpu_p & CPU_STAT_CARRY)) {
		cpu_pc += RELATIVE(cpu_op_args[0]);
	}
}

void cpu_bcs() {
	if (cpu_p & CPU_STAT_CARRY) {
		cpu_pc += RELATIVE(cpu_op_args[0]);
	}
}

void cpu_beq() {
	if (cpu_p & CPU_STAT_ZERO) {
		cpu_pc += RELATIVE(cpu_op_args[0]);
	}
}

void cpu_bit() {
	unsigned char arg;
	if (cpu_op == 0x24) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0x2C) {
		arg = cpu_read_abs();
	}

	SETNEGATIVE(arg);
	SETOVERFLOW(arg & 0x40);
	SETZERO(cpu_acc & arg);
}

void cpu_bmi() {
	if (cpu_p & CPU_STAT_NEGATIVE) {
		cpu_pc += RELATIVE(cpu_op_args[0]);
	}
}

void cpu_bne() {
	if (!(cpu_p & CPU_STAT_ZERO)) {
		cpu_pc += RELATIVE(cpu_op_args[0]);
	}
}

void cpu_bpl() {
	if (!(cpu_p & CPU_STAT_NEGATIVE)) {
		cpu_pc += RELATIVE(cpu_op_args[0]);
	}
}

void cpu_brk() {
	cpu_pc++;
	cpu_stack_push((cpu_pc >> 8) & 0xff);
	cpu_stack_push(cpu_pc & 0xFF);
	cpu_p |= CPU_STAT_BREAK;
	cpu_stack_push(cpu_p);
	cpu_p |= CPU_STAT_IRQ_DISABLE;

	cpu_pc = bus_read(0xFFFE) | (bus_read(0xFFFF) << 8);
}

void cpu_bvc() {
	if (!(cpu_p & CPU_STAT_OVERFLOW)) {
		cpu_pc += RELATIVE(cpu_op_args[0]);
	}
}

void cpu_bvs() {
	if (cpu_p & CPU_STAT_OVERFLOW) {
		cpu_pc += RELATIVE(cpu_op_args[0]);
	}
}

void cpu_clc() {
	SETCARRYIF(false);
}

void cpu_cld() {
	cpu_p &= ~CPU_STAT_DEC;
}

void cpu_cli() {
	cpu_p &= ~CPU_STAT_IRQ_DISABLE;
}

void cpu_clv() {
	SETOVERFLOW(false);
}

void cpu_cmp() {
	unsigned char arg;
	if (cpu_op == 0xC9) {
		arg = cpu_op_args[0];
	}
	else if (cpu_op == 0xC5) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0xD5) {
		arg = cpu_read_zpx();
	}
	else if (cpu_op == 0xCD) {
		arg = cpu_read_abs();
	}
	else if (cpu_op == 0xDD) {
		arg = cpu_read_abx();
	}
	else if (cpu_op == 0xD9) {
		arg = cpu_read_aby();
	}
	else if (cpu_op == 0xC1) {
		arg = cpu_read_izx();
	}
	else if (cpu_op == 0xD1) {
		arg = cpu_read_izy();
	}
	else cpu_kil();

	unsigned int result = cpu_acc - arg;

	if (result < 0x100) {
		cpu_p |= CPU_STAT_CARRY;
	}
	else {
		cpu_p &= ~CPU_STAT_CARRY;
	}

	SETZN(result & 0xff);
}

void cpu_cpx() {
	unsigned char arg;
	if (cpu_op == 0xE0) {
		arg = cpu_op_args[0];
	}
	else if (cpu_op == 0xE4) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0xEC) {
		arg = cpu_read_abs();
	}
	else cpu_kil();

	unsigned int result = cpu_x - arg;
	
	if (result < 0x100) {
		cpu_p |= CPU_STAT_CARRY;
	}
	else {
		cpu_p &= ~CPU_STAT_CARRY;
	}

	SETZN(result & 0xff);
}

void cpu_cpy() {
	unsigned char arg;
	if (cpu_op == 0xC0) {
		arg = cpu_op_args[0];
	}
	else if (cpu_op == 0xC4) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0xCC) {
		arg = cpu_read_abs();
	}
	else cpu_kil();

	unsigned int result = cpu_y - arg;

	if (result < 0x100) {
		cpu_p |= CPU_STAT_CARRY;
	}
	else {
		cpu_p &= ~CPU_STAT_CARRY;
	}

	SETZN(result & 0xff);
}

void cpu_dec() {
	unsigned char arg;
	if (cpu_op == 0xC6) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0xD6) {
		arg = cpu_read_zpx();
	}
	else if (cpu_op == 0xCE) {
		arg = cpu_read_abs();
	}
	else if (cpu_op == 0xDE) {
		arg = cpu_read_abx();
	}
	else cpu_kil();

	--arg;

	SETZN(arg)

	if (cpu_op == 0xC6) {
		cpu_write_zp(arg);
	}
	else if (cpu_op == 0xD6) {
		cpu_write_zpx(arg);
	}
	else if (cpu_op == 0xCE) {
		cpu_write_abs(arg);
	}
	else if (cpu_op == 0xDE) {
		cpu_write_abx(arg);
	}
	else cpu_kil();
}

void cpu_dex() {
	--cpu_x;
	SETZN(cpu_x);
}

void cpu_dey() {
	--cpu_y;
	SETZN(cpu_y);
}

void cpu_eor() {
	unsigned char arg;
	if (cpu_op == 0x49) {
		arg = cpu_op_args[0];
	}
	else if (cpu_op == 0x45) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0x55) {
		arg = cpu_read_zpx();
	}
	else if (cpu_op == 0x4D) {
		arg = cpu_read_abs();
	}
	else if (cpu_op == 0x5D) {
		arg = cpu_read_abx();
	}
	else if (cpu_op == 0x59) {
		arg = cpu_read_aby();
	}
	else if (cpu_op == 0x41) {
		arg = cpu_read_izx();
	}
	else if (cpu_op == 0x51) {
		arg = cpu_read_izy();
	}
	else cpu_kil();

	cpu_acc ^= arg;
	SETZN(cpu_acc);
}

void cpu_inc() {
	unsigned char arg;
	if (cpu_op == 0xE6) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0xF6) {
		arg = cpu_read_zpx();
	}
	else if (cpu_op == 0xEE) {
		arg = cpu_read_abs();
	}
	else if (cpu_op == 0xFE) {
		arg = cpu_read_abx();
	}
	else cpu_kil();

	++arg;

	SETZN(arg);

	if (cpu_op == 0xE6) {
		cpu_write_zp(arg);
	}
	else if (cpu_op == 0xF6) {
		cpu_write_zpx(arg);
	}
	else if (cpu_op == 0xEE) {
		cpu_write_abs(arg);
	}
	else if (cpu_op == 0xFE) {
		cpu_write_abx(arg);
	}
	else cpu_kil();
}

void cpu_inx() {
	++cpu_x;
	SETZN(cpu_x);
}

void cpu_iny() {
	++cpu_y;
	SETZN(cpu_y);
}

void cpu_jmp() {
	if (cpu_op == 0x4C) {
		cpu_pc = cpu_op_args[0] + (cpu_op_args[1] << 8);
	}
	else if (cpu_op == 0x6C) {
		cpu_pc = cpu_read_ind();
	}
}

void cpu_jsr() {
	cpu_pc--;
	cpu_stack_push((cpu_pc >> 8) & 0xff);
	cpu_stack_push(cpu_pc & 0xFF);
	cpu_pc = cpu_op_args[0] + (cpu_op_args[1] << 8);
}

void cpu_lda() {
	unsigned char arg;
	if (cpu_op == 0xA9) {
		arg = cpu_op_args[0];
	}
	else if (cpu_op == 0xA5) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0xB5) {
		arg = cpu_read_zpx();
	}
	else if (cpu_op == 0xAD) {
		arg = cpu_read_abs();
	}
	else if (cpu_op == 0xBD) {
		arg = cpu_read_abx();
	}
	else if (cpu_op == 0xB9) {
		arg = cpu_read_aby();
	}
	else if (cpu_op == 0xA1) {
		arg = cpu_read_izx();
	}
	else if (cpu_op == 0xB1) {
		arg = cpu_read_izy();
	}
	else cpu_kil();

	cpu_acc = arg;
	
	SETZN(cpu_acc);
}

void cpu_ldx() {
	unsigned char arg;
	if (cpu_op == 0xA2) {
		arg = cpu_op_args[0];
	}
	else if (cpu_op == 0xA6) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0xB6) {
		arg = cpu_read_zpy();
	}
	else if (cpu_op == 0xAE) {
		arg = cpu_read_abs();
	}
	else if (cpu_op == 0xBE) {
		arg = cpu_read_aby();
	}
	else cpu_kil();

	cpu_x = arg;
	SETZN(cpu_x);
}

void cpu_ldy() {
	unsigned char arg;
	if (cpu_op == 0xA0) {
		arg = cpu_op_args[0];
	}
	else if (cpu_op == 0xA4) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0xB4) {
		arg = cpu_read_zpx();
	}
	else if (cpu_op == 0xAC) {
		arg = cpu_read_abs();
	}
	else if (cpu_op == 0xBC) {
		arg = cpu_read_abx();
	}
	else cpu_kil();

	cpu_y = arg;
	SETZN(cpu_y);
}

void cpu_lsr() {
	unsigned char arg;
	if (cpu_op == 0x4A) {
		arg = cpu_acc;
	}
	else if (cpu_op == 0x46) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0x56) {
		arg = cpu_read_zpx();
	}
	else if (cpu_op == 0x4E) {
		arg = cpu_read_abs();
	}
	else if (cpu_op == 0x5E) {
		arg = cpu_read_abx();
	}
	else cpu_kil();

	if (arg & 0x01) {
		cpu_p |= CPU_STAT_CARRY;
	}
	else {
		cpu_p &= ~CPU_STAT_CARRY;
	}

	arg >>= 1;

	SETZN(arg);

	if (cpu_op == 0x4A) {
		cpu_acc = arg;
	}
	else if (cpu_op == 0x46) {
		cpu_write_zp(arg);
	}
	else if (cpu_op == 0x56) {
		cpu_write_zpx(arg);
	}
	else if (cpu_op == 0x4E) {
		cpu_write_abs(arg);
	}
	else if (cpu_op == 0x5E) {
		cpu_write_abx(arg);
	}
	else cpu_kil();
}

void cpu_nop() {
}

void cpu_ora() {
	
	unsigned char arg;
	switch (cpu_op) {
		case 0x09: cpu_acc |= cpu_op_args[0]; break;
		case 0x05: cpu_acc |= cpu_read_zp(); break;
		case 0x15: cpu_acc |= cpu_read_zpx(); break;
		case 0x0D: cpu_acc |= cpu_read_abs(); break;
		case 0x1D: cpu_acc |= cpu_read_abx(); break;
		case 0x19: cpu_acc |= cpu_read_aby(); break;
		case 0x01: cpu_acc |= cpu_read_izx(); break;
		case 0x11: cpu_acc |= cpu_read_izy(); break;
		default: cpu_kil();
	}

	SETZN(cpu_acc);
}

void cpu_pha() {
	cpu_stack_push(cpu_acc);
}

void cpu_php() {
	cpu_stack_push(cpu_p);
}

void cpu_pla() {
	cpu_acc = cpu_stack_pop();
	SETZN(cpu_acc);
}

void cpu_plp() {
	cpu_p = cpu_stack_pop();
}

void cpu_rol() {
	unsigned char arg;
	if (cpu_op == 0x2A) {
		arg = cpu_acc;
	}
	else if (cpu_op == 0x26) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0x36) {
		arg = cpu_read_zpx();
	}
	else if (cpu_op == 0x2E) {
		arg = cpu_read_abs();
	}
	else if (cpu_op == 0x3E) {
		arg = cpu_read_abx();
	}
	else cpu_kil();

	bool newcarry = (arg & 0x80) != 0;

	arg <<= 1;

	if (cpu_p & CPU_STAT_CARRY) {
		arg |= 0x01;
	}

	if (newcarry) {
		cpu_p |= CPU_STAT_CARRY;
	}
	else {
		cpu_p &= ~CPU_STAT_CARRY;
	}

	SETZN(arg);

	if (cpu_op == 0x2A) {
		cpu_acc = arg;
	}
	else if (cpu_op == 0x26) {
		cpu_write_zp(arg);
	}
	else if (cpu_op == 0x36) {
		cpu_write_zpx(arg);
	}
	else if (cpu_op == 0x2E) {
		cpu_write_abs(arg);
	}
	else if (cpu_op == 0x3E) {
		cpu_write_abx(arg);
	}
	else cpu_kil();
}

void cpu_ror() {
	unsigned char arg;
	switch (cpu_op) {
		case 0x6A: arg = cpu_acc; break;
		case 0x66: arg = cpu_read_zp(); break;
		case 0x76: arg = cpu_read_zpx(); break;
		case 0x6E: arg = cpu_read_abs(); break;
		case 0x7E: arg = cpu_read_abx(); break;
		default: cpu_kil();
	}

	bool newcarry = (arg & 0x01) != 0;

	arg >>= 1;

	if (cpu_p & CPU_STAT_CARRY) {
		arg |= 0x80;
	}

	if (newcarry) {
		cpu_p |= CPU_STAT_CARRY;
	}
	else {
		cpu_p &= ~CPU_STAT_CARRY;
	}

	SETZN(arg);

	switch (cpu_op) {
		case 0x6A: cpu_acc = arg; break;
		case 0x66: cpu_write_zp(arg); break;
		case 0x76: cpu_write_zpx(arg); break;
		case 0x6E: cpu_write_abs(arg); break;
		case 0x7E: cpu_write_abx(arg); break;
		default: cpu_kil();
	}
}

void cpu_rti() {
	cpu_p = cpu_stack_pop();
	unsigned char lo = cpu_stack_pop();
	unsigned char hi = cpu_stack_pop();
	cpu_pc = lo + (hi << 8);
}

void cpu_rts() {
	unsigned char lo = cpu_stack_pop();
	unsigned char hi = cpu_stack_pop();
	cpu_pc = lo + (hi << 8) + 1;
}

void cpu_sbc() {
	unsigned char arg;
	switch (cpu_op) {
		case 0xE9:
		case 0xEB: arg = cpu_op_args[0]; break;
		case 0xE5: arg = cpu_read_zp(); break;
		case 0xF5: arg = cpu_read_zpx(); break;
		case 0xED: arg = cpu_read_abs(); break;
		case 0xFD: arg = cpu_read_abx(); break;
		case 0xF9: arg = cpu_read_aby(); break;
		case 0xE1: arg = cpu_read_izx(); break;
		case 0xF1: arg = cpu_read_izy(); break;
		default: cpu_kil();
	}

	unsigned int result = cpu_acc - arg - ((cpu_p & CPU_STAT_CARRY) ? 0 : 1);
	SETOVERFLOW(((cpu_acc ^ result) & (cpu_acc ^ arg) & 0x80) != 0);
	
	SETCARRYIF(!(result & 0x100));
	cpu_acc = result & 0xff;
	SETZN(cpu_acc)
}

void cpu_sec() {
	SETCARRYIF(true);
}

void cpu_sed() {
	cpu_p |= CPU_STAT_DEC;
}

void cpu_sei() {
	cpu_p |= CPU_STAT_IRQ_DISABLE;
}

void cpu_sta() {
	switch (cpu_op) {
		case 0x85: cpu_write_zp(cpu_acc); break;
		case 0x95: cpu_write_zpx(cpu_acc); break;
		case 0x8D: cpu_write_abs(cpu_acc); break;
		case 0x9D: cpu_write_abx(cpu_acc); break;
		case 0x99: cpu_write_aby(cpu_acc); break;
		case 0x81: cpu_write_izx(cpu_acc); break;
		case 0x91: cpu_write_izy(cpu_acc); break;
		default: cpu_kil();
	}
}

void cpu_stx() {
	switch (cpu_op) {
		case 0x86: cpu_write_zp(cpu_x); break;
		case 0x96: cpu_write_zpy(cpu_x); break;
		case 0x8E: cpu_write_abs(cpu_x); break;
		default: cpu_kil();
	}
}

void cpu_sty() {
	switch (cpu_op) {
		case 0x84: cpu_write_zp(cpu_y); break;
		case 0x94: cpu_write_zpx(cpu_y); break;
		case 0x8C: cpu_write_abs(cpu_y); break;
		default: cpu_kil();
	}
}

void cpu_tax() {
	cpu_x = cpu_acc;
	SETZN(cpu_acc);
}

void cpu_tay() {
	cpu_y = cpu_acc;
	SETZN(cpu_acc)
}

void cpu_tsx() {
	cpu_x = cpu_stack;
	SETZN(cpu_x);
}

void cpu_txa() {
	cpu_acc = cpu_x;
	SETZN(cpu_x);
}

void cpu_txs() {
	cpu_stack = cpu_x;
}

void cpu_tya() {
	cpu_acc = cpu_y;
	SETZN(cpu_acc);
}

/**
 * Illegal opcodes
 */
void cpu_kil() {
	g_stop=true;
}

void cpu_slo() {
	cpu_kil();
}

void cpu_rla() {
	cpu_kil();
}

void cpu_sre() {
	cpu_kil();
}

void cpu_rra() {
	cpu_kil();
}

void cpu_sax() {
	cpu_kil();
}

void cpu_lax() {
	cpu_kil();
}

void cpu_dcp() {
	cpu_kil();
}

void cpu_isc() {
	cpu_kil();
}

void cpu_anc() {
	cpu_kil();
}

void cpu_alr() {
	cpu_kil();
}

void cpu_arr() {
	cpu_kil();
}

void cpu_xaa() {
	cpu_kil();
}

void cpu_axs() {
	cpu_kil();
}

void cpu_ahx() {
	cpu_kil();
}

void cpu_shy() {
	cpu_kil();
}

void cpu_shx() {
	cpu_kil();
}

void cpu_tas() {
	cpu_kil();
}

void cpu_las() {
	cpu_kil();
}
