#include "memory.h"

unsigned char cpu_x = 0;
unsigned char cpu_y = 0;
unsigned char cpu_acc = 0;
unsigned char cpu_stack = 0;
unsigned short cpu_pc = 0;
int cpu_irq = 0;
int cpu_nmi = 0;

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
unsigned char cpu_p = 0x0;

#define CPU_CYCLE_FETCH_OP 1
#define CPU_CYCLE_FETCH_ARG_SINGLE 2
#define CPU_CYCLE_FETCH_ARG_DOUBLE_1 3
#define CPU_CYCLE_FETCH_ARG_DOUBLE_2 4
#define CPU_CYCLE_EXECUTE_OP 5
unsigned char cpu_cycle = CPU_CYCLE_FETCH_OP;

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

void cpu_stack_push(char);
char cpu_stack_pop();

char cpu_op_width[] = {
/*     x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF */
/*0x*/ 1, 2, 0, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/*1x*/ 2, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/*2x*/ 3, 2, 0, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/*3x*/ 2, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/*4x*/ 1, 2, 0, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/*5x*/ 2, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/*6x*/ 1, 2, 0, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/*7x*/ 2, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/*8x*/ 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/*9x*/ 2, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/*Ax*/ 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/*Bx*/ 2, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/*Cx*/ 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/*Dx*/ 2, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/*Ex*/ 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/*Fx*/ 2, 2, 0, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3
};

typedef void(*t_cpu_opcode)();

t_cpu_opcode cpu_opcode_vectors[] = {
/*      x0       x1       x2       x3       x4       x5       x6       x7       x8       x9       xA       xB       xC       xD       xE       xF   */
/*0x*/cpu_brk, cpu_ora, cpu_kil, cpu_slo, cpu_nop, cpu_ora, cpu_asl, cpu_slo, cpu_php, cpu_ora, cpu_asl, cpu_anc, cpu_nop, cpu_ora, cpu_asl, cpu_slo,
/*1x*/cpu_bpl, cpu_ora, cpu_kil, cpu_slo, cpu_nop, cpu_ora, cpu_asl, cpu_slo, cpu_clc, cpu_ora, cpu_nop, cpu_slo, cpu_nop, cpu_ora, cpu_asl, cpu_slo,
/*2x*/cpu_jsr, cpu_and, cpu_kil, cpu_rla, cpu_bit, cpu_and, cpu_rol, cpu_rla, cpu_plp, cpu_and, cpu_rol, cpu_anc, cpu_bit, cpu_and, cpu_rol, cpu_rla,
/*3x*/cpu_bmi, cpu_and, cpu_kil, cpu_rla, cpu_nop, cpu_and, cpu_rol, cpu_rla, cpu_sec, cpu_and, cpu_nop, cpu_rla, cpu_nop, cpu_and, cpu_rol, cpu_rla,
/*4x*/cpu_rti, cpu_eor, cpu_kil, cpu_sre, cpu_nop, cpu_eor, cpu_lsr, cpu_sre, cpu_pha, cpu_eor, cpu_lsr, cpu_alr, cpu_jmp, cpu_eor, cpu_lsr, cpu_sre,
/*5x*/cpu_bvc, cpu_eor, cpu_kil, cpu_sre, cpu_nop, cpu_eor, cpu_lsr, cpu_sre, cpu_cli, cpu_eor, cpu_nop, cpu_sre, cpu_nop, cpu_eor, cpu_lsr, cpu_sre,
/*6x*/cpu_rts, cpu_adc, cpu_kil, cpu_rra, cpu_nop, cpu_adc, cpu_ror, cpu_rra, cpu_pla, cpu_adc, cpu_ror, cpu_arr, cpu_jmp, cpu_adc, cpu_ror, cpu_rra,
/*7x*/cpu_bvs, cpu_adc, cpu_kil, cpu_rra, cpu_nop, cpu_adc, cpu_ror, cpu_rra, cpu_sei, cpu_adc, cpu_nop, cpu_rra, cpu_nop, cpu_adc, cpu_ror, cpu_rra,
/*8x*/cpu_nop, cpu_sta, cpu_nop, cpu_sax, cpu_sty, cpu_sta, cpu_stx, cpu_sax, cpu_dey, cpu_nop, cpu_txa, cpu_xaa, cpu_sty, cpu_sta, cpu_stx, cpu_sax,
/*9x*/cpu_bcc, cpu_sta, cpu_kil, cpu_ahx, cpu_sty, cpu_sta, cpu_stx, cpu_sax, cpu_tya, cpu_sta, cpu_txs, cpu_tas, cpu_shy, cpu_sta, cpu_shx, cpu_ahx,
/*Ax*/cpu_ldy, cpu_lda, cpu_ldx, cpu_lax, cpu_ldy, cpu_lda, cpu_ldx, cpu_lax, cpu_tay, cpu_lda, cpu_tax, cpu_lax, cpu_ldy, cpu_lda, cpu_ldx, cpu_lax,
/*Bx*/cpu_bcs, cpu_lda, cpu_kil, cpu_lax, cpu_ldy, cpu_lda, cpu_ldx, cpu_lax, cpu_clv, cpu_lda, cpu_tsx, cpu_las, cpu_ldy, cpu_lda, cpu_ldx, cpu_lax,
/*Cx*/cpu_cpy, cpu_cmp, cpu_nop, cpu_dcp, cpu_cpy, cpu_cmp, cpu_dec, cpu_dcp, cpu_iny, cpu_cmp, cpu_dex, cpu_axs, cpu_cpy, cpu_cmp, cpu_dec, cpu_dcp,
/*Dx*/cpu_bne, cpu_cmp, cpu_kil, cpu_dcp, cpu_nop, cpu_cmp, cpu_dec, cpu_dcp, cpu_cld, cpu_cmp, cpu_nop, cpu_dcp, cpu_nop, cpu_cmp, cpu_dec, cpu_dcp,
/*Ex*/cpu_cpx, cpu_sbc, cpu_nop, cpu_isc, cpu_cpx, cpu_sbc, cpu_inc, cpu_isc, cpu_inx, cpu_sbc, cpu_nop, cpu_sbc, cpu_cpx, cpu_sbc, cpu_inc, cpu_isc,
/*Fx*/cpu_beq, cpu_sbc, cpu_kil, cpu_isc, cpu_nop, cpu_sbc, cpu_inc, cpu_isc, cpu_sed, cpu_sbc, cpu_nop, cpu_isc, cpu_nop, cpu_sbc, cpu_inc, cpu_isc
};

unsigned int cpu_cycle_count;
void cpu_init() {
	cpu_cycle_count = 0;
	// init settings
	mem_write(0x00, 47);
	mem_write(0x01, 55);
	// read program counter
	unsigned char lo = mem_read(0xFFFC);
	unsigned char hi = mem_read(0xFFFD);
	cpu_pc = lo + (hi<<8);
	cpu_cycle = CPU_CYCLE_FETCH_OP;
	cpu_stack = 0xFF;
}

void cpu_tick() {
	cpu_cycle_count++;
	switch(cpu_cycle) {
	case CPU_CYCLE_FETCH_OP:
		if (cpu_nmi) {
			cpu_p |= CPU_STAT_IRQ_DISABLE;
			cpu_p &= ~CPU_STAT_BREAK;

			cpu_stack_push(cpu_pc & 0xFF);
			cpu_stack_push(cpu_pc >> 8);
			cpu_stack_push(cpu_p);

			cpu_pc = mem_read(0xFFFA) + (mem_read(0xFFFB) << 8);
		}
		else if (cpu_irq && !(cpu_p & CPU_STAT_IRQ_DISABLE)) {
			cpu_p |= CPU_STAT_IRQ_DISABLE;
			cpu_p &= ~CPU_STAT_BREAK;

			cpu_stack_push(cpu_pc & 0xFF);
			cpu_stack_push(cpu_pc >> 8);
			cpu_stack_push(cpu_p);

			cpu_pc = mem_read(0xFFFE) + (mem_read(0xFFFF) << 8);
		}
		else {
			cpu_op = mem_read(cpu_pc++);
			cpu_prepare_op();
		}
		break;
	case CPU_CYCLE_FETCH_ARG_SINGLE:
		cpu_op_args[0] = mem_read(cpu_pc++);
		cpu_cycle = CPU_CYCLE_EXECUTE_OP;
		break;
	case CPU_CYCLE_FETCH_ARG_DOUBLE_1:
		cpu_op_args[0] = mem_read(cpu_pc++);
		cpu_cycle = CPU_CYCLE_FETCH_ARG_DOUBLE_2;
		break;
	case CPU_CYCLE_FETCH_ARG_DOUBLE_2:
		cpu_op_args[1] = mem_read(cpu_pc++);
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
char cpu_read_zp() {
	return mem_read(cpu_op_args[0]);
}

char cpu_read_zpx() {
	return mem_read(cpu_op_args[0] + cpu_x & 0xFF);
}

char cpu_read_zpy() {
	return mem_read(cpu_op_args[0] + cpu_y & 0xFF);
}

char cpu_read_izx() {
	unsigned char lo = mem_read(cpu_op_args[0] + cpu_x & 0xFF);
	unsigned char hi = mem_read(cpu_op_args[0] + cpu_x + 1 & 0xFF);
	return mem_read(lo + (hi<<8));
}

char cpu_read_izy() {
	unsigned char lo = mem_read(cpu_op_args[0]);
	unsigned char hi = mem_read(cpu_op_args[0] + 1 & 0xFF);
	return mem_read(lo + (hi<<8) + cpu_y & 0xFFFF);
}

char cpu_read_abs() {
	return mem_read(cpu_op_args[0] + (cpu_op_args[1]<<8));
}

char cpu_read_abx() {
	return mem_read(cpu_op_args[0] + (cpu_op_args[1]<<8) + cpu_x & 0xFFFF);
}

char cpu_read_aby() {
	return mem_read(cpu_op_args[0] + (cpu_op_args[1]<<8) + cpu_y);
}

short cpu_read_ind() {
	unsigned char lo = mem_read(cpu_op_args[0] + (cpu_op_args[1]<<8));
	unsigned char hi = mem_read(((cpu_op_args[0]+1) & 0xFF) + (cpu_op_args[1]<<8)); // indirect jmp 6502/6510 bug (feature?)
	return lo + (hi<<8);
}

/**
 * Write modes
 */
void cpu_write_zp(char data) {
	mem_write(cpu_op_args[0], data);
}

void cpu_write_zpx(char data) {
	mem_write((cpu_op_args[0] + cpu_x) & 0xFF, data);
}

void cpu_write_zpy(char data) {
	mem_write((cpu_op_args[0] + cpu_y) & 0xFF, data);
}

void cpu_write_izx(char data) {
	unsigned char lo = mem_read((cpu_op_args[0] + cpu_x) & 0xFF);
	unsigned char hi = mem_read((cpu_op_args[0] + cpu_x + 1) & 0xFF);
	mem_write(lo + (hi<<8), data);
}

void cpu_write_izy(char data) {
	unsigned char lo = mem_read(cpu_op_args[0]);
	unsigned char hi = mem_read((cpu_op_args[0] + 1) & 0xFF);
	mem_write(lo + (hi<<8) + cpu_y, data);
}

void cpu_write_abs(char data) {
	mem_write(cpu_op_args[0] + (cpu_op_args[1]<<8), data);
}

void cpu_write_abx(char data) {
	mem_write(cpu_op_args[0] + (cpu_op_args[1]<<8) + cpu_x, data);
}

void cpu_write_aby(char data) {
	mem_write(cpu_op_args[0] + (cpu_op_args[1]<<8) + cpu_y, data);
}

void cpu_write_ind(char data) {
	unsigned char lo = mem_read(cpu_op_args[0] + (cpu_op_args[1]<<8));
	unsigned char hi = mem_read((cpu_op_args[0]+1) & 0xFF + (cpu_op_args[1]<<8));
	mem_write(lo + (hi<<8), data);
}

/**
 * Stack operations
 */
void cpu_stack_push(char data) {
	mem_write(0x0100 + cpu_stack, data);
	--cpu_stack;
}

char cpu_stack_pop() {
	++cpu_stack;
	return mem_read(0x0100 + cpu_stack);
}

/**
 * 6510 Opcodes
 */
void cpu_adc() {
	unsigned char arg;
	if (cpu_op == 0x69) {
		arg = cpu_op_args[0];
	}
	else if (cpu_op == 0x65) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0x75) {
		arg = cpu_read_zpx();
	}
	else if (cpu_op == 0x6D) {
		arg = cpu_read_abs();
	}
	else if (cpu_op == 0x7D) {
		arg = cpu_read_abx();
	}
	else if (cpu_op == 0x79) {
		arg = cpu_read_aby();
	}
	else if (cpu_op == 0x61) {
		arg = cpu_read_izx();
	}
	else if (cpu_op == 0x71) {
		arg = cpu_read_izy();
	}

	unsigned short result = cpu_acc + arg + (cpu_p && CPU_STAT_CARRY ? 1 : 0);
	cpu_acc = result;

	if (result & 0x100) {
		cpu_p |= CPU_STAT_CARRY;
	}
	else {
		cpu_p &= ~CPU_STAT_CARRY;
	}

	if (result > 127 || result < -128) {
		cpu_p |= CPU_STAT_OVERFLOW;
	}
	else {
		cpu_p &= ~CPU_STAT_OVERFLOW;
	}
	
	if (cpu_acc) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}

	if (cpu_acc & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
}

void cpu_and() {
	unsigned char arg;
	if (cpu_op == 0x29) {
		arg = cpu_op_args[0];
	}
	else if (cpu_op == 0x25) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0x35) {
		arg = cpu_read_zpx();
	}
	else if (cpu_op == 0x2D) {
		arg = cpu_read_abs();
	}
	else if (cpu_op == 0x3D) {
		arg = cpu_read_abx();
	}
	else if (cpu_op == 0x39) {
		arg = cpu_read_aby();
	}
	else if (cpu_op == 0x21) {
		arg = cpu_read_izx();
	}
	else if (cpu_op == 0x31) {
		arg = cpu_read_izy();
	}

	cpu_acc &= arg;
	if (cpu_acc) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
	if (cpu_acc & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
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

	if (arg & 0x80) {
		cpu_p |= CPU_STAT_CARRY;
	}
	else {
		cpu_p &= ~CPU_STAT_CARRY;
	}

	arg <<= 1;

	if (arg) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
	if (arg & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}

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
}

void cpu_bcc() {
	if (!(cpu_p & CPU_STAT_CARRY)) {
		cpu_pc += (char)cpu_op_args[0];
	}
}

void cpu_bcs() {
	if (cpu_p & CPU_STAT_CARRY) {
		cpu_pc += (char)cpu_op_args[0];
	}
}

void cpu_beq() {
	if (cpu_p & CPU_STAT_ZERO) {
		cpu_pc += (char)cpu_op_args[0];
	}
}

void cpu_bit() {
	char arg;
	if (cpu_op == 0x24) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0x2C) {
		arg = cpu_read_abs();
	}

	char result = cpu_acc & arg;

	if (result & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}

	if (result & 0x40) {
		cpu_p |= CPU_STAT_OVERFLOW;
	}
	else {
		cpu_p &= ~CPU_STAT_OVERFLOW;
	}

	if (result) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
}

void cpu_bmi() {
	if (cpu_p & CPU_STAT_NEGATIVE) {
		cpu_pc += (char)cpu_op_args[0];
	}
}

void cpu_bne() {
	if (!(cpu_p & CPU_STAT_ZERO)) {
		cpu_pc += (char)cpu_op_args[0];
	}
}

void cpu_bpl() {
	if (!(cpu_p & CPU_STAT_NEGATIVE)) {
		cpu_pc += (char)cpu_op_args[0];
	}
}

void cpu_brk() {
	cpu_p |= CPU_STAT_BREAK | CPU_STAT_IRQ_DISABLE;

	cpu_stack_push(cpu_pc & 0xFF);
	cpu_stack_push(cpu_pc >> 8);
	cpu_stack_push(cpu_p);

	cpu_pc = mem_read(0xFFFE) + (mem_read(0xFFFF) << 8);
}

void cpu_bvc() {
	if (!(cpu_p & CPU_STAT_OVERFLOW)) {
		cpu_pc += (char)cpu_op_args[0];
	}
}

void cpu_bvs() {
	if (cpu_p & CPU_STAT_OVERFLOW) {
		cpu_pc += (char)cpu_op_args[0];
	}
}

void cpu_clc() {
	cpu_p &= ~CPU_STAT_CARRY;
}

void cpu_cld() {
	cpu_p &= ~CPU_STAT_DEC;
}

void cpu_cli() {
	cpu_p &= ~CPU_STAT_IRQ_DISABLE;
}

void cpu_clv() {
	cpu_p &= ~CPU_STAT_OVERFLOW;
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

	short result = cpu_acc - arg;

	if (result >= 0) {
		cpu_p |= CPU_STAT_CARRY;
	}
	else {
		cpu_p &= ~CPU_STAT_CARRY;
	}

	if (result > 127 || result < -128) {
		cpu_p |= CPU_STAT_OVERFLOW;
	}
	else {
		cpu_p &= ~CPU_STAT_OVERFLOW;
	}

	if (result) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}

	if (result & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
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

	short result = cpu_x - arg;
	
	if (result >= 0) {
		cpu_p |= CPU_STAT_CARRY;
	}
	else {
		cpu_p &= ~CPU_STAT_CARRY;
	}

	if (result > 127 || result < -128) {
		cpu_p |= CPU_STAT_OVERFLOW;
	}
	else {
		cpu_p &= ~CPU_STAT_OVERFLOW;
	}

	if (result) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
	if (result & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
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

	short result = cpu_y - arg;

	if (result >= 0) {
		cpu_p |= CPU_STAT_CARRY;
	}
	else {
		cpu_p &= ~CPU_STAT_CARRY;
	}

	if (result > 127 || result < -128) {
		cpu_p |= CPU_STAT_OVERFLOW;
	}
	else {
		cpu_p &= ~CPU_STAT_OVERFLOW;
	}

	if (result) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
	if (result & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
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

	--arg;

	if (arg) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
	if (arg & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}

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
}

void cpu_dex() {
	--cpu_x;
	if (cpu_x) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
	if (cpu_x & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
}

void cpu_dey() {
	--cpu_y;
	if (cpu_y) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
	if (cpu_y & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
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

	cpu_acc ^= arg;

	if (cpu_acc) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
	if (cpu_acc & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}

	if (cpu_op == 0x49) {
		cpu_op_args[0];
	}
	else if (cpu_op == 0x45) {
		cpu_write_zp(arg);
	}
	else if (cpu_op == 0x55) {
		cpu_write_zpx(arg);
	}
	else if (cpu_op == 0x4D) {
		cpu_write_abs(arg);
	}
	else if (cpu_op == 0x5D) {
		cpu_write_abx(arg);
	}
	else if (cpu_op == 0x59) {
		cpu_write_aby(arg);
	}
	else if (cpu_op == 0x41) {
		cpu_write_izx(arg);
	}
	else if (cpu_op == 0x51) {
		cpu_write_izy(arg);
	}
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

	++arg;

	if (arg) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
	if (arg & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}

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
}

void cpu_inx() {
	++cpu_x;
	if (cpu_x) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
	if (cpu_x & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
}

void cpu_iny() {
	++cpu_y;
	if (cpu_y) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
	if (cpu_y & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
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
	cpu_stack_push((cpu_pc-1) & 0xFF);
	cpu_stack_push((cpu_pc-1) >> 8);
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

	cpu_acc = arg;
	if (cpu_acc) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
	if (cpu_acc & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
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

	cpu_x = arg;
	if (cpu_x) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
	if (cpu_x & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
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

	cpu_y = arg;
	if (cpu_y) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
	if (cpu_y & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
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

	cpu_p &= ~CPU_STAT_NEGATIVE;
	if (arg & 0x01) {
		cpu_p |= CPU_STAT_CARRY;
	}
	else {
		cpu_p &= ~CPU_STAT_CARRY;
	}

	arg >>= 1;

	if (arg) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}

	if (cpu_op == 0x4A) {
		cpu_acc;
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
}

void cpu_nop() {
}

void cpu_ora() {
	
	unsigned char arg;
	if (cpu_op == 0x09) {
		arg = cpu_op_args[0];
	}
	else if (cpu_op == 0x05) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0x15) {
		arg = cpu_read_zpx();
	}
	else if (cpu_op == 0x0D) {
		arg = cpu_read_abs();
	}
	else if (cpu_op == 0x1D) {
		arg = cpu_read_abx();
	}
	else if (cpu_op == 0x19) {
		arg = cpu_read_aby();
	}
	else if (cpu_op == 0x01) {
		arg = cpu_read_izx();
	}
	else if (cpu_op == 0x11) {
		arg = cpu_read_izy();
	}

	cpu_acc |= arg;
	
	if (cpu_acc) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}
	if (cpu_acc & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
}

void cpu_pha() {
	cpu_stack_push(cpu_acc);
}

void cpu_php() {
	cpu_stack_push(cpu_p);
}

void cpu_pla() {
	cpu_acc = cpu_stack_pop();
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

	if (arg & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}

	if (arg) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}

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
}

void cpu_ror() {
	unsigned char arg;
	if (cpu_op == 0x6A) {
		arg = cpu_acc;
	}
	else if (cpu_op == 0x66) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0x76) {
		arg = cpu_read_zpx();
	}
	else if (cpu_op == 0x6E) {
		arg = cpu_read_abs();
	}
	else if (cpu_op == 0x7E) {
		arg = cpu_read_abx();
	}

	bool newcarry = arg & 0x01;

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

	if (arg & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}

	if (arg) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}

	if (cpu_op == 0x6A) {
		cpu_acc = arg;
	}
	else if (cpu_op == 0x66) {
		cpu_write_zp(arg);
	}
	else if (cpu_op == 0x76) {
		cpu_write_zpx(arg);
	}
	else if (cpu_op == 0x6E) {
		cpu_write_abs(arg);
	}
	else if (cpu_op == 0x7E) {
		cpu_write_abx(arg);
	}
}

void cpu_rti() {
	cpu_p = cpu_stack_pop();
	unsigned char hi = cpu_stack_pop();
	unsigned char lo = cpu_stack_pop();
	cpu_pc = lo + (hi << 8);
}

void cpu_rts() {
	unsigned char hi = cpu_stack_pop();
	unsigned char lo = cpu_stack_pop();
	cpu_pc = lo + (hi << 8) + 1;
}

void cpu_sbc() {
	unsigned char arg;
	if (cpu_op == 0xE9) {
		arg = cpu_op_args[0];
	}
	else if (cpu_op == 0xE5) {
		arg = cpu_read_zp();
	}
	else if (cpu_op == 0xF5) {
		arg = cpu_read_zpx();
	}
	else if (cpu_op == 0xED) {
		arg = cpu_read_abs();
	}
	else if (cpu_op == 0xFD) {
		arg = cpu_read_abx();
	}
	else if (cpu_op == 0xF9) {
		arg = cpu_read_aby();
	}
	else if (cpu_op == 0xE1) {
		arg = cpu_read_izx();
	}
	else if (cpu_op == 0xF1) {
		arg = cpu_read_izy();
	}

	unsigned char carry = (cpu_p & CPU_STAT_CARRY ? 0 : 1);
	short result = cpu_acc - arg - carry;
	cpu_acc = result;

	if (result >= 0) {
		cpu_p |= CPU_STAT_CARRY;
	}
	else {
		cpu_p &= ~CPU_STAT_CARRY;
	}

	if (result > 127 || result < -128) {
		cpu_p |= CPU_STAT_OVERFLOW;
	}
	else {
		cpu_p &= ~CPU_STAT_OVERFLOW;
	}

	if (cpu_acc) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}

	if (cpu_acc & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
}

void cpu_sec() {
	cpu_p |= CPU_STAT_CARRY;
}

void cpu_sed() {
	cpu_p |= CPU_STAT_DEC;
}

void cpu_sei() {
	cpu_p |= CPU_STAT_IRQ_DISABLE;
}

void cpu_sta() {
	if (cpu_op == 0x85) {
		cpu_write_zp(cpu_acc);
	}
	else if (cpu_op == 0x95) {
		cpu_write_zpx(cpu_acc);
	}
	else if (cpu_op == 0x8D) {
		cpu_write_abs(cpu_acc);
	}
	else if (cpu_op == 0x9D) {
		cpu_write_abx(cpu_acc);
	}
	else if (cpu_op == 0x99) {
		cpu_write_aby(cpu_acc);
	}
	else if (cpu_op == 0x81) {
		cpu_write_izx(cpu_acc);
	}
	else if (cpu_op == 0x91) {
		cpu_write_izy(cpu_acc);
	}
}

void cpu_stx() {
	if (cpu_op == 0x86) {
		cpu_write_zp(cpu_x);
	}
	else if (cpu_op == 0x96) {
		cpu_write_zpy(cpu_x);
	}
	else if (cpu_op == 0x8E) {
		cpu_write_abs(cpu_x);
	}
}

void cpu_sty() {
	if (cpu_op == 0x84) {
		cpu_write_zp(cpu_y);
	}
	else if (cpu_op == 0x94) {
		cpu_write_zpx(cpu_y);
	}
	else if (cpu_op == 0x8C) {
		cpu_write_abs(cpu_y);
	}
}

void cpu_tax() {
	cpu_x = cpu_acc;
	if (cpu_acc) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}

	if (cpu_acc & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
}

void cpu_tay() {
	cpu_y = cpu_acc;
	if (cpu_acc) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}

	if (cpu_acc & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
}

void cpu_tsx() {
	cpu_x = cpu_stack;
	if (cpu_x) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}

	if (cpu_x & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
}

void cpu_txa() {
	cpu_acc = cpu_x;
	if (cpu_x) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}

	if (cpu_x & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
}

void cpu_txs() {
	cpu_stack = cpu_x;
	if (cpu_x) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}

	if (cpu_x & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
}

void cpu_tya() {
	cpu_acc = cpu_y;
	if (cpu_acc) {
		cpu_p &= ~CPU_STAT_ZERO;
	}
	else {
		cpu_p |= CPU_STAT_ZERO;
	}

	if (cpu_acc & 0x80) {
		cpu_p |= CPU_STAT_NEGATIVE;
	}
	else {
		cpu_p &= ~CPU_STAT_NEGATIVE;
	}
}

/**
 * Illegal opcodes
 */
extern bool g_stop;
void cpu_kil() {
	g_stop=true;
}

void cpu_slo() {
}

void cpu_rla() {
}

void cpu_sre() {
}

void cpu_rra() {
}

void cpu_sax() {
}

void cpu_lax() {
}

void cpu_dcp() {
}

void cpu_isc() {
}

void cpu_anc() {
}

void cpu_alr() {
}

void cpu_arr() {
}

void cpu_xaa() {
}

void cpu_axs() {
}

void cpu_ahx() {
}

void cpu_shy() {
}

void cpu_shx() {
}

void cpu_tas() {
}

void cpu_las() {
}
