
/* Debug.C */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "debug.h"
#include "types.h"

/* Constants */

#define STRING_SIZE 256

/* Functions */

/* Malloc() */

void *Malloc(int Size) {

   void *Address;

   assert(Size>=0);

   if (Size == 0) Warning("Malloc(0)");

   Address = malloc(Size);
   if (Address == NULL) FatalError("Malloc(%d): malloc() = NULL",Size);

   return Address;
}

/* Nalloc() */

void *Nalloc(int Size, const char *Name) {

   assert(Name!=NULL);

   return Malloc(Size);
}

/* Free() */

void Free(void *Address) {

   assert(Address!=NULL);

   free(Address);
}

/* Warning() */

void Warning(const char *Message, ...) {

   va_list Args;
   char    String[STRING_SIZE];

   va_start(Args,Message);
   vsprintf(String,Message,Args);
   va_end(Args);

   fprintf(stderr,"WARNING : %s...\n",String);
}

/* Error() */

void Error(const char *Message, ...) {

   va_list Args;
   char    String[STRING_SIZE];

   va_start(Args,Message);
   vsprintf(String,Message,Args);
   va_end(Args);

   fprintf(stderr,"ERROR : %s !\n",String);
}

/* FatalError() */

void FatalError(const char *Message, ...) {

   va_list Args;
   char    String[STRING_SIZE];

   va_start(Args,Message);
   vsprintf(String,Message,Args);
   va_end(Args);

   fprintf(stderr,"FATAL ERROR : %s !!!\n",String);

   exit(EXIT_FAILURE);
}

/* End of Debug.C */

