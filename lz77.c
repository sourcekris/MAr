
/* LZ77.C */

#include <stdio.h>
#include <stdlib.h>

#include "lz77.h"
#include "types.h"
#include "algo.h"
#include "bitio.h"
#include "debug.h"
#include "huffman.h"

/* Constants */

#define SYMBOL_NB      0x320
#define LEN_MAX        25
#define FORMAT         HUFFMAN

#define HASH_SIZE      1048576
#define DIST_MAX       65535
#define NONE           DIST_MAX

#define BLOCK_SIZE_MIN 1024
#define BLOCK_SIZE_MAX 65536
#define BLOCK_SIZE_BIT 16

/* Types */

typedef struct {
   int Start;
   int Code;
   int Len;
} info;

typedef struct {
   ushort Succ;
   ushort Pred;
} hash_node;

typedef struct {
   ushort Head;
   ushort Tail;
} hash_list;

typedef struct block_node block_node;

struct block_node {
   block_node *Succ;
   block_node *Pred;
   int         Start;
   int         End;
   int         Size;
   int         Len;
   int         MergeLen;
   int         Freq[SYMBOL_NB];
};

typedef struct {
   block_node *Head;
   block_node *Tail;
} block_list;

/* "constants" */

static const int LenMin  = 3;
static const int LenMax  = 259;
static const int DistMax = 65535;

static const info Info[32] = {
   { 0x0000,  0,  0 },
   { 0x0001,  1,  0 },
   { 0x0002,  2,  0 },
   { 0x0003,  3,  0 },
   { 0x0004,  4,  1 },
   { 0x0006,  5,  1 },
   { 0x0008,  6,  2 },
   { 0x000C,  7,  2 },
   { 0x0010,  8,  3 },
   { 0x0018,  9,  3 },
   { 0x0020, 10,  4 },
   { 0x0030, 11,  4 },
   { 0x0040, 12,  5 },
   { 0x0060, 13,  5 },
   { 0x0080, 14,  6 },
   { 0x00C0, 15,  6 },
   { 0x0100, 16,  7 },
   { 0x0180, 17,  7 },
   { 0x0200, 18,  8 },
   { 0x0300, 19,  8 },
   { 0x0400, 20,  9 },
   { 0x0600, 21,  9 },
   { 0x0800, 22, 10 },
   { 0x0C00, 23, 10 },
   { 0x1000, 24, 11 },
   { 0x1800, 25, 11 },
   { 0x2000, 26, 12 },
   { 0x3000, 27, 12 },
   { 0x4000, 28, 13 },
   { 0x6000, 29, 13 },
   { 0x8000, 30, 14 },
   { 0xC000, 31, 14 }
};

/* Variables */

static hash_list  *HashList;
static hash_node  *HashNode;

static ushort     *Length;
static ushort     *Distance;
static int         SymbolNb;

static block_list  BlockList[1];
static int         BlockNb;
static int         BlockLen;

/* Prototypes */

static void AllocLZ77 (void);
static void FreeLZ77  (void);

static void FastLZ77  (void);
static void SlowLZ77  (void);
static void BestLZ77  (void);

static void InitHash  (void);
static int  HashKey   (int P);
static void AddHash   (int P);
static void RemHash   (int P);

static int  MatchLen  (int P1, int P2);

static int  MatchCmp  (const void *M1, const void *M2);

static int  BitCode   (int N);

/* Functions */

/* CodeLZ77() */

void CodeLZ77(void) {

   int I, Len, Dist, LastDist, LiteralNb, StringNb, StringLen, StringDist;
   int Start, Code, LenCode, DistCode, LenLen, DistLen;
   int Gain, BestGain, Freq[SYMBOL_NB];
   block_node *Block, *BestBlock;
   huftable HufTable[1];
   
   AllocLZ77();

   Length   = Nalloc(N*sizeof(ushort),"LZ77 length array");
   Distance = Nalloc(N*sizeof(ushort),"LZ77 distance array");

   FastLZ77();

   LiteralNb  = 0;
   StringNb   = 0;
   StringLen  = 0;
   StringDist = 0;
   
   for (I = 0; I < SymbolNb; I++) {
      if (Length[I] >= LenMin) {
         StringNb++;
         StringLen  += Length[I];
         StringDist += Distance[I];
      } else {
         LiteralNb++;
      }
   }

   if (Verbosity >= 2) fprintf(stderr,"%d codes, %d literals (%.2f%%) and %d strings (%.2f%%), len %.2f, dist %.2f\n",LiteralNb+StringNb,LiteralNb,100.0*(double)LiteralNb/(double)(StringNb+LiteralNb),StringNb,100.0*(double)StringNb/(double)(StringNb+LiteralNb),(double)StringLen/(double)StringNb,(double)StringDist/(double)StringNb);

   AllocHufTable(HufTable,SYMBOL_NB,LEN_MAX,FORMAT);

   BlockList->Head = NULL;
   BlockList->Tail = NULL;
   
   BlockNb  = 0;
   BlockLen = 0;

   LastDist = 1;

   for (Start = 0; Start < SymbolNb; Start += BLOCK_SIZE_MIN) {
      
      Block = Nalloc(sizeof(block_node),"LZ77 block node");
      
      Block->Start = Start;
      Block->End = Block->Start + BLOCK_SIZE_MIN;
      if (Block->End > SymbolNb) Block->End = SymbolNb;
      Block->Size = Block->End - Block->Start;

      for (I = 0; I < SYMBOL_NB; I++) Block->Freq[I] = 0;

      for (I = Block->Start; I < Block->End; I++) {
	 if (Length[I] >= LenMin) {
	    Len     = Length[I] - LenMin;
	    LenCode = BitCode(Len);
            Dist    = Distance[I]; /* - 1 */
            if (Dist == LastDist) {
               Dist = 0;
            } else {
               LastDist = Dist;
            }
	    DistCode = BitCode(Dist);
	    Code     = 0x100 + ((LenCode << 5) | DistCode);
	 } else {
	    Code = Distance[I];
	 }
	 Block->Freq[Code]++;
      }

      CompLens(HufTable,Block->Freq);
      Block->Len = PredictLen(HufTable);

      Block->Pred = BlockList->Tail;
      Block->Succ = NULL;

      if (BlockList->Tail != NULL) {
         BlockList->Tail->Succ = Block;
      } else {
         BlockList->Head = Block;
      }
      BlockList->Tail = Block;

      BlockNb++;
      BlockLen += Block->Len;
   }

   for (Block = BlockList->Head; Block != NULL; Block = Block->Succ) {
      if (Block->Succ != NULL) {
         for (I = 0; I < SYMBOL_NB; I++) Freq[I] = Block->Freq[I] + Block->Succ->Freq[I];
         CompLens(HufTable,Freq);
         Block->MergeLen = PredictLen(HufTable);
      }
   }

   if (Verbosity >= 2) fprintf(stderr,"BlockNb = %4d, BlockSize = %9.2f, TotalLen = %10.2f\n",BlockNb,(double)(LiteralNb+StringNb)/(double)BlockNb,(double)BlockLen/8.0);

   if (Group) {

      while (TRUE) {

         BestBlock = NULL;
         BestGain  = -1;

         for (Block = BlockList->Head; Block != NULL; Block = Block->Succ) {
            if (Block->Succ != NULL && Block->Size + Block->Succ->Size <= BLOCK_SIZE_MAX) {
               Gain = Block->Len + Block->Succ->Len - Block->MergeLen + BLOCK_SIZE_BIT + 1;
               if (Gain > BestGain) {
                  BestGain  = Gain;
                  BestBlock = Block;
               }
            }
         }

         if (BestGain < 0) break;

         BlockNb--;
         BlockLen -= BestGain;

         if (Verbosity >= 3) fprintf(stderr,"BlockNb = %4d, BlockSize = %9.2f, TotalLen = %10.2f, Gain = %7.2f\n",BlockNb,(double)(LiteralNb+StringNb)/(double)BlockNb,(double)BlockLen/8.0,(double)BestGain/8.0);

         BestBlock->End   = BestBlock->Succ->End;
         BestBlock->Size += BestBlock->Succ->Size;
         BestBlock->Len   = BestBlock->MergeLen;

         for (I = 0; I < SYMBOL_NB; I++) BestBlock->Freq[I] += BestBlock->Succ->Freq[I];

         BestBlock->Succ = BestBlock->Succ->Succ;

         if (BestBlock->Succ != NULL) {
            BestBlock->Succ->Pred = BestBlock;
            for (I = 0; I < SYMBOL_NB; I++) Freq[I] = BestBlock->Freq[I] + BestBlock->Succ->Freq[I];
            CompLens(HufTable,Freq);
            BestBlock->MergeLen = PredictLen(HufTable);
         }

         if (BestBlock->Pred != NULL) {
            for (I = 0; I < SYMBOL_NB; I++) Freq[I] = BestBlock->Pred->Freq[I] + BestBlock->Freq[I];
            CompLens(HufTable,Freq);
            BestBlock->Pred->MergeLen = PredictLen(HufTable);
         }
      }

      if (Verbosity >= 2) fprintf(stderr,"BlockNb = %4d, BlockSize = %9.2f, TotalLen = %10.2f\n",BlockNb,(double)(LiteralNb+StringNb)/(double)BlockNb,(double)BlockLen/8.0);
   }

   LastDist = 1;

   for (Block = BlockList->Head; Block != NULL; Block = Block->Succ) {

      SendBit(1);

      SendBits(BLOCK_SIZE_BIT,Block->Size-1);

      CompLens(HufTable,Block->Freq);
      SendLens(HufTable);

      CompCodes(HufTable);
      for (I = Block->Start; I < Block->End; I++) {
	 if (Length[I] >= LenMin) {
	    Len      = Length[I] - LenMin;
	    LenCode  = BitCode(Len);
	    LenLen   = (Length[I] == LenMax) ? 0 : Info[LenCode].Len;
            Dist     = Distance[I]; /* - 1 */
            if (Dist == LastDist) {
               Dist = 0;
            } else {
               LastDist = Dist;
            }
	    DistCode = BitCode(Dist);
	    DistLen  = Info[DistCode].Len;
	    Code     = 0x100 + ((LenCode << 5) | DistCode);
	    SendHufSym(HufTable,Code);
	    if (LenLen  != 0) SendBits(LenLen,Len-Info[LenCode].Start);
	    if (DistLen != 0) SendBits(DistLen,Dist-Info[DistCode].Start);
	 } else {
	    Code = Distance[I];
	    SendHufSym(HufTable,Code);
	 }
      }
   }

   CheckFreqs(HufTable);

   SendBit(0);

   FreeHufTable(HufTable);

   Free(Length);
   Free(Distance);

   FreeLZ77();
}

/* DecodeLZ77() */

void DecodeLZ77(void) {

   int I, SymbolNb, Len, Dist, LastDist, Code, LenCode, DistCode, LenLen, DistLen;
   huftable HufTable[1];

   I = 0;

   AllocHufTable(HufTable,SYMBOL_NB,LEN_MAX,FORMAT);

   LastDist = 1;

   while (GetBit() == 1) {

      SymbolNb = GetBits(BLOCK_SIZE_BIT) + 1;

      GetLens(HufTable);

      CompCodes(HufTable);
      CompDecodeTable(HufTable);

      do {
	 Code = GetHufSym(HufTable);
	 if (Code >= 0x100) {
	    Code     -= 0x100;
	    LenCode   = Code >> 5;
	    if (LenCode < 16) {
	       Len    = Info[LenCode].Start + LenMin;
	       LenLen = Info[LenCode].Len;
	    } else {
	       Len    = LenMax;
	       LenLen = 0;
	    }
	    DistCode = Code & 0x1F;
	    Dist     = Info[DistCode].Start; /* + 1 */
	    DistLen  = Info[DistCode].Len;
	    if (LenLen  != 0) Len  += GetBits(LenLen);
	    if (DistLen != 0) Dist += GetBits(DistLen);
            if (Dist == 0) {
               Dist = LastDist;
            } else {
               LastDist = Dist;
            }
	    do {
	       S[I] = S[I-Dist];
	       I++;
	    } while (--Len > 0);
	 } else {
	    S[I++] = Code; /* Literal */
	 }
      } while (--SymbolNb > 0);
   }

   FreeHufTable(HufTable);
}

/* AllocLZ77() */

static void AllocLZ77(void) {

   HashList = Nalloc(HASH_SIZE*sizeof(hash_list),"LZ77 hash array");
   HashNode = Nalloc(DIST_MAX*sizeof(hash_node),"LZ77 hash window");
}

/* FreeLZ77() */

static void FreeLZ77(void) {

   if (HashList != NULL) {
      Free(HashList);
      HashList = NULL;
   }

   if (HashNode != NULL) {
      Free(HashNode);
      HashNode = NULL;
   }
}

/* FastLZ77() */

static void FastLZ77(void) {

   int I, J, K, P, LastP, Index, Len, BestLen, BestDist;

   LastP = 0;

   if (Verbosity >= 2) {
      LastP = 0;
      fprintf(stderr,"Collecting strings ... %2d%%",LastP);
      fflush(stderr);
   }

   SymbolNb = 0;
   InitHash();

   if (N > 0) {
      Length[SymbolNb]   = 1;
      Distance[SymbolNb] = S[0];
      SymbolNb++;
      AddHash(0);
   }

   Index = (DistMax > 1) ? 0 : 1;

   for (I = 1; I <= N-LenMin; SymbolNb++) {

      if (Verbosity >= 2) {
         P = 100 * I / N;
         if (P != LastP) {
            LastP = P;
            fprintf(stderr,"\b\b\b%2d%%",LastP);
            fflush(stderr);
         }
      }

      if (I >= Index + DistMax) Index += DistMax;
      BestLen  = 1;
      BestDist = 0;

      for (J = HashList[HashKey(I)].Head; J != NONE; J = HashNode[J].Succ) {
	 K = Index + J;
	 if (K >= I) K -= DistMax;
	 Len = MatchLen(K,I);
	 if (Len >= LenMin && Len > BestLen) {
	    BestLen  = Len;
	    BestDist = I - K;
	    if (Len >= LenMax) break;
	 }
      }

      Length[SymbolNb] = BestLen;
      if (BestLen >= LenMin) {
         Distance[SymbolNb] = BestDist;
      } else {
         Distance[SymbolNb] = S[I];
      }

      do {
         if (I >= DistMax) RemHash(I-DistMax);
         AddHash(I);
         I++;
      } while (--BestLen > 0);
   }

   for (; I < N; I++, SymbolNb++) {
      Length[SymbolNb]   = 1;
      Distance[SymbolNb] = S[I];
   }

   if (Verbosity >= 2) fprintf(stderr,"\b\b\bDone.\n");
}

/* SlowLZ77() */

static void SlowLZ77(void) {

   int I, J, K, P, LastP, Index, Len, BestLen, BestDist, StringNb, *Match;

   LastP = 0;

   if (Verbosity >= 2) {
      LastP = 0;
      fprintf(stderr,"Collecting strings ... %2d%%",LastP);
      fflush(stderr);
   }

   StringNb = 0;
   InitHash();

   if (N > 0) {
      Length[0] = 1;
      AddHash(0);
   }

   Index = (DistMax > 1) ? 0 : 1;

   for (I = 1; I <= N-LenMin; I++) {

      if (Verbosity >= 2) {
         P = 100 * I / N;
         if (P != LastP) {
            LastP = P;
            fprintf(stderr,"\b\b\b%2d%%",LastP);
            fflush(stderr);
         }
      }

      if (I >= Index + DistMax) Index += DistMax;
      BestLen  = 1;
      BestDist = 0;

      for (J = HashList[HashKey(I)].Head; J != NONE; J = HashNode[J].Succ) {
	 K = Index + J;
	 if (K >= I) K -= DistMax;
	 Len = MatchLen(K,I);
	 if (Len >= LenMin && Len > BestLen) {
	    BestLen  = Len;
	    BestDist = I - K;
	    if (Len == LenMax) break;
	 }
      }

      if (BestLen >= LenMin) {
         Length[I]   = BestLen;
         Distance[I] = BestDist;
         StringNb++;
      } else {
         Length[I] = 1;
      }

      if (I >= DistMax) RemHash(I-DistMax);
      AddHash(I);
   }

   while (I < N) Length[I++] = 1;

   if (Verbosity >= 2) fprintf(stderr,"\b\b\bDone.\n");

   Match = Nalloc(StringNb*sizeof(int),"LZ77 match array");

   for (I = 0, J = 0; I < N; I++) if (Length[I] >= LenMin) Match[J++] = I;

   if (Verbosity >= 2) {
      fprintf(stderr,"Sorting matches    ... ");
      fflush(stderr);
   }

   qsort(Match,(size_t)StringNb,sizeof(int),&MatchCmp);

   if (Verbosity >= 2) {

      fprintf(stderr,"Done.\n");

      fprintf(stderr,"Selecting matches  ... ");
      fflush(stderr);
   }

   for (I = 0; I < StringNb; I++) {
      J = Match[I];
      if (Length[J] >= LenMin) {
         for (K = J+1; K < J+Length[J]; K++) {
            if (Length[K] >= Length[J] && MatchCmp(&J,&K) > 0) {
	       Length[J] = K - J;
               break;
            }
            Length[K] = 1;
         }
      }
   }

   if (Verbosity >= 2) fprintf(stderr,"Done.\n");

   Free(Match);

   for (I = 0, J = 0; I < N; J++) {
      if (Length[I] >= LenMin) {
         Length[J]   = Length[I];
         Distance[J] = Distance[I];
         I += Length[I];
      } else {
         Length[J]   = 1;
         Distance[J] = S[I];
         I++;
      }
   }

   SymbolNb = J;
}

/* BestLZ77() */

static void BestLZ77(void) {

   int I, J, K, P, LastP, Index, Len, BestLen, BestDist, StringNb;
   int *Size, CurrSize, BestSize;

   LastP = 0;

   if (Verbosity >= 2) {
      LastP = 0;
      fprintf(stderr,"Collecting strings ... %2d%%",LastP);
      fflush(stderr);
   }

   StringNb = 0;
   InitHash();

   if (N > 0) {
      Length[0] = 1;
      AddHash(0);
   }

   Index = (DistMax > 1) ? 0 : 1;

   for (I = 1; I <= N-LenMin; I++) {

      if (Verbosity >= 2) {
         P = 100 * I / N;
         if (P != LastP) {
            LastP = P;
            fprintf(stderr,"\b\b\b%2d%%",LastP);
            fflush(stderr);
         }
      }

      if (I >= Index + DistMax) Index += DistMax;
      BestLen  = 1;
      BestDist = 0;

      for (J = HashList[HashKey(I)].Head; J != NONE; J = HashNode[J].Succ) {
	 K = Index + J;
	 if (K >= I) K -= DistMax;
	 Len = MatchLen(K,I);
	 if (Len >= LenMin && Len > BestLen) {
	    BestLen  = Len;
	    BestDist = I - K;
	    if (Len == LenMax) break;
	 }
      }

      if (BestLen >= LenMin) {
         Length[I]   = BestLen;
         Distance[I] = BestDist;
         StringNb++;
      } else {
         Length[I] = 1;
      }

      if (I >= DistMax) RemHash(I-DistMax);
      AddHash(I);
   }

   while (I < N) Length[I++] = 1;

   if (Verbosity >= 2) {

      fprintf(stderr,"\b\b\bDone.\n");

      fprintf(stderr,"Selecting matches  ... ");
      fflush(stderr);
   }

   Size = Nalloc((N+1)*sizeof(int),"LZ77 size array");

   Size[N] = 0;
   BestLen = 0;

   for (I = N-1; I >= 0; I--) {
      if (Length[I] >= LenMin) {
         BestSize = 999999999;
         for (Len = Length[I]; Len >= LenMin; Len--) {
            CurrSize = Size[I+Len] + 1;
            if (CurrSize < BestSize) {
               BestLen  = Len;
               BestSize = CurrSize;
            }
         }
         Len = 1;
         CurrSize = Size[I+Len] + 1;
         if (CurrSize < BestSize) {
            BestLen  = Len;
            BestSize = CurrSize;
         }
         Length[I] = BestLen;
         Size[I]   = Size[I+Length[I]] + 1;
      } else {
         Size[I] = Size[I+Length[I]] + 1;
      }
   }

   Free(Size);

   if (Verbosity >= 2) fprintf(stderr,"Done.\n");

   for (I = 0, J = 0; I < N; J++) {
      if (Length[I] >= LenMin) {
         Length[J]   = Length[I];
         Distance[J] = Distance[I];
         I += Length[I];
      } else {
         Length[J]   = 1;
         Distance[J] = S[I];
         I++;
      }
   }

   SymbolNb = J;
}

/* HashKey() */

static int HashKey(int P) {

   return (S[P] << 12) ^ S[P+1] ^ (S[P+2] << 6);
}

/* InitHash() */

static void InitHash(void) {

   int I;

   for (I = 0; I < HASH_SIZE; I++) {
      HashList[I].Head = NONE;
      HashList[I].Tail = NONE;
   }

   for (I = 0; I < DistMax; I++) {
      HashNode[I].Pred = NONE;
      HashNode[I].Succ = NONE;
   }
}

/* AddHash() */

static void AddHash(int P) {

   int Key, Index;

   Key   = HashKey(P);
   Index = P % DistMax;

   HashNode[Index].Pred = NONE;
   HashNode[Index].Succ = HashList[Key].Head;

   if (HashList[Key].Head == NONE) {
      HashList[Key].Tail = Index;
   } else {
      HashNode[HashList[Key].Head].Pred = Index;
   }

   HashList[Key].Head = Index;
}

/* RemHash() */

static void RemHash(int P) {

   int Key, Index;

   Key   = HashKey(P);
   Index = P % DistMax;

   HashList[Key].Tail = HashNode[Index].Pred;

   if (HashNode[Index].Pred == NONE) {
      HashList[Key].Head = NONE;
   } else {
      HashNode[HashNode[Index].Pred].Succ = NONE;
   }

   HashNode[Index].Pred = NONE;
   HashNode[Index].Succ = NONE;
}

/* MatchLen() */

static int MatchLen(int P1, int P2) {

   int P0, P3;

   if (S[P1+2] != S[P2+2]) return 0;

   /* Match of at least 3 chars due to hash key nature */

   P0 = P1;

   P3 = P2 + LenMax;
   if (P3 > N) P3 = N;

   P1 += 3;
   P2 += 3;

   while (P2 < P3 && S[P1] == S[P2]) {
      P1++;
      P2++;
   }

   return P1 - P0;
}

/* MatchCmp() */

static int MatchCmp(const void *M1, const void *M2) {

   int I1, I2, L1, L2, D1, D2;

   I1 = *((const int *) M1);
   I2 = *((const int *) M2);

   L1 = Length[I1];
   L2 = Length[I2];

   if (L1 != L2) return L2 - L1;

   D1 = Distance[I1];
   D2 = Distance[I2];

   if (D1 != D2) return D1 - D2;

   return I1 - I2;
}

/* BitCode() */

static int BitCode(int N) {

   int BitCode;

   for (BitCode = 0; BitCode < 31 && N >= Info[BitCode+1].Start; BitCode++)
      ;

   return BitCode;
}

/* End of LZ77.C */

