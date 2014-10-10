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
			conceal_spatial_2(frame);
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

//No adjacent blocks missing, use interpolate with the 2 closest pixels to solve this.
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
}


enum MBSTATE { OK, MISSING, CONCEALED };
//fix method for conceal_spatial_2
void f(Macroblock* MB, 
	   int* exist_l, int* exist_r, int* exist_t, int* exist_b, 
	   MBSTATE* MBstate, 
	   int MBx, 
	   Frame *frame){
		   
			//What block DO we have?
		   Macroblock* MB_l ;
		   Macroblock* MB_r ;
		   Macroblock* MB_t ;
		   Macroblock* MB_b ;
		   MacroblockEmpty* MBEmpty = new MacroblockEmpty();

		   //determine MB_l
		   if (MB->getXPos() == 0){//at the left border => left is always missing
			   MB_l = MBEmpty;
			   *exist_l = 0;
		   }else{
			   if (MBstate[MBx - 1] == MISSING){//left one is missing
				   MB_l = MBEmpty;
				   *exist_l = 0;
			   }else{ //left one is not missing
				   MB_l = frame->getMacroblock(MBx - 1);
			   }
		   }
		   //determine MB_r
		   if (MB->getXPos() == frame->getWidth() - 1){
			   MB_r = MBEmpty;
			   *exist_r = 0;
		   }else{
			   if (MBstate[MBx + 1] == MISSING){
				   MB_r = MBEmpty;
				   *exist_r = 0;
			   }else{
				   MB_r = frame->getMacroblock(MBx + 1);
			   }
		   }
		   //determine MB_t
		   if (MB->getYPos() == 0){
			   MB_t = MBEmpty;
			   *exist_t = 0;
		   }else{
			   if (MBstate[MBx - frame->getWidth()] == MISSING){
				   MB_t = MBEmpty;
				   *exist_t = 0;
			   }else{
				   MB_t = frame->getMacroblock(MBx - frame->getWidth());
			   }
		   }
		   //determine MB_b
		   if (MB->getYPos() == frame->getHeight() - 1){
			   MB_b = MBEmpty;
			   *exist_b = 0;
		   }else{
			   if (MBstate[MBx + frame->getWidth()] == MISSING){
				   MB_b = MBEmpty;
				   *exist_b = 0;
			   }else{
				   MB_b = frame->getMacroblock(MBx + frame->getWidth());
			   }
		   }
		   //interpolate between existing blocks
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
//can fix even if adjacent blocks are missing
void ErrorConcealer::conceal_spatial_2(Frame *frame)
{
	//init
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

	//get states for each macro block
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

	//conceal multiple
	int loop = 0;
	while (nrOfMBsMissing > 0){

		MBsConcealedL1L2 = 1;
		while (MBsConcealedL1L2 > 0){
			MBsConcealedL1 = 1;
			MBsConcealedL1L2 = 0;
			while (MBsConcealedL1 > 0){
				MBsConcealedL1 = 0;
				for (int MBx = 0; MBx < numMB; ++MBx){
					MB = frame->getMacroblock(MBx);
					exist_t = 1;
					exist_b = 1;
					exist_r = 1;
					exist_l = 1;
					if (MBstate[MBx] == MISSING){
						f( MB, &exist_l,  &exist_r,  &exist_t,  &exist_b, MBstate,MBx,frame);
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
			for (int MBx = 0; MBx < numMB && !oneMBConcealed; ++MBx){
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


		//conceal singles
		bool oneMBConcealed = false;
		for (int MBx = 0; MBx < numMB && !oneMBConcealed; ++MBx){
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

//uses edge information
void ErrorConcealer::conceal_spatial_3(Frame *frame)
{

}

//zero motion temporal reconstruction=> block missing? = use previous block
void ErrorConcealer::conceal_temporal_1(Frame *frame, Frame *referenceFrame)
{
	//block missing? use previous block (assume no motion in block)
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
//getters
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

//Fill MB based on usedMB
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

//how much does the edge of MB differ from usedMB ?
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
		//if the frame is not Predictibely coded (we should have the whole frame), then we conceal using the spacial method instead.
		conceal_spatial_2(frame);
	}else{
		float error = 99999999;
		MBPOSITION bestResult = NONE;

		int numMB = frame->getNumMB();
		Macroblock *usedMB; //MB in same frame, used for MV (TOP, BOTTOM,...)

		for (int MBx = 0; MBx < numMB; ++MBx){
			if (frame->getMacroblock(MBx)->isMissing()){
				Macroblock *MB = frame->getMacroblock(MBx);

				//get best matching block
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

				//what block is best matching? -> use motion vector from that block
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


