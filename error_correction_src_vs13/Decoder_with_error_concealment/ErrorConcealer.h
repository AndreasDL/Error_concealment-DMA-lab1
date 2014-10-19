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
	//spatial
	//use 2 nearest blocks
	void conceal_spatial_1(Frame* frame);
	//reduce complexity || spatial for complex
	void conceal_spatial_2(Frame *frame, const bool setConcealed);
	//Edge detection
	void conceal_spatial_3(Frame* frame);
	//temporal
	//no motion
	void conceal_temporal_1(Frame* frame, Frame* referenceFrame);
	//motion and switch too spatial if error is too big
	void conceal_temporal_2(Frame* frame, Frame* referenceFrame, const int size);
	//motion and switch too spatial if error is too big, auto-determine the size of the subblocks
	void conceal_temporal_2_dynamic(Frame* frame, Frame* referenceFrame);
	//smart version design to tackle the complex pattern
	void conceal_temporal_3(Frame* frame, Frame* referenceFrame);

};

#endif
