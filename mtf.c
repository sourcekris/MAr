
/* MTF.C */

#include <stdlib.h>

#include "mtf.h"
#include "types.h"
#include "debug.h"

/* Variables */

static uchar M2F[256];

/* Functions */

/* CodeMTF() */

void CodeMTF(void *Block, int Size) {

   int I, J, C;
   uchar *Buffer;

   Buffer = Block;

   for (C = 0; C < 256; C++) M2F[C] = C;

   for (I = 0; I < Size; I++) {
      C = Buffer[I];
      for (J = 0; M2F[J] != C; J++)
	 ;
      Buffer[I] = J;
      for (; J > 0; J--) M2F[J] = M2F[J-1];
      M2F[0] = C;
   }
}

/* DecodeMTF() */

void DecodeMTF(void *Block, int Size) {

   int I, J, C;
   uchar *Buffer;

   Buffer = Block;

   for (C = 0; C < 256; C++) M2F[C] = C;

   for (I = 0; I < Size; I++) {
      J = Buffer[I];
      C = M2F[J];
      Buffer[I] = C;
      for (; J > 0; J--) M2F[J] = M2F[J-1];
      M2F[0] = C;
   }
}

/* End of MTF.C */

