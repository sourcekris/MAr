
/* Ari.H */

#ifndef ARI_H
#define ARI_H

/* Types */

typedef struct {
   int Freq;
   int CFreq;
} arisym;

typedef struct {
   int     N;
   arisym *AriSym;
} aritable;

/* Prototypes */

extern void AllocAriTable (aritable *AriTable, int N);
extern void FreeAriTable  (aritable *AriTable);

extern void SetFreqs      (aritable *AriTable, const int Freq[]);

extern void GetFreqs      (aritable *AriTable);
extern void SendFreqs     (const aritable *AriTable);

extern void GetStart      (void);
extern void GetEnd        (void);

extern int  GetAriSym     (const aritable *AriTable);
extern int  GetAriRange   (int SymTot);
extern void SkipAriRange  (int SymLow, int SymHigh, int SymTot);

extern void SendStart     (void);
extern void SendEnd       (void);

extern void SendAriSym    (const aritable *AriTable, int Symbol);
extern void SendAriRange  (int RangeLow, int RangeHigh, int RangeTot);

#endif /* ! defined ARI_H */

/* End of Ari.H */

