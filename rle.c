
/* RLE.C */

#include "rle.h"
#include "types.h"
#include "algo.h"
#include "bwt.h"
#include "hufblock.h"
#include "debug.h"

/* Functions */

/* CodeRLE() */

void CodeRLE(void) {

   int BlockPos, Len;

   HufBlockSize = 0;

   for (BlockPos = 0; BlockPos < N;) {
      if (L[BlockPos] == 0) {
         for (Len = -1; BlockPos < N && L[BlockPos] == 0; Len++) BlockPos++;
	 while (TRUE) {
	    HufBlock[HufBlockSize++] = Len & 1;
	    Len -= 2;
	    if (Len < 0) break;
	    Len >>= 1;
	 }
      } else {
         HufBlock[HufBlockSize++] = L[BlockPos++] + 1;
      }
   }

   if (HufBlockSize > N) FatalError("HufBlockSize (%d) > N (%d) in CodeRLE()",HufBlockSize,N);
}

/* DecodeRLE() */

void DecodeRLE(void) {

   int I, BlockPos, Len, Inc, Code;

   BlockPos = 0;
   Len      = 0;
   Inc      = 1;

   for (I = 0; I < HufBlockSize; I++) {
      Code = HufBlock[I];
      if (Code == 0) {
         Len += Inc;
         Inc <<= 1;
      } else if (Code == 1) {
         Inc <<= 1;
         Len += Inc;
      } else {
         for (; Len > 0; Len--) L[BlockPos++] = 0;
         Len = 0;
         Inc = 1;
         L[BlockPos++] = Code - 1;
      }
   }

   for (; Len > 0; Len--) L[BlockPos++] = 0;

   if (BlockPos != N) FatalError("BlockPos (%d) != N (%d) in DecodeRLE()",BlockPos,N);
}

/* End of RLE.C */

