
/* Algo.C */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "algo.h"
#include "types.h"
#include "bitio.h"
#include "bwt.h"
#include "crc.h"
#include "debug.h"
#include "delta.h"
#include "hufblock.h"
#include "lz77.h"
#include "mtf.h"
#include "ppm.h"
#include "rle.h"

/* Variables */

const char *Source;
const char *Destination;

int    Algorithm, Delta, Group, Order, Verbosity;

uchar *S;
int    N;

static const char *AlgoName[ALGO_NB+1] = {
   "STORE", "LZH", "BWT", "PPM", NULL
};

/* Prototypes */

static void LoadBlock (void);
static void SaveBlock (void);

/* Functions */

/* AlgorithmNo() */

int AlgorithmNo(const char *Name) {

   int No;

   if (Name == NULL) return ALGO_LZH; /* Default algorithm */

   for (No = 0; No < ALGO_NB; No++) {
      if (strcmp(Name,AlgoName[No]) == 0) return No;
   }

   return -1;
}

/* AlgorithmName() */

const char *AlgorithmName(int No) {

   if (No < 0 || No >= ALGO_NB) return NULL;

   return AlgoName[No];
}

/* CrunchFile() */

void CrunchFile(void) {

   uint Crc32;

   OpenInStream(Source);

   N = SIZE;
   AllocBlock();

   OpenOutStream(Destination);

   SendUInt8('M');
   SendUInt8('C');
   SendUInt8('r');
   SendUInt8('0'+Algorithm);

   OpenBitStream(OutStream);

   while (! EndOfFile()) {

      LoadBlock();
      if (N == 0) break; /* Late end of file */
      if (Verbosity >= 2) fprintf(stderr,"N = %d\n",N);

      SendBit(1);
      SendBits(25,N);

      Crc32 = CRC(S,N);
      if (Verbosity >= 2) fprintf(stderr,"CRC = 0x%08X\n",Crc32);

      if (Algorithm != ALGO_STORE) {
	 if (Delta == -1) Delta = BestDelta(S,N);
	 if (Verbosity >= 2) fprintf(stderr,"Delta = %d\n",Delta);
	 if (Delta != 0) CodeDelta(S,N);
      }

      CrunchBlock();

      if (Algorithm != ALGO_STORE) SendBits(DELTA_BIT,Delta);

      SendBits(16,(Crc32>>16)&0xFFFF);
      SendBits(16,Crc32&0xFFFF);
   }

   SendBit(0);

   CloseBitStream(OutStream);

   FreeBlock();

   CloseOutStream();
   CloseInStream();
}

/* DecrunchFile() */

void DecrunchFile(void) {

   uint Crc32;

   OpenInStream(Source);

   if (GetUInt8() != 'M' || GetUInt8() != 'C' || GetUInt8() != 'r') {
      FatalError("Not an MCr file");
   }

   Algorithm = GetUInt8() - '0';
   if (Algorithm < 0 || Algorithm >= ALGO_NB) {
      FatalError("Unknown algorithm %d",Algorithm);
   }

   OpenOutStream(Destination);

   OpenBitStream(InStream);

   while (GetBit() == 1) {

      N = GetBits(25);
      if (Verbosity >= 2) fprintf(stderr,"N = %d\n",N);

      AllocBlock();

      DecrunchBlock();

      if (Algorithm != ALGO_STORE) {
	 Delta = GetBits(DELTA_BIT);
	 if (Verbosity >= 2) fprintf(stderr,"Delta = %d\n",Delta);
	 if (Delta != 0) DecodeDelta(S,N);
      }

      Crc32 =  GetBits(16) << 16;
      Crc32 |= GetBits(16);
      if (Verbosity >= 2) fprintf(stderr,"CRC = 0x%08X\n",Crc32);

      if (CRC(S,N) != Crc32) Error("Bad CRC (0x%08X exp 0x%08X found)",Crc32,CRC(S,N));

      SaveBlock();
      FreeBlock();
   }

   CloseBitStream(InStream);

   CloseInStream();
   CloseOutStream();
}

/* CrunchBlock() */

void CrunchBlock(void) {

   switch (Algorithm) { 
   case ALGO_STORE :
      CloseBitStream(OutStream);
      SendBlock(S,N);
      OpenBitStream(OutStream);
      break;
   case ALGO_LZH :
      CodeLZ77();
      break;
   case ALGO_BWT :
      CodeBWT();
      break;
   case ALGO_PPM :
      CodePPM();
      break;
   }
}

/* DecrunchBlock() */

void DecrunchBlock(void) {

   switch (Algorithm) {
   case ALGO_STORE :
      CloseBitStream(InStream);
      GetBlock(S,N);
      OpenBitStream(InStream);
      break;
   case ALGO_LZH :
      DecodeLZ77();
      break;
   case ALGO_BWT :
      DecodeBWT();
      break;
   case ALGO_PPM :
      DecodePPM();
      break;
   }
}

/* AllocBlock() */

void AllocBlock(void) {

   S = Nalloc(N,"Cruncher block");
}

/* FreeBlock() */

void FreeBlock(void) {

   if (S != NULL) {
      Free(S);
      S = NULL;
   }
}

/* LoadBlock() */

static void LoadBlock(void) {

   N = GetBlock(S,SIZE);
}

/* SaveBlock() */

static void SaveBlock(void) {

   SendBlock(S,N);
}

/* End of Algo.C */

