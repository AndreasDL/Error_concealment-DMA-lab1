#ifndef ERRORCONCEALER_H
#define ERRORCONCEALER_H

#include "Frame.h"

class ErrorConcealer
{
public:
	ErrorConcealer(short conceal_method);
	~ErrorConcealer(void);

	void concealErrors(Frame* frame, Frame* referenceFrame);

protected:
	short conceal_method;
	void conceal_spatial_1(Frame* frame);
	void conceal_spatial_2(Frame* frame);
	void conceal_spatial_3(Frame* frame);
	void conceal_temporal_1(Frame* frame, Frame* referenceFrame);
	void conceal_temporal_2(Frame* frame, Frame* referenceFrame, int size);
	void conceal_temporal_3(Frame* frame, Frame* referenceFrame);
};

#endif
