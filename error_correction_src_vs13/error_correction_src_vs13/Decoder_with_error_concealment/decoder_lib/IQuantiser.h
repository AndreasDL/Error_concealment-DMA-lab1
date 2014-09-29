#ifndef IQUANTISER_H
#define IQUANTISER_H

#include "Macroblock.h"

class IQuantiser
{
public:
	static void IQuantise(Macroblock *mb, int qp);
};

#endif