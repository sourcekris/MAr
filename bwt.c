
/* BWT.C */

#include <stdio.h>
#include <stdlib.h>

#include "bwt.h"
#include "types.h"
#include "algo.h"
#include "bitio.h"
#include "debug.h"
#include "hufblock.h"
#include "mtf.h"
#include "rle.h"

/* Variables */

int    Index;
uchar *L;

static int   *Pos, *Prm, *Count;
static uchar *BH, *B2H;

static int    C[256], *P;

/* Prototypes */

static void AllocSuffix (void);
static void FreeSuffix  (void);
                        
static void AllocLast   (void);
static void FreeLast    (void);
                        
static void AllocArray  (void);
static void FreeArray   (void);
                        
static void SortSuffix  (void);
                        
static void CompLast    (void);
static void CompFirst   (void);
static void CompBlock   (void);
                        
/* Functions */

/* CodeBWT() */

void CodeBWT(void) {

   AllocSuffix();
   SortSuffix();
   AllocLast();
   CompLast();
   FreeSuffix();

   if (Verbosity >= 2) fprintf(stderr,"I = %d\n",Index);

   SendBits(25,Index);

   CodeMTF(L,N);

   AllocHufBlock();
   CodeRLE();
   SendHufBlock();
   FreeHufBlock();
   FreeLast();
}

/* DecodeBWT() */

void DecodeBWT(void) {

   Index = GetBits(25);
   if (Verbosity >= 2) fprintf(stderr,"I = %d\n",Index);

   AllocLast();
   AllocHufBlock();
   GetHufBlock();
   DecodeRLE();
   FreeHufBlock();

   DecodeMTF(L,N);

   AllocArray();
   CompFirst();
   CompBlock();
   FreeArray();
   FreeLast();
}

/* AllocSuffix() */

static void AllocSuffix(void) {

   Pos = malloc(N*sizeof(int));
   if (Pos == NULL) FatalError("AllocSuffix(): Not enough memory");

   Prm = malloc(N*sizeof(int));
   if (Prm == NULL) FatalError("AllocSuffix(): Not enough memory");

   Count = malloc(N*sizeof(int));
   if (Count == NULL) FatalError("AllocSuffix(): Not enough memory");

   BH = malloc((size_t)N);
   if (BH == NULL) FatalError("AllocSuffix(): Not enough memory");

   B2H = malloc((size_t)N);
   if (B2H == NULL) FatalError("AllocSuffix(): Not enough memory");
}

/* FreeSuffix() */

static void FreeSuffix(void) {

   if (Pos != NULL) {
      free(Pos);
      Pos = NULL;
   }

   if (Prm != NULL) {
      free(Prm);
      Prm = NULL;
   }

   if (Count != NULL) {
      free(Count);
      Count = NULL;
   }

   if (BH != NULL) {
      free(BH);
      BH = NULL;
   }

   if (B2H != NULL) {
      free(B2H);
      B2H = NULL;
   }
}

/* AllocLast() */

static void AllocLast(void) {

   L = malloc((size_t)N);
   if (L == NULL) FatalError("AllocLast(): Not enough memory");
}

/* FreeLast() */

static void FreeLast(void) {

   if (L != NULL) {
      free(L);
      L = NULL;
   }
}

/* AllocArray() */

static void AllocArray(void) {

   P = malloc((size_t)(N*sizeof(int)));
   if (P == NULL) FatalError("AllocArray(): Not enough memory");
}

/* FreeArray() */

static void FreeArray(void) {

   if (P != NULL) {
      free(P);
      P = NULL;
   }
}

/* SortSuffix() */

static void SortSuffix(void) {

   int I, J, B, D, H, L, P, R;
   int Bucket[256+1];

   for (I = 0; I <= 256; I++) Bucket[I] = 0;
   for (I = 0; I <  N;   I++) Bucket[S[I]+1]++;
   for (I = 1; I <= 256; I++) Bucket[I] += Bucket[I-1];

   for (I = 0; I < N; I++) {
      Prm[I] = Bucket[S[I]];
      Pos[Prm[I]] = I;
      Bucket[S[I]]++;
   }

   BH[0] = TRUE;
   for (I = 1; I < N; I++) BH[I] = S[Pos[I]] != S[Pos[I-1]];

   for (H = 1; H < 2*N; H *= 2) {

      B = 0; /* To avoid warnings */
      for (I = 0; I < N; I++) {
         if (BH[I]) {
            B = I;
            Count[B] = 0;
         }
         Prm[Pos[I]] = B;
         B2H[I] = FALSE;
      }

      for (L = 0; L < N; L = R) {

         for (R = L+1; R < N && ! BH[R]; R++)
            ;

         for (I = L; I < R; I++) {

            D = Pos[I] - H;
            if (D < 0) D += N;
            if (D < 0) D += N;

            B = Prm[D];
            P = B + Count[B];

            Prm[D] = P;
            B2H[P] = TRUE;

            Count[B]++;
         }

         for (I = L; I < R; I++) {

            D = Pos[I] - H;
            if (D < 0) D += N;
            if (D < 0) D += N;

            P = Prm[D];

            if (B2H[P]) {
               for (J = P+1; ! BH[J] && B2H[J]; J++) B2H[J] = FALSE;
            }
         }
      }

      for (I = 0; I < N; I++) {
         Pos[Prm[I]] = I;
         BH[I] = B2H[I];
      }
   }
}

/* CompLast() */

static void CompLast(void) {

   int I, J;

   for (I = 0; I < N; I++) {
      if (Pos[I] == 0) Index = I;
      J = Pos[I] - 1;
      if (J < 0) J += N;
      L[I] = S[J];
   }
}

/* CompFirst() */

static void CompFirst(void) {

   int I, Char, Sum, SumOld;

   for (Char = 0; Char < 256; Char++) C[Char] = 0;

   for (I = 0; I < N; I++) {
      Char = L[I];
      P[I] = C[Char];
      C[Char]++;
   }

   Sum = 0;
   for (Char = 0; Char < 256; Char++) {
      SumOld = Sum;
      Sum += C[Char];
      C[Char] = SumOld;
   }
}

/* CompBlock() */

static void CompBlock(void) {

   int I, J, Char;

   I = Index;
   for (J = N-1; J >= 0; J--) {
      Char = L[I];
      S[J] = L[I];
      I = C[Char] + P[I];
   }
}

/* End of BWT.C */

