
/* BWT.H */

#ifndef BWT_H
#define BWT_H

#include "types.h"

/* Constants */

#define SIZE 2097152

/* Variables */

extern int    Index;
extern uchar *L;

/* Prototypes */

extern void CodeBWT   (void);
extern void DecodeBWT (void);

#endif /* ! defined BWT_H */

/* End of BWT.H */

