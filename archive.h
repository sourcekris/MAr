
/* Archive.H */

#ifndef ARCHIVE_H
#define ARCHIVE_H

#include "types.h"
#include "bitio.h"

/* Types */

typedef struct {
   int  HeaderSize;      /* 2 bytes */
   int  ArcSize;         /* 4 bytes */
   int  FileNameSize;    /* 2 bytes */
   char FileName[256+1]; /* ? bytes */
   int  FileSize;        /* 4 bytes */
   int  Algorithm;       /* 1 byte  */
   int  Delta;           /* 1 byte  */
   uint FileCRC;         /* 4 bytes */
   uint HeaderCRC;       /* 4 bytes */
} header;

typedef struct {
   char    Name[255+1];
   header  Header[1];
   int     End;
   stream *Stream;      /* Private */
   int     Pos;         /* Private */
} archive;

/* Prototypes */

extern archive *OpenArchive  (const char *Name);
extern void     CloseArchive (archive *Archive);

extern int      FirstFile    (archive *Archive);
extern int      NextFile     (archive *Archive);

extern void     InitCruncher (void);
extern void     ReadFile     (archive *Archive, void *Address);

extern void     GetHeader    (stream *Stream, header *Header);
extern void     SendHeader   (stream *Stream, const header *Header);

#endif /* ! defined ARCHIVE_H */

/* End of Archive.H */

