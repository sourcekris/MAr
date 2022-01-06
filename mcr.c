
/* MCr.C */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mcr.h"
#include "types.h"
#include "algo.h"

/* Constants */

enum { MODE_CRUNCH, MODE_DECRUNCH };

/* Variables */

static char *Program;

/* Prototypes */

static void Usage (void);

/* Functions */

/* Main() */

int main(int argc, char *argv[]) {

   char *C, *Program;
   int Mode;

   Program     = *argv;

   Mode        = MODE_CRUNCH;
   Algorithm   = ALGO_LZH;

   Source      = NULL;
   Destination = NULL;

   Delta       = 0;     /* Delta */
   Group       = FALSE; /* Huffman tree grouping */
   Order       = 3;     /* PPM Order */
   Verbosity   = 0;

   /* Options */

   for (argv++; *argv != NULL && (*argv)[0] == '-'; argv++) {
      switch ((*argv)[1]) {
      case 'a' : /* Algorithm */
         argv++;
         if (*argv != NULL) {
	    for (C = *argv; *C != '\0'; C++) *C = toupper(*C);
	    Algorithm = AlgorithmNo(*argv);
         }
         break;
      case 'd' : /* Decrunch */
         Mode = MODE_DECRUNCH;
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
      }
   }

   /* Source */

   if (*argv != NULL) {
      Source = *argv;
      argv++;
   }

   /* Destination */

   if (*argv != NULL) {
      Destination = *argv;
      argv++;
   }

   /* No more arguments => ok */

   if (*argv != NULL) Usage();

   /* Let's go ! */

   if (Verbosity >= 1) {

      fprintf(stderr,MCR_NAME "\n");

      switch (Mode) {
      case MODE_CRUNCH :
         fprintf(stderr,"Crunching");
         break;
      case MODE_DECRUNCH :
         fprintf(stderr,"Decrunching");
         break;
      }

      if (Source != NULL) {
         fprintf(stderr," %s",Source);
      } else {
         fprintf(stderr," %s","<stdin>");
      }

      if (Destination != NULL) {
         fprintf(stderr," to %s",Destination);
      } else {
         fprintf(stderr," to %s","<stdout>");
      }

      if (Mode == MODE_CRUNCH) {
	 fprintf(stderr," using %s algorithm",AlgorithmName(Algorithm));
      }

      fprintf(stderr,"\n");
   }

   switch (Mode) {
   case MODE_CRUNCH :
      CrunchFile();
      break;
   case MODE_DECRUNCH :
      DecrunchFile();
      break;
   }

   return EXIT_SUCCESS;
}

/* Usage() */

static void Usage(void) {
 
   fprintf(stderr,"Usage: %s [<options>] [<source> [<destination>]]\n",Program);
   fprintf(stderr,"       <option> = -a <algo> | -d | -g | -o <order> | -t [<delta>] | -v [<level>]\n");

   exit(EXIT_FAILURE);
}

/* End of MCr.C */

