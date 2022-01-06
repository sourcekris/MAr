
/* BitIO.H */

#ifndef BITIO_H
#define BITIO_H

#include <stdio.h>

#include "types.h"

/* Constants */

enum { STREAM_CLOSED, STREAM_MEMORY, STREAM_FILE  }; /* Type Field */
enum { STREAM_NONE,   STREAM_READ,   STREAM_WRITE }; /* Mode Field */

/* Types */

typedef struct {
   int    Type;
   int    Mode;
   FILE  *File;
   uchar *Buffer;
   uchar *BufferEnd;
   int    IsBitStream;
   uint   BitBuffer;
   int    BitNb;
   int    ByteNb;
} stream;

/* Variables */

extern stream InStream[1];
extern stream OutStream[1];

/* Prototypes */

extern void CloseStream     (stream *Stream);
extern void OpenBitStream   (stream *Stream);
extern void CloseBitStream  (stream *Stream);

extern void OpenInStream    (const char *FileName); /* stdin  if NULL */
extern int  EndOfFile       (void);
extern void CloseInStream   (void);

extern void OpenOutStream   (const char *FileName); /* stdout if NULL */
extern void AppendOutStream (const char *FileName);
extern void CloseOutStream  (void);

extern int  GetBit          (void);
extern int  GetBits         (int N);

extern void SendBit         (int Bit);
extern void SendBits        (int N, int Bits);

extern int  GetUInt8        (void);
extern int  GetUInt16       (void);
extern uint GetUInt32       (void);

extern void SendUInt8       (int UInt8);
extern void SendUInt16      (int UInt16);
extern void SendUInt32      (uint UInt32);

extern int  GetBlock        (void *Block, int Size);
extern void SendBlock       (const void *Block, int Size);

#endif /* ! defined BITIO_H */

/* End of BitIO.H */

