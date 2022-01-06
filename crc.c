
/* CRC.C */

#include "crc.h"
#include "types.h"

/* Constants */

#define BASE  65521 /* Largest prime smaller than 65536 */
#define N_MAX 5552  /* Largest n such that 255n(n+1)/2 + (n+1)(BASE-1) < 2^32 */

/* Functions */

/* CRC() */

uint CRC(const void *Block, int Size) { /* Adler32 */

   uint S1, S2;
   int N1, N2;
   const uchar *Buffer;

   S1 = 1;
   S2 = 0;

   Buffer = Block;

   while (Size != 0) {

      N1 = Size;
      if (N1 > N_MAX) N1 = N_MAX;
      Size -= N1;

      N2 = N1 & 0xF;
      N1 >>= 4;

      if (N1 != 0) {

	 do {

	    S1 += Buffer[0];
	    S2 += S1;
	    S1 += Buffer[1];
	    S2 += S1;
	    S1 += Buffer[2];
	    S2 += S1;
	    S1 += Buffer[3];
	    S2 += S1;
	    S1 += Buffer[4];
	    S2 += S1;
	    S1 += Buffer[5];
	    S2 += S1;
	    S1 += Buffer[6];
	    S2 += S1;
	    S1 += Buffer[7];
	    S2 += S1;
	    S1 += Buffer[8];
	    S2 += S1;
	    S1 += Buffer[9];
	    S2 += S1;
	    S1 += Buffer[10];
	    S2 += S1;
	    S1 += Buffer[11];
	    S2 += S1;
	    S1 += Buffer[12];
	    S2 += S1;
	    S1 += Buffer[13];
	    S2 += S1;
	    S1 += Buffer[14];
	    S2 += S1;
	    S1 += Buffer[15];
	    S2 += S1;

	    Buffer += 16;

	 } while (--N1 != 0);
      }

      if (N2 != 0) {
	 do {
	    S1 += *Buffer++;
	    S2 += S1;
	 } while (--N2 != 0);
      }

      S1 %= BASE;
      S2 %= BASE;
   }

   return (S2 << 16) | S1;
}

/* End of CRC.C */

