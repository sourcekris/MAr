
/* Archive.C */

#include <stdio.h>
#include <string.h>

#include "archive.h"
#include "types.h"
#include "algo.h"
#include "bitio.h"
#include "bwt.h"
#include "crc.h"
#include "debug.h"
#include "delta.h"

/* Prototypes */

static void GetFileInfo (archive *Archive);
static void DispHeader  (const header *Header);

/* Functions */

/* OpenArchive() */

archive *OpenArchive(const char *Name) {

   archive *Archive;

   OpenInStream(Name);

   if (GetUInt8() != 'M' || GetUInt8() != 'A' || GetUInt8() != 'r') {
      CloseInStream();
      return NULL;
   }
   GetUInt8();

   Archive = Nalloc(sizeof(archive),"Archive structure");

   strcpy(Archive->Name,Name);
   Archive->Stream = InStream;

   GetFileInfo(Archive);

   return Archive;
}

/* CloseArchive() */

void CloseArchive(archive *Archive) {

/*
   InStream = Archive->Stream;
*/
   CloseInStream();

   Free(Archive);
}

/* FirstFile() */

int FirstFile(archive *Archive) {

/*
   InStream = Archive->Stream;
*/

   if (fseek(Archive->Stream->File,4,SEEK_SET) != 0) Error("FirstFile(): fseek()");
   GetFileInfo(Archive);

   return ! Archive->End;
}

/* NextFile() */

int NextFile(archive *Archive) {

/*
   InStream = Archive->Stream;
*/

   if (Archive->End) return FALSE;

   if (fseek(Archive->Stream->File,Archive->Pos+Archive->Header->HeaderSize+Archive->Header->ArcSize,SEEK_SET) != 0) {
      Error("NextFile(): fseek()");
      Archive->End = TRUE;
      return FALSE;
   }
   if (ftell(Archive->Stream->File) != Archive->Pos+Archive->Header->HeaderSize+Archive->Header->ArcSize) {
      Error("NextFile(): ftell()");
      Archive->End = TRUE;
      return FALSE;
   }

   GetFileInfo(Archive);

   return ! Archive->End;
}

/* ReadFile() */

void ReadFile(archive *Archive, void *Address) {

   fseek(Archive->Stream->File,Archive->Pos+Archive->Header->HeaderSize,SEEK_SET);

   if (Archive->Header->Algorithm == ALGO_STORE) {

      GetBlock(Address,Archive->Header->FileSize);

   } else {

      InitCruncher();
      Algorithm = Archive->Header->Algorithm;

      OpenBitStream(Archive->Stream);

      S = Address;
      N = Archive->Header->FileSize;
      DecrunchBlock();

      CloseBitStream(Archive->Stream);

      if (Archive->Header->Delta != 0) {
	 Delta = Archive->Header->Delta;
	 DecodeDelta(Address,Archive->Header->FileSize);
      }
   }

   if (CRC(Address,Archive->Header->FileSize) != Archive->Header->FileCRC) FatalError("Bad CRC");
}

/* InitCruncher() */

void InitCruncher(void) {

   Algorithm = ALGO_LZH;

   Delta     = 0; /* Delta coding distance */
   Order     = 3; /* PPM Order */
   Verbosity = 0;
}

/* GetFileInfo() */

static void GetFileInfo(archive *Archive) {

/*
   InStream = Archive->Stream;
*/

   Archive->Pos = ftell(Archive->Stream->File);
   if (Archive->Pos == -1) {
      Error("GetFileInfo(): ftell()");
      Archive->End = TRUE;
      return;
   }

   GetHeader(Archive->Stream,Archive->Header);
   if (Archive->Header->HeaderSize == 0 || EndOfFile()) {
      Archive->End = TRUE;
      return;
   }

   if (fseek(Archive->Stream->File,Archive->Pos+Archive->Header->HeaderSize+Archive->Header->ArcSize,SEEK_SET) != 0) {
      Error("GetFileInfo(): fseek()");
      Archive->End = TRUE;
      return;
   }

   Archive->End = EndOfFile();

   if (fseek(Archive->Stream->File,Archive->Pos,SEEK_SET) != 0) {
      Error("GetFileInfo(): fseek()");
      Archive->End = TRUE;
      return;
   }
}

/* GetHeader() */

void GetHeader(stream *Stream, header *Header) {

/*
   InStream = Stream;
*/

   Header->HeaderSize = GetUInt16();
   Header->ArcSize = GetUInt32();
   Header->FileNameSize = GetUInt16();
   GetBlock(Header->FileName,Header->FileNameSize);
   Header->FileName[Header->FileNameSize] = '\0';
   Header->FileSize = GetUInt32();
   Header->Algorithm = GetUInt8();
   Header->Delta = GetUInt8();
   Header->FileCRC = GetUInt32();
   Header->HeaderCRC = GetUInt32();
}

/* SendHeader() */

void SendHeader(stream *Stream, const header *Header) {

/*
   OutStream = Stream;
*/

   SendUInt16(Header->HeaderSize);
   SendUInt32(Header->ArcSize);
   SendUInt16(Header->FileNameSize);
   SendBlock(Header->FileName,Header->FileNameSize);
   SendUInt32(Header->FileSize);
   SendUInt8(Header->Algorithm);
   SendUInt8(Header->Delta);
   SendUInt32(Header->FileCRC);
   SendUInt32(Header->HeaderCRC);
}

/* DispHeader() */

static void DispHeader(const header *Header) {

   printf("HeaderSize   = %d\n",Header->HeaderSize);
   printf("ArcSize      = %d\n",Header->ArcSize);
   printf("FileNameSize = %d\n",Header->FileNameSize);
   printf("FileName     = \"%s\"\n",Header->FileName);
   printf("FileSize     = %d\n",Header->FileSize);
   printf("Algo         = \"%s\"\n",AlgorithmName(Header->Algorithm));
   printf("Delta        = %d\n",Header->Delta);
   printf("FileCRC      = 0x%08X\n",Header->FileCRC);
   printf("HeaderCRC    = 0x%08X\n",Header->HeaderCRC);
}

/* End of Archive.C */

