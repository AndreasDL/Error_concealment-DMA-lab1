#ifndef MACROBLOCK_H
#define MACROBLOCK_H

typedef unsigned char byte;
typedef int pixel;
typedef pixel** Plane;
typedef struct { int x; int y; } MotionVector;

enum MB_State { YUV, MC, DCT, QT };

class Macroblock
{
public:
	Macroblock();
	Macroblock(int mb_num, int x_pos, int y_pos);
	Macroblock(const Macroblock &mb);
	Macroblock &operator=(const Macroblock &mb);
	~Macroblock();

	int getMBNum();
	int getXPos();
	int getYPos();

	Plane luma;
	Plane cb;
	Plane cr;
	MotionVector mv;

	MB_State state;

	bool isMissing();
	void erase();
	void setConcealed();

private:
	void init();

	int mb_num;
	int x_pos;
	int y_pos;

	bool missing;
};

#endif