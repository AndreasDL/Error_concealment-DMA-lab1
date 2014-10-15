#ifndef FRAME_H
#define FRAME_H

#include "Macroblock.h"

class Frame
{
public:
	Frame(int width, int height);
	Frame(const Frame &frame);
	Frame &operator=(const Frame &frame);
	~Frame();

	int getWidth();
	int getHeight();
	int getNumMB();

	Macroblock *getMacroblock(int index);
	void eraseMacroblock(int index);

	void set_p_frame(bool p_frame);
	bool is_p_frame();
protected:
	int num_mb;
	int width;
	int height;

	Macroblock *macroblocks;

	bool p_frame;

	friend class YUVFileInput;
	friend class YUVFileOutput;

	void Init();
};

#endif