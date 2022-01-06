
/* Delta.H */

#ifndef DELTA_H
#define DELTA_H

#include "types.h"

/* Constants */

#define DELTA_BIT 3
#define DELTA_MAX 4

/* Prototypes */

extern int  BestDelta   (const void *Block, int Size);

extern void CodeDelta   (void *Block, int Size);
extern void DecodeDelta (void *Block, int Size);

#endif /* ! defined DELTA_H */

/* End of Delta.H */

