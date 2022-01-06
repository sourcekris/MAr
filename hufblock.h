
/* HufBlock.H */

#ifndef HUFBLOCK_H
#define HUFBLOCK_H

#include "types.h"

/* Variables */

extern ushort *HufBlock;
extern int     HufBlockSize;

/* Prototypes */

extern void AllocHufBlock (void);
extern void FreeHufBlock  (void);

extern void GetHufBlock   (void);
extern void SendHufBlock  (void);

#endif /* ! defined HUFBLOCK_H */

/* End of HufBlock.H */

