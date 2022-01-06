
/* Huffman.H */

#ifndef HUFFMAN_H
#define HUFFMAN_H

/* Constants */

enum { STORE, DELTA, HUFFMAN };

/* Types */

typedef struct hufsym hufsym;

struct hufsym {
   int     Freq;
   int     Len;
   int     Code;
   hufsym *Parent;
};

typedef struct {
   int     N;
   int     LenMax;
   int     Format;
   hufsym *HufSym;
} huftable;

/* Prototypes */

extern void AllocHufTable   (huftable *HufTable, int N, int LenMax, int Format);
extern void FreeHufTable    (huftable *HufTable);

extern void CompLens        (huftable *HufTable, const int Freq[]);
extern void CompCodes       (huftable *HufTable);
extern void CompDecodeTable (const huftable *HufTable);

extern int  PredictLen      (const huftable *HufTable);
extern int  PredictLens     (const huftable *HufTable);

extern void GetLens         (huftable *HufTable);
extern void SendLens        (const huftable *HufTable);

extern int  GetHufSym       (const huftable *HufTable);
extern void SendHufSym      (const huftable *HufTable, int Symbol);

extern void CheckFreqs      (const huftable *HufTable);

#endif /* ! defined HUFFMAN_H */

/* End of Huffman.H */

