#ifndef VIC_H
#define VIC_H

class Vic {
public:
	Vic();
	~Vic();

	void tick();

	char read(unsigned short address);
	void write(unsigned short address, char data);

	void setBank(char bank);

	const unsigned int* getFrameBuffer() const;
	int getFrameWidth() const;
	int getFrameHeight() const;
	bool frameStarted() const;

private:
	unsigned int frameBuffer[320*200];
	unsigned int interruptLine;
	unsigned int x;
	unsigned int y;
	unsigned char borderColor;
	unsigned char backgroundColor;
	unsigned char backgroundColor2;
	unsigned char backgroundColor3;
	unsigned char backgroundColor4;
	char lineChars[40];
	char lineColors[40];
	unsigned short memBase;
	unsigned short gfxBase;
	unsigned short charBase;
	unsigned short colorBase;

	static const unsigned int vic_colors[];

	char registers[64];
};

extern Vic* vic;

#endif