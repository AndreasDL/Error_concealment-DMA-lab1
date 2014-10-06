#ifndef IDCTTRANSFORM_H
#define IDCTTRANSFORM_H

#include "Macroblock.h"

class IDCTTransform
{
public:
	IDCTTransform();
	void ITransform(Macroblock *mb);

private:
	void ITransform_8x8(pixel *blk, int stride);
	void IDCTRow(pixel *blk);
	void IDCTCol(pixel *blk, int stride);

	short iclip[1024];	// clipping table
	short *iclp;

	int iclipFunction(int i);
};

#define W1 2841 /* 2048*sqrt(2)*cos(1*pi/16) */
#define W2 2676 /* 2048*sqrt(2)*cos(2*pi/16) */
#define W3 2408 /* 2048*sqrt(2)*cos(3*pi/16) */
#define W5 1609 /* 2048*sqrt(2)*cos(5*pi/16) */
#define W6 1108 /* 2048*sqrt(2)*cos(6*pi/16) */
#define W7 565  /* 2048*sqrt(2)*cos(7*pi/16) */

#endif