#ifndef CIA_H
#define CIA_H

class Vic;

class Cia {
public:
	Cia(Vic* vic);
	~Cia();

	void tick();

	char read(unsigned short address);
	void write(unsigned short address, char data);

private:
	Vic* vic;
	char registers[2][16];
};

extern Cia* cia;

#endif