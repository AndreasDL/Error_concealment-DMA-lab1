#include "ErrorConcealer.h"
#include "MacroblockEmpty.h"
#include <stdio.h>
#include <iostream>
#include <math.h>

ErrorConcealer::ErrorConcealer(short conceal_method)
{
	this->conceal_method = conceal_method;
}

ErrorConcealer::~ErrorConcealer(void)
{
}

void ErrorConcealer::concealErrors(Frame *frame, Frame *referenceFrame)
{
	switch(conceal_method){
		case 0:
			conceal_spatial_1(frame);
			break;
		case 1:
			conceal_spatial_2(frame);
			break;
		case 2:
			conceal_spatial_4(frame);
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
		case 10:
			conceal_spatial_4(frame);
		default:
			printf("\nWARNING: NO ERROR CONCEALMENT PERFORMED! (conceal_method %d unknown)\n\n",conceal_method);
	}
}

void ErrorConcealer::conceal_spatial_1(Frame *frame)
{
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



			//Spatial interpolate pixels
			for (int i = 0; i < 16; ++i)	{
				for (int j = 0; j < 16; ++j)		{

					// To easily make sure we only use 2 block, we will say only 2 blocks exist (the 2 closest blocks).
					if (i >= 8){
						exist_b = 1;
						exist_t = 0;
					}
					else{
						exist_b = 0;
						exist_t = 1;
					}
					if (j >= 8){
						exist_r = 1;
						exist_l = 0;
					}
					else{
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
			for (int i = 0; i < 8; ++i)	{
				for (int j = 0; j < 8; ++j)		{
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
}


enum MBSTATE { OK, MISSING, CONCEALED };

void f(Macroblock* MB, 
	   int* exist_l, int* exist_r, int* exist_t, int* exist_b, 
	   MBSTATE* MBstate, 
	   int MBx, 
	   Frame *frame){
		   Macroblock* MB_l ;
		   Macroblock* MB_r ;
		   Macroblock* MB_t ;
		   Macroblock* MB_b ;
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

		   if (*exist_l + *exist_r + *exist_t + *exist_b > 0){
			   //Spatial interpolate pixels
			   for (int i = 0; i < 16; ++i)	{
				   for (int j = 0; j < 16; ++j)		{
					   MB->luma[i][j] = ((17 - j - 1)*MB_l->luma[i][15] * *exist_l + 
						   (j + 1)*MB_r->luma[i][0] * *exist_r + 
						   (17 - i - 1)*MB_t->luma[15][j] * *exist_t + 
						   (i + 1)*MB_b->luma[0][j] * *exist_b ) 
						   / ( 
						   ( (17 - j - 1) * *exist_l) + 
						   ( (j + 1) * *exist_r) +
						   ( (17 - i - 1) * *exist_t ) + 
						   ( (i + 1) * *exist_b )
						   );					
				   }
			   }
			   for (int i = 0; i < 8; ++i)	{
				   for (int j = 0; j < 8; ++j)		{
					   MB->cb[i][j] = ((9 - j - 1)*MB_l->cb[i][7] * *exist_l + 
						   (j + 1)*MB_r->cb[i][0] * *exist_r + 
						   (9 - i - 1)*MB_t->cb[7][j] * *exist_t + 
						   (i + 1)*MB_b->cb[0][j] * *exist_b ) 
						   / ( 
						   ( (9 - j - 1) * *exist_l) + 
						   ( (j + 1) * *exist_r) +
						   ( (9 - i - 1) * *exist_t ) + 
						   ( (i + 1) * *exist_b )
						   );
					   MB->cr[i][j] = ((9 - j - 1)*MB_l->cr[i][7] * *exist_l + 
						   (j + 1)*MB_r->cr[i][0] * *exist_r + 
						   (9 - i - 1)*MB_t->cr[7][j] * *exist_t + 
						   (i + 1)*MB_b->cr[0][j] * *exist_b ) 
						   / ( 
						   ( (9 - j - 1) * *exist_l) + 
						   ( (j + 1) * *exist_r) +
						   ( (9 - i - 1) * *exist_t ) + 
						   ( (i + 1) * *exist_b )
						   );
				   }
			   }
		   }
		   delete MBEmpty;
}

void ErrorConcealer::conceal_spatial_2(Frame *frame)
{
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
		}
		else{
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
						f( MB, &exist_l,  &exist_r,  &exist_t,  &exist_b, 
							MBstate,MBx,frame);
						if (exist_l + exist_r + exist_t + exist_b > 2){
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
			for (int MBx = 0; MBx < numMB && !oneMBConcealed; ++MBx)
			{
				MB = frame->getMacroblock(MBx);
				exist_t = 1;
				exist_b = 1;
				exist_r = 1;
				exist_l = 1;
				if (MBstate[MBx] == MISSING)
				{
					f( MB, &exist_l,  &exist_r,  &exist_t,  &exist_b, 
						MBstate,MBx,frame);
					if (exist_l + exist_r + exist_t + exist_b > 1){
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
		for (int MBx = 0; MBx < numMB && !oneMBConcealed; ++MBx)
		{
			MB = frame->getMacroblock(MBx);
			exist_t = 1;
			exist_b = 1;
			exist_r = 1;
			exist_l = 1;
			if (MBstate[MBx] == MISSING)
			{
				f( MB, &exist_l,  &exist_r,  &exist_t,  &exist_b, 
					MBstate,MBx,frame);
				if (exist_l + exist_r + exist_t + exist_b > 0){
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
}


// The edge detection function from exercise 2.C is situated in f2. This will be called from within conceal_spatial_3

void f2(Macroblock* MB,
	int* exist_l, int* exist_r, int* exist_t, int* exist_b,
	MBSTATE* MBstate,
	int MBx,
	Frame *frame){

	// This method is based upon the paper 'XXX YYY ZZZ' by xxx, yyy, zzz (link).


	// The used kernels are 2 3x3 Sobel-kernels (1 horizontal and 1 vertical).
	double kernel_x[3][3] = { { -1, 0, 1 }, { -2, 0, 2 }, { -1, 0, 1 } };
	double kernel_y[3][3] = { { 1, 2, 1 }, { 0, 0, 0 }, { -1, -2, -1 } };

	int numMB_hor = frame->getWidth();
	int numMB_ver = frame->getHeight();

	// This boolean will determine whether or not the edge detection method will be used.
	// In case there are too less MB's available (0 or 1), the conceal_spatial_2 method will be executed. (When there aren't edges available, we won't use edges).
	bool useConceal2 = false;

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

	// If there are at least 2 edges: perform interpolation.
	if (*exist_l + *exist_r + *exist_t + *exist_b < 2){
		f(MB, exist_l, exist_r, exist_t, exist_b, MBstate, MBx, frame);
	} else if (*exist_l + *exist_r + *exist_t + *exist_b > 0){
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

		int nrEdgePixels = (*exist_b + *exist_l + *exist_r + *exist_t) * 14;
		double* gradients = new double[nrEdgePixels];
		double* slopes = new double[nrEdgePixels];

		//// Edge gradients and slopes will be stored the following order: top, right, bottom, left

		int counter = 0;	// This counter will keep track of the position in the gradients and slopes array.

		// Calculate gradients and slope for top
		if (*exist_t == 1){
			for (int j = 1; j < 15; j++) {

				double gradient_x = MB_t->luma[13][j - 1] * kernel_x[0][0] + MB_t->luma[13][j] * kernel_x[0][1] + MB_t->luma[13][j + 1] * kernel_x[0][2]
					+ MB_t->luma[14][j - 1] * kernel_x[1][0] + MB_t->luma[14][j + 1] * kernel_x[1][2]
					+ MB_t->luma[15][j - 1] * kernel_x[2][0] + MB_t->luma[15][j] * kernel_x[2][1] + MB_t->luma[15][j + 1] * kernel_x[2][2];

				double gradient_y = MB_t->luma[13][j - 1] * kernel_y[0][0] + MB_t->luma[13][j] * kernel_y[0][1] + MB_t->luma[13][j + 1] * kernel_y[0][2]
					+ MB_t->luma[14][j - 1] * kernel_y[1][0] + MB_t->luma[14][j + 1] * kernel_y[1][2]
					+ MB_t->luma[15][j - 1] * kernel_y[2][0] + MB_t->luma[15][j] * kernel_y[2][1] + MB_t->luma[15][j + 1] * kernel_y[2][2];

				gradient_x /= 4.0;
				gradient_y /= 4.0;

				gradients[j - 1 + counter] = pow((pow(gradient_x, 2.0) + pow(gradient_y, 2.0)), 1.0 / 2.0);
				slopes[j - 1 + counter] = 1.0 / tan(gradient_y / gradient_x);

			}
			counter += 14;
		}

		// Calculate gradients and slope for right
		if (*exist_r == 1){
			for (int j = 1; j < 15; j++){

				double gradient_x = MB_r->luma[j - 1][0] * kernel_x[0][0] + MB_r->luma[j - 1][1] * kernel_x[0][1] + MB_r->luma[j - 1][2] * kernel_x[0][2]
					+ MB_r->luma[j][0] * kernel_x[1][0] + MB_r->luma[j][2] * kernel_x[1][2]
					+ MB_r->luma[j + 1][0] * kernel_x[2][0] + MB_r->luma[j + 1][1] * kernel_x[2][1] + MB_r->luma[j + 1][2] * kernel_x[2][2];
				
				double gradient_y = MB_r->luma[j - 1][0] * kernel_y[0][0] + MB_r->luma[j - 1][1] * kernel_y[0][1] + MB_r->luma[j - 1][2] * kernel_y[0][2]
					+ MB_r->luma[j][0] * kernel_y[1][0] + MB_r->luma[j][2] * kernel_y[1][2]
					+ MB_r->luma[j + 1][0] * kernel_y[2][0] + MB_r->luma[j + 1][1] * kernel_y[2][1] + MB_r->luma[j + 1][2] * kernel_y[2][2];

				gradient_x /= 4.0;
				gradient_y /= 4.0;

				gradients[j - 1 + counter] = pow((pow(gradient_x, 2.0) + pow(gradient_y, 2.0)), 1.0 / 2.0);
				slopes[j - 1 + counter] = 1.0 / tan(gradient_y / gradient_x);

			}
			counter += 14;
		}

		// Calculate gradients and slope for bottom
		if (*exist_b == 1){
			for (int j = 1; j < 15; j++){

				double gradient_x = MB_b->luma[0][j - 1] * kernel_x[0][0] + MB_b->luma[0][j] * kernel_x[0][1] + MB_b->luma[0][j + 1] * kernel_x[0][2]
					+ MB_b->luma[1][j - 1] * kernel_x[1][0] + MB_b->luma[1][j + 1] * kernel_x[1][2]
					+ MB_b->luma[2][j - 1] * kernel_x[2][0] + MB_b->luma[2][j] * kernel_x[2][1] + MB_b->luma[2][j + 1] * kernel_x[2][2];

				double gradient_y = MB_b->luma[0][j - 1] * kernel_y[0][0] + MB_b->luma[0][j] * kernel_y[0][1] + MB_b->luma[0][j + 1] * kernel_y[0][2]
					+ MB_b->luma[1][j - 1] * kernel_y[1][0] + MB_b->luma[1][j + 1] * kernel_y[1][2]
					+ MB_b->luma[2][j - 1] * kernel_y[2][0] + MB_b->luma[2][j] * kernel_y[2][1] + MB_b->luma[2][j + 1] * kernel_y[2][2];

				gradient_x /= 4.0;
				gradient_y /= 4.0;

				gradients[j - 1 + counter] = pow((pow(gradient_x, 2.0) + pow(gradient_y, 2.0)), 1.0 / 2.0);
				slopes[j - 1 + counter] = 1.0 / tan(gradient_y / gradient_x);

			}
			counter += 14;
		}

		// Calculate gradients and slope for left
		if (*exist_l == 1){
			for (int j = 1; j < 15; j++){

				double gradient_x = MB_l->luma[j - 1][13] * kernel_x[0][0] + MB_l->luma[j - 1][14] * kernel_x[0][1] + MB_l->luma[j - 1][15] * kernel_x[0][2]
					+ MB_l->luma[j][13] * kernel_x[1][0] + MB_l->luma[j][15] * kernel_x[1][2]
					+ MB_l->luma[j + 1][13] * kernel_x[2][0] + MB_l->luma[j + 1][14] * kernel_x[2][1] + MB_l->luma[j + 1][15] * kernel_x[2][2];

				double gradient_y = MB_l->luma[j - 1][13] * kernel_y[0][0] + MB_l->luma[j - 1][14] * kernel_y[0][1] + MB_l->luma[j - 1][15] * kernel_y[0][2]
					+ MB_l->luma[j][13] * kernel_y[1][0] + MB_l->luma[j][15] * kernel_y[1][2]
					+ MB_l->luma[j + 1][13] * kernel_y[2][0] + MB_l->luma[j + 1][14] * kernel_y[2][1] + MB_l->luma[j + 1][15] * kernel_y[2][2];

				gradient_x /= 4.0;
				gradient_y /= 4.0;

				gradients[j - 1 + counter] = pow((pow(gradient_x, 2.0) + pow(gradient_y, 2.0)), 1.0 / 2.0);
				slopes[j - 1 + counter] = 1.0 / tan(gradient_y / gradient_x);
			
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

		// Calculate the mean of the gradient.
		double mean = 0.0;
		for (int j = 0; j < sizeof(*gradients); j++){
			mean += gradients[j];
		}
		mean /= sizeof(*gradients);

		// Calculate the variance of the gradient
		double variance = 0;
		for (int j = 0; j < sizeof(*gradients); j++){
			variance += (gradients[j] - mean) * (gradients[j] - mean);
		}
		variance /= (sizeof(*gradients) - 1);

		// If the gradient is larger than the variance, the gradient will be taken into account for the calculation of the dominant direction.
		// The gradient has to be greater dan zero (but will normally always be, because it also has to be greater than the variance), to avoid dividing by zero.
		// The slope may not be larger than 16, this is the maximum slope that can be used in a 16x16 macroblock (larger would interpolate pixels from e.g. below to MB to above the MB).
		for (int j = 0; j < counter; j++){
			if (gradients[j] > variance && abs(slopes[j]) < 16 && gradients[j] > 0) {
				numerator += abs(gradients[j])*slopes[j];
				denominator += abs(gradients[j]);
			}
		}


		// Dominant direction = sum of (gradients*slopes)/sum of gradients (== weighted with gradient magnitude!) 
		double dominant_direction = numerator*1.0 / denominator*1.0;

		if (numerator == 0 || denominator == 0){
			dominant_direction = 1;
		}

		//////////////////////////////////////////////////////////////////////////////////
		/////// 4. Perform interpolation
		//////////////////////////////////////////////////////////////////////////////////

		//// The interpolation is now implemented with 1 main direction. It is possible to define 4, 8 (or another number) of dominant directions
		//// to partition the macroblock in more segments. This should yield better results.

		double slope = 1.0 / tan(dominant_direction);


		// The absolute value of the slope can't be greater than 16, because of the fact that a slope greater than 16 can't be interpolated within a 16x16 MB.
		if (slope > 16.0){
			slope = 16.0;
		}
		else if (slope < -16.0){
			slope = -16.0;
		}

		//// The interpolation formula is p(j,k) = 1/(d1+d2) * [d2p1 + d1p2]
		//// with p1 and p2 the points in the boundaries used for interpolation.
		////		d1 and d2 the distances from the interpolated pixel to the boundary pixels.
		////		
		//// p1 = p(j1,k1), p2 = p(j2,k2)

		double slope_rel = slope*(-1.0)*(-1.0);

		////////////////////////////////////////////
		// Interpolation of luma (Y) value
		////////////////////////////////////////////

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

				if (slope_rel > 0){						// P1 will be part of the left or upper MB.

					int delta_y = j;
					if (delta_y % 2 != 0) delta_y++;
					int d1_x_temp = delta_y / slope_rel;

					int delta_x = k;
					if (delta_x % 2 != 0) delta_x--;
					int d1_y_temp = delta_x * slope_rel;

					if (d1_x_temp < d1_y_temp){			// p1 is part of the left MB
						d1_x = d1_x_temp;
						d1_y = j;

						int xcoord = k - d1_x;
						if (xcoord < 0) xcoord = 0;

						if (*exist_t == 1){				// If the necessary pixel exists, conceal with edge data.
							p1_luma = MB_t->luma[15][xcoord];
						}
						else if (*exist_l == 1){		// Otherwise conceal spatially without edge data (conceal_spatial_2)
							//useConceal2 = true;
							p1_luma = MB_l->luma[0][15];
						}
						else {							// Otherwise conceal spatially without edge data (conceal_spatial_2)
							useConceal2 = true;
							p1_luma = 0;
						}
					}
					else {								// p1 is part of the right MB.
						d1_x = k;
						d1_y = d1_y_temp;

						if (*exist_l == 1){				// If the necessary pixel exists, conceal with edge data.
							int ycoord = j - d1_y;
							if (ycoord < 0) ycoord = 0;
							p1_luma = MB_l->luma[ycoord][15];
						}
						else if (*exist_t == 1){		// Otherwise conceal spatially without edge data (conceal_spatial_2)
							//useConceal2 = true;
							p1_luma = MB_t->luma[15][0];
						}
						else {							// Otherwise conceal spatially without edge data (conceal_spatial_2)
							useConceal2 = true;
							p1_luma = 128;
						}
					}
				}
				else {									// P1 will be part of the left or lower MB.
					int delta_y = 16 - j;
					if (delta_y % 2 != 0) delta_y++;
					int d1_x_temp = delta_y / abs(slope_rel);

					int delta_x = k;
					if (delta_x % 2 != 0) delta_x--;
					int d1_y_temp = delta_x * abs(slope_rel);

					if (d1_x_temp < d1_y_temp){					// p1 is part of the lower macroblock.
						d1_x = d1_x_temp;
						d1_y = 16 - j;	
						if (*exist_b == 1){						// If the necessary pixel exists, conceal with edge data.
							int xcoord = k - d1_x;
							if (xcoord < 0) xcoord = 0;
							p1_luma = MB_b->luma[0][xcoord];
						}
						else if (*exist_l == 1){				// Otherwise conceal spatially without edge data (conceal_spatial_2)
							//useConceal2 = true;
							p1_luma = MB_l->luma[15][15];		// When bottom doesn't exist, we take the closest edge pixel that might be available: p(15,15) of the left MB.
						}
						else {									// Otherwise conceal spatially without edge data (conceal_spatial_2)
							useConceal2 = true;
							p1_luma = 128;						// When none of the above are available, we take a 'neutral' value of 128.
						}
					}
					else {										// p1 is part of the left macroblock.
						d1_x = k;
						d1_y = d1_y_temp;

						if (*exist_l == 1){						// If the necessary pixel exists, conceal with edge data.

							int ycoord = j + d1_y;
							if (ycoord > 15) ycoord = 15;
							p1_luma = MB_l->luma[ycoord][15];
						}
						else if (*exist_b == 1){				// Otherwise conceal spatially without edge data (conceal_spatial_2)
							//useConceal2 = true;
							p1_luma = MB_b->luma[0][0];
						}
						else {									// Otherwise conceal spatially without edge data (conceal_spatial_2)
							useConceal2 = true;		
							p1_luma = 128;
						}
					}
				}

				/////////////
				// P2
				/////////////

				if (slope_rel > 0){								// When slope < 0, p2 will be part of the right or lower MB.

					int delta_y = 16 - j;
					if (delta_y % 2 != 0) delta_y++;
					int d2_x_temp = delta_y / slope_rel;

					int delta_x = 16 - k;
					if (delta_x % 2 != 0) delta_x--;
					int d2_y_temp = delta_x * slope_rel;

					if (d2_x_temp < d2_y_temp){					// p2 is part of the lower macroblock.
						d2_x = d2_x_temp;
						d2_y = 16 - j;

						if (*exist_b == 1){						// If the necessary pixel exists, conceal with edge data.
							int xcoord = k + d2_x;
							if (xcoord > 15) xcoord = 15;
							p2_luma = MB_b->luma[0][xcoord];
						}
						else if (*exist_r == 1){				// Otherwise conceal spatially without edge data (conceal_spatial_2)
							//useConceal2 = true;	
							p2_luma = MB_r->luma[15][0];
						}
						else {									// Otherwise conceal spatially without edge data (conceal_spatial_2)
							useConceal2 = true;
							p2_luma = p1_luma;					// When lower or right block doesn't exist, we interpolate between p1 and p1...
						}
					}
					else {										// p2 is part of the right macroblock.
						d2_x = 16 - k;
						d2_y = d2_y_temp;
						if (*exist_r == 1){						// If the necessary pixel exists, conceal with edge data.
							int ycoord = j + d2_y;
							if (ycoord > 15) ycoord = 15;
							p2_luma = MB_r->luma[ycoord][0];
						}
						else if (*exist_l == 1){				// Otherwise conceal spatially without edge data (conceal_spatial_2)
							//useConceal2 = true;
							p2_luma = MB_l->luma[0][15];
						}
						else {									// Otherwise conceal spatially without edge data (conceal_spatial_2)
							useConceal2 = true;
							p2_luma = p1_luma;
						}
					}

				}

				else {											// When slope < 1, p1 will be part of the upper or right macroblock.
					int delta_y = j;
					if (delta_y % 2 != 0) delta_y++;
					int d2_x_temp = delta_y / abs(slope_rel);

					int delta_x = 16 - k;
					if (delta_x % 2 != 0) delta_x--;
					int d2_y_temp = delta_x * abs(slope_rel);

					if (d2_x_temp < d2_y_temp){					// p2 will be in the upper macroblock
						d2_x = d2_x_temp;
						d2_y = j;
						if (*exist_t == 1){						// If the necessary pixel exists, conceal with edge data.
							int xcoord = k + d2_x;
							if (xcoord > 15) xcoord = 15;
							p2_luma = MB_t->luma[15][xcoord];
						}
						else if (*exist_r == 1){				// Otherwise conceal spatially without edge data (conceal_spatial_2)
							//useConceal2 = true;
							p2_luma = MB_r->luma[0][0];
						}
						else {									// Otherwise conceal spatially without edge data (conceal_spatial_2)
							useConceal2 = true;
							p2_luma = p1_luma;
						}
					}
					else {										// p2 will be part of the right macroblock.
						d2_x = 16 - k;
						d2_y = d2_y_temp;
						int ycoord = j - d2_y;
						if (ycoord < 0) ycoord = 0;
						if (*exist_r == 1){						// If the necessary pixel exists, conceal with edge data.
							p2_luma = MB_r->luma[ycoord][0];
						}
						else if (*exist_t == 1){				// Otherwise conceal spatially without edge data (conceal_spatial_2)
							//useConceal2 = true;
							p2_luma = MB_t->luma[15][15];
						}
						else {									// Otherwise conceal spatially without edge data (conceal_spatial_2)						
							useConceal2 = true;
							p2_luma = p1_luma;
						}
					}
				}

				// d1 = distance to p1, d2 = distance to p2.
				// d = sqrt(d1_x² + d1_y²)

				double d1 = pow(pow(d1_x*1.0, 2) + pow(d1_y*1.0, 2), 1.0 / 2.0);
				double d2 = pow(pow(d2_x*1.0, 2) + pow(d2_y*1.0, 2), 1.0 / 2.0);

				if (d1 == 0 && d2 == 0){	// When both distances are zero, the interpolation formula will result in 0/0. We give both distance the same weight to solve this issue.
					d1++;
					d2++;
				}

				// Save the outcome of the luma calculations in the pixel of the macroblock.
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

				if (slope_rel > 0){								// p1 will be part of the left or upper MB.
					int delta_y = j;
					if (delta_y % 2 != 0) delta_y++;
					int d1_x_temp = delta_y / slope_rel;

					int delta_x = k;
					if (delta_x % 2 != 0) delta_x--;
					int d1_y_temp = delta_x * slope_rel;

					if (d1_x_temp < d1_y_temp){					// p1 will be part of the upper MB.
						d1_x = d1_x_temp;
						d1_y = j;
						if (*exist_t == 1){						// If the necessary pixel exists, conceal with edge data.
							int xcoord = k - d1_x;
							if (xcoord < 0) xcoord = 0;

							p1_cb = MB_t->cb[7][xcoord];
							p1_cr = MB_t->cr[7][xcoord];
						}
						else if (*exist_l == 1){				// Otherwise conceal spatially without edge data (conceal_spatial_2)
							//useConceal2 = true;
							p1_cb = MB_l->cb[0][7];
							p1_cr = MB_l->cr[0][7];
						}
						else{									// Otherwise conceal spatially without edge data (conceal_spatial_2)
							useConceal2 = true;
							p1_cb = 0;
							p1_cr = 0;
						}
					}
					else {										// p2 will be part of the left MB.
						d1_x = k;
						d1_y = d1_y_temp;

						int ycoord = j - d1_y;
						if (ycoord < 0) ycoord = 0;

						if (*exist_l == 1){						// If the necessary pixel exists, conceal with edge data.
							p1_cb = MB_l->cb[ycoord][7];
							p1_cr = MB_l->cr[ycoord][7];
						}
						else if (*exist_t == 1){				// Otherwise conceal spatially without edge data (conceal_spatial_2)
							useConceal2 = true;
							//p1_cb = MB_t->cb[7][0];
							p1_cr = MB_t->cr[7][0];
						}
						else {									// Otherwise conceal spatially without edge data (conceal_spatial_2)
							useConceal2 = true;
							p1_cb = 0;
							p1_cr = 0;
						}
					}
				}
				else {											// p1 will be part of the left or lower MB.
					int delta_y = 8 - j;
					if (delta_y % 2 != 0) delta_y++;
					int d1_x_temp = delta_y / abs(slope_rel);

					int delta_x = k;
					if (delta_x % 2 != 0) delta_x--;
					int d1_y_temp = delta_x * abs(slope_rel);

					if (d1_x_temp < d1_y_temp){					// p1 will be part of the lower MB.
						d1_x = d1_x_temp;
						d1_y = 8 - j;
						if (*exist_b == 1){						// If the necessary pixel exists, conceal with edge data.
							int xcoord = k - d1_x;
							if (xcoord < 0) xcoord = 0;
							p1_cb = MB_b->cb[0][xcoord];
							p1_cr = MB_b->cr[0][xcoord];
						}
						else if (*exist_l == 1){				// Otherwise conceal spatially without edge data (conceal_spatial_2)
							//useConceal2 = true;
							p1_cb = MB_l->cb[7][7];					// When bottom doesn't exist, we take the closest edge pixel that will might be available: p(7,7) of the left MB.
							p1_cr = MB_l->cr[7][7];
						}
						else {									// Otherwise conceal spatially without edge data (conceal_spatial_2)
							useConceal2 = true;
							p1_cb = 0;
							p1_cr = 0;
						}
					}											// p1 will be part of the left MB.
					else {
						d1_x = k;
						d1_y = d1_y_temp;
						int ycoord = j + d1_y;
						if (ycoord > 7) ycoord = 7;
						if (*exist_l == 1){						// If the necessary pixel exists, conceal with edge data.
							p1_cb = MB_l->cb[ycoord][7];
							p1_cr = MB_l->cr[ycoord][7];
						}
						else if (*exist_b == 1){				// Otherwise conceal spatially without edge data (conceal_spatial_2)
							//useConceal2 = true;
							p1_cb = MB_b->cb[0][0];
							p1_cr = MB_b->cr[0][0];
						}
						else{									// Otherwise conceal spatially without edge data (conceal_spatial_2)
							useConceal2 = true;
							p1_cb = 0;
							p1_cr = 0;
						}
					}
				}

				/////////////
				// P2
				/////////////

				if (slope_rel > 0){								// When slope > 0, p2 will be part of the right or lower MB.
					int delta_y = 8 - j;
					if (delta_y % 2 != 0) delta_y++;
					int d2_x_temp = delta_y / slope_rel;

					int delta_x = 8 - k;
					if (delta_x % 2 != 0) delta_x--;
					int d2_y_temp = delta_x * slope_rel;

					if (d2_x_temp < d2_y_temp){					// p2 will be part of the lower MB.
						d2_x = d2_x_temp;
						d2_y = 8 - j;
						if (*exist_b == 1){						// If the necessary pixel exists, conceal with edge data.
							int xcoord = k + d2_x;
							if (xcoord > 7) xcoord = 7;
							p2_cb = MB_b->cb[0][xcoord];
							p2_cr = MB_b->cr[0][xcoord];
						}
						else if (*exist_r == 1){				// Otherwise conceal spatially without edge data (conceal_spatial_2)	
							//useConceal2 = true;
							p2_cb = MB_r->cb[7][0];
							p2_cr = MB_r->cr[7][0];
						}
						else {									// Otherwise conceal spatially without edge data (conceal_spatial_2)
							useConceal2 = true;
							p2_cb = p1_cb;							// When lower or right block doesn't exist, we interpolate between p1 and p1, which will extend the existing edges.
							p2_cr = p1_cr;
						}
					}
					else {										// p2 will be part of the right MB.
						d2_x = 8 - k;
						d2_y = d2_y_temp;
						if (*exist_r == 1){						// If the necessary pixel exists, conceal with edge data.
							int ycoord = j + d2_y;
							if (ycoord > 7) ycoord = 7;
							p2_cb = MB_r->cb[ycoord][0];
							p2_cr = MB_r->cr[ycoord][0];
						}
						else if (*exist_l == 1){				// Otherwise conceal spatially without edge data (conceal_spatial_2)	
							//useConceal2 = true;
							p2_cb = MB_l->cb[0][7];
							p2_cr = MB_l->cr[0][7];
						}
						else {									// Otherwise conceal spatially without edge data (conceal_spatial_2)
							useConceal2 = true;
							p2_cb = p1_cb;
							p2_cr = p1_cr;
						}
					}

				}
				else {											// When slope < 0, p1 will be part of the upper or right border.
					int delta_y = j;
					if (delta_y % 2 != 0) delta_y++;
					int d2_x_temp = delta_y / abs(slope_rel);

					int delta_x = 8 - k;
					if (delta_x % 2 != 0) delta_x--;
					int d2_y_temp = delta_x * abs(slope_rel);

					if (d2_x_temp < d2_y_temp){					// p2 will be part of the upper macroblock.
						d2_x = d2_x_temp;
						d2_y = j;
						if (*exist_t == 1){						// If the necessary pixel exists, conceal with edge data.
							int xcoord = k + d2_x;
							if (xcoord > 7) xcoord = 7;
							p2_cb = MB_t->cb[7][xcoord];
							p2_cr = MB_t->cr[7][xcoord];
						}
						else if (*exist_r == 1){				// Otherwise conceal spatially without edge data (conceal_spatial_2)	
							//useConceal2 = true;
							p2_cb = MB_r->cb[0][0];
							p2_cr = MB_r->cr[0][0];
						}	
						else {									// Otherwise conceal spatially without edge data (conceal_spatial_2)	
							useConceal2 = true;
							p2_cb = p1_cb;
							p2_cr = p1_cr;
						}
					}
					else {										// p2 will be part of the right macroblock.
						d2_x = 8 - k;
						d2_y = d2_y_temp;
						if (*exist_r == 1){						// If the necessary pixel exists, conceal with edge data.
							int ycoord = j - d2_y;
							if (ycoord < 0) ycoord = 0;
							p2_cb = MB_r->cb[ycoord][0];
							p2_cr = MB_r->cr[ycoord][0];
						}
						else if (*exist_t == 1){				// Otherwise conceal spatially without edge data (conceal_spatial_2)	
							//useConceal2 = true;
							p2_cb = MB_t->cb[7][7];
							p2_cr = MB_t->cr[7][7];
						}
						else {									// Otherwise conceal spatially without edge data (conceal_spatial_2)	
							useConceal2 = true;
							p2_cb = p1_cb;
							p2_cr = p1_cr;
						}
					}
				}


				// d1 = distance to p1, d2 = distance to p2.
				// d = sqrt(d1_x² + d1_y²)
				double d1 = pow(pow(d1_x*1.0, 2) + pow(d1_y*1.0, 2), 1.0 / 2.0);
				double d2 = pow(pow(d2_x*1.0, 2) + pow(d2_y*1.0, 2), 1.0 / 2.0);

				if (d1 == 0 && d2 == 0){	// When both distances are zero, the interpolation formula will result in 0/0. We give both distance the same weight to solve this issue.
					d1++;
					d2++;
				}

				// Save the outcomes of the cb an cr calculations in the pixel of the macroblock.
				MB->cb[j][k] = 1.0 / (d1*1.0 + d2*1.0) * (d2*1.0*p1_cb + d1*1.0*p2_cb);
				MB->cr[j][k] = 1.0 / (d1*1.0 + d2*1.0) * (d2*1.0*p1_cr + d1*1.0*p2_cr);

			}
		}
	}

	if (useConceal2){		// When useConceal2 was set during the calculations, conceal spatially without edge data (conceal_spatial_2).
		f(MB, exist_l, exist_r, exist_t, exist_b, MBstate, MBx, frame);
	}

	delete MBEmpty;
}

void ErrorConcealer::conceal_spatial_4(Frame *frame)
{
	// This method is the same is in conceal_spatial_2, to optimally deal with the complex error pattern.

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
		}
		else{
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
						f2(MB, &exist_l, &exist_r, &exist_t, &exist_b,
							MBstate, MBx, frame);
						if (exist_l + exist_r + exist_t + exist_b > 2){		// First commit concealment of the macroblocks with at least 3 existing neighbour macroblocks.
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
			for (int MBx = 0; MBx < numMB && !oneMBConcealed; ++MBx)
			{
				MB = frame->getMacroblock(MBx);
				exist_t = 1;
				exist_b = 1;
				exist_r = 1;
				exist_l = 1;
				if (MBstate[MBx] == MISSING)
				{
					f2(MB, &exist_l, &exist_r, &exist_t, &exist_b,
						MBstate, MBx, frame);
					if (exist_l + exist_r + exist_t + exist_b > 1){			// Commit concealment of the macroblocks with at least 2 existing neighbour macroblocks.
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
		for (int MBx = 0; MBx < numMB && !oneMBConcealed; ++MBx)
		{
			MB = frame->getMacroblock(MBx);
			exist_t = 1;
			exist_b = 1;
			exist_r = 1;
			exist_l = 1;
			if (MBstate[MBx] == MISSING)
			{
				f2(MB, &exist_l, &exist_r, &exist_t, &exist_b,
					MBstate, MBx, frame);
				if (exist_l + exist_r + exist_t + exist_b > 0){			// Commit concealment of macroblocks with at least 1 neighbour.
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
}


void ErrorConcealer::conceal_spatial_3(Frame *frame)
{
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

			double slope_rel = slope;

			////////////////////////////////////////////
			// Interpolation of luma (Y) value
			////////////////////////////////////////////

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



void ErrorConcealer::conceal_temporal_1(Frame *frame, Frame *referenceFrame)
{
	int numMB = frame->getNumMB();
	for (int MBx = 0; MBx < numMB; ++MBx){
		if (frame->getMacroblock(MBx)->isMissing()){
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
}

enum MBPOSITION { NONE, TOP, BOTTOM, LEFT, RIGHT , SPATIAL};

pixel getYPixel(Frame *frame, int posx, int posy){
	int MBx = (int(posy/16) * frame->getWidth() ) ;
	MBx += (int(posx/16));
	Macroblock *MB = frame->getMacroblock(MBx);
	pixel luma = MB->luma[posy%16][posx%16];
	return luma;	
}

pixel getCbPixel(Frame *frame, int posx, int posy){
	int MBx = (int(posy/8) * frame->getWidth() ) ;
	MBx += (int(posx/8));
	Macroblock *MB = frame->getMacroblock(MBx);
	pixel cb = MB->cb[posy%8][posx%8];
	return cb;	
}


pixel getCrPixel(Frame *frame, int posx, int posy){
	int MBx = (int(posy/8) * frame->getWidth() ) ;
	MBx += (int(posx/8));
	Macroblock *MB = frame->getMacroblock(MBx);
	pixel cr = MB->cr[posy%8][posx%8];
	return cr;	
}


void FillMB(Macroblock *MB, Macroblock *usedMB, Frame *frame, Frame *referenceFrame){
	int MBxpos = MB->getXPos();
	int MBypos = MB->getYPos();
	for (int i = 0; i < 16; ++i){
		for (int j = 0; j < 16; ++j){
			int xposref = (MBxpos*16) + j + usedMB->mv.x;
			int yposref = (MBypos*16) + i + usedMB->mv.y;
			if(xposref < 0)
				xposref = 0;
			if(xposref > (frame->getWidth() * 16 - 1))
				xposref = (frame->getWidth() * 16 - 1);
			if(yposref < 0)
				yposref = 0;
			if(yposref > (frame->getHeight() * 16 - 1))
				yposref = (frame->getHeight() * 16 - 1);
			MB->luma[i][j] = getYPixel(referenceFrame, xposref, yposref);
			MB->cb[i/2][j/2] = getCbPixel(referenceFrame, xposref/2, yposref/2);
			MB->cr[i/2][j/2] = getCrPixel(referenceFrame, xposref/2, yposref/2);
		}
	}
}

float CheckMB(Macroblock *MB, Macroblock *usedMB, Frame *frame, int MBx){
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

void ErrorConcealer::conceal_temporal_2(Frame *frame, Frame *referenceFrame, int size)
{
// Sub-macroblock size to be completed. Add explanatory notes in English.
	if(!frame->is_p_frame()){
		conceal_spatial_2(frame);
	}
	else{
		float error = 99999999;
		MBPOSITION bestResult = NONE;
		int numMB = frame->getNumMB();
		Macroblock *usedMB; //MB in same frame, used for MV (TOP, BOTTOM,...)
		for (int MBx = 0; MBx < numMB; ++MBx){
			if (frame->getMacroblock(MBx)->isMissing()){
				Macroblock *MB = frame->getMacroblock(MBx);
				//check TOP 
				if (MB->getYPos() != 0){
					usedMB = frame->getMacroblock(MBx - frame->getWidth());
					FillMB(MB, usedMB, frame, referenceFrame);
					float errorperpixel = CheckMB(MB, usedMB, frame, MBx);
					if(errorperpixel < error){
						error = errorperpixel;
						bestResult = TOP;
					}
				}
				//check BOTTOM 
				if (MB->getYPos() != (frame->getHeight() - 1)){
					usedMB = frame->getMacroblock(MBx + frame->getWidth());
					FillMB(MB, usedMB, frame, referenceFrame);
					float errorperpixel = CheckMB(MB, usedMB, frame, MBx);
					if(errorperpixel < error){
						error = errorperpixel;
						bestResult = BOTTOM;
					}
				}
				//check LEFT 
				if (MB->getXPos() != 0){
					usedMB = frame->getMacroblock(MBx - 1);
					FillMB(MB, usedMB, frame, referenceFrame);
					float errorperpixel = CheckMB(MB, usedMB, frame, MBx);
					if(errorperpixel < error){
						error = errorperpixel;
						bestResult = LEFT;
					}
				}
				//check RIGHT 
				if (MB->getXPos() != (frame->getWidth() - 1)){
					usedMB = frame->getMacroblock(MBx + 1);
					FillMB(MB, usedMB, frame, referenceFrame);
					float errorperpixel = CheckMB(MB, usedMB, frame, MBx);
					if(errorperpixel < error){
						error = errorperpixel;
						bestResult = RIGHT;
					}
				}

				switch (bestResult){
				case TOP:
					usedMB = frame->getMacroblock(MBx - frame->getWidth());
					FillMB(MB, usedMB, frame, referenceFrame);
					printf("T");
					break;
				case BOTTOM:
					usedMB = frame->getMacroblock(MBx + frame->getWidth());
					FillMB(MB, usedMB, frame, referenceFrame);
					printf("B");
					break;
				case LEFT:
					usedMB = frame->getMacroblock(MBx - 1);
					FillMB(MB, usedMB, frame, referenceFrame);
					printf("L");
					break;
				case RIGHT:
					usedMB = frame->getMacroblock(MBx + 1);
					FillMB(MB, usedMB, frame, referenceFrame);
					printf("R");
					break;				
				}
			}
		}
	}
}


void conceal_spatial_2_zonder_setConcealed(Frame *frame)
{
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
		}
		else{
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
						f( MB, &exist_l,  &exist_r,  &exist_t,  &exist_b, 
							MBstate,MBx,frame);
						if (exist_l + exist_r + exist_t + exist_b > 2){
							//MB->setConcealed();
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
			for (int MBx = 0; MBx < numMB && !oneMBConcealed; ++MBx)
			{
				MB = frame->getMacroblock(MBx);
				exist_t = 1;
				exist_b = 1;
				exist_r = 1;
				exist_l = 1;
				if (MBstate[MBx] == MISSING)
				{
					f( MB, &exist_l,  &exist_r,  &exist_t,  &exist_b, 
						MBstate,MBx,frame);
					if (exist_l + exist_r + exist_t + exist_b > 1){
						//MB->setConcealed();
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
		for (int MBx = 0; MBx < numMB && !oneMBConcealed; ++MBx)
		{
			MB = frame->getMacroblock(MBx);
			exist_t = 1;
			exist_b = 1;
			exist_r = 1;
			exist_l = 1;
			if (MBstate[MBx] == MISSING)
			{
				f( MB, &exist_l,  &exist_r,  &exist_t,  &exist_b, 
					MBstate,MBx,frame);
				if (exist_l + exist_r + exist_t + exist_b > 0){
					//MB->setConcealed();
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
}

void FillMB_temporal_3(Macroblock *MB, Macroblock *tempMB, Macroblock *usedMB, Frame *frame, Frame *referenceFrame){
	int MBxpos = MB->getXPos();
	int MBypos = MB->getYPos();
	for (int i = 0; i < 16; ++i){
		for (int j = 0; j < 16; ++j){
			int xposref = (MBxpos*16) + j + usedMB->mv.x;
			int yposref = (MBypos*16) + i + usedMB->mv.y;
			if(xposref < 0)
				xposref = 0;
			if(xposref > (frame->getWidth() * 16 - 1))
				xposref = (frame->getWidth() * 16 - 1);
			if(yposref < 0)
				yposref = 0;
			if(yposref > (frame->getHeight() * 16 - 1))
				yposref = (frame->getHeight() * 16 - 1);
			tempMB->luma[i][j] = getYPixel(referenceFrame, xposref, yposref);
			tempMB->cb[i/2][j/2] = getCbPixel(referenceFrame, xposref/2, yposref/2);
			tempMB->cr[i/2][j/2] = getCrPixel(referenceFrame, xposref/2, yposref/2);
		}
	}
}
float CheckMB_temporal_3(Macroblock *MB,Macroblock *tempMB, Frame *frame, int MBx){
	int verschil = 0;
	int aantalvglnpixels = 0;
	int MBxpos = MB->getXPos() * 16;
	int MBypos = MB->getYPos() * 16;				
	for (int t = 0; t < 16; ++t){
		if (MB->getYPos() != 0){
			verschil += abs(tempMB->luma[0][t] - frame->getMacroblock(MBx - frame->getWidth())->luma[15][t]);
			++aantalvglnpixels;
		}
		if (MB->getYPos() != (frame->getHeight() - 1)){
			verschil += abs(tempMB->luma[15][t] - frame->getMacroblock(MBx + frame->getWidth())->luma[0][t]);
			++aantalvglnpixels;
		}
		if (MB->getXPos() != 0){
			verschil += abs(tempMB->luma[t][0] - frame->getMacroblock(MBx - 1)->luma[t][15]);
			++aantalvglnpixels;
		}
		if (MB->getXPos() != (frame->getWidth() - 1)){
			verschil += abs(tempMB->luma[t][15] - frame->getMacroblock(MBx + 1)->luma[t][0]);
			++aantalvglnpixels;
		}
	}
	float errorperpixel = float(verschil)/aantalvglnpixels;
	return errorperpixel;
}

void ErrorConcealer::conceal_temporal_3(Frame *frame, Frame *referenceFrame)
{
	conceal_spatial_2_zonder_setConcealed(frame);
	float error = 99999999;
	MBPOSITION bestResult = NONE;
	int numMB = frame->getNumMB();
	Macroblock *usedMB; //MB in same frame, used for MV (TOP, BOTTOM,...)
	Macroblock tempMB;
	for (int MBx = 0; MBx < numMB; ++MBx){
		if (frame->getMacroblock(MBx)->isMissing()){
			Macroblock *MB = frame->getMacroblock(MBx);
			//check TOP 
			if (MB->getYPos() != 0){
				usedMB = frame->getMacroblock(MBx - frame->getWidth());
				FillMB_temporal_3(MB,&tempMB, usedMB, frame, referenceFrame);
				float errorperpixel = CheckMB_temporal_3(MB,&tempMB, frame, MBx);
				if(errorperpixel < error){
					error = errorperpixel;
					bestResult = TOP;
				}
			}
			//check BOTTOM 
			if (MB->getYPos() != (frame->getHeight() - 1)){
				usedMB = frame->getMacroblock(MBx + frame->getWidth());
				FillMB_temporal_3(MB,&tempMB, usedMB, frame, referenceFrame);
				float errorperpixel = CheckMB_temporal_3(MB,&tempMB, frame, MBx);
				if(errorperpixel < error){
					error = errorperpixel;
					bestResult = BOTTOM;
				}
			}
			//check LEFT 
			if (MB->getXPos() != 0){
				usedMB = frame->getMacroblock(MBx - 1);
				FillMB_temporal_3(MB,&tempMB, usedMB, frame, referenceFrame);
				float errorperpixel = CheckMB_temporal_3(MB,&tempMB, frame, MBx);
				if(errorperpixel < error){
					error = errorperpixel;
					bestResult = LEFT;
				}
			}
			//check RIGHT 
			if (MB->getXPos() != (frame->getWidth() - 1)){
				usedMB = frame->getMacroblock(MBx + 1);
				FillMB_temporal_3(MB,&tempMB, usedMB, frame, referenceFrame);
				float errorperpixel = CheckMB_temporal_3(MB,&tempMB, frame, MBx);
				if(errorperpixel < error){
					error = errorperpixel;
					bestResult = RIGHT;
				}
			}
			//check spatial
			//float errorperpixel = CheckMB_temporal_3(MB,MB, frame, MBx);
			if(error > 25){
				bestResult = SPATIAL;
			}
			switch (bestResult){
				case TOP:
					usedMB = frame->getMacroblock(MBx - frame->getWidth());
					FillMB(MB, usedMB, frame, referenceFrame);
					printf("T");
					break;
				case BOTTOM:
					usedMB = frame->getMacroblock(MBx + frame->getWidth());
					FillMB(MB, usedMB, frame, referenceFrame);
					printf("B");
					break;
				case LEFT:
					usedMB = frame->getMacroblock(MBx - 1);
					FillMB(MB, usedMB, frame, referenceFrame);
					printf("L");
					break;
				case RIGHT:
					usedMB = frame->getMacroblock(MBx + 1);
					FillMB(MB, usedMB, frame, referenceFrame);
					printf("R");
					break;	
				case SPATIAL:
					printf("S");
					break;	
			}
		}
	}
}


