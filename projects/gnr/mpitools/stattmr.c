/* (c) Copyright 1992-2016, The Rockefeller University *21115* */
/* $Id: stattmr.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                              stattmr.c                               *
*                                                                      *
*  Data structure and routines that create a timer facility, for       *
*  analysis of parallel computer runs.  This is the MPI version.       *
*  It is assumed here that if timing is not wanted, the application    *
*  will leave the macro TIMERS undefined before including mpitools.h   *
*  That will convert all calls to stattmr() into empty code and we     *
*  can leave statmr() in all versions of the mpitools library.         *
************************************************************************
*  V1A, 05/14/92, ABP - Initial version.                               *
*  Rev, 11/02/92, ABP - Add SUN4 code                                  *
*  Rev, 11/16/96, GNR - Change SUN4 to !PARn, printf() to dbgprt()     *
*  Rev, 12/31/96, GNR - Use ROCKS second() routine instead of time()   *
*  Rev, 03/29/98, GNR - Use ssprintf() instead of sprintf()            *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 06/30/16, GNR - Changes for MPI, remove ICC, D1 stuff          *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#define TIMERS
#include "sysdef.h"
#include "mpitools.h"
#include "rksubs.h"

double second(void);             /* From ROCKS library */

#define ERRTOOPOP 0x01
#define ERRSTKFUL 0x02
#define ERRBADRES 0x04
#define ERRNOSTOP 0x08
#define ERRBADOPC 0x10

#define STKSIZE  64

/* Structure for a timer */
typedef struct {
   double strticks;  /* Ticks when clock started */
   double culticks;  /* Accumulated ticks */
   int   onnow;      /* Set when timer is running */
   } stattimer;

/* Timer names, for printing */
char *tnames[NUMOFTIMERS] =
                 {"Graphics timer:.......................",
                  "Number Crunching timer:...............",
                  "Inter node comm. (excl. d1exch) timer:",
                  "Graph data file timer:................",
                  "Savenet file timer:...................",
                  "TV plot timer:........................",
                  "Default/overhead timer:..............."};

/* Array of timers, size defined in header */
stattimer stimers[NUMOFTIMERS];

int tmrstk[STKSIZE];  /* Timer stack */

/* The stack pointer is initialized to -2 to indicate that timers
*  have not been initialized yet.  Initializing, thru a call
*  with opcode RESET_ALL, sets the stack pointer to -1, the normal
*  empty stack indication.  All calls prior to the first RESET_ALL
*  are ignored. */
int stkptr = -2;    /* Timer stack pointer */

static int errmsk = 0;  /* Error mask */

/* InSample is a flag whose value is set to 0 or 1 upon the first call
*  to stattmr().  It determines whether the node is in the sampling
*  group, i.e. if timing information will be gathered.  The argument
*  to that first call (a RESET operation) determines how many nodes
*  will be in the sample group.  */

static int InSample = 1;

/*---------------------------------------------------------------------*
*                            stop_timer()                              *
*                                                                      *
*  Subtract the starting time for the current interval from the        *
*  current time and add the difference into the time accumulator.      *
*  Then mark the timer as not running now.                             *
*---------------------------------------------------------------------*/

void stop_timer(int id) {

   if (stimers[id].onnow) {
      stimers[id].culticks += (second() - stimers[id].strticks);
      stimers[id].onnow = OFF;
      }
   return;
   } /* End stop_timer() */

/*---------------------------------------------------------------------*
*                            start_timer()                             *
*                                                                      *
*  Store the starting time value in the timer data structure and       *
*  mark the timer as running.                                          *
*---------------------------------------------------------------------*/

void start_timer(int id) {

   stimers[id].strticks = second();
   stimers[id].onnow = ON;
   return;
   }

/*---------------------------------------------------------------------*
*                              stattmr()                               *
*                                                                      *
*  General control routine for timer facility                          *
*---------------------------------------------------------------------*/

void stattmr(int opcode, int tmarg) {

   int i;

   /* First call must be with opcode RESET, otherwise ignored.
   *  Following calls ignored if node not in sample group.  */

   if (!InSample  ||
       (stkptr == -2 && opcode != OP_RESET_ALL)) {

      /* Even if we are not part of sample group, must synch
      *  when others print results */
      if (opcode == OP_PRINT_ALL) {
         if (NC.dim >= 0) isynch();
         sleep(5);
         }
      return;
      }

   switch(opcode) {

      case OP_RESET_ALL:

/* In this command, the second parameter is interpreted
*  as the number of nodes, in addition nodes 0,1,N-1,
*  which will be in the sample group. */

         stkptr = -1;
         for (i=0 ; i<NUMOFTIMERS ; i++) {
            stimers[i].culticks = 0.0;
            stimers[i].onnow = OFF;
            }

         InSample = (is_host(NC.node)) ? TRUE :
            (NC.node == NC.headnode || NC.node == NC.tailnode);

         if (tmarg <= 0) {
            errmsk |= ERRBADRES;
            break; }

         if (!is_host(NC.node) &&  /* Add up to tmarg interior nodes */
               NC.node > NC.headnode && NC.node < NC.tailnode) {
            int myrnode = rnode(NC.node);
            InSample |= (myrnode % ((NC.cnodes-3)/(tmarg+1))) == 0;
            }
         break;

      case OP_POP_TMR:

         if (stkptr == -1) {
            errmsk |= ERRTOOPOP;
            break; }
         stop_timer(tmrstk[stkptr--]);
         if (stkptr > -1) start_timer(tmrstk[stkptr]);
         break;

      case OP_PUSH_TMR:

         if (stkptr == STKSIZE-1) {
            errmsk |= ERRSTKFUL;
            break; }
         if (stkptr > -1) stop_timer(tmrstk[stkptr]);
         tmrstk[++stkptr] = tmarg;
         start_timer(tmarg);
         break;

      case OP_STOP_ALL:

         for (i=stkptr; i>=0; i--)
            stop_timer(tmrstk[i]);
         stkptr = -1;
         break;

      case OP_PRINT_ALL:

         if (stkptr > -1) errmsk |= ERRNOSTOP;
         if (errmsk) {
            /* Node number will be added by dbgprt */
            dbgprt( ssprintf(NULL,
               "stattmr errors detected, mask = 0x%x", errmsk) );
            dbgprt("Results may be invalid.");
            }

         dbgprt("\n");
         for (i=0; i<NUMOFTIMERS; i++) {
            /* Laboriously convert times to integers so we can
            *  use the parsimonious ssprintf output routine. */
            long cuts = (long)stimers[i].culticks;
            long cutu = (long)(1000000.0 *
               (stimers[i].culticks - (double)cuts));
            if (is_host(NC.node))
               dbgprt(ssprintf(NULL, "HOST: %s total time=%ld sec. +"
                  " %ld microsec.", tnames[i], cuts, cutu) );
            else
               dbgprt(ssprintf(NULL, "Node %d: %s total time=%ld sec. "
                  "+ %ld microsec.", NC.node, tnames[i], cuts, cutu) );
            }

         if (NC.dim >= 0) isynch();
         sleep(5);

         /* Return to initial state, to wait for another RESET */
         stkptr = -2;
         InSample = 1;
         break;

      default:
         errmsk |= ERRBADOPC;
      } /* End switch */
   } /* End stattmr() */

