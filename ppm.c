
/* PPM.C */

#include <stdio.h>
#include <stdlib.h>

#include "ppm.h"
#include "types.h"
#include "algo.h"
#include "ari.h"
#include "bitio.h"
#include "debug.h"

/* Constants */

#define ORDER_BIT 4
#define ORDER_MAX 15

/* Types */

typedef struct node node;

struct node {
   short  Char;
   short  Freq;
   short  SymTot;
   short  SymEsc;
   node  *Son;
   node  *Brother;
};

/* Variables */

static node *Root;
static int   Memory;

/* Prototypes */

static node *AddString  (node *Node, const uchar String[], int Size);
static node *StringNode (node *Node, const uchar String[], int Size);
static node *CharNode   (node *Node, int Char);

static node *NewNode    (void);

/* Functions */

/* CodePPM() */

void CodePPM(void) {

   int I, C, O, SymLow, SymHigh, SymTot, SymEsc;
   int Excluded[256];
   node *Node, *Context[ORDER_MAX+1], *Char, **Pred;

   if (Order < 0) {
      Order = 0;
   } else if (Order > ORDER_MAX) {
      Order = ORDER_MAX;
   }
   SendBits(ORDER_BIT,Order);

   Memory = 0;

   SendStart();

   Root = NewNode();

   Context[0] = Root;
   for (O = 1; O <= Order; O++) Context[O] = NULL;

   for (I = 0; I < N; I++) {

      for (C = 0; C < 256; C++) Excluded[C] = FALSE;

      for (O = Order; O >= 0; O--) {

         if (O <= I) {

            if (Context[O]->Son != NULL) {

               SymLow  = 0;
               SymHigh = 0;
               SymTot  = 0;
               SymEsc  = 0;

               for (Node = Context[O]->Son; Node != NULL; Node = Node->Brother) {
                  if (Node->Freq != 0) {
                     if (! Excluded[Node->Char]) {
                        Excluded[Node->Char] = TRUE;
                        if (Node->Char == S[I]) {
                           SymLow  = SymTot;
                           SymHigh = SymTot + Node->Freq;
                        }
                        SymTot += Node->Freq;
                     }
                     SymEsc++;
                  }
               }

               if (SymHigh != 0) {
                  SendAriRange(SymLow,SymHigh,SymTot+SymEsc);
                  break;
               } else {
                  SendAriRange(SymTot,SymTot+SymEsc,SymTot+SymEsc);
               }
            }
         }
      }

      if (O == -1) {

         SymLow  = 0;
         SymHigh = 0;
         SymTot  = 0;

         for (C = 0; C < 256; C++) {
            if (! Excluded[C]) {
               if (C == S[I]) {
                  SymLow  = SymTot;
                  SymHigh = SymTot + 1;
               }
               SymTot++;
            }
         }

         SendAriRange(SymLow,SymHigh,SymTot);
      }

      for (O = Order; O >= 0; O--) {

         if (O <= I) {

            SymTot = Context[O]->SymTot;
            SymEsc = Context[O]->SymEsc;

            C = S[I];

            Pred = &Context[O]->Son;
            for (Char = *Pred; Char != NULL; Char = *Pred) {
               if (Char->Char == C) break;
               Pred = &Char->Brother;
            }

            if (Char == NULL) {

               Char            = NewNode();
               Char->Char      = C;
               Char->Brother   = Context[O]->Son;
               Context[O]->Son = Char;

               SymTot++;
               SymEsc++;

            } else {

               if (Char != Context[O]->Son) {
                  *Pred           = Char->Brother;
                  Char->Brother   = Context[O]->Son;
                  Context[O]->Son = Char;
               }
            }

            Char->Freq++;
            SymTot++;

            if (SymTot >= 4096) {
               SymTot = 0;
               SymEsc = 0;
               for (Node = Context[O]->Son; Node != NULL; Node = Node->Brother) {
                  Node->Freq >>= 1;
                  if (Node->Freq != 0) {
                     SymTot += Node->Freq;
                     SymEsc++;
                  }
               }
            }

            Context[O]->SymTot = SymTot;
            Context[O]->SymEsc = SymEsc;

            if (O < Order) Context[O+1] = Char;
         }
      }
   }

   SendEnd();

   if (Verbosity >= 2) fprintf(stderr,"%d bytes allocated\n",Memory);
}

/* DecodePPM() */

void DecodePPM(void) {

   int I, C, O, SymLow, SymHigh, SymTot, SymEsc, SymCode;
   int Excluded[256];
   node *Node, *Context[ORDER_MAX+1], *Char, **Pred;

   Order = GetBits(ORDER_BIT);

   Memory = 0;

   GetStart();

   Root = NewNode();

   Context[0] = Root;
   for (O = 1; O <= Order; O++) Context[O] = NULL;

   for (I = 0; I < N; I++) {

      for (C = 0; C < 256; C++) Excluded[C] = FALSE;

      for (O = Order; O >= 0; O--) {

         if (O <= I) {

            if (Context[O]->Son != NULL) {

               SymTot = 0;
               SymEsc = 0;

               for (Node = Context[O]->Son; Node != NULL; Node = Node->Brother) {
                  if (Node->Freq != 0) {
                     if (! Excluded[Node->Char]) SymTot += Node->Freq;
                     SymEsc++;
                  }
               }

               SymCode = GetAriRange(SymTot+SymEsc);

               if (SymCode >= SymTot) {

                  for (Node = Context[O]->Son; Node != NULL; Node = Node->Brother) {
                     if (Node->Freq != 0) Excluded[Node->Char] = TRUE;
                  }
                  SkipAriRange(SymTot,SymTot+SymEsc,SymTot+SymEsc);

               } else {

                  SymLow  = 0;
                  SymHigh = 0;
                  for (Node = Context[O]->Son; Node != NULL; Node = Node->Brother) {
                     if (Node->Freq != 0 && ! Excluded[Node->Char]) {
                        SymHigh += Node->Freq;
                        if (SymHigh > SymCode) {
                           SymLow = SymHigh - Node->Freq;
                           break;
                        }
                     }
                  }
                  SkipAriRange(SymLow,SymHigh,SymTot+SymEsc);

                  S[I] = Node->Char;
                  break;
               }
            }
         }
      }

      if (O == -1) {

         SymTot = 0;
         for (C = 0; C < 256; C++) {
            if (! Excluded[C]) SymTot++;
         }

         SymCode = GetAriRange(SymTot);

         SymLow  = 0;
         SymHigh = 0;
         for (C = 0; C < 256; C++) {
            if (! Excluded[C]) {
               SymHigh++;
               if (SymHigh > SymCode) {
                  SymLow = SymHigh - 1;
                  break;
               }
            }
         }
         SkipAriRange(SymLow,SymHigh,SymTot);

         S[I] = C;
      }

      for (O = Order; O >= 0; O--) {

         if (O <= I) {

            SymTot = Context[O]->SymTot;
            SymEsc = Context[O]->SymEsc;

            C = S[I];

            Pred = &Context[O]->Son;
            for (Char = *Pred; Char != NULL; Char = *Pred) {
               if (Char->Char == C) break;
               Pred = &Char->Brother;
            }

            if (Char == NULL) {

               Char            = NewNode();
               Char->Char      = C;
               Char->Brother   = Context[O]->Son;
               Context[O]->Son = Char;

               SymTot++;
               SymEsc++;

            } else {

               if (Char != Context[O]->Son) {
                  *Pred           = Char->Brother;
                  Char->Brother   = Context[O]->Son;
                  Context[O]->Son = Char;
               }
            }

            Char->Freq++;
            SymTot++;

            if (SymTot >= 4096) {
               SymTot = 0;
               SymEsc = 0;
               for (Node = Context[O]->Son; Node != NULL; Node = Node->Brother) {
                  Node->Freq >>= 1;
                  if (Node->Freq != 0) {
                     SymTot += Node->Freq;
                     SymEsc++;
                  }
               }
            }

            Context[O]->SymTot = SymTot;
            Context[O]->SymEsc = SymEsc;

            if (O < Order) Context[O+1] = Char;
         }
      }
   }

   GetEnd();

   if (Verbosity >= 2) fprintf(stderr,"%d bytes allocated\n",Memory);
}

/* AddString() */

static node *AddString(node *Node, const uchar String[], int Size) {

   int I, C;
   node *Father, *Older;

   for (I = 0; I < Size; I++) {

      C = String[I];

      Father = Node;
      Node   = Node->Son;

      if (Node == NULL) {
         Node = NewNode();
         Node->Char  = C;
         Father->Son = Node;
      }

      while (Node->Char != C) {

         Older = Node;
         Node  = Node->Brother;

         if (Node == NULL) {
            Node = NewNode();
            Node->Char     = C;
            Older->Brother = Node;
         }
      }
   }

   return Node;
}

/* StringNode() */

static node *StringNode(node *Node, const uchar String[], int Size) {

   int I;

   for (I = 0; Node != NULL && I < Size; I++) {
      for (Node = Node->Son; Node != NULL && Node->Char != String[I]; Node = Node->Brother)
         ;
   }

   return Node;
}

/* CharNode() */

static node *CharNode(node *Node, int Char) {

   for (Node = Node->Son; Node != NULL && Node->Char != Char; Node = Node->Brother)
      ;

   return Node;
}

/* NewNode() */

static node *NewNode(void) {

   node *Node;

   Node = malloc(sizeof(node));
   if (Node == NULL) FatalError("NewNode(): Not enough memory");

   Memory += sizeof(node);

   Node->Char    = '\0';
   Node->Freq    = 0;
   Node->SymTot  = 0;
   Node->SymEsc  = 0;
   Node->Son     = NULL;
   Node->Brother = NULL;

   return Node;
}

/* End of PPM.C */

