
/* Delta.C */

#include <stdlib.h>

#include "delta.h"
#include "types.h"
#include "algo.h"

/* Variables */

static uint Count[256];

/* Functions */

/* BestDelta() */

int BestDelta(const void *Block, int Size) {

   int I, C, Delta;
   const uchar *Buffer;

   Buffer = Block;

   Delta = 0;

   for (C = 0; C < 256; C++) Count[C] = 0;
   for (I = 0; I < Size; I++) {
      C = Buffer[I];
   }
   
   return 0;
}

/* CodeDelta() */

void CodeDelta(void *Block, int Size) {

   int I;
   uchar *Buffer;

   Buffer = Block;

   if (Size != 0) {
      for (I = Size-1; I >= Delta; I--) Buffer[I] -= Buffer[I-Delta];
   }
}

/*
void CodeDelta(void *Block, int Size) {

   int I;
   uchar *Buffer, Buffer2[1000000];

   Buffer = Block;

   for (I = 0; I < Size; I++) Buffer2[I] = Buffer[I];
   for (I = 0; I < Size; I++) Buffer[I]  = Buffer2[(Delta*I)%Size];
}
*/

/* DecodeDelta() */

void DecodeDelta(void *Block, int Size) {

   int I;
   uchar *Buffer;

   Buffer = Block;

   for (I = Delta; I < Size; I++) Buffer[I] += Buffer[I-Delta];
}

/* End of Delta.C */

