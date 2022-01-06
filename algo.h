
/* Algo.H */

#ifndef ALGO_H
#define ALGO_H

#include "types.h"

/* Constants */

enum { ALGO_STORE, ALGO_LZH, ALGO_BWT, ALGO_PPM, ALGO_NB };

/* Variables */

extern const char *Source;
extern const char *Destination;

extern int    Algorithm, Delta, Group, Order, Verbosity;

extern uchar *S;
extern int    N;

/* Prototypes */

extern int         AlgorithmNo   (const char *Name);
extern const char *AlgorithmName (int No);

extern void CrunchFile    (void);
extern void DecrunchFile  (void);

extern void CrunchBlock   (void);
extern void DecrunchBlock (void);

extern void AllocBlock    (void);
extern void FreeBlock     (void);

#endif /* ! defined ALGO_H */

/* End of Algo.H */

