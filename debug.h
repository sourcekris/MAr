
/* Debug.H */

#ifndef DEBUG_H
#define DEBUG_H

#include "types.h"

#ifdef DEBUG
#undef DEBUG
#define DEBUG TRUE
#endif

#if ! DEBUG
#define NDEBUG
#endif
#include <assert.h>

/* Prototypes */

extern void  Warning    (const char *Message, ...);
extern void  Error      (const char *Message, ...);
extern void  FatalError (const char *Message, ...);

extern void *Malloc     (int Size);
extern void *Nalloc     (int Size, const char *Name);
extern void  Free       (void *Address);

#endif /* ! defined DEBUG_H */

/* End of Debug.H */

