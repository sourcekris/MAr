
/* BitIO.C */

#include <stdio.h>

#include "bitio.h"
#include "types.h"
#include "debug.h"

/* Variables */

stream InStream[1];
stream OutStream[1];

/* Functions */

/* CloseStream() */

void CloseStream(stream *Stream) {

   assert(Stream->Type!=STREAM_CLOSED);
   assert(Stream->Mode!=STREAM_NONE);
   assert(!Stream->IsBitStream);

   fclose(Stream->File);
   Stream->File = NULL;

   Stream->Type = STREAM_CLOSED;
   Stream->Mode = STREAM_NONE;
}

/* OpenBitStream() */

void OpenBitStream(stream *Stream) {

   assert(Stream->Type!=STREAM_CLOSED);
   assert(Stream->Mode!=STREAM_NONE);
   assert(!Stream->IsBitStream);

   Stream->IsBitStream = TRUE;

   Stream->BitBuffer = 0;
   Stream->BitNb     = 0;
}

/* CloseBitStream() */

void CloseBitStream(stream *Stream) {

   assert(Stream->Type!=STREAM_CLOSED);
   assert(Stream->Mode!=STREAM_NONE);
   assert(Stream->IsBitStream);

   if (Stream->Mode == STREAM_WRITE && Stream->BitNb != 0) {
      fputc((int)(Stream->BitBuffer>>24),Stream->File);
      Stream->ByteNb++;
   }

   Stream->BitBuffer = 0;
   Stream->BitNb     = 0;

   Stream->IsBitStream = FALSE;
}

/* OpenInStream() */

void OpenInStream(const char *FileName) {

   stream *Stream;

   Stream = InStream;

   Stream->ByteNb = 0;

   Stream->Type = STREAM_FILE;
   Stream->Mode = STREAM_READ;

   if (FileName != NULL) {
      Stream->File = fopen(FileName,"rb");
      if (Stream->File == NULL) FatalError("Couldn't open file \"%s\" for reading",FileName);
   } else {
      Stream->File = stdin;
   }

   Stream->IsBitStream = FALSE;
}

/* EndOfFile() */

int EndOfFile(void) {

   stream *Stream;

   Stream = InStream;

   return feof(Stream->File);
}

/* CloseInStream() */

void CloseInStream(void) {

   CloseStream(InStream);
}

/* OpenOutStream() */

void OpenOutStream(const char *FileName) {

   stream *Stream;

   Stream = OutStream;

   Stream->ByteNb = 0;

   Stream->Type = STREAM_FILE;
   Stream->Mode = STREAM_WRITE;

   if (FileName != NULL) {
      Stream->File = fopen(FileName,"wb");
      if (Stream->File == NULL) FatalError("Couldn't open file \"%s\" for writing",FileName);
   } else {
      Stream->File = stdout;
   }

   Stream->IsBitStream = FALSE;
}

/* AppendOutStream() */

void AppendOutStream(const char *FileName) {

   stream *Stream;

   Stream = OutStream;

   Stream->ByteNb = 0;

   Stream->Type = STREAM_FILE;
   Stream->Mode = STREAM_WRITE;

   Stream->File = fopen(FileName,"rb+");
   if (Stream->File == NULL) FatalError("Couldn't open file \"%s\" for appending",FileName);

   fseek(Stream->File,0,SEEK_END);

   Stream->IsBitStream = FALSE;
}

/* CloseOutStream() */

void CloseOutStream(void) {

   CloseStream(OutStream);
}

/* GetBit() */

int GetBit(void) {

   stream *Stream;
   int Bit;

   Stream = InStream;

   assert(Stream->Type!=STREAM_CLOSED);
   assert(Stream->Mode==STREAM_READ);
   assert(Stream->IsBitStream);

   if (Stream->BitNb == 0) {
      Bit = fgetc(Stream->File);
      if (Bit == EOF) FatalError("GetBit(): unexpected EOF in input stream");
      Stream->ByteNb++;
      Stream->BitBuffer = (uint) Bit << 24;
      Stream->BitNb     = 8;
   }

   Bit = (int) (Stream->BitBuffer >> 31);
   Stream->BitBuffer <<= 1;
   Stream->BitNb--;

   return Bit;
}

/* GetBits() */

int GetBits(int N) {

   stream *Stream;
   int Bits;

   Stream = InStream;

   assert(Stream->Type!=STREAM_CLOSED);
   assert(Stream->Mode==STREAM_READ);
   assert(Stream->IsBitStream);

   assert(N>0&&N<=25);

   while (Stream->BitNb < N) {
      Bits = fgetc(Stream->File);
      if (Bits == EOF) FatalError("GetBits(): unexpected EOF in input stream");
      Stream->ByteNb++;
      Stream->BitBuffer |= (uint) Bits << (24 - Stream->BitNb);
      Stream->BitNb += 8;
   }

   Bits = (int) (Stream->BitBuffer >> (32 - N));
   Stream->BitBuffer <<= N;
   Stream->BitNb -= N;

   return Bits;
}

/* SendBit() */

void SendBit(int Bit) {

   stream *Stream;

   Stream = OutStream;

   assert(Stream->Type!=STREAM_CLOSED);
   assert(Stream->Mode==STREAM_WRITE);
   assert(Stream->IsBitStream);

   assert(Bit==0||Bit==1);

   Stream->BitBuffer |= (uint) Bit << (31 - Stream->BitNb);
   Stream->BitNb++;

   if (Stream->BitNb >= 8) {
      fputc((int)(Stream->BitBuffer>>24),Stream->File);
      Stream->ByteNb++;
      Stream->BitBuffer <<= 8;
      Stream->BitNb -= 8;
   }
}

/* SendBits() */

void SendBits(int N, int Bits) {

   stream *Stream;

   Stream = OutStream;

   assert(Stream->Type!=STREAM_CLOSED);
   assert(Stream->Mode==STREAM_WRITE);
   assert(Stream->IsBitStream);

   assert(N>0&&N<=25);
   assert(Bits>=0&&Bits<(1<<N));

/*
   if (N > 25) {
      if (Bits >= 1 << 25) FatalError("SendBits(): More then 25 bits");
      do SendBit(0); while (--N > 25);
   }
*/

   Stream->BitBuffer |= (uint) Bits << (32 - Stream->BitNb - N);
   Stream->BitNb += N;

   while (Stream->BitNb >= 8) {
      fputc((int)(Stream->BitBuffer>>24),Stream->File);
      Stream->ByteNb++;
      Stream->BitBuffer <<= 8;
      Stream->BitNb -= 8;
   }
}

/* GetUInt8() */

int GetUInt8(void) {

   stream *Stream;

   Stream = InStream;

   assert(Stream->Type!=STREAM_CLOSED);
   assert(Stream->Mode==STREAM_READ);
   assert(!Stream->IsBitStream);

   return fgetc(Stream->File);
}

/* SendUInt8() */

void SendUInt8(int UInt8) {

   stream *Stream;

   Stream = OutStream;

   assert(Stream->Type!=STREAM_CLOSED);
   assert(Stream->Mode==STREAM_WRITE);
   assert(!Stream->IsBitStream);

   assert(UInt8>=0x00&&UInt8<0x100);

   fputc(UInt8,Stream->File);
}

/* GetUInt16() */

int GetUInt16(void) {

   stream *Stream;
   int     UInt16;

   Stream = InStream;

   assert(Stream->Type!=STREAM_CLOSED);
   assert(Stream->Mode==STREAM_READ);
   assert(!Stream->IsBitStream);

   UInt16 = 0;
   UInt16 <<= 8;
   UInt16 |= fgetc(Stream->File);
   UInt16 <<= 8;
   UInt16 |= fgetc(Stream->File);

   return UInt16;
}

/* SendUInt16() */

void SendUInt16(int UInt16) {

   stream *Stream;

   Stream = OutStream;

   assert(Stream->Type!=STREAM_CLOSED);
   assert(Stream->Mode==STREAM_WRITE);
   assert(!Stream->IsBitStream);

   assert(UInt16>=0x0000&&UInt16<0x10000);

   fputc((UInt16>>8)&0xFF,Stream->File);
   fputc(UInt16&0xFF,Stream->File);
}

/* GetUInt32() */

uint GetUInt32(void) {
  
   stream *Stream;
   uint UInt32;

   Stream = InStream;

   assert(Stream->Type!=STREAM_CLOSED);
   assert(Stream->Mode==STREAM_READ);
   assert(!Stream->IsBitStream);

   UInt32 = 0;
   UInt32 <<= 8;
   UInt32 |= (uint) fgetc(Stream->File);
   UInt32 <<= 8;
   UInt32 |= (uint) fgetc(Stream->File);
   UInt32 <<= 8;
   UInt32 |= (uint) fgetc(Stream->File);
   UInt32 <<= 8;
   UInt32 |= (uint) fgetc(Stream->File);

   return UInt32;
}

/* SendUInt32() */

void SendUInt32(uint UInt32) {

   stream *Stream;

   Stream = OutStream;

   assert(Stream->Type!=STREAM_CLOSED);
   assert(Stream->Mode==STREAM_WRITE);
   assert(!Stream->IsBitStream);

   fputc((UInt32>>24)&0xFF,Stream->File);
   fputc((UInt32>>16)&0xFF,Stream->File);
   fputc((UInt32>>8)&0xFF,Stream->File);
   fputc(UInt32&0xFF,Stream->File);
}

/* GetBlock() */

int GetBlock(void *Block, int Size) {

   stream *Stream;

   Stream = InStream;

   assert(Stream->Type!=STREAM_CLOSED);
   assert(Stream->Mode==STREAM_READ);
   assert(!Stream->IsBitStream);

   return fread(Block,1,Size,Stream->File);
}

/* SendBlock() */

void SendBlock(const void *Block, int Size) {

   stream *Stream;

   Stream = OutStream;

   assert(Stream->Type!=STREAM_CLOSED);
   assert(Stream->Mode==STREAM_WRITE);
   assert(!Stream->IsBitStream);

   fwrite(Block,1,Size,Stream->File);
}

/* End of BitIO.C */

