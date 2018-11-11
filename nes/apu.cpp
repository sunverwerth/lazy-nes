#include "apu.h"
#include "ppu.h"
#include "bus.h"

#include <cmath>

unsigned char apu_input_up;
unsigned char apu_input_down;
unsigned char apu_input_left;
unsigned char apu_input_right;
unsigned char apu_input_a;
unsigned char apu_input_b;
unsigned char apu_input_select;
unsigned char apu_input_start;
int readindex;
int apu_tick_count;


struct Envelope {
	bool isConstant;
	double constantVolume;
	double timer;
};

struct PulseChannel {
	Envelope envelope;
	double frequency;
	double duty;
	double phase;
	int period;
	bool sweepEnable;
	bool sweepNeg;
	int sweepShift;
	int sweepPeriod;
	int nextSweepTick;
	int lengthCounter;
};

PulseChannel apu_pulse[2];

double apu_tri_frequency;
double apu_tri_phase;
int apu_tri_period;
int apu_tri_length_counter;

unsigned char apu_read(unsigned short address) {
	switch (address) {
	case 0x4016: {
		switch (readindex++) {
		case 0: return apu_input_a;
		case 1: return apu_input_b;
		case 2: return apu_input_select;
		case 3: return apu_input_start;
		case 4: return apu_input_up;
		case 5: return apu_input_down;
		case 6: return apu_input_left;
		case 7: return apu_input_right;
		default: return 0;
		}
		break;
	}
	}
	return 0;
}

void apu_write(unsigned short address, unsigned char data) {
	// TODO
	int pulseIndex = 0;
	switch (address) {
	case 0x4004: pulseIndex++;
	case 0x4000: {
		int d = (data & 0xc0) >> 6;
		apu_pulse[pulseIndex].duty = (d == 0) ? 0.125 : 0.25 * d;
		apu_pulse[pulseIndex].envelope.isConstant = (data & 0x10);
		if (apu_pulse[pulseIndex].envelope.isConstant) {
			apu_pulse[pulseIndex].envelope.constantVolume = double(data & 0x0f) / 15;
		}
		else {
			apu_pulse[pulseIndex].envelope.constantVolume = 0;
		}
		break;
	}
	case 0x4005: pulseIndex++;
	case 0x4001: {
		apu_pulse[pulseIndex].sweepEnable = data & 0x80;
		apu_pulse[pulseIndex].sweepPeriod = (data & 0x70) >> 4;
		apu_pulse[pulseIndex].sweepNeg = data & 0x08;
		apu_pulse[pulseIndex].sweepShift = data & 0x07;
		apu_pulse[pulseIndex].nextSweepTick = apu_tick_count;
		break;
	}
	case 0x4006: pulseIndex++;
	case 0x4002:
		apu_pulse[pulseIndex].period = (apu_pulse[pulseIndex].period & 0xffffff00) | data;
		apu_pulse[pulseIndex].frequency = 111860.8 / (apu_pulse[pulseIndex].period + 1);
		break;
	case 0x4007: pulseIndex++;
	case 0x4003:
		apu_pulse[pulseIndex].phase = 0;
		apu_pulse[pulseIndex].period = (apu_pulse[pulseIndex].period & 0xfffff8ff) | ((data & 0x03) << 8);
		apu_pulse[pulseIndex].frequency = 111860.8 / (apu_pulse[pulseIndex].period + 1);
		apu_pulse[pulseIndex].lengthCounter = ((data & 0xf8 ) >> 3) * 10000;
		break;
	case 0x4008:
		apu_tri_length_counter = (data & 0x7f) * 10000;
		break;
	case 0x400a:
		apu_tri_period = (apu_tri_period & 0xffffff00) | data;
		apu_tri_frequency = 111860.8 * 0.5 / (apu_tri_period + 1);
		break;
	case 0x400b:
		apu_tri_phase = 0;
		apu_tri_period = (apu_tri_period & 0xfffff8ff) | ((data & 0x03) << 8);
		apu_tri_frequency = 111860.8 * 0.5 / (apu_tri_period + 1);
		break;
	case 0x4014: {
		auto dmaAddress = data << 8;
		for (int i = 0; i < 256; ++i) {
			auto data = bus_read(dmaAddress++);
			ppu_write(4, data);
		}
		break;
	}
	case 0x4016:
		if (data & 1) {
			readindex = 0;
		}
		break;
	}
}

void apu_tick() {
	apu_tick_count++;

	for (int p = 0; p < 2; ++p) {
		if (apu_pulse[p].sweepEnable && apu_tick_count > apu_pulse[p].nextSweepTick) {
			apu_pulse[p].nextSweepTick = apu_tick_count + 100000;
			auto amount = apu_pulse[p].period >> apu_pulse[p].sweepShift;
			int newPeriod = apu_pulse[p].sweepNeg ? apu_pulse[p].period - amount : apu_pulse[p].period + amount;
			if (newPeriod > 0x7ff || newPeriod < 8) {
				apu_pulse[p].envelope.constantVolume = 0;
				apu_pulse[p].sweepEnable = false;
			}
			else {
				apu_pulse[p].period = newPeriod;
				apu_pulse[p].frequency = 111860.8 / (apu_pulse[p].period + 1);
			}
		}
	}
}

void apu_gen_audio(void* buf, int numSamples) {
	auto buffer = (int16_t*)buf;
	double intpart;
	for (int i = 0; i < numSamples; ++i) {
		double pulse[2];
		for (int p = 0; p < 2; ++p) {
			if (apu_pulse[p].period > 7) {
				pulse[p] = (apu_pulse[p].phase > apu_pulse[p].duty ? 1.0 : -1.0) * apu_pulse[p].envelope.constantVolume;
				apu_pulse[p].phase = modf(apu_pulse[p].phase + apu_pulse[p].frequency / 44100, &intpart);
				if (apu_pulse[p].lengthCounter-- <= 0) {
					apu_pulse[p].period = 0;
				}
			}
			else {
				pulse[p] = 0;
			}
		}

		double tri = 0;
		if (apu_tri_period > 0) {
			tri = (apu_tri_phase > 0.5 ? 0.5 - apu_tri_phase : apu_tri_phase) - 0.25;
			apu_tri_phase = modf(apu_tri_phase + apu_tri_frequency / 44100, &intpart);
			if (apu_tri_length_counter-- <= 0) {
				apu_tri_period = 0;
			}
		}
		
		double sum = fmin(fmax(pulse[0] + pulse[1] + tri, -1.0), 1.0);

		buffer[i] = sum * 16000;
	}
}
