
/* Huffman.C */

#include <stdlib.h>

#include "types.h"
#include "huffman.h"
#include "bitio.h"
#include "debug.h"

/* Constants */

#define SYMBOL_MAX 1024
#define LEN_MAX    25

#define ROOT       1

/* Types */

typedef struct {
   int HufSymNb;
   int CodeMin;
   int HufSymIndex;
} codelen;

/* Macros */

#define MAX(A,B)       (((A) >= (B)) ? (A) : (B))
#define SMALLER(S1,S2) ((S1)->Freq <  (S2)->Freq \
                    || ((S1)->Freq == (S2)->Freq && (S1)->Len < (S2)->Len))

/* "Constants" */

static int Code2Freq[29] = {  0,  1,  2, 16, 14, 12, 10,  8,  6,  4,  3,  5,  7,  9, 11, 13, 15, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 };
static int Freq2Code[29] = {  0,  1,  2, 10,  9, 11,  8, 12,  7, 13,  6, 14,  5, 15,  4, 16,  3, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 };

/* Variables */

static int     HeapSize;
static hufsym *Heap[SYMBOL_MAX+1];

static hufsym  Node[SYMBOL_MAX-1]; /* Internal nodes of the huffman tree */

static codelen CodeLen[LEN_MAX+1];
static int     HufSymArray[SYMBOL_MAX], LenMin, LenMax;

/* Prototypes */

static void SimRleLen  (int RepLen, int Len, int LenFreq[]);
static void SendRleLen (const huftable *HufTable, int RepLen, int Len);

static void UpdateHeap (int Root);

static int  Log2       (int N);

/* Functions */

/* AllocHufTable() */

void AllocHufTable(huftable *HufTable, int N, int LenMax, int Format) {

   assert(N>=0&&N<=SYMBOL_MAX);
   assert(LenMax>=0&&LenMax<=LEN_MAX);

   HufTable->N      = N;
   HufTable->LenMax = LenMax;
   HufTable->Format = Format;
   HufTable->HufSym = malloc((size_t)(N*sizeof(hufsym)));
   if (HufTable->HufSym == NULL) FatalError("AllocHufTable(): Not enough memory");
}

/* FreeHufTable() */

void FreeHufTable(huftable *HufTable) {

   HufTable->N = 0;

   if (HufTable->HufSym != NULL) {
      free(HufTable->HufSym);
      HufTable->HufSym = NULL;
   }
}

/* CompLens() */

void CompLens(huftable *HufTable, const int Freq[]) {

   int Tree;
   hufsym *S, *S1, *S2;

   HeapSize = 0;

   for (S = HufTable->HufSym; S < &HufTable->HufSym[HufTable->N]; S++) {
      S->Freq   = *Freq++;
      S->Len    = 0;
      S->Code   = 0;
      S->Parent = NULL;
      if (S->Freq != 0) Heap[++HeapSize] = S;
   }

   if (HeapSize < 2) {
      Warning("HeapSize (%d) < 2 in CompLens()",HeapSize);
      S = &HufTable->HufSym[0];
      if (S->Freq == 0) S++;
      Heap[++HeapSize] = S;
   }

   for (Tree = HeapSize>>1; Tree > 0; Tree--) UpdateHeap(Tree);

   for (S = Node; HeapSize >= 2; S++) {
      S1 = Heap[ROOT];
      Heap[ROOT] = Heap[HeapSize--];
      if (HeapSize >= 2) UpdateHeap(ROOT);
      S2 = Heap[ROOT];
      S1->Parent = S;
      S2->Parent = S;
      S->Freq    = S1->Freq + S2->Freq;
      S->Len     = MAX(S1->Len,S2->Len) + 1; /* To minimize code max length */
      S->Code    = 0;
      S->Parent  = NULL;
      Heap[ROOT] = S;
      if (HeapSize >= 2) UpdateHeap(ROOT);
   }

   S--;
   S->Len = 0;

   for (S--; S >= Node; S--) S->Len = S->Parent->Len + 1;

   for (S = HufTable->HufSym; S < &HufTable->HufSym[HufTable->N]; S++) {
      if (S->Parent != NULL) S->Len = S->Parent->Len + 1;
   }
}

/* CompCodes() */

void CompCodes(huftable *HufTable) {

   int I, CodeMin;
   hufsym *S;

   for (I = 0; I <= HufTable->LenMax; I++) {
      CodeLen[I].HufSymNb = 0;
      CodeLen[I].CodeMin  = 0;
   }

   for (S = HufTable->HufSym; S < &HufTable->HufSym[HufTable->N]; S++) {
      if (S->Len < 0 || S->Len > HufTable->LenMax) Error("S->Len = %d in CompCodes()",S->Len);
      if (S->Len != 0) CodeLen[S->Len].HufSymNb++;
   }

   for (LenMin = 0; LenMin < HufTable->LenMax && CodeLen[LenMin].HufSymNb == 0; LenMin++)
      ;
   for (LenMax = HufTable->LenMax; LenMax > 0 && CodeLen[LenMax].HufSymNb == 0; LenMax--)
      ;

   CodeMin = 0;
   for (I = LenMax; I > 0; I--) {
      CodeLen[I].CodeMin = CodeMin;
      CodeMin += CodeLen[I].HufSymNb;
      if ((CodeMin & 1) != 0) Error("CodeMin = %d (Len = %d) in CompCodes()",CodeMin,I);
      CodeMin >>= 1;
   }
   if (CodeMin != 1) Error("CodeMin (%d) != 1 in CompCodes()",CodeMin);

   for (S = HufTable->HufSym; S < &HufTable->HufSym[HufTable->N]; S++) {
      if (S->Len != 0) {
         S->Code = CodeLen[S->Len].CodeMin;
         CodeLen[S->Len].CodeMin++;
      }
   }

   for (I = LenMin; I <= LenMax; I++) CodeLen[I].CodeMin -= CodeLen[I].HufSymNb;
}

/* CompDecodeTable() */

void CompDecodeTable(const huftable *HufTable) {

   int I, Index, Len;

   Index = 0;
   for (I = LenMin; I <= LenMax; I++) {
      CodeLen[I].HufSymIndex = Index;
      Index += CodeLen[I].HufSymNb;
   }

   for (I = 0; I < HufTable->N; I++) {
      Len = HufTable->HufSym[I].Len;
      if (Len != 0) {
         HufSymArray[CodeLen[Len].HufSymIndex] = I;
         CodeLen[Len].HufSymIndex++;
      }
   }

   for (I = LenMin; I <= LenMax; I++) {
      CodeLen[I].HufSymIndex -= CodeLen[I].CodeMin + CodeLen[I].HufSymNb;
   }
}

/* PredictLen() */

int PredictLen(const huftable *HufTable) {

   int Last, Len, LastLen, RepLen, HufSymBit, LenBit, Size, LenFreq[LEN_MAX+4];
   hufsym *S;
   huftable LenTable[1];

   Size = 0;

   HufSymBit = Log2(HufTable->N-1) + 1;
   for (Last = HufTable->N-1; Last > 0 && HufTable->HufSym[Last].Len == 0; Last--)
      ;
   Size += HufSymBit;

   switch (HufTable->Format) {

   case STORE :

      LenBit = Log2(HufTable->LenMax) + 1;
      Size += (Last + 1) * LenBit;

      break;

   case DELTA :

      LenBit = Log2(HufTable->LenMax) + 1;
      Len = HufTable->HufSym[0].Len;
      Size += LenBit;

      for (S = HufTable->HufSym+1; S <= &HufTable->HufSym[Last]; S++) {
         if (S->Len == 0) {
            Size++;
         } else if (Len == S->Len) {
            Size += 2;
         } else if (Len > S->Len) {
            Size += 3 + (Len - S->Len);
         } else {
            Size += 3 + (S->Len - Len);
         }
      }

      break;

   case HUFFMAN :

      for (Len = 0; Len <= LEN_MAX+3; Len++) LenFreq[Len] = 0;

      LastLen = -1;
      RepLen  = 0;
      for (S = HufTable->HufSym; S <= &HufTable->HufSym[Last]; S++) {
         Len = S->Len;
         if (Len == LastLen) {
            RepLen++;
         } else {
            if (RepLen > 0) SimRleLen(RepLen,LastLen,LenFreq);
            LastLen = Len;
            RepLen  = 1;
         }
      }
      if (RepLen > 0) SimRleLen(RepLen,LastLen,LenFreq);

      AllocHufTable(LenTable,LEN_MAX+4,15,STORE);

      CompLens(LenTable,LenFreq);
      Size += PredictLen(LenTable);
      FreeHufTable(LenTable);

      Size += 3 * LenFreq[0];
      Size += 7 * LenFreq[1];
      Size += 2 * LenFreq[2];

      break;
   }

   for (S = HufTable->HufSym; S < &HufTable->HufSym[HufTable->N]; S++) {
      if (S->Freq != 0) Size += S->Freq * S->Len;
   }

   return Size;
}

/* GetLens() */

void GetLens(huftable *HufTable) {

   int Last, Code, Len, RepLen, HufSymBit, LenBit;
   hufsym *S;
   huftable LenTable[1];

   for (S = HufTable->HufSym; S < &HufTable->HufSym[HufTable->N]; S++) {
      S->Freq   = 0;
      S->Len    = 0;
      S->Code   = 0;
      S->Parent = NULL;
   }

   HufSymBit = Log2(HufTable->N-1) + 1;
   Last = GetBits(HufSymBit);

   switch (HufTable->Format) {

   case STORE :

      LenBit = Log2(HufTable->LenMax) + 1;

      for (S = HufTable->HufSym; S <= &HufTable->HufSym[Last]; S++) {
         Len = GetBits(LenBit);
         S->Len = Len;
      }

      break;

   case DELTA :

      LenBit = Log2(HufTable->LenMax) + 1;
      Len = GetBits(LenBit);
      HufTable->HufSym[0].Len = Len;

      for (S = HufTable->HufSym+1; S <= &HufTable->HufSym[Last]; S++) {
         if (GetBit() == 1) { /* 1 */
            S->Len = 0;
         } else if (GetBit() == 1) { /* 01 */
            S->Len = Len;
         } else if (GetBit() == 0) { /* 000 */
            do Len--; while (GetBit() == 1);
            S->Len = Len;
         } else { /* 001 */
            do Len++; while (GetBit() == 1);
            S->Len = Len;
         }
      }

      break;

   case HUFFMAN :

      AllocHufTable(LenTable,LEN_MAX+4,15,STORE);

      GetLens(LenTable);
      CompCodes(LenTable);
      CompDecodeTable(LenTable);

      Len = -1;

      for (S = HufTable->HufSym; S <= &HufTable->HufSym[Last];) {
         Code = GetHufSym(LenTable);
         if (Code == 0) { /* REPZ 3-10 */
            RepLen = GetBits(3) + 3;
            while (--RepLen >= 0) (S++)->Len = 0;
         } else if (Code == 1) { /* REPZ 11-138 */
            RepLen = GetBits(7) + 11;
            while (--RepLen >= 0) (S++)->Len = 0;
         } else if (Code == 2) { /* REP 3-6 */
            RepLen = GetBits(2) + 3;
            while (--RepLen >= 0) (S++)->Len = Len;
         } else {
            Len = Code - 3;
            (S++)->Len = Len;
         }
      }

      FreeHufTable(LenTable);

      break;
   }

   for (; S < &HufTable->HufSym[HufTable->N]; S++) S->Len = 0;
}

/* SendLens() */

void SendLens(const huftable *HufTable) {

   int Last, Len, LastLen, RepLen, HufSymBit, LenBit, LenFreq[LEN_MAX+4];
   hufsym *S;
   huftable LenTable[1];

   HufSymBit = Log2(HufTable->N-1) + 1;
   for (Last = HufTable->N-1; Last > 0 && HufTable->HufSym[Last].Len == 0; Last--)
      ;

   SendBits(HufSymBit,Last);

   switch (HufTable->Format) {

   case STORE :

      LenBit = Log2(HufTable->LenMax) + 1;
      for (S = HufTable->HufSym; S <= &HufTable->HufSym[Last]; S++) {
         Len = S->Len;
         SendBits(LenBit,Len);
      }

      break;

   case DELTA :

      LenBit = Log2(HufTable->LenMax) + 1;
      Len = HufTable->HufSym[0].Len;
      SendBits(LenBit,Len);

      for (S = HufTable->HufSym+1; S <= &HufTable->HufSym[Last]; S++) {
         if (S->Len == 0) {
            SendBit(1); /* 1 */
         } else if (Len == S->Len) {
            SendBits(2,1); /* 01 */
         } else if (Len > S->Len) {
            SendBits(3,0); /* 000 */
            while (--Len > S->Len) SendBit(1);
            SendBit(0);
         } else {
            SendBits(3,1); /* 001 */
            while (++Len < S->Len) SendBit(1);
            SendBit(0);
         }
      }

      break;

   case HUFFMAN :

      for (Len = 0; Len <= LEN_MAX+3; Len++) LenFreq[Len] = 0;

      LastLen = -1;
      RepLen  = 0;
      for (S = HufTable->HufSym; S <= &HufTable->HufSym[Last]; S++) {
         Len = S->Len;
         if (Len == LastLen) {
            RepLen++;
         } else {
            if (RepLen > 0) SimRleLen(RepLen,LastLen,LenFreq);
            LastLen = Len;
            RepLen  = 1;
         }
      }
      if (RepLen > 0) SimRleLen(RepLen,LastLen,LenFreq);

      AllocHufTable(LenTable,LEN_MAX+4,15,STORE);
      CompLens(LenTable,LenFreq);
      CompCodes(LenTable);

      SendLens(LenTable);

      LastLen = -1;
      RepLen  = 0;
      for (S = HufTable->HufSym; S <= &HufTable->HufSym[Last]; S++) {
         Len = S->Len;
         if (Len == LastLen) {
            RepLen++;
         } else {
            if (RepLen > 0) SendRleLen(LenTable,RepLen,LastLen);
            LastLen = Len;
            RepLen  = 1;
         }
      }
      if (RepLen > 0) SendRleLen(LenTable,RepLen,LastLen);

      FreeHufTable(LenTable);

      break;
   }
}

/* SimRleLen() */

static void SimRleLen(int RepLen, int Len, int LenFreq[]) {

   if (Len == 0) {
      while (RepLen >= 139) {
         LenFreq[1]++; /* REPZ 11-138 */
         RepLen -= 138;
      }
      if (RepLen >= 11) {
         LenFreq[1]++; /* REPZ 11-138 */
      } else if (RepLen >= 3) {
         LenFreq[0]++; /* REPZ 3-10 */
      } else {
         LenFreq[3] += RepLen; /* 0 */
      }
   } else {
      LenFreq[Len+3]++; /* Len */
      RepLen--;
      while (RepLen >= 7) {
         LenFreq[2]++; /* REP 3-6 */
         RepLen -= 6;
      }
      if (RepLen >= 3) {
         LenFreq[2]++; /* REP 3-6 */
      } else {
         LenFreq[Len+3] += RepLen; /* Len */
      }
   }
}

/* SendRleLen() */

static void SendRleLen(const huftable *LenTable, int RepLen, int Len) {

   if (Len == 0) {
      while (RepLen >= 139) {
         SendHufSym(LenTable,1); /* REPZ 11-138 */
         SendBits(7,138-11);
         RepLen -= 138;
      }
      if (RepLen >= 11) {
         SendHufSym(LenTable,1); /* REPZ 11-138 */
         SendBits(7,RepLen-11);
      } else if (RepLen >= 3) {
         SendHufSym(LenTable,0); /* REPZ 3-10 */
         SendBits(3,RepLen-3);
      } else {
         while (--RepLen >= 0) SendHufSym(LenTable,3); /* 0 */
      }
   } else {
      SendHufSym(LenTable,Len+3); /* Len */
      RepLen--;
      while (RepLen >= 7) {
         SendHufSym(LenTable,2); /* REP 3-6 */
         SendBits(2,6-3);
         RepLen -= 6;
      }
      if (RepLen >= 3) {
         SendHufSym(LenTable,2); /* REP 3-6 */
         SendBits(2,RepLen-3);
      } else {
         while (--RepLen >= 0) SendHufSym(LenTable,Len+3); /* Len */
      }
   }
}

/* GetHufSym() */

int GetHufSym(const huftable *HufTable) {

   int Code, Len;

   Len  = 0;
   Code = 0;

   do {
      Code = (Code << 1) | GetBit();
      Len++;
   } while (Code < CodeLen[Len].CodeMin);

   return HufSymArray[Code+CodeLen[Len].HufSymIndex];
}

/* SendHufSym() */

void SendHufSym(const huftable *HufTable, int Symbol) {

   hufsym *S; /* const */

   S = (hufsym *) &HufTable->HufSym[Symbol];

   assert(S->Freq>0);
   assert(S->Len>0);

   SendBits(S->Len,S->Code);

   S->Freq--;
}

/* CheckFreqs() */

void CheckFreqs(const huftable *HufTable) {

   const hufsym *S;

   for (S = HufTable->HufSym; S < &HufTable->HufSym[HufTable->N]; S++) {
      assert(S->Freq==0);
   }
}

/* UpdateHeap() */

static void UpdateHeap(int Root) {

   int Node, Son;
   hufsym *HufSym;

   if (HeapSize <= 1) Warning("HeapSize = %d in UpdateHeap()",HeapSize);

   HufSym = Heap[Root];

   for (Node = Root; TRUE; Node = Son) {
      Son = Node << 1;
      if (Son > HeapSize) break;
      if (Son < HeapSize && SMALLER(Heap[Son+1],Heap[Son])) Son++;
      if (! SMALLER(Heap[Son],HufSym)) break;
      Heap[Node] = Heap[Son];
   }

   Heap[Node] = HufSym;
}

/* Log2() */

static int Log2(int N) {

   int L;

   for (L = -1; N > 0; L++, N >>= 1)
      ;

   return L;
}

/* End of Huffman.C */

