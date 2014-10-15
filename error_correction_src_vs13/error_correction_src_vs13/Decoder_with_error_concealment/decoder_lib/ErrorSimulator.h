#ifndef ERRORSIMULATOR_H
#define ERRORSIMULATOR_H

#include "Frame.h"
#include <stdio.h>
#include <fstream>

class ErrorSimulator
{
public:
	ErrorSimulator(char *error_pattern);
	~ErrorSimulator(void);

	void simulateErrors(Frame* frame);

private:
	std::ifstream *ep;
};

#endif
