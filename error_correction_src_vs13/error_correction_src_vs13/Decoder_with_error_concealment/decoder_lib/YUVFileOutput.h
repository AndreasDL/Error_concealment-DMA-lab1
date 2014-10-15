#ifndef YUVFILEOUTPUT_H
#define YUVFILEOUTPUT_H

#include <stdio.h>
#include "Frame.h"

class YUVFileOutput
{
public:
	YUVFileOutput(char* filename);
	~YUVFileOutput();

	void outputFrame(Frame *frame);

protected:
	FILE *outputfile;
};

#endif