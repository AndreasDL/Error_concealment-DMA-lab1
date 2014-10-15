#ifndef ENTROPYDECODER_H
#define ENTROPYDECODER_H

#include "BitFileInput.h"
#include "Macroblock.h"

class EntropyDecoder
{
public:
	EntropyDecoder(BitFileInput *bfi);

	void ReadMB(Macroblock *mb, bool inter, bool do_mc, bool do_dct, bool do_qt, int qp);
	byte ReadByte();

	bool hasMore();

	int DecodeInt();
	int DecodeUInt();

protected:
	BitFileInput *bfi;

	int DecodeExpGolomb();
};

#endif