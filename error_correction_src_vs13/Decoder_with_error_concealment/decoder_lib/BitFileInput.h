#ifndef BITFILEINPUT_H
#define BITFILEINPUT_H

#include <stdio.h>

typedef unsigned char byte;

class BitFileInput
{
public:
	BitFileInput(char *filename);
	~BitFileInput();

	byte ReadByte();
	int Read(byte* buffer, int length);

	bool ReadBit();
	int ReadBits(int num);
	bool hasMore();

protected:
	FILE *inputfile;

	byte bitbuffer;
	int bits_in_buffer;
};

#endif