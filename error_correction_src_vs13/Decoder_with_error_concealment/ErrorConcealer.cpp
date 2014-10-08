#include "ErrorConcealer.h"
#include "MacroblockEmpty.h"
#include <stdio.h>
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

void ErrorConcealer::conceal_spatial_3(Frame *frame)
{
	int kernel_x[3][3] = { { -1, 0, 1 }, { -2, 0, 2 }, { -1, 0, 1 } };
	int kernel_y[3][3] = { { 1, 2, 1 }, { 0, 0, 0 }, { -1, -2, -1 } };

	int numMB_hor = frame->getWidth() / 16;
	int numMB_ver = frame->getHeight() / 16;

	for (int i = 0; i < frame->getNumMB(); i++){
		Macroblock *MB = frame->getMacroblock(i);
		if (MB->isMissing()){

			////////////////////////////////////////////////////////////////////////////////
			///// 1. Determine whether or not the MB's around the missing MB are available.
			////////////////////////////////////////////////////////////////////////////////

			Macroblock *MB_t, *MB_b, *MB_l, *MB_r;
			int exist_t = 1, exist_b = 1, exist_l = 1, exist_r = 1;

			// Determine upper macroblock
			if (i >= numMB_hor && !frame->getMacroblock(i - numMB_hor)->isMissing()){	// For having an upper MB we should at least be on the second row, and the MB above may not be missing.
				MB_t = frame->getMacroblock(i - numMB_hor);
			}
			else {
				exist_t = 0;
			}

			// Determine lower macroblock
			if (i < frame->getNumMB()-numMB_hor && !frame->getMacroblock(i + numMB_hor)->isMissing()){	// For having a lower MB we may not be on the last row, and the lower MB may not be missing.
				MB_b = frame->getMacroblock(i + numMB_hor);
			}
			else {
				exist_b = 0;
			}

			// Determine left macroblock
			if (i % numMB_hor != 0 && !frame->getMacroblock(i - 1)->isMissing()){	// For having a left MB we may not be in the left column, and the left MB may not be missing.
				MB_l = frame->getMacroblock(i - 1);
			}
			else {
				exist_l = 0;
			}

			// Determine right macroblock
			if (i % numMB_hor != numMB_hor - 1 && !frame->getMacroblock(i - numMB_hor)->isMissing()){	// For having a right MB we may not be in the right column, and the right MB may not be missing. 
				MB_r = frame->getMacroblock(i - numMB_hor);
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
			// For simplicity reasons, we also won't compute edge data for the pixels at the borders of the MB's, because otherwise we would also have to check if the upper-left, upper-right, ... 
			// of the missing MB's, and then use these pixels.
			// Please note that in the 3x3-MB case it seems really inefficient to do this, because we now only use 1 pixel out of 3 in a row or column, but in the 16x16-MB case we use 14 out of 16 pixels, 
			// which will be much more representative for the edge.

			int nrEdgePixels = (exist_b + exist_l + exist_r + exist_t) * 14;
			double* gradients = new double[nrEdgePixels];
			double* slopes = new double[nrEdgePixels];

			//// Edge gradients and slopes will be stored the following order: top, right, bottom, left

			int counter = 1;	// This counter will keep track of the position in the gradients and slopes array.

			//// Calculate gradients and slope for top
			if (exist_t == 1){
				for (int j = counter; j < counter+14; j++){

					double gradient_x = MB_t->luma[13][j - 1] * kernel_x[0][0] + MB_t->luma[13][j] * kernel_x[0][1] + MB_t->luma[13][j + 1] * kernel_x[0][2]
						+ MB_t->luma[14][j - 1] * kernel_x[1][0] + MB_t->luma[14][j + 1] * kernel_x[1][2]
						+ MB_t->luma[15][j - 1] * kernel_x[2][0] + MB_t->luma[15][j] * kernel_x[2][1] + MB_t->luma[15][j + 1] * kernel_x[2][2];

					double gradient_y = MB_t->luma[13][j - 1] * kernel_y[0][0] + MB_t->luma[13][j] * kernel_y[0][1] + MB_t->luma[13][j + 1] * kernel_y[0][2]
						+ MB_t->luma[14][j - 1] * kernel_y[1][0] + MB_t->luma[14][j + 1] * kernel_y[1][2]
						+ MB_t->luma[15][j - 1] * kernel_y[2][0] + MB_t->luma[15][j] * kernel_y[2][1] + MB_t->luma[15][j + 1] * kernel_y[2][2];

					gradients[j - 1] = pow((pow(gradient_x, 2) + pow(gradient_y, 2)), 1 / 2);
					slopes[j - 1] = 1 / tan(gradient_y / gradient_x);
				}
				counter += 14;
			}
			
			//// Calculate gradients and slope for right
			//if (exist_r == 1){
			//	for (int j = counter; j < counter + 14; j++){
			//		double gradient_x = MB_r->luma[j - 1][0] * kernel_x[0][0] + MB_r->luma[j - 1][1] * kernel_x[0][1] + MB_r->luma[j - 1][2] * kernel_x[0][2]
			//			+ MB_r->luma[j][0] * kernel_x[1][0] + MB_r->luma[j][2] * kernel_x[1][2]
			//			+ MB_r->luma[j + 1][0] * kernel_x[2][0] + MB_r->luma[j + 1][1] * kernel_x[2][1] + MB_r->luma[j + 1][2] * kernel_x[2][2];

			//		double gradient_y = MB_r->luma[j - 1][0] * kernel_y[0][0] + MB_r->luma[j - 1][1] * kernel_y[0][1] + MB_r->luma[j - 1][2] * kernel_y[0][2]
			//			+ MB_r->luma[j][0] * kernel_y[1][0] + MB_r->luma[j][2] * kernel_y[1][2]
			//			+ MB_r->luma[j + 1][0] * kernel_y[2][0] + MB_r->luma[j + 1][1] * kernel_y[2][1] + MB_r->luma[j + 1][2] * kernel_y[2][2];

			//		gradients[j - 1] = pow((pow(gradient_x, 2) + pow(gradient_y, 2)), 1 / 2);
			//		slopes[j - 1] = 1 / tan(gradient_y / gradient_x);
			//	}
			//	counter += 14;
			//}


			//// Calculate gradients and slope for bottom
			//if (exist_b == 1){
			//	for (int j = counter; j < counter + 14; j++){

			//		double gradient_x = MB_b->luma[0][j - 1] * kernel_x[0][0] + MB_b->luma[0][j] * kernel_x[0][1] + MB_b->luma[0][j + 1] * kernel_x[0][2]
			//			+ MB_b->luma[1][j - 1] * kernel_x[1][0] + MB_b->luma[1][j + 1] * kernel_x[1][2]
			//			+ MB_b->luma[2][j - 1] * kernel_x[2][0] + MB_b->luma[2][j] * kernel_x[2][1] + MB_b->luma[2][j + 1] * kernel_x[2][2];

			//		double gradient_y = MB_b->luma[0][j - 1] * kernel_y[0][0] + MB_b->luma[0][j] * kernel_y[0][1] + MB_b->luma[0][j + 1] * kernel_y[0][2]
			//			+ MB_b->luma[1][j - 1] * kernel_y[1][0] + MB_b->luma[1][j + 1] * kernel_y[1][2]
			//			+ MB_b->luma[2][j - 1] * kernel_y[2][0] + MB_b->luma[2][j] * kernel_y[2][1] + MB_b->luma[2][j + 1] * kernel_y[2][2];

			//		gradients[j - 1] = pow((pow(gradient_x, 2) + pow(gradient_y, 2)), 1 / 2);
			//		slopes[j - 1] = 1 / tan(gradient_y / gradient_x);
			//	}
			//	counter += 14;
			//}

			//// Calculate gradients and slope for left
			//if (exist_r == 1){
			//	for (int j = counter; j < counter + 14; j++){
			//		double gradient_x = MB_l->luma[j - 1][13] * kernel_x[0][0] + MB_l->luma[j - 1][14] * kernel_x[0][1] + MB_l->luma[j - 1][15] * kernel_x[0][2]
			//			+ MB_l->luma[j][13] * kernel_x[1][0] + MB_l->luma[j][15] * kernel_x[1][2]
			//			+ MB_l->luma[j + 1][0] * kernel_x[2][0] + MB_l->luma[j + 1][1] * kernel_x[2][1] + MB_l->luma[j + 1][2] * kernel_x[2][2];

			//		double gradient_y = MB_l->luma[j - 1][13] * kernel_y[0][0] + MB_l->luma[j - 1][14] * kernel_y[0][1] + MB_l->luma[j - 1][15] * kernel_y[0][2]
			//			+ MB_l->luma[j][13] * kernel_y[1][0] + MB_l->luma[j][15] * kernel_y[1][2]
			//			+ MB_l->luma[j + 1][13] * kernel_y[2][0] + MB_l->luma[j + 1][14] * kernel_y[2][1] + MB_l->luma[j + 1][15] * kernel_y[2][2];

			//		gradients[j - 1] = pow((pow(gradient_x, 2) + pow(gradient_y, 2)), 1 / 2);
			//		slopes[j - 1] = 1 / tan(gradient_y / gradient_x);
			//	}
			//	counter += 14;
			//}

			//////////////////////////////////////////////////////////////////////////////////
			/////// 3. Determine dominant gradient direction.
			//////////////////////////////////////////////////////////////////////////////////

			//// The dominant gradient direction kan be expressed as the sum of all pixel gradients, weighted by their magnitude:
			//// thèta_dominant = sum(slope_i*gradient_i) / sum(gradient_i)

			//double nominator = 0;
			//double denominator = 0;

			//for (int j = 0; j < counter - 1; j++){
			//	nominator += abs(gradients[i])*slopes[i];
			//	denominator += abs(gradients[i]);
			//}

			//double dominant_direction = nominator / denominator;

			//////////////////////////////////////////////////////////////////////////////////
			/////// 4. Perform interpolation
			//////////////////////////////////////////////////////////////////////////////////

			//// For now, the interpolation is implemented with 1 direction. It is possible to divide 4, 8 (or another number) of dominant directions
			//// to partition the macroblock in more segments. This should yield better results.

			//double slope = 1 / tan(dominant_direction);

			//// The interpolation formula is p(j,k) = 1/(d1+d2) * [d2p1 + d1p2]
			//// with p1 and p2 the points in the boundaries used for interpolation.
			////		d1 and d2 the distances from the interpolated pixel to the boundary pixels.
			////		
			//// p1 = p(j1,k1), p2 = p(j2,k2)
			//// j1 = max(j - j*1/slope; 0), k1 = max(k - k*slope;0)
			//// j2 = min(j + j*1/slope; N+1), k2 = min(k + k*slope; N+1) with N the size of a macroblock

			for (int j = 0; j < 16; j++){
				for (int k = 0; k < 16; k++){
					//int j1 = fmax(j - j * 1 / slope, 0);
					//int k1 = fmax(k - k * slope, 0);

					//int j2 = fmin(j + j * 1 / slope, 16 + 1);
					//int k2 = fmin(k + k * slope, 16 + 1);

					//int d1_x = j * 1.0 - j1 * 1.0;
					//int d1_y = k * 1.0 - k1 * 1.0;
					//int d2_x = j * 1.0 + j2 * 1.0;
					//int d2_y = k * 1.0 + k2 * 1.0;

					//double d1 = pow(pow(d1_x*1.0, 2) + pow(d1_y*1.0, 2), 1 / 2);
					//double d2 = pow(pow(d2_x*1.0, 2) + pow(d2_y*1.0, 2), 1 / 2);

					//int p1;
					//int p2;

			//		// We need the value of the pixels we are going to interpolate. We can do this with the distances d1x,y, d2x,y we calculated.
			//		// We can derive from the formulas that p1 will never be a border pixel of the right macroblock. We will check for every other macroblock.
			//		if (j + d1_y < 0 && exist_t == 1){
			//			// We need a pixel from the upper border.
			//			p1 = MB_t->luma[15][k + d1_x];
			//		}
			//		else if (j+d1_y > 15 && exist_b == 1){
			//			// We need a pixel from the lower border.
			//			p1 = MB_b->luma[0][k + d1_x];
			//		}
			//		else if (k+d1_x < 0 && exist_l == 1){
			//			// We need a pixel from the left border.
			//			p1 = MB_l->luma[j + d1_y][15];
			//		}


			//		// p2 will never be a border pixel of the left macroblock. We will check for every other macroblock.
			//		if (j + d2_y < 0 && exist_t == 1){
			//			// We need a pixel from the upper border.
			//			p2 = MB_t->luma[15][k + d2_x];
			//		}
			//		else if (j + d2_y > 15 && exist_b == 1){
			//			// We need a pixel from the lower border.
			//			p2 = MB_b->luma[0][k + d2_x];
			//		}
			//		else if (k + d1_x < 0 && exist_r == 1){
			//			// We need a pixel from the right border.
			//			p2 = MB_r->luma[j+d2_y][0];
			//		}

			//		//MB->luma[j][k] = 1.0 / (d1*1.0 + d2*1.0) * (d2*1.0*p1 + d1*1.0*p2);

				}
			}

			//MB->setConcealed();

		}
	}


	/*
	int kRows = 3;
	int kCols = 3;

	int kCenterX = 1;
	int kCenterY = 1;

	int nn, mm, ii, jj;

	Frame filtered(frame->getWidth(), frame->getHeight());


	for (int i = 0; i < frame->getHeight(); i++){			// rows
		for (int j = 0; j < frame->getWidth(); j++){		// columns
			for (int m = 0; m < kRows; m++){				// kernel rows
				mm = kRows -1 - m;							// row index of flipped kernel
				for (int n = 0; n < kCols; n++){			// kernel columns
					nn = kCols - 1 - n;						// column index of flipped kernel
					
					// index of input signal, used for checking boundary
					ii = i + (m - kCenterY);
					jj = j + (n - kCenterX);

					// ignore input samples which are out of bounds
					if (ii >= 0 && ii < frame->getHeight() && jj >= 0 && jj < frame->getWidth()){
						Macroblock MB = frame->getMacroblock(jj/16 + ii/16*frame->getWidth());
						filtered[i][j] += 
					}

				}
			}
		}
	}
	*/


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


