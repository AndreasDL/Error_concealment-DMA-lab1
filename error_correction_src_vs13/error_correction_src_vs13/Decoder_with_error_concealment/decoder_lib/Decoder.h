#ifndef DECODER_H
#define DECODER_H

class Decoder
{
public:
	int Decode(char *inputfile, char* outputfile, char* error_pattern, short conceal_method);
};

#endif