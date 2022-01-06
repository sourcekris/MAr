
/* HufBlock.C */

#include <stdio.h>
#include <stdlib.h>

#include "hufblock.h"
#include "types.h"
#include "algo.h"
#include "bitio.h"
#include "bwt.h"
#include "debug.h"
#include "huffman.h"
#include "mtf.h"

/* Constants */

#define SYMBOL_NB      257
#define LEN_MAX        25
#define FORMAT         DELTA

#define BLOCK_SIZE_MIN 256
#define BLOCK_SIZE_MAX 65536
#define BLOCK_SIZE_BIT 16

#define TABLE_NB       256
#define TABLE_NB_MAX   256
#define TABLE_BIT      3

/* Types */

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

/* Variables */

ushort *HufBlock;
int     HufBlockSize;

static block_list BlockList[1];
static int        BlockNb;
static int        BlockLen;

/* Functions */

/* AllocHufBlock() */

void AllocHufBlock(void) {

   HufBlock = malloc((size_t)(N*sizeof(int)));
   if (HufBlock == NULL) FatalError("AllocHufBlock(): malloc() = NULL");

   HufBlockSize = 0;
}

/* FreeHufBlock() */

void FreeHufBlock(void) {

   if (HufBlock != NULL) {
      free(HufBlock);
      HufBlock = NULL;
   }
}

/* GetHufBlock() */

void GetHufBlock(void) {

   int I, J, Size, BlockSize, TableBit, TableNb;
   uchar TableNo[TABLE_NB];
   huftable HufTable[1], Table[TABLE_NB][1];

   HufBlockSize = GetBits(25) + 1;
   BlockSize    = GetBits(BLOCK_SIZE_BIT) + 1;
   BlockNb      = (HufBlockSize + BlockSize - 1) / BlockSize;

   TableBit = GetBits(TABLE_BIT) + 1;
   TableNb  = GetBits(TableBit) + 1;

   if (TableBit == 2 && TableNb == 1) { /* Fake header => blocks */

      Size = 0;

      AllocHufTable(HufTable,SYMBOL_NB,LEN_MAX,FORMAT);

      while (GetBit() == 1) {

         BlockSize = GetBits(BLOCK_SIZE_BIT) + 1;

         GetLens(HufTable);

         CompCodes(HufTable);
         CompDecodeTable(HufTable);

         do HufBlock[Size++] = GetHufSym(HufTable); while (--BlockSize != 0);
      }

      FreeHufTable(HufTable);

      HufBlockSize = Size;

   } else {

      if (Verbosity >= 2) {
         fprintf(stderr,"FullBlockSize = %d\n",HufBlockSize);
         fprintf(stderr,"BlockSize = %d\n",BlockSize);
         fprintf(stderr,"BlockNb = %d\n",BlockNb);
      }

      if (Verbosity >= 2) fprintf(stderr,"TableNb = %d\n",TableNb);

      for (I = 0; I < TableNb; I++) {
         AllocHufTable(Table[I],SYMBOL_NB,LEN_MAX,FORMAT);
         GetLens(Table[I]);
      }

      for (I = 0; I < BlockNb; I++) {
         TableNo[I] = 0;
         while (GetBit() == 1) TableNo[I]++;
      }

      DecodeMTF(TableNo,BlockNb);

      Size = 0;

      for (I = 0; I < BlockNb; I++) {
         CompCodes(Table[TableNo[I]]);
         CompDecodeTable(Table[TableNo[I]]);
         for (J = 0; J < BlockSize && Size < HufBlockSize; J++) {
            HufBlock[Size++] = GetHufSym(Table[TableNo[I]]);
         }
      }

      for (I = 0; I < TableNb; I++) FreeHufTable(Table[I]);
   }
}

/* SendHufBlock() */

void SendHufBlock(void) {

   int I, Start, Gain, BestGain, Freq[SYMBOL_NB];
   block_node *Block, *BestBlock;
   huftable HufTable[1];

   if (HufBlockSize <= 0) FatalError("HufBlockSize (%d) <= 0 in SendHufBlock()",HufBlockSize);

   AllocHufTable(HufTable,SYMBOL_NB,LEN_MAX,FORMAT);

   BlockList->Head = NULL;
   BlockList->Tail = NULL;
   
   BlockNb  = 0;
   BlockLen = 0;

   for (Start = 0; Start < HufBlockSize; Start += BLOCK_SIZE_MIN) {
      
      Block = Nalloc(sizeof(block_node),"BWT block node");
      
      Block->Start = Start;
      Block->End = Block->Start + BLOCK_SIZE_MIN;
      if (Block->End > HufBlockSize) Block->End = HufBlockSize;
      Block->Size = Block->End - Block->Start;

      for (I = 0; I < SYMBOL_NB; I++)             Block->Freq[I] = 0;
      for (I = Block->Start; I < Block->End; I++) Block->Freq[HufBlock[I]]++;

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

   if (Verbosity >= 2) fprintf(stderr,"BlockNb = %4d, BlockSize = %9.2f, TotalLen = %10.2f\n",BlockNb,(double)(HufBlockSize)/(double)BlockNb,(double)BlockLen/8.0);

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

         if (Verbosity >= 3) fprintf(stderr,"BlockNb = %4d, BlockSize = %9.2f, TotalLen = %10.2f, Gain = %7.2f\n",BlockNb,(double)(HufBlockSize)/(double)BlockNb,(double)BlockLen/8.0,(double)BestGain/8.0);

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
   }

   if (Verbosity >= 2) fprintf(stderr,"BlockNb = %4d, BlockSize = %9.2f, TotalLen = %10.2f\n",BlockNb,(double)(HufBlockSize)/(double)BlockNb,(double)BlockLen/8.0);

   /* Fake header for compatibility */

   SendBits(25,0);
   SendBits(BLOCK_SIZE_BIT,0);
   SendBits(TABLE_BIT,2-1); /* 001 */
   SendBits(2,1-1); /* 00 */

   for (Block = BlockList->Head; Block != NULL; Block = Block->Succ) {

      SendBit(1);

      SendBits(BLOCK_SIZE_BIT,Block->Size-1);

      CompLens(HufTable,Block->Freq);
      SendLens(HufTable);

      CompCodes(HufTable);
      for (I = Block->Start; I < Block->End; I++) {
	 SendHufSym(HufTable,HufBlock[I]);
      }
   }

   CheckFreqs(HufTable);

   SendBit(0);

   FreeHufTable(HufTable);
}

/* End of HufBlock.C */

