
/* Ari.C */

#include <stdio.h>
#include <stdlib.h>

#include "ari.h"
#include "types.h"
#include "bitio.h"
#include "debug.h"

/* Constants */

#define CODE_BIT 16
#define FREQ_BIT 14

/* Variables */

static int One, Half, Quarter, ThreeQuarters;
static int Low, High;
static int Bpf, Code;

/* Prototypes */

static void InitFreqs     (aritable *AriTable);
static void IncFreq       (aritable *AriTable, int Symbol);

static void HalveFreqs    (aritable *AriTable);

static void BitPlusFollow (int Bit);

/* Functions */

/* AllocAriTable() */

void AllocAriTable(aritable *AriTable, int N) {

   assert(N>=0);

   AriTable->N      = N;
   AriTable->AriSym = Nalloc((size_t)((N+1)*sizeof(arisym)),"Ari table");
}

/* FreeAriTable() */

void FreeAriTable(aritable *AriTable) {

   AriTable->N = 0;

   if (AriTable->AriSym != NULL) {
      Free(AriTable->AriSym);
      AriTable->AriSym = NULL;
   }
}

/* SetFreqs() */

void SetFreqs(aritable *AriTable, const int Freq[]) {

   int S;

   for (S = 0; S < AriTable->N; S++) AriTable->AriSym[S].Freq = Freq[S];

   InitFreqs(AriTable);
}

/* InitFreqs() */

static void InitFreqs(aritable *AriTable) {

   int S, CFreq;

   while (TRUE) {

      CFreq = 0;
      for (S = 0; S < AriTable->N; S++) {
         AriTable->AriSym[S].CFreq = CFreq;
         CFreq += AriTable->AriSym[S].Freq;
      }
      AriTable->AriSym[S].CFreq = CFreq;

      if (CFreq < 1<<FREQ_BIT) break;

      HalveFreqs(AriTable);
   }
}

/* IncFreq() */

static void IncFreq(aritable *AriTable, int Symbol) {

   int S, CFreq;

   S = Symbol;
   AriTable->AriSym[S].Freq++;

   CFreq = AriTable->AriSym[S].CFreq + AriTable->AriSym[S].Freq;
   for (S++; S < AriTable->N; S++) {
      AriTable->AriSym[S].CFreq = CFreq;
      CFreq += AriTable->AriSym[S].Freq;
   }
   AriTable->AriSym[S].CFreq = CFreq;

   if (CFreq >= 1<<FREQ_BIT) {
      HalveFreqs(AriTable);
      InitFreqs(AriTable);
   }
}

/* HalveFreqs() */

static void HalveFreqs(aritable *AriTable) {

   int S;

   for (S = 0; S < AriTable->N; S++) {
      AriTable->AriSym[S].Freq = (AriTable->AriSym[S].Freq + 1) / 2;
   }
}

/* GetFreqs() */

void GetFreqs(aritable *AriTable) {

   int S;

   for (S = 0; S < AriTable->N; S++) {
      AriTable->AriSym[S].Freq = GetBits(FREQ_BIT);
   }

   InitFreqs(AriTable);
}

/* SendFreqs() */

void SendFreqs(const aritable *AriTable) {

   int S;

   for (S = 0; S < AriTable->N; S++) {
      SendBits(FREQ_BIT,AriTable->AriSym[S].Freq);
   }
}

/* SendStart() */

void SendStart(void) {

   One           = 1 << CODE_BIT;
   Half          = One >> 1;
   Quarter       = Half >> 1;
   ThreeQuarters = Half + Quarter;

   Low  = 0;
   High = One;

   Bpf = 0;
}

/* SendEnd() */

void SendEnd(void) {

   if (Low == 0 && High == One && Bpf == 0) {
      ;
   } else if (Low == 0) {
      BitPlusFollow(0);
   } else if (High == One) {
      BitPlusFollow(1);
   } else {
      Bpf++;
      if (Low < Quarter) {
         BitPlusFollow(0);
      } else {
         BitPlusFollow(1);
      }
   }

   SendBits(CODE_BIT,0); /* because of decoder lookup */
}

/* SendAriSym() */

void SendAriSym(const aritable *AriTable, int Symbol) {

   SendAriRange(AriTable->AriSym[Symbol].CFreq,AriTable->AriSym[Symbol+1].CFreq,AriTable->AriSym[AriTable->N].CFreq);
}

/* SendAriRange() */

void SendAriRange(int RangeLow, int RangeHigh, int RangeTot) {

   int Range;

   Range = High - Low;

   High = Low + RangeHigh * Range / RangeTot;
   Low  = Low + RangeLow  * Range / RangeTot;

   while (TRUE) {

      if (High <= Half) {
         BitPlusFollow(0);
         Bpf = 0;
      } else if (Low >= Half) {
         BitPlusFollow(1);
         Bpf = 0;
         Low  -= Half;
         High -= Half;
      } else if (Low >= Quarter && High <= ThreeQuarters) {
         Bpf++;
         Low  -= Quarter;
         High -= Quarter;
      } else {
         break;
      }

      Low  += Low;
      High += High;
   }
}

/* BitPlusFollow() */

static void BitPlusFollow(int Bit) {

   SendBit(Bit);
   for (; Bpf > 0; Bpf--) SendBit(1-Bit);
}

/* GetStart() */

void GetStart(void) {

   One           = 1 << CODE_BIT;
   Half          = One >> 1;
   Quarter       = Half >> 1;
   ThreeQuarters = Half + Quarter;

   Low  = 0;
   High = One;

   Code = GetBits(CODE_BIT);
}

/* GetEnd() */

void GetEnd(void) {

   if (Low  != 0)   GetBit();
   if (High != One) GetBit();
}

/* GetAriSym() */

int GetAriSym(const aritable *AriTable) {

   int Range, NewCode, Symbol;

   Range = High - Low;

   NewCode = GetAriRange(AriTable->AriSym[AriTable->N].CFreq);

   for (Symbol = 0; NewCode >= AriTable->AriSym[Symbol+1].CFreq; Symbol++)
      ;

   SkipAriRange(AriTable->AriSym[Symbol].CFreq,AriTable->AriSym[Symbol+1].CFreq,AriTable->AriSym[AriTable->N].CFreq);

   return Symbol;
}

/* GetAriRange() */

int GetAriRange(int SymTot) {

   return ((Code - Low + 1) * SymTot - 1) / (High - Low);
}

/* SkipAriRange() */

void SkipAriRange(int SymLow, int SymHigh, int SymTot) {

   int Range;

   Range = High - Low;

   High = Low + SymHigh * Range / SymTot;
   Low  = Low + SymLow  * Range / SymTot;

   while (TRUE) {

      if (High <= Half) {
         ;
      } else if (Low >= Half) {
         Code -= Half;
         Low  -= Half;
         High -= Half;
      } else if (Low >= Quarter && High <= ThreeQuarters) {
         Code -= Quarter;
         Low  -= Quarter;
         High -= Quarter;
      } else {
         break;
      }

      Code += Code + GetBit();
      Low  += Low;
      High += High;
   }
}

/* End of Ari.C */

