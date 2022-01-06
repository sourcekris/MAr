
/* MAr.C */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mar.h"
#include "types.h"
#include "algo.h"
#include "archive.h"
#include "bitio.h"
#include "bwt.h"
#include "crc.h"
#include "debug.h"
#include "delta.h"

/* Variables */

static char *Program;

/* Prototypes */

static void Usage          (void);

static void DeleteArchive  (const char *ArchiveName, const char **PatternList);
static void ListArchive    (const char *ArchiveName, const char **PatternList);
static void TestArchive    (const char *ArchiveName, const char **PatternList);
static void ExtractArchive (const char *ArchiveName, const char **PatternList);

static void AddFile        (stream *Arc, const char *FileName);
static void TestFile       (archive *Archive);
static void SaveFile       (archive *Archive);

static int  MatchList      (const char *String, const char **PatternList);
static int  Match          (const char *String, const char *Pattern);

/* Functions */

/* Main() */

int main(int argc, char *argv[]) {

   char *C;
   const char *Command, *ArchiveName, **PatternList;
   stream *Arc;

   Program     = *argv++;

   Algorithm   = ALGO_LZH;

   Delta       = 0;     /* Delta */
   Group       = FALSE; /* Huffman tree grouping */
   Order       = 3;     /* PPM Order */
   Verbosity   = 0;

/* Options */

   for (; *argv != NULL && (*argv)[0] == '-'; argv++) {
      switch ((*argv)[1]) {
      case 'a' : /* Algorithm */
         argv++;
         if (*argv != NULL) {
	    for (C = *argv; *C != '\0'; C++) *C = toupper(*C);
	    Algorithm = AlgorithmNo(*argv);
	    if (Algorithm < 0) Usage();
         }
         break;
      case 'g' : /* Group */
         Group = TRUE;
         break;
      case 'o' : /* Order */
         if (argv[1] != NULL && isdigit(argv[1][0])) {
            argv++;
            Order = atoi(*argv);
         }
         break;
      case 't' : /* Delta */
	 Delta = -1;
         if (argv[1] != NULL && isdigit(argv[1][0])) {
            argv++;
            Delta = atoi(*argv);
         }
         break;
      case 'v' : /* Verbosity */
         Verbosity = 1;
         if (argv[1] != NULL && isdigit(argv[1][0])) {
            argv++;
            Verbosity = atoi(*argv);
         }
         break;
      default :
	 Usage();
	 break;
      }
   }

/* Arguments */

   Command = *argv++;
   if (Command == NULL || Command[1] != '\0') Usage();

   ArchiveName = *argv++;
   if (ArchiveName == NULL) Usage();

   PatternList = (const char **) argv;

   if (Verbosity >= 1) fprintf(stderr,MAR_NAME "\n");

/* Command */

   switch (toupper(Command[0])) {

   case 'A' : /* Add */

      if (*PatternList == NULL) Usage();

      AppendOutStream(ArchiveName);
      Arc = OutStream;
      for (; *PatternList != NULL; PatternList++) {
	 printf("%s\n",*PatternList);
	 AddFile(Arc,*PatternList);
      }
      CloseOutStream();

      break;

   case 'C' : /* Create */

      if (*PatternList == NULL) Usage();

      OpenOutStream(ArchiveName);
      SendUInt8('M');
      SendUInt8('A');
      SendUInt8('r');
      SendUInt8('0'+MAR_VERSION);
      Arc = OutStream;
      for (; *PatternList != NULL; PatternList++) {
	 printf("%s\n",*PatternList);
	 AddFile(Arc,*PatternList);
      }
      CloseOutStream();

      break;

   case 'D' : /* Delete */

      DeleteArchive(ArchiveName,PatternList);
      break;

   case 'L' : /* List */

      ListArchive(ArchiveName,PatternList);
      break;

   case 'T' : /* Test */

      TestArchive(ArchiveName,PatternList);
      break;

   case 'X' : /* Extract */

      ExtractArchive(ArchiveName,PatternList);
      break;

   default :

      Usage();
      break;
   }

   return EXIT_SUCCESS;
}

/* Usage() */

static void Usage(void) {

   fprintf(stderr,"Usage: %s [<options>] <command> <archive> [<files>]\n",Program);
   fprintf(stderr,"       <command>   = (a)dd | (c)reate | (d)elete | (l)ist | (t)est | e(x)tract all\n");
   fprintf(stderr,"       <option>    = -a <algorithm> | -g | -o <order> | -t [<delta>] | -v [<level>]\n");
   fprintf(stderr,"       <algorithm> = store | lzh | bwt | ppm\n");

   exit(EXIT_FAILURE);
}

/* DeleteArchive() */

static void DeleteArchive(const char *ArchiveName, const char **PatternList) {

   archive *Archive;
   FILE *NewArc;
   void *Block;
   char NewArcName[1024];

   assert(ArchiveName!=NULL);

   Archive = OpenArchive(ArchiveName);
   if (Archive == NULL) {
      fprintf(stderr,"missing or corrupt archive \"%s\"",ArchiveName);
      exit(EXIT_FAILURE);
   }

   sprintf(NewArcName,"%s.tmp",ArchiveName);
   NewArc = fopen(NewArcName,"wb");

   fputc('M',NewArc);
   fputc('A',NewArc);
   fputc('r',NewArc);
   fputc('0'+MAR_VERSION,NewArc);

   while (! Archive->End) {

      if (MatchList(Archive->Header->FileName,PatternList)) {

	 printf("%s\n",Archive->Header->FileName);

      } else {

	 Block = Nalloc(Archive->Header->HeaderSize,"Header");
	 fread(Block,1,Archive->Header->HeaderSize,Archive->Stream->File);
	 fwrite(Block,1,Archive->Header->HeaderSize,NewArc);
	 Free(Block);

	 Block = Nalloc(Archive->Header->ArcSize,"File");
	 fread(Block,1,Archive->Header->ArcSize,Archive->Stream->File);
	 fwrite(Block,1,Archive->Header->ArcSize,NewArc);
	 Free(Block);
      }

      NextFile(Archive);
   }

   CloseArchive(Archive);

   rename(NewArcName,ArchiveName);
}

/* ListArchive() */

static void ListArchive(const char *ArchiveName, const char **PatternList) {

   archive *Archive;
   double N, U, C, UT, CT, Ratio, Bpb, RatioTot, BpbTot;

   assert(ArchiveName!=NULL);

   Archive = OpenArchive(ArchiveName);
   if (Archive == NULL) {
      fprintf(stderr,"missing or corrupt archive \"%s\"",ArchiveName);
      exit(EXIT_FAILURE);
   }

   N  = 0.0;

   UT = 0.0;
   CT = 0.0;

   RatioTot = 0.0;
   BpbTot   = 0.0;

   printf("  Size   Crunched Algo  D Ratio  BPB    CRC          Name      \n");
   printf("-------- -------- ----- - ----- ----- -------- ----------------\n");

   for (; ! Archive->End; NextFile(Archive)) {

      if (MatchList(Archive->Header->FileName,PatternList)) {

	 U = (double) Archive->Header->FileSize;
	 C = (double) Archive->Header->ArcSize;

	 N++;

	 UT += U;
	 CT += C;

	 Ratio = 100.0 * (U - C) / U;
	 Bpb   = 8.0 * C / U;

	 RatioTot += Ratio;
	 BpbTot   += Bpb;

	 printf("%8.0f %8.0f %-5.5s %1d %4.1f%% %5.3f %08X %s\n",U,C,AlgorithmName(Archive->Header->Algorithm),Archive->Header->Delta,Ratio,Bpb,Archive->Header->FileCRC,Archive->Header->FileName);
      }
   }

   CloseArchive(Archive);

   if (N != 0.0) {

      printf("-------- -------- ----- - ----- ----- -------- ----------------\n");

      U = UT / N;
      C = CT / N;

      Ratio = RatioTot / N;
      Bpb   = BpbTot   / N;

      printf("%8.0f %8.0f %-5.5s %-1.1s %4.1f%% %5.3f %-8.8s %s\n",U,C,"-----","-",Ratio,Bpb,"--------","mean");

      U = UT;
      C = CT;

      Ratio = 100.0 * (U - C) / U;
      Bpb   = 8.0 * C / U;

      printf("%8.0f %8.0f %-5.5s %-1.1s %4.1f%% %5.3f %-8.8s %s\n",U,C,"-----","-",Ratio,Bpb,"--------","total");
   }

   printf("%.0f file(s)\n",N);
}

/* TestArchive() */

static void TestArchive(const char *ArchiveName, const char **PatternList) {

   archive *Archive;

   assert(ArchiveName!=NULL);

   Archive = OpenArchive(ArchiveName);
   if (Archive == NULL) {
      fprintf(stderr,"missing or corrupt archive \"%s\"",ArchiveName);
      exit(EXIT_FAILURE);
   }

   for (; ! Archive->End; NextFile(Archive)) {
      if (MatchList(Archive->Header->FileName,PatternList)) {
	 printf("%s\n",Archive->Header->FileName);
	 TestFile(Archive);
      }
   }

   CloseArchive(Archive);
}

/* ExtractArchive() */

static void ExtractArchive(const char *ArchiveName, const char **PatternList) {

   archive *Archive;

   assert(ArchiveName!=NULL);

   Archive = OpenArchive(ArchiveName);
   if (Archive == NULL) {
      fprintf(stderr,"missing or corrupt archive \"%s\"",ArchiveName);
      exit(EXIT_FAILURE);
   }

   while (! Archive->End) {
      if (MatchList(Archive->Header->FileName,PatternList)) {
	 printf("%s\n",Archive->Header->FileName);
	 SaveFile(Archive);
      }
      NextFile(Archive);
   }

   CloseArchive(Archive);
}

/* TestFile() */

static void TestFile(archive *Archive) {

   void *Block;

   assert(Archive!=NULL);
   assert(!Archive->End);

   Block = Nalloc(Archive->Header->FileSize,Archive->Header->FileName);

   ReadFile(Archive,Block);

   Free(Block);
}

/* SaveFile() */

static void SaveFile(archive *Archive) {

   void *Block;
   FILE *File;

   assert(Archive!=NULL);
   assert(!Archive->End);

   Block = Nalloc(Archive->Header->FileSize,Archive->Header->FileName);

   ReadFile(Archive,Block);

   File = fopen(Archive->Header->FileName,"wb");
   if (File == NULL) FatalError("Couldn't open file \"%s\" for writing",Archive->Header->FileName);

   fwrite(Block,1,Archive->Header->FileSize,File);
   fclose(File);

   Free(Block);
}

/* AddFile() */

static void AddFile(stream *Arc, const char *FileName) {

   header  Header[1];
   FILE   *File;
   int     FileNameSize, FileSize;
   void   *Block;
   int     HeaderPos, FilePos, EndPos;

   FileNameSize = strlen(FileName);

   File = fopen(FileName,"rb");
   if (File == NULL) {
      printf("couldn't open file \"%s\", skipping",FileName);
      return;
   }

   if (fseek(File,0,SEEK_END) != 0) {
      printf("not a plain file \"%s\", skipping",FileName);
      return;
   }

   FileSize = ftell(File);
   rewind(File);

   Header->HeaderSize = 0;
   Header->ArcSize = 0;
   Header->FileNameSize = FileNameSize;
   strcpy(Header->FileName,FileName);
   Header->FileSize = FileSize;
   Header->Algorithm = Algorithm;
   Header->Delta = Delta;
   Header->FileCRC = 0;
   Header->HeaderCRC = 0;

   HeaderPos = ftell(Arc->File);
   if (HeaderPos == -1) Error("ftell()");
   if (ferror(Arc->File)) Error("ferror()");

   SendHeader(Arc,Header);

   FilePos = ftell(Arc->File);
   if (FilePos == -1) Error("ftell()");
   if (ferror(Arc->File)) Error("ferror()");

   Header->HeaderSize = FilePos - HeaderPos;

   Block = Nalloc(FileSize,FileName);
   fread(Block,1,FileSize,File);
   fclose(File);

   Header->FileCRC = CRC(Block,FileSize);

   if (Algorithm == ALGO_STORE) {

      SendBlock(Block,FileSize);

   } else {

      if (Header->Delta != 0) {
	 Delta = Header->Delta;
	 CodeDelta(Block,FileSize);
      }

      OpenBitStream(Arc);

      S = Block;
      N = FileSize; 
      CrunchBlock();

      CloseBitStream(Arc);
   }

   Free(Block);

   EndPos = ftell(Arc->File);
   if (EndPos == -1) Error("ftell()");
   if (ferror(Arc->File)) Error("ferror()");

   Header->ArcSize = EndPos - FilePos;

   if (fseek(Arc->File,HeaderPos,SEEK_SET) != 0) Error("fseek()");
   if (ferror(Arc->File)) Error("ferror()");

   HeaderPos = ftell(Arc->File);
   if (HeaderPos == -1) Error("ftell()");
   if (ferror(Arc->File)) Error("ferror()");

   SendHeader(Arc,Header);

   if (fseek(Arc->File,EndPos,SEEK_SET) != 0) Error("fseek()");
   if (ferror(Arc->File)) Error("ferror()");
}

/* Match() */

static int Match(const char *String, const char *Pattern) {

   switch (*Pattern) {
   case '\0' :
      return *String == '\0';
      break;
   case '?' :
      return *String != '\0' && Match(String+1,Pattern+1);
      break;
   case '*' :
      do Pattern++; while (*Pattern == '*');
      do if (Match(String,Pattern)) return TRUE; while (*String++ != '\0');
      return FALSE;
      break;
   default :
      return *String == *Pattern && Match(String+1,Pattern+1);
      break;
   }
}

/* MatchList() */

static int MatchList(const char *String, const char **PatternList) {

   const char *Pattern;

   if (PatternList == NULL || *PatternList == NULL) return TRUE;

   for (; (Pattern = *PatternList) != NULL; PatternList++) {
      if (Match(String,Pattern)) return TRUE;
   }

   return FALSE;
}

/* End of MAr.C */

