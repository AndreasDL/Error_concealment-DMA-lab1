#include "ErrorConcealer.h"
#include "MacroblockEmpty.h"
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <iostream>
#include <ctime>
#include <queue>
#include <functional>
using namespace std;

//types
typedef pair<int,Macroblock*> task; //task for priority queue
enum MBSTATE { OK, MISSING, CONCEALED }; //states
enum position{ pos_TOP, pos_BOT, pos_LEFT, pos_RIGHT }; //positions

//constructors & 'main method'
ErrorConcealer::ErrorConcealer(short conceal_method){
	this->conceal_method = conceal_method;
}
ErrorConcealer::~ErrorConcealer(void){
}
void ErrorConcealer::concealErrors(Frame *frame, Frame *referenceFrame){
	switch(conceal_method){
		case 0:
			conceal_spatial_1(frame);
			break;
		case 1:
			conceal_spatial_2(frame,true);
			break;
		case 2:
			conceal_spatial_3(frame);
			break;
		case 3:
			conceal_temporal_1(frame, referenceFrame);
			break;
		case 4:
			conceal_temporal_2(frame, referenceFrame, 1);
			break;
		case 5:
			conceal_temporal_2(frame, referenceFrame, 2);
			break;
		case 6:
			conceal_temporal_2(frame, referenceFrame, 3);
			break;
		case 7:
			conceal_temporal_2(frame, referenceFrame, 4);
			break;
		case 8:
			conceal_temporal_3(frame, referenceFrame);
			break;
		case 9:
			// To be completed. Add explanatory notes in English.
		default:
			printf("\nWARNING: NO ERROR CONCEALMENT PERFORMED! (conceal_method %d unknown)\n\n",conceal_method);
	}
}

//Aid functions
inline pixel getYPixel(Frame *frame, int posx, int posy){
	int MBx = (int(posy / 16) * frame->getWidth());
	MBx += (int(posx / 16));
	Macroblock *MB = frame->getMacroblock(MBx);
	pixel luma = MB->luma[posy % 16][posx % 16];
	return luma;
}
inline pixel getCbPixel(Frame *frame, int posx, int posy){
	int MBx = (int(posy / 8) * frame->getWidth());
	MBx += (int(posx / 8));
	Macroblock *MB = frame->getMacroblock(MBx);
	pixel cb = MB->cb[posy % 8][posx % 8];
	return cb;
}
inline pixel getCrPixel(Frame *frame, int posx, int posy){
	int MBx = (int(posy / 8) * frame->getWidth());
	MBx += (int(posx / 8));
	Macroblock *MB = frame->getMacroblock(MBx);
	pixel cr = MB->cr[posy % 8][posx % 8];
	return cr;
}
//return the number of available neighbours for one block.
inline int getNeighbours(Frame *frame, const int MBx){
	Macroblock* mb = frame->getMacroblock(MBx);
	//what blocks exists
	bool exists_l = mb->getXPos() != 0 && !frame->getMacroblock(MBx - 1)->isMissing();//left?
	bool exists_r = mb->getXPos() < frame->getWidth() - 1 && !frame->getMacroblock(MBx + 1)->isMissing();//right?
	bool exists_t = mb->getYPos() != 0 && !frame->getMacroblock(MBx - frame->getWidth())->isMissing();//top?
	bool exists_b = mb->getYPos() < frame->getHeight() - 1 && !frame->getMacroblock(MBx + frame->getWidth())->isMissing();//bot?
	return exists_l + exists_b + exists_r + exists_t;
}

//timing functions for debugging and evaluation purposes only!
double starttime;
void startChrono(){
	starttime = double(clock()) / CLOCKS_PER_SEC;
}
double stopChrono(){
	return (double(clock()) / CLOCKS_PER_SEC) - starttime;
}

//*****************************************************************************************************
//******************************************Conceal Methods********************************************
//*****************************************************************************************************
//No adjacent blocks missing, use interpolate with the 2 closest pixels to solve this.
void ErrorConcealer::conceal_spatial_1(Frame *frame){
	//debug & evaluation
	startChrono();
	int missing = 0;

	int numMB = frame->getNumMB();
	Macroblock* MB;
	Macroblock* MB_l;
	Macroblock* MB_r;
	Macroblock* MB_t;
	Macroblock* MB_b;
	MacroblockEmpty* MBEmpty = new MacroblockEmpty();
	int exist_t = 1;
	int exist_b = 1;
	int exist_r = 1;
	int exist_l = 1;
	for (int MBx = 0; MBx < numMB; ++MBx)
	{
		MB = frame->getMacroblock(MBx);
		exist_t = 1;
		exist_b = 1;
		exist_r = 1;
		exist_l = 1;
		if (MB->isMissing())
		{
			missing++;
			//determine MB_l
			if (MB->getXPos() == 0){
				MB_l = MBEmpty;
				exist_l = 0;
			}
			else{
				MB_l = frame->getMacroblock(MBx - 1);
			}
			//determine MB_r
			if (MB->getXPos() == frame->getWidth() - 1){
				MB_r = MBEmpty;
				exist_r = 0;
			}
			else{
				MB_r = frame->getMacroblock(MBx + 1);
			}
			//determine MB_t
			if (MB->getYPos() == 0){
				MB_t = MBEmpty;
				exist_t = 0;
			}
			else{
				MB_t = frame->getMacroblock(MBx - frame->getWidth());
			}
			//determine MB_b
			if (MB->getYPos() == frame->getHeight() - 1){
				MB_b = MBEmpty;
				exist_b = 0;
			}
			else{
				MB_b = frame->getMacroblock(MBx + frame->getWidth());
			}

			//Spatial interpolate pixels using only the two closest pixels
			//luma vals 16 per macroblock
			for (int i = 0; i < 16; ++i)	{
				for (int j = 0; j < 16; ++j) {
					
					// To easily make sure we only use 2 blocks, we will say only 2 blocks exist (the 2 closest blocks).
					if (i >= 8){ //use bottom
						exist_b = 1;
						exist_t = 0;
					}else{ //use top
						exist_b = 0;
						exist_t = 1;
					}
					if (j >= 8){ //use right
						exist_r = 1;
						exist_l = 0;
					}else{ //use left
						exist_r = 0;
						exist_l = 1;
					}

					MB->luma[i][j] = ((17 - j - 1)*MB_l->luma[i][15] * exist_l + 
						(j + 1)*MB_r->luma[i][0] * exist_r + 
						(17 - i - 1)*MB_t->luma[15][j] * exist_t + 
						(i + 1)*MB_b->luma[0][j] * exist_b ) 
						/ ( 
						( (17 - j - 1) * exist_l) + 
						( (j + 1) * exist_r) +
						( (17 - i - 1) * exist_t ) + 
						( (i + 1) * exist_b )
						);					
				}
			}

			//cb & cr -> 8 values per block
			for (int i = 0; i < 8; ++i)	{
				for (int j = 0; j < 8; ++j)	{
					// To easily make sure we only use 2 blocks, we will say only 2 blocks exist (the 2 closest blocks).
					if (i >= 4){ //use bottom
						exist_b = 1;
						exist_t = 0;
					}
					else{ //use top
						exist_b = 0;
						exist_t = 1;
					}
					if (j >= 4){ //use right
						exist_r = 1;
						exist_l = 0;
					}
					else{ //use left
						exist_r = 0;
						exist_l = 1;
					}

					MB->cb[i][j] = ((9 - j - 1)*MB_l->cb[i][7] + 
						(j + 1)*MB_r->cb[i][0] + 
						(9 - i - 1)*MB_t->cb[7][j] + 
						(i + 1)*MB_b->cb[0][j] ) 
						/ ( 
						( (9 - j - 1) * exist_l) + 
						( (j + 1) * exist_r) +
						( (9 - i - 1) * exist_t ) + 
						( (i + 1) * exist_b )
						);
					MB->cr[i][j] = ((9 - j - 1)*MB_l->cr[i][7] + 
						(j + 1)*MB_r->cr[i][0] + 
						(9 - i - 1)*MB_t->cr[7][j] + 
						(i + 1)*MB_b->cr[0][j] ) 
						/ ( 
						( (9 - j - 1) * exist_l) + 
						( (j + 1) * exist_r) +
						( (9 - i - 1) * exist_t ) + 
						( (i + 1) * exist_b )
						);
				}
			}
			MB->setConcealed();

		}
	}
	delete MBEmpty;

	std::cout << "\t[Spatial 1] Missing macroblocks: " << missing << " time needed : " << stopChrono() << endl;
}

//fix method for conceal_spatial_2
void f(Macroblock* MB,int* exist_l, int* exist_r, int* exist_t, int* exist_b,MBSTATE* MBstate, int MBx, int neighbs, Frame *frame){
	Macroblock* MB_l;
	Macroblock* MB_r;
	Macroblock* MB_t;
	Macroblock* MB_b;
	MacroblockEmpty* MBEmpty = new MacroblockEmpty();
	//determine MB_l
	if (MB->getXPos() == 0){
		MB_l = MBEmpty;
		*exist_l = 0;
	}
	else{
		if (MBstate[MBx - 1] == MISSING){
			MB_l = MBEmpty;
			*exist_l = 0;
		}
		else{
			MB_l = frame->getMacroblock(MBx - 1);
		}
	}
	//determine MB_r
	if (MB->getXPos() == frame->getWidth() - 1){
		MB_r = MBEmpty;
		*exist_r = 0;
	}
	else{
		if (MBstate[MBx + 1] == MISSING){
			MB_r = MBEmpty;
			*exist_r = 0;
		}
		else{
			MB_r = frame->getMacroblock(MBx + 1);
		}
	}
	//determine MB_t
	if (MB->getYPos() == 0){
		MB_t = MBEmpty;
		*exist_t = 0;
	}
	else{
		if (MBstate[MBx - frame->getWidth()] == MISSING){
			MB_t = MBEmpty;
			*exist_t = 0;
		}
		else{
			MB_t = frame->getMacroblock(MBx - frame->getWidth());
		}
	}
	//determine MB_b
	if (MB->getYPos() == frame->getHeight() - 1){
		MB_b = MBEmpty;
		*exist_b = 0;
	}
	else{
		if (MBstate[MBx + frame->getWidth()] == MISSING){
			MB_b = MBEmpty;
			*exist_b = 0;
		}
		else{
			MB_b = frame->getMacroblock(MBx + frame->getWidth());
		}
	}

	if (*exist_l + *exist_r + *exist_t + *exist_b > neighbs){
		//Spatial interpolate pixels
		for (int i = 0; i < 16; ++i)	{
			for (int j = 0; j < 16; ++j){
				if (neighbs > 1){
					if (*exist_l + *exist_r + *exist_t + *exist_b > 2){
						if ((*exist_b * *exist_t == 1) == 1){
							if (i >= 8){
								*exist_b = 1;
								*exist_t = 0;
							}
							else{
								*exist_b = 0;
								*exist_t = 1;
							}
						}

						if (*exist_b * *exist_t == 1){
							if (j >= 8){
								*exist_r = 1;
								*exist_l = 0;
							}
							else{
								*exist_r = 0;
								*exist_l = 1;
							}
						}
					}
				}
				MB->luma[i][j] = ((17 - j - 1)*MB_l->luma[i][15] * *exist_l +
					(j + 1)*MB_r->luma[i][0] * *exist_r +
					(17 - i - 1)*MB_t->luma[15][j] * *exist_t +
					(i + 1)*MB_b->luma[0][j] * *exist_b)
					/ (
					((17 - j - 1) * *exist_l) +
					((j + 1) * *exist_r) +
					((17 - i - 1) * *exist_t) +
					((i + 1) * *exist_b)
					);
			}
		}
		for (int i = 0; i < 8; ++i)	{
			for (int j = 0; j < 8; ++j){
				if (neighbs > 2){
					if (*exist_l + *exist_r + *exist_t + *exist_b > 2){
						if (*exist_b * *exist_t == 1){
							if (i >= 8){
								*exist_b = 1;
								*exist_t = 0;
							}
							else{
								*exist_b = 0;
								*exist_t = 1;
							}
						}
						if (*exist_b * *exist_t == 1){
							if (j >= 8){
								*exist_r = 1;
								*exist_l = 0;
							}
							else{
								*exist_r = 0;
								*exist_l = 1;
							}
						}
					}
				}
				MB->cb[i][j] = ((9 - j - 1)*MB_l->cb[i][7] * *exist_l +
					(j + 1)*MB_r->cb[i][0] * *exist_r +
					(9 - i - 1)*MB_t->cb[7][j] * *exist_t +
					(i + 1)*MB_b->cb[0][j] * *exist_b)
					/ (
					((9 - j - 1) * *exist_l) +
					((j + 1) * *exist_r) +
					((9 - i - 1) * *exist_t) +
					((i + 1) * *exist_b)
					);
				MB->cr[i][j] = ((9 - j - 1)*MB_l->cr[i][7] * *exist_l +
					(j + 1)*MB_r->cr[i][0] * *exist_r +
					(9 - i - 1)*MB_t->cr[7][j] * *exist_t +
					(i + 1)*MB_b->cr[0][j] * *exist_b)
					/ (
					((9 - j - 1) * *exist_l) +
					((j + 1) * *exist_r) +
					((9 - i - 1) * *exist_t) +
					((i + 1) * *exist_b)
					);
			}
		}
	}
	delete MBEmpty;
}
//can fix even if adjacent blocks are missing
void ErrorConcealer::conceal_spatial_2(Frame *frame,const bool setConcealed){
	//debug & evaluation
	startChrono();
	int missing = 0;

	int numMB = frame->getNumMB();
	Macroblock* MB;
	int exist_t = 1;
	int exist_b = 1;
	int exist_r = 1;
	int exist_l = 1;
	int MBsConcealedL1 = 1;
	int MBsConcealedL1L2 = 1;
	int nrOfMBsMissing = 0;
	int totalConcealed = 0;
	MBSTATE* MBstate = new MBSTATE[numMB];
	for (int MBx = 0; MBx < numMB; ++MBx){
		if (frame->getMacroblock(MBx)->isMissing()){
			MBstate[MBx] = MISSING;
			++nrOfMBsMissing;
			missing++;
		}else{
			MBstate[MBx] = OK;
		}
	}
	int loop = 0;
	while (nrOfMBsMissing > 0){

		MBsConcealedL1L2 = 1;
		while (MBsConcealedL1L2 > 0){
			MBsConcealedL1 = 1;
			MBsConcealedL1L2 = 0;
			while (MBsConcealedL1 > 0){
				MBsConcealedL1 = 0;
				for (int MBx = 0; MBx < numMB; ++MBx)
				{
					MB = frame->getMacroblock(MBx);
					exist_t = 1;
					exist_b = 1;
					exist_r = 1;
					exist_l = 1;
					if (MBstate[MBx] == MISSING)
					{
						f(MB, &exist_l, &exist_r, &exist_t, &exist_b,
							MBstate, MBx, 2, frame);
						if (exist_l + exist_r + exist_t + exist_b > 2){
							if (setConcealed)
								MB->setConcealed();
							++MBsConcealedL1;
							++MBsConcealedL1L2;
							--nrOfMBsMissing;
							++totalConcealed;
							MBstate[MBx] = CONCEALED;
						}
					}
				}
				//zet alle CONCEALED om naar OK
				for (int MBx = 0; MBx < numMB; ++MBx){
					if (MBstate[MBx] == CONCEALED){
						MBstate[MBx] = OK;
					}
				}
			}
			bool oneMBConcealed = false;
			for (int MBx = 0; MBx < numMB && !oneMBConcealed; ++MBx){
				MB = frame->getMacroblock(MBx);
				exist_t = 1;
				exist_b = 1;
				exist_r = 1;
				exist_l = 1;
				if (MBstate[MBx] == MISSING)
				{
					f(MB, &exist_l, &exist_r, &exist_t, &exist_b,
						MBstate, MBx, 1, frame);
					if (exist_l + exist_r + exist_t + exist_b > 1){
						if (setConcealed)
							MB->setConcealed();
						--nrOfMBsMissing;
						++totalConcealed;
						MBstate[MBx] = CONCEALED;
						oneMBConcealed = true;
						++MBsConcealedL1L2;
					}
				}
			}
			//zet alle CONCEALED om naar OK
			for (int MBx = 0; MBx < numMB; ++MBx){
				if (MBstate[MBx] == CONCEALED){
					MBstate[MBx] = OK;
				}
			}
		}
		bool oneMBConcealed = false;
		for (int MBx = 0; MBx < numMB && !oneMBConcealed; ++MBx){
			MB = frame->getMacroblock(MBx);
			exist_t = 1;
			exist_b = 1;
			exist_r = 1;
			exist_l = 1;
			if (MBstate[MBx] == MISSING){
				f(MB, &exist_l, &exist_r, &exist_t, &exist_b,
					MBstate, MBx, 0, frame);
				if (exist_l + exist_r + exist_t + exist_b > 0){
					if (setConcealed)
						MB->setConcealed();
					--nrOfMBsMissing;
					++totalConcealed;
					MBstate[MBx] = CONCEALED;
					oneMBConcealed = true;
					++MBsConcealedL1L2;
				}
			}
		}
		//zet alle CONCEALED om naar OK
		for (int MBx = 0; MBx < numMB; ++MBx){
			if (MBstate[MBx] == CONCEALED){
				MBstate[MBx] = OK;
			}
		}
	}
	std::cout << "\t[Spatial 2] Missing macroblocks: " << missing << " time needed : " << stopChrono() << endl;
}
//uses edge information
void ErrorConcealer::conceal_spatial_3(Frame *frame){
	double kernel_x[3][3] = { { -1, 0, 1 }, { -2, 0, 2 }, { -1, 0, 1 } };
	double kernel_y[3][3] = { { 1, 2, 1 }, { 0, 0, 0 }, { -1, -2, -1 } };

	int numMB_hor = frame->getWidth();
	int numMB_ver = frame->getHeight();

	for (int i = 0; i < frame->getNumMB(); i++){
		Macroblock *MB = frame->getMacroblock(i);
		if (MB->isMissing()){

			////////////////////////////////////////////////////////////////////////////////
			///// 1. Determine whether or not the MB's around the missing MB are available.
			////////////////////////////////////////////////////////////////////////////////

			Macroblock *MB_t, *MB_b, *MB_l, *MB_r;
			int exist_t = 1, exist_b = 1, exist_l = 1, exist_r = 1;

			// Determine upper macroblock
			if (MB->getYPos() != 0 && !frame->getMacroblock(i - numMB_hor)->isMissing()){	// For having an upper MB we should at least be on the second row, and the MB above may not be missing.
				MB_t = frame->getMacroblock(i - numMB_hor);
			}
			else {
				exist_t = 0;
			}

			// Determine lower macroblock
			if (MB->getYPos() != frame->getHeight()-1 && !frame->getMacroblock(i + numMB_hor)->isMissing()){	// For having a lower MB we may not be on the last row, and the lower MB may not be missing.
				MB_b = frame->getMacroblock(i + numMB_hor);
			}
			else {
				exist_b = 0;
			}
			
			// Determine left macroblock
			if (MB->getXPos() != 0 && !(frame->getMacroblock(i - 1)->isMissing())){	// For having a left MB we may not be in the left column, and the left MB may not be missing.
				MB_l = frame->getMacroblock(i - 1);
			}
			else {
				exist_l = 0;
			}

			// Determine right macroblock
			if (MB->getXPos() != frame->getWidth()-1 && !frame->getMacroblock(i+1)->isMissing()){// && frame->getMacroblock(i+1)->state == 0){	// For having a right MB we may not be in the right column, and the right MB may not be missing. 
				MB_r = frame->getMacroblock(i + 1);
			}
			else {
				exist_r = 0;
			}

			////////////////////////////////////////////////////////////////////////////////
			///// 2. Extract edge data by using horizontal and vertical Sobel operator.
			////////////////////////////////////////////////////////////////////////////////

			// Example: assume 3x3 MB's. The missing MB is denoted with M.
			//
			//	. . . x x x . . .
			//	. . . x O x . . .
			//	. . . x x x . . .
			//	x x x M M M x x x
			//	x O x M M M x O x
			//	x x x M M M x x x
			//	. . . x x x . . .
			//	. . . x O x . . .
			//	. . . x x x . . .
			//
			//	We want to extract edge data at the border of the missing macroblock. To be able to convolve our Sobel kernels with these edges, we convolve the 3x3 Sobel kernel
			//	with the rows or columns denoted with 'O' (we apply one row or column interleavin with the border, so wouldn't need pixels of the missing MB to convolve our 3x3 Sobel kernel).
			// 
			// For simplicity reasons, we also won't compute edge data for the pixels at the borders of the MB's (pixel 0 and 15 of a row/column), because otherwise we would also have to check if the upper-left, upper-right, ... 
			// of the missing MB's, and then use these pixels.
			// Please note that in the 3x3-MB case it seems really inefficient to do this, because we now only use 1 pixel out of 3 in a row or column, but in the 16x16-MB case we use 14 out of 16 pixels, 
			// which will be much more representative for the edge.

			int nrEdgePixels = (exist_b + exist_l + exist_r + exist_t) * 14;
			double* gradients = new double[nrEdgePixels];
			double* slopes = new double[nrEdgePixels];

			//// Edge gradients and slopes will be stored the following order: top, right, bottom, left

			int counter = 0;	// This counter will keep track of the position in the gradients and slopes array.

			// Calculate gradients and slope for top
			if (exist_t == 1){
				for (int j = 1; j < 15; j++) {
					double gradient_x = MB_t->luma[13][j - 1] * kernel_x[0][0] + MB_t->luma[13][j] * kernel_x[0][1] + MB_t->luma[13][j + 1] * kernel_x[0][2]
						+ MB_t->luma[14][j - 1] * kernel_x[1][0] + MB_t->luma[14][j + 1] * kernel_x[1][2]
						+ MB_t->luma[15][j - 1] * kernel_x[2][0] + MB_t->luma[15][j] * kernel_x[2][1] + MB_t->luma[15][j + 1] * kernel_x[2][2];

					double gradient_y = MB_t->luma[13][j - 1] * kernel_y[0][0] + MB_t->luma[13][j] * kernel_y[0][1] + MB_t->luma[13][j + 1] * kernel_y[0][2]
						+ MB_t->luma[14][j - 1] * kernel_y[1][0] + MB_t->luma[14][j + 1] * kernel_y[1][2]
						+ MB_t->luma[15][j - 1] * kernel_y[2][0] + MB_t->luma[15][j] * kernel_y[2][1] + MB_t->luma[15][j + 1] * kernel_y[2][2];
					
					gradient_x /= 4.0;
					gradient_y /= 4.0;

					// When gradient_y = 0, we will divide by zero during the slope calculation. To avoid this, we set both gradient_x and gradient_y
					// to zero. During the calculation of the dominant direction, these values will be filtered (has to be > variance).
					
					if (gradient_y != 0){
						gradients[j - 1 + counter] = pow((pow(gradient_x, 2.0) + pow(gradient_y, 2.0)), 1.0 / 2.0);
						slopes[j - 1 + counter] = 1.0 / tan(gradient_y / gradient_x);
					} else {
						gradients[j - 1 + counter] = 0;
						slopes[j - 1 + counter] = 0;
					}
				}
				counter += 14;
			}
			
			// Calculate gradients and slope for right
			if (exist_r == 1){
				for (int j = 1; j < 15; j++){

					double gradient_x = MB_r->luma[j - 1][0] * kernel_x[0][0] + MB_r->luma[j - 1][1] * kernel_x[0][1] + MB_r->luma[j - 1][2] * kernel_x[0][2]
						+ MB_r->luma[j][0] * kernel_x[1][0] + MB_r->luma[j][2] * kernel_x[1][2]
						+ MB_r->luma[j + 1][0] * kernel_x[2][0] + MB_r->luma[j + 1][1] * kernel_x[2][1] + MB_r->luma[j + 1][2] * kernel_x[2][2];
					double gradient_y = MB_r->luma[j - 1][0] * kernel_y[0][0] + MB_r->luma[j - 1][1] * kernel_y[0][1] + MB_r->luma[j - 1][2] * kernel_y[0][2]
						+ MB_r->luma[j][0] * kernel_y[1][0] + MB_r->luma[j][2] * kernel_y[1][2]
						+ MB_r->luma[j + 1][0] * kernel_y[2][0] + MB_r->luma[j + 1][1] * kernel_y[2][1] + MB_r->luma[j + 1][2] * kernel_y[2][2];

					gradient_x /= 4.0;
					gradient_y /= 4.0;

					// When gradient_y = 0, we will divide by zero during the slope calculation. To avoid this, we set both gradient_x and gradient_y
					// to zero. During the calculation of the dominant direction, these values will be filtered (has to be > variance).
					if (gradient_y != 0){
						gradients[j - 1 + counter] = pow((pow(gradient_x, 2.0) + pow(gradient_y, 2.0)), 1.0 / 2.0);
						slopes[j - 1 + counter] = 1.0 / tan(gradient_y / gradient_x);
					}
					else {
						gradients[j - 1 + counter] = 0;
						slopes[j - 1 + counter] = 0;
					}
				}
				counter += 14;
			}

			// Calculate gradients and slope for bottom
			if (exist_b == 1){
				for (int j = 1; j < 15; j++){

					double gradient_x = MB_b->luma[0][j - 1] * kernel_x[0][0] + MB_b->luma[0][j] * kernel_x[0][1] + MB_b->luma[0][j + 1] * kernel_x[0][2]
						+ MB_b->luma[1][j - 1] * kernel_x[1][0] + MB_b->luma[1][j + 1] * kernel_x[1][2]
						+ MB_b->luma[2][j - 1] * kernel_x[2][0] + MB_b->luma[2][j] * kernel_x[2][1] + MB_b->luma[2][j + 1] * kernel_x[2][2];

					double gradient_y = MB_b->luma[0][j - 1] * kernel_y[0][0] + MB_b->luma[0][j] * kernel_y[0][1] + MB_b->luma[0][j + 1] * kernel_y[0][2]
						+ MB_b->luma[1][j - 1] * kernel_y[1][0] + MB_b->luma[1][j + 1] * kernel_y[1][2]
						+ MB_b->luma[2][j - 1] * kernel_y[2][0] + MB_b->luma[2][j] * kernel_y[2][1] + MB_b->luma[2][j + 1] * kernel_y[2][2];

					gradient_x /= 4.0;
					gradient_y /= 4.0;

					// When gradient_y = 0, we will divide by zero during the slope calculation. To avoid this, we set both gradient_x and gradient_y
					// to zero. During the calculation of the dominant direction, these values will be filtered (has to be > variance).
					if (gradient_y != 0){
						gradients[j - 1 + counter] = pow((pow(gradient_x, 2.0) + pow(gradient_y, 2.0)), 1.0 / 2.0);
						slopes[j - 1 + counter] = 1.0 / tan(gradient_y / gradient_x);
					}
					else {
						gradients[j - 1 + counter] = 0;
						slopes[j - 1 + counter] = 0;
					}
				}
				counter += 14;
			}

			// Calculate gradients and slope for left
			if (exist_l == 1){
				for (int j = 1; j < 15; j++){
					double gradient_x = MB_l->luma[j - 1][13] * kernel_x[0][0] + MB_l->luma[j - 1][14] * kernel_x[0][1] + MB_l->luma[j - 1][15] * kernel_x[0][2]
						+ MB_l->luma[j][13] * kernel_x[1][0] + MB_l->luma[j][15] * kernel_x[1][2]
						+ MB_l->luma[j + 1][13] * kernel_x[2][0] + MB_l->luma[j + 1][14] * kernel_x[2][1] + MB_l->luma[j + 1][15] * kernel_x[2][2];

					double gradient_y = MB_l->luma[j - 1][13] * kernel_y[0][0] + MB_l->luma[j - 1][14] * kernel_y[0][1] + MB_l->luma[j - 1][15] * kernel_y[0][2]
						+ MB_l->luma[j][13] * kernel_y[1][0] + MB_l->luma[j][15] * kernel_y[1][2]
						+ MB_l->luma[j + 1][13] * kernel_y[2][0] + MB_l->luma[j + 1][14] * kernel_y[2][1] + MB_l->luma[j + 1][15] * kernel_y[2][2];

					gradient_x /= 4.0;
					gradient_y /= 4.0;

					// When gradient_y = 0, we will divide by zero during the slope calculation. To avoid this, we set both gradient_x and gradient_y
					// to zero. During the calculation of the dominant direction, these values will be filtered (has to be > variance).
					if (gradient_y != 0){
						gradients[j - 1 + counter] = pow((pow(gradient_x, 2.0) + pow(gradient_y, 2.0)), 1.0 / 2.0);
						slopes[j - 1 + counter] = 1.0 / tan(gradient_y / gradient_x);
					}
					else {
						gradients[j - 1 + counter] = 0;
						slopes[j - 1 + counter] = 0;
					}
				}
				counter += 14;
			}

			//////////////////////////////////////////////////////////////////////////////////
			/////// 3. Determine dominant gradient direction.
			//////////////////////////////////////////////////////////////////////////////////

			// The dominant gradient direction kan be expressed as the sum of all pixel gradients, weighted by their magnitude:
			// thèta_dominant = sum(slope_i*gradient_i) / sum(gradient_i)

			double numerator = 0.0;
			double denominator = 0.0;

			double mean = 0.0;
			for (int j = 0; j < sizeof(*gradients); j++){
				mean += gradients[j];
			}
			mean /= sizeof(*gradients);

			double variance = 0;
			for (int j = 0; j < sizeof(*gradients); j++){
				variance += (gradients[j] - mean) * (gradients[j] - mean);
			}
			variance /= (sizeof(*gradients) - 1);
			
			for (int j = 0; j < counter; j++){
				if (gradients[j] > variance && abs(slopes[j]) < 100 && gradients[j] > 0) {
					numerator += abs(gradients[j])*slopes[j];
					denominator += abs(gradients[j]);
				}
			}

			double dominant_direction = numerator*1.0 / denominator*1.0;

			if (numerator == 0 ||denominator == 0){
				dominant_direction = 1;
			}

			//////////////////////////////////////////////////////////////////////////////////
			/////// 4. Perform interpolation
			//////////////////////////////////////////////////////////////////////////////////

			//// For now, the interpolation is implemented with 1 direction. It is possible to divide 4, 8 (or another number) of dominant directions
			//// to partition the macroblock in more segments. This should yield better results.

			double slope = 1.0 / tan(dominant_direction);
			if (slope > 16.0){
				slope = 16.0;
			} else if(slope < -16.0){
				slope = -16.0;
			}

			//// The interpolation formula is p(j,k) = 1/(d1+d2) * [d2p1 + d1p2]
			//// with p1 and p2 the points in the boundaries used for interpolation.
			////		d1 and d2 the distances from the interpolated pixel to the boundary pixels.
			////		
			//// p1 = p(j1,k1), p2 = p(j2,k2)

			double slope_rel = slope*(-1.0)*(-1.0);

			// First, we interpolate the luma values:
			for (int j = 0; j < 16; j++){
				for (int k = 0; k < 16; k++){

					int p1_luma;
					int p2_luma;

					int d1_x;
					int d1_y;
					int d2_x;
					int d2_y;

					/////////////
					// P1
					/////////////

					if (slope_rel > 0){	// P1 will be part of the left or upper MB.

						int delta_y = j;
						if (delta_y % 2 != 0) delta_y++;
						int d1_x_temp = delta_y / slope_rel;

						int delta_x = k;
						if (delta_x % 2 != 0) delta_x--;
						int d1_y_temp = delta_x * slope_rel;

						if (d1_x_temp < d1_y_temp){
							d1_x = d1_x_temp;
							d1_y = j;

							int xcoord = k - d1_x;
							if (xcoord < 0) xcoord = 0;

							if (exist_t == 1){		// 
								p1_luma = MB_t->luma[15][xcoord];
							}
							else if (exist_l == 1){
								p1_luma = MB_l->luma[0][15];
							} else {
								p1_luma = 0;
							}
						}
						else {
							d1_x = k;
							d1_y = d1_y_temp;

							if (exist_l == 1){

								int ycoord = j - d1_y;
								if (ycoord < 0) ycoord = 0;

								p1_luma = MB_l->luma[ycoord][15];
							}
							else if (exist_t == 1){
								p1_luma = MB_t->luma[15][0];
							}
							else {
								p1_luma = 128;
							}
						}
					}
					else { // P1 will be part of the left or lower MB.
						int delta_y = 16 - j;
						if (delta_y % 2 != 0) delta_y++;
						int d1_x_temp = delta_y / abs(slope_rel);

						int delta_x = k;
						if (delta_x % 2 != 0) delta_x--;
						int d1_y_temp = delta_x * abs(slope_rel);

						if (d1_x_temp < d1_y_temp){					// A pixel from the bottom macroblock should be interpolated
							d1_x = d1_x_temp;
							d1_y = 16 - j;
							if (exist_b == 1){
								int xcoord = k - d1_x;
								if (xcoord < 0) xcoord = 0;
								p1_luma = MB_b->luma[0][xcoord];
							}
							else if (exist_l == 1){
								p1_luma = MB_l->luma[15][15];		// When bottom doesn't exist, we take the closest edge pixel that might be available: p(15,15) of the left MB.
							}
							else {
								p1_luma = 128;						// When none of the above are available, we take a 'neutral' value of 128.
							}
						}
						else {										// A pixel from the left macroblock should be interpolated
							d1_x = k;
							d1_y = d1_y_temp;

							if (exist_l == 1){

								int ycoord = j + d1_y;
								if (ycoord > 15) ycoord = 15;
								p1_luma = MB_l->luma[ycoord][15];
							}
							else if (exist_b == 1){
								p1_luma = MB_b->luma[0][0];
							}
							else {
								p1_luma = 128;
							}				
						}
					}

					/////////////
					// P2
					/////////////

					if (slope_rel > 0){				// When slope < 0, p2 will be part of the right or lower MB.
						
						int delta_y = 16 - j;
						if (delta_y % 2 != 0) delta_y++;
						int d2_x_temp = delta_y / slope_rel;

						int delta_x = 16-k;
						if (delta_x % 2 != 0) delta_x--;
						int d2_y_temp = delta_x * slope_rel;

						if (d2_x_temp < d2_y_temp){
							d2_x = d2_x_temp;
							d2_y = 16 - j;

							if (exist_b == 1){
								int xcoord = k + d2_x;
								if (xcoord > 15) xcoord = 15;
								p2_luma = MB_b->luma[0][xcoord];
							}
							else {
								p2_luma = p1_luma;	// When lower or right block doesn't exist, we interpolate between p1 and p1...
							}
						}
						else {							// Slope > 0, p2 will be part of the right or upper MB
							d2_x = 16 - k;
							d2_y = d2_y_temp;
							if (exist_r == 1){
								int ycoord = j + d2_y;
								if (ycoord > 15) ycoord = 15;
								p2_luma = MB_r->luma[ycoord][0];
							}
							else {
								p2_luma = p1_luma;
							}
						}
						
					}

					else {	// When slope < 1, p1 will be part of the upper or right border.
						int delta_y = j;
						if (delta_y%2 != 0) delta_y++;
						int d2_x_temp = delta_y / abs(slope_rel);

						int delta_x = 16 - k;
						if (delta_x % 2 != 0) delta_x--;
						int d2_y_temp = delta_x * abs(slope_rel);

						if (d2_x_temp < d2_y_temp){	// p2 will be in the upper macroblock
							d2_x = d2_x_temp;
							d2_y = j;
							if (exist_t == 1){	
								int xcoord = k + d2_x;
								if (xcoord > 15) xcoord = 15;
								p2_luma = MB_t->luma[15][xcoord];
							}
							else {
								p2_luma = p1_luma;
							}
						}
						else {
							d2_x = 16 - k;
							d2_y = d2_y_temp;
							int ycoord = j - d2_y;
							if (ycoord < 0) ycoord = 0;
							if (exist_r == 1){
								p2_luma = MB_r->luma[ycoord][0];
							}
							else {
								p2_luma = p1_luma;
							}
						}
					}

					double d1 = pow(pow(d1_x*1.0, 2) + pow(d1_y*1.0, 2), 1.0 / 2.0);
					double d2 = pow(pow(d2_x*1.0, 2) + pow(d2_y*1.0, 2), 1.0 / 2.0);

					if (d1 == 0 && d2 == 0){	// When both distances are zero, the interpolation formula will result in 0/0. We give both distance the same weight to solve this issue.
						d1++;
						d2++;
					}

					MB->luma[j][k] = 1.0 / (d1*1.0 + d2*1.0) * (d2*1.0*p1_luma + d1*1.0*p2_luma);

				}
			}

			////////////////////////////////////////////
			// Interpolation of Cb and Cr values
			////////////////////////////////////////////

			// The slope can't be larger than 8 for Cb and Cr, because we only have 8 vertical or horizontal pixels. For these reason, we further quantize the slope between -8 and 8.

			if (slope > 8.0){
				slope = 8.0;
			}
			else if (slope < -8.0){
				slope = -8.0;
			}

			for (int j = 0; j < 8; j++){
				for (int k = 0; k < 8; k++){
					int p1_cb;
					int p2_cb;

					int p1_cr;
					int p2_cr;

					int d1_x;
					int d1_y;
					int d2_x;
					int d2_y;

					/////////////
					// P1
					/////////////

					if (slope_rel > 0){	// P1 will be part of the left or upper MB.
						int delta_y = j;
						if (delta_y % 2 != 0) delta_y++;
						int d1_x_temp = delta_y / slope_rel;

						int delta_x = k;
						if (delta_x % 2 != 0) delta_x--;
						int d1_y_temp = delta_x * slope_rel;

						if (d1_x_temp < d1_y_temp){		// p1 will be part of the upper MB.
							d1_x = d1_x_temp;
							d1_y = j;
							if (exist_t == 1){
								int xcoord = k - d1_x;
								if (xcoord < 0) xcoord = 0;

								p1_cb = MB_t->cb[7][xcoord];
								p1_cr = MB_t->cr[7][xcoord];
							}
							else if (exist_l == 1){
								p1_cb = MB_l->cb[0][7];
								p1_cr = MB_l->cr[0][7];
							}
							else{
								p1_cb = 0;
								p1_cr = 0;
							}
						}
						else {							// p2 will be part of the left MB.
							d1_x = k;
							d1_y = d1_y_temp;

							int ycoord = j - d1_y;
							if (ycoord < 0) ycoord = 0;

							if (exist_l == 1){
								p1_cb = MB_l->cb[ycoord][7];
								p1_cr = MB_l->cr[ycoord][7];
							}
							else if (exist_t == 1){
								p1_cb = MB_t->cb[7][0];
								p1_cr = MB_t->cr[7][0];
							}
							else {
								p1_cb = 0;
								p1_cr = 0;
							}
						}
					}
					else {								// p1 will be part of the left or lower MB.
						int delta_y = 8 - j;
						if (delta_y % 2 != 0) delta_y++;
						int d1_x_temp = delta_y / abs(slope_rel);

						int delta_x = k;
						if (delta_x % 2 != 0) delta_x--;
						int d1_y_temp = delta_x * abs(slope_rel);

						if (d1_x_temp < d1_y_temp){		// p1 will be part of the lower MB.
							d1_x = d1_x_temp;
							d1_y = 8 - j;
							if (exist_b == 1){
								int xcoord = k - d1_x;
								if (xcoord < 0) xcoord = 0;
								p1_cb = MB_b->cb[0][xcoord];
								p1_cr = MB_b->cr[0][xcoord];
							}
							else if (exist_l == 1){
								p1_cb = MB_l->cb[7][7];		// When bottom doesn't exist, we take the closest edge pixel that will might be available: p(7,7) of the left MB.
								p1_cr = MB_l->cr[7][7];
							}
							else {
								p1_cb = 0;
								p1_cr = 0;
							}
						}								// p1 will be part of the left MB.
						else {
							d1_x = k;
							d1_y = d1_y_temp;
							int ycoord = j + d1_y;
							if (ycoord > 7) ycoord = 7;
							if (exist_l == 1){
								p1_cb = MB_l->cb[ycoord][7];
								p1_cr = MB_l->cr[ycoord][7];
							}
							else if (exist_b == 1){
								p1_cb = MB_b->cb[0][0];
								p1_cr = MB_b->cr[0][0];
							} else{
								p1_cb = 0;
								p1_cr = 0;
							}
						}
					}

					/////////////
					// P2
					/////////////

					if (slope_rel > 0){				// When slope < 0, p2 will be part of the right or lower MB.
						int delta_y = 8 - j;
						if (delta_y % 2 != 0) delta_y++;
						int d2_x_temp = delta_y / slope_rel;

						int delta_x = 8 - k;
						if (delta_x % 2 != 0) delta_x--;
						int d2_y_temp = delta_x * slope_rel;

						if (d2_x_temp < d2_y_temp){		// p2 will be part of the lower MB.
							d2_x = d2_x_temp;
							d2_y = 8 - j;
							if (exist_b == 1){
								int xcoord = k + d2_x;
								if (xcoord > 7) xcoord = 7;
								p2_cb = MB_b->cb[0][xcoord];
								p2_cr = MB_b->cr[0][xcoord];
							}
							else {
								p2_cb = p1_cb;	// When lower or right block doesn't exist, we interpolate between p1 and p1, which will extend the existing edges.
								p2_cr = p1_cr;
							}
						}
						else {							// p2 will be part of the right MB.
							d2_x = 8 - k;
							d2_y = d2_y_temp;
							if (exist_r == 1){
								int ycoord = j + d2_y;
								if (ycoord > 7) ycoord = 7;
								p2_cb = MB_r->cb[ycoord][0];
								p2_cr = MB_r->cr[ycoord][0];
							}
							else {
								p2_cb = p1_cb;
								p2_cr = p1_cr;
							}
						}

					}
					else {	// When slope < 1, p1 will be part of the upper or right border.
						int delta_y = j;
						if (delta_y % 2 != 0) delta_y++;
						int d2_x_temp = delta_y / abs(slope_rel);

						int delta_x = 8 - k;
						if (delta_x % 2 != 0) delta_x--;
						int d2_y_temp = delta_x * abs(slope_rel);

						if (d2_x_temp < d2_y_temp){	// p2 will be part of the upper macroblock.
							d2_x = d2_x_temp;
							d2_y = j;
							if (exist_t == 1){	
								int xcoord = k + d2_x;
								if (xcoord > 7) xcoord = 7;
								p2_cb = MB_t->cb[7][xcoord];
								p2_cr = MB_t->cr[7][xcoord];
							}
							else {
								p2_cb = p1_cb;
								p2_cr = p1_cr;
							}
						}
						else {						// p2 will be part of the right macroblock.
							d2_x = 8 - k;
							d2_y = d2_y_temp;
							if (exist_r == 1){
								int ycoord = j - d2_y;
								if (ycoord < 0) ycoord = 0;
								p2_cb = MB_r->cb[ycoord][0];
								p2_cr = MB_r->cr[ycoord][0];
							}
							else {
								p2_cb = p1_cb;
								p2_cr = p1_cr;
							}
						}
					}

					double d1 = pow(pow(d1_x*1.0, 2) + pow(d1_y*1.0, 2), 1.0 / 2.0);
					double d2 = pow(pow(d2_x*1.0, 2) + pow(d2_y*1.0, 2), 1.0 / 2.0);

					if (d1 == 0 && d2 == 0){
						d1++;
						d2++;
					}
					
					MB->cb[j][k] = 1.0 / (d1*1.0 + d2*1.0) * (d2*1.0*p1_cb + d1*1.0*p2_cb);
					MB->cr[j][k] = 1.0 / (d1*1.0 + d2*1.0) * (d2*1.0*p1_cr + d1*1.0*p2_cr);
				}
			}

			MB->setConcealed();			// Macroblock processed. --> Set concealed.

		}
	}
}

//assume no motion & use previous block
void ErrorConcealer::conceal_temporal_1(Frame *frame, Frame *referenceFrame){
	//debug & evaluation
	startChrono();
	int missing = 0;

	//block missing? use previous block (assume no motion in block)
	int numMB = frame->getNumMB();
	for (int MBx = 0; MBx < numMB; ++MBx){
		if (frame->getMacroblock(MBx)->isMissing()){
			missing++;
			Macroblock *MB = frame->getMacroblock(MBx);
			Macroblock *MBref = referenceFrame->getMacroblock(MBx);
			for (int i = 0; i < 16; ++i)	{
				for (int j = 0; j < 16; ++j)		{
					MB->luma[i][j] = MBref->luma[i][j];
					MB->cb[i/2][j/2] = MBref->cb[i/2][j/2];
					MB->cr[i/2][j/2] = MBref->cr[i/2][j/2];
				}
			}
		}
	}
	std::cout << "\t[temporal 1] Missing macroblocks: " << missing << " time needed : " << stopChrono() << endl;
}

//temporal 2
//fill a sub block based on a motion vector
inline void FillSubBMV(Macroblock *MB, Frame *frame, Frame *referenceFrame, const MotionVector &vec, const int _x, const int _y, const int subsize){

	int MBxpos = MB->getXPos() * 16;
	int MBypos = MB->getYPos() * 16;

	for (int y = MBypos + _y; y < MBypos + _y + subsize;  y++){
		int yposref = y + vec.y;

		for (int x = MBxpos + _x ; x < MBxpos + _x+subsize; x++){
			int xposref = x + vec.x;
			
			if (xposref < 0)
				xposref = 0;
			if (xposref >(frame->getWidth() * 16 - 1))
				xposref = (frame->getWidth() * 16 - 1);
			if (yposref < 0)
				yposref = 0;
			if (yposref >(frame->getHeight() * 16 - 1))
				yposref = (frame->getHeight() * 16 - 1);

			MB->luma[y - MBypos][x - MBxpos] = getYPixel(referenceFrame, xposref, yposref);
			MB->cb[(y - MBypos) / 2][(x - MBxpos) / 2] = getCbPixel(referenceFrame, xposref / 2, yposref / 2);
			MB->cr[(y - MBypos) / 2][(x - MBxpos) / 2] = getCrPixel(referenceFrame, xposref / 2, yposref / 2);
		}
	}
}
//how much does the edge of MB differ from usedMB ?
inline float CheckMB(Macroblock *MB, Frame *frame,const int MBx){
	int verschil = 0;
	int aantalvglnpixels = 0;
	int MBxpos = MB->getXPos() * 16;
	int MBypos = MB->getYPos() * 16;		

	for (int t = 0; t < 16; ++t){
		if (MB->getYPos() != 0){
			verschil += abs(MB->luma[0][t] - frame->getMacroblock(MBx - frame->getWidth())->luma[15][t]);
			++aantalvglnpixels;
		}
		if (MB->getYPos() != (frame->getHeight() - 1)){
			verschil += abs(MB->luma[15][t] - frame->getMacroblock(MBx + frame->getWidth())->luma[0][t]);
			++aantalvglnpixels;
		}
		if (MB->getXPos() != 0){
			verschil += abs(MB->luma[t][0] - frame->getMacroblock(MBx - 1)->luma[t][15]);
			++aantalvglnpixels;
		}
		if (MB->getXPos() != (frame->getWidth() - 1)){
			verschil += abs(MB->luma[t][15] - frame->getMacroblock(MBx + 1)->luma[t][0]);
			++aantalvglnpixels;
		}
	}

	float errorperpixel = float(verschil)/aantalvglnpixels;
	return errorperpixel;
}
//get motion vector for a subblock (_x;_y) in block MBx.
inline MotionVector getMV(Frame* frame, const int MBx, const int sub_x, const int sub_y, const int subsize){
	MotionVector mv;
	Macroblock* mb = frame->getMacroblock(MBx);
	bool exists_top = mb->getYPos() != 0;
	bool exists_bot = mb->getYPos() != frame->getHeight() -1;
	bool exists_left = mb->getXPos() != 0;
	bool exists_right = mb->getXPos() != frame->getWidth() - 1;

	MotionVector top, bot, left, right;
	MotionVector notthere; notthere.x = 0; notthere.y = 0;

	top = exists_top ? frame->getMacroblock(MBx - frame->getWidth())->mv : notthere;
	bot = exists_bot ? frame->getMacroblock(MBx + frame->getWidth())->mv : notthere;
	left = exists_left ? frame->getMacroblock(MBx -1)->mv : notthere;
	right = exists_right ? frame->getMacroblock(MBx +1 )->mv : notthere;

	//x
	int denominatorX = ((16 - sub_x) / subsize)*exists_left +
		((sub_x / subsize)+1)*exists_right +
		((16 - sub_y) / subsize)*exists_top +
		((sub_y / subsize)+1)*exists_bot;
	if (denominatorX != 0){
		mv.x = (
			(((16 - sub_x) / subsize)*left.x) +
			(((sub_x / subsize)+1)*right.x) +
			(((16 - sub_y) / subsize)*top.x) +
			(((sub_y / subsize)+1)*bot.x)
			) / denominatorX;
	}else{
		mv.x = 0;
	}

	//y
	int denominatorY = ((16 - sub_x) / subsize)*exists_left +
		((sub_x / subsize)+1)*exists_right +
		((16 - sub_y) / subsize)*exists_top +
		((sub_y / subsize)+1)*exists_bot;
	if (denominatorY != 0){
		mv.y = (
			(((16 - sub_x) / subsize)*left.y) +
			(((sub_x / subsize)+1)*right.y) +
			(((16 - sub_y) / subsize)*top.y) +
			(((sub_y / subsize)+1)*bot.y)
			) / denominatorY;
	}else{
		mv.y = 0;
	}


	return (denominatorX == 0 && denominatorY == 0 ? notthere : mv);
}
//conceals a macroblock by using motion estimation from 3B and returns the error. Does not set this block to concealed
float conceal_temporal_2_macroblock(Frame *frame, Frame* referenceFrame,Macroblock * MB, const int MBx, const int subsize){
	//foreach subblock
	for (int y = 0; y < 16; y += subsize){
		for (int x = 0; x < 16; x += subsize){
			//get motion vector => avg
			MotionVector vec = getMV(frame, MBx, x, y, subsize);
			//fill
			FillSubBMV(MB, frame, referenceFrame, vec, x, y, subsize);
		}
	}
	return CheckMB(MB, frame, MBx);
}
//conceals all subblock by first using motion estimation. If the error is too high then spatial interpollation is used.
void ErrorConcealer::conceal_temporal_2(Frame *frame, Frame *referenceFrame,const int size){
	//Debug and evaluation
	startChrono();
	int missing = 0;
	if (!frame->is_p_frame()){
		//if the frame is not Predictibely coded (we should have the whole frame), then we conceal using the spacial method instead.
		conceal_spatial_2(frame,true);
	}else{
		int numMB = frame->getNumMB();
		//calc subsize	
		int subsize = 1;
		for (int i = 0; i < size; i++){
			subsize *= 2;
		}
		queue<int> todo;//keep track of blocks with big errors
		MBSTATE* MBstate = new MBSTATE[numMB];

		for (int MBx = 0; MBx < numMB; ++MBx){
			Macroblock *MB = frame->getMacroblock(MBx);
			if (MB->isMissing()){
				missing++;
				float err = conceal_temporal_2_macroblock(frame, referenceFrame, MB, MBx, subsize);
				if (err > 20){ //Error too big => use spatial
					todo.push(MBx);
					MBstate[MBx] = MISSING;
				}else{
					MB->setConcealed();
					MBstate[MBx] = CONCEALED;
				}	
			}
			MBstate[MBx] = OK;
		}

		//fix blocks with big errors
		while (!todo.empty()){
			int num = todo.front();
			todo.pop();
			Macroblock* block = frame->getMacroblock(num);
			int exists_top = block->getYPos() != 0 ? 1 : 0;
			int exists_bot = block->getYPos() < frame->getHeight() - 1 ? 1 : 0;
			int exists_left = block->getXPos() != 0 ? 1 : 0;
			int exists_right = block->getXPos() < frame->getWidth() - 1 ? 1 : 0;
			f(block, &exists_left, &exists_right, &exists_top, &exists_bot, MBstate, num,getNeighbours(frame,num), frame);
			block->setConcealed();
			MBstate[num] = CONCEALED;
		}
		delete[] MBstate;
	}
	std::cout << "\t[temporal 2 (" << size << ")] Missing macroblocks: " << missing << " time needed : " << stopChrono() << endl;
}

float conceal_temporal_2_subblock(Frame * frame, Frame * referenceFrame, Macroblock* mb, const int subsize, const int _x, const int _y){
	//TODO
	//recursive
	for (int y = 0; y < 2; y++){
		for (int x = 0; x < 2; x++){

		}
	}
	return 0;
}
//same as conceal_temporal_2 but now with dynamic block sizes
void ErrorConcealer::conceal_temporal_2_dynamic(Frame* frame, Frame *referenceFrame, const int MBx){
	Macroblock *mb = frame->getMacroblock(MBx);
	//preform 16x16
	float besterr = conceal_temporal_2_macroblock(frame, referenceFrame, mb, MBx, 16);
	
	//try 8x8
	//MotionVector mv = getMV(frame, MBx, 0, 0, subsize);

	//tr 
		//try 4x4
			//tr
				//try 2x2

	//tl 
	//dr
	//dl

	


	//*/
}

//temporal 3
//same as temporal 2 but also possible when multiple adjecent blocks are missing
void ErrorConcealer::conceal_temporal_3(Frame *frame, Frame *referenceFrame){
	//debug & evaluation
	startChrono();
	int missing = 0;

	if (!frame->is_p_frame()){
		//if the frame is not Predictibely coded (we should have the whole frame), then we conceal using the spacial method instead.
		// we can't use the temporal method because the reference frame is this frame.
		conceal_spatial_2(frame, true);
	}else{
		//init
		const int numMB = frame->getNumMB();
		MBSTATE* mbstate = new MBSTATE[numMB];
		priority_queue<task, vector<task>, std::less<task>> todo;
		const int offset[] = { -frame->getWidth(), frame->getWidth(), -1, 1 };

		//Cover up almost everything, then improve the solution.
		conceal_spatial_2(frame, false);

		//determine state, fix motion && fill queue first time (we can always do motion since it only depends on the previous frame; all macroblocks, concealed or not, will be there
		for (int i = 0; i < numMB; i++){
			Macroblock* mb = frame->getMacroblock(i);
			if (mb->isMissing()){
				if (conceal_temporal_2_macroblock(frame, referenceFrame, mb, i, 2) > 20){
					mbstate[i] = MISSING;
					task element(getNeighbours(frame, i), frame->getMacroblock(i));
					todo.push(element);
				}
				else{
					mbstate[i] = CONCEALED;
					mb->setConcealed();
				}

				missing++;
			}
			else{
				mbstate[i] = OK;
			}
		}

		while (!todo.empty()){
			Macroblock* mb = todo.top().second;
			todo.pop();
			const int MBx = mb->getMBNum();

			//what blocks exists?
			int exists[] = { mb->getYPos() != 0, mb->getYPos() < frame->getHeight() - 1, mb->getXPos() != 0, mb->getXPos() < frame->getWidth() - 1 };
			f(mb, &exists[pos_LEFT], &exists[pos_RIGHT], &exists[pos_TOP], &exists[pos_BOT], mbstate, MBx, getNeighbours(frame, MBx), frame);

			mb->setConcealed();
			mbstate[MBx] = CONCEALED;
			//add neighbours again to the queue
			for (int i = 0; i < 4; i++){
				if (exists[i]){
					int item = MBx + offset[i];
					task t(getNeighbours(frame, item), frame->getMacroblock(item));
					todo.push(t);
				}
			}

			//cleanup - skip already concealed
			while (!todo.empty() && mbstate[todo.top().second->getMBNum()] != MISSING){
				todo.pop();
			}
		}
	}
	std::cout << "\t[temporal 3] Missing macroblocks: " << missing << " time needed : " << stopChrono() << endl;
}
