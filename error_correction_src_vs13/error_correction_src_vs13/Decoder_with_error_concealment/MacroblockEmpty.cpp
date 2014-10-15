#include "MacroblockEmpty.h"



MacroblockEmpty::MacroblockEmpty(){
	for (int x = 0; x < 16; ++x)
	{
		for (int y = 0; y < 16; ++y)
		{
			luma[x][y] = 0;
			cb[int(x/2)][int(y/2)] = 0;
			cr[int(x/2)][int(y/2)] = 0;
		}
	}
	
}