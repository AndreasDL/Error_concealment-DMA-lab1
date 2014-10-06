#ifndef IMOTIONCOMPENSATOR_H
#define IMOTIONCOMPENSATOR_H

#include "Frame.h"
#include "Macroblock.h"

class IMotionCompensator
{
public:
	IMotionCompensator();
	~IMotionCompensator();

	void setReferenceFrame(Frame *frame);
	Frame *getReferenceFrame();
	void iMotionCompensate(Macroblock *mb);

protected:
	Frame *reference_frame;
	int ref_width;
	int ref_height;

	Plane reference_data;

	int GetRefPixelLuma(int x, int y);
	int GetRefPixelCb(int x, int y);
	int GetRefPixelCr(int x, int y);
};

#endif