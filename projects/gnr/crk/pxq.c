/* (c) Copyright 1989-2010, The Rockefeller University *11115* */
/* $Id: pxq.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                        PXON, PXQ, and PXOFF                          *
*                                                                      *
*        These routines are part of the C-language implementation      *
*        of the ROCKS interface routines.  They provide a standard     *
*        method for a user to interrupt an executing application.      *
*        The interruption here is accomplished with Ctrl-C.            *
*                                                                      *
************************************************************************
*  V1A, 03/18/89, GNR - Convert from FORTRAN-compatible version        *
*  Rev, 04/03/89, GNR - Modify to make compilable on NCUBE             *
*  Rev, 04/19/89, GNR - Implement revised system definitions           *
*  ==>, 08/18/07, GNR - Last date before committing to svn repository  *
*  Rev, 10/04/10, NCH - Ported from DOS to POSIX:                      *
*  ...  Uses Ctrl-C/SIGINT rather than PAUSE or CTRL/BRK               *
***********************************************************************/
/*
         To install the Ctrl-C handler:
               int pxon(void);
         The value returned is 0 if the interactive interface was
               successfully established, otherwise -1.
         After this call has been completed, the terminal user may
               at any time enter Ctrl-C, at which time a 'PX>' prompt
               will be displayed.  Any command acceptable to the
               application may be entered, and will be passed to the
               application at the next pxq call.
         If Ctrl-C is hit again at the prompt, or if the command string
               is 'quit' or 'hx', the program is terminated at once.

         To determine whether a Ctrl-C command has been issued:
               int pxq(char *string, int maxstr)
         'string' is a pointer to a string which will receive the
               command string if one has been entered.  If no command
               has been entered, the first character of the string is
               set to an end-of-string character.  The string should
               be long enough to contain the longest command line that
               the operating system permits (min CDSIZE characters).
         'maxstr' gives the number of characters (not including the
               end-of-string marker) that will fit in 'string'.  pxq
               will truncate the input command, if necessary, to fit
               in 'string'.
         The value returned is -1 if no Ctrl-C command has been
               issued or if the pxon call failed.  Otherwise, pxq
               returns the number of characters in the command string,
               not including the end-of-string character.  If this
               value is larger than 'maxstr', the command has been
               truncated.  The pending command state is cleared so
               that one command from the terminal can only give one
               positive pxq return.


         To cancel an existing Ctrl-C command linkage:
               void pxoff(void)
         After this call is completed, Ctrl-C will no longer be a
               recognized immediate command.  The command is also
               automatically cancelled when the program terminates,
               normally or abnormally.  pxoff performs no operation
               if the pxon call failed.

*/


#include <signal.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "sysdef.h"           /* System definitions */

#ifdef IBM370
#error "*** Use PXQ ASSEMBLE for IBM 370 version ***"
#endif

#include "rocks.h"            /* Be sure prototype gets checked */

/* Define static storage. PXS.flag2 is zero if pxon has not been called,
   otherwise it is the maximum cmd length.
   This nonzero value serves as a PX-loaded flag2. */

static struct {                 /* Reduce number of externs */
   volatile sig_atomic_t ttyio; /* TRUE if we can do prompt/reply */
   volatile sig_atomic_t flag1; /* TRUE if Ctl-C was hit; else FALSE */
   volatile byte flag2;         /* CDSIZE+1 if handler installed */
   char cmd[CDSIZE+1];          /* Command string */
   pthread_t iothrd;            /* Pthreads ID# for pxthread() */
   } PXS = {TRUE, FALSE, 0};    /* Set up for uninstalled interface */

void pxgo(int sig);

static struct sigaction
   onhndl = {.sa_handler = pxgo,},
   ofhndl = {.sa_handler = SIG_DFL};

/*--------------------------------------------------------------------*
*                                PXGO                                 *
*--------------------------------------------------------------------*/

/* Here is the actual code executed during the interrupt. One must be
   careful not to use any nonreentrant library functions that might
   have been executing when the interruption occurred. */

void pxgo(int sig) {
   PXS.flag1 = TRUE;
   }

/*--------------------------------------------------------------------*
*                                PXIO                                 *
*--------------------------------------------------------------------*/

/* pxio tells when it is ok to do terminal I/O; it should be called
   with FALSE as argument when a batch of statistical output is about
   to be performed, and called again with TRUE when this is done. */

void pxio(byte doio) {
   PXS.ttyio = doio;
   }

/*--------------------------------------------------------------------*
*                              PXTHREAD                               *
*--------------------------------------------------------------------*/

/* A separate thread created to do I/O because it canot be done in a
   signal handler ('nothing' is required by the prototype
   and is not needed here; neither is the return value.) */

void *pxthread(void *nothing) {

   while (TRUE) { /* loop until terminated by pxoff() */
      if (PXS.flag1 && PXS.ttyio) {
       /*sigaction(SIGINT,&ofhndl,NULL); /* Exit if Ctl-C hit twice */
         fwrite("\nPX>",4,sizeof(char),stdout); /* Give a prompt */
         fflush(stdout);
         getc(stdin); /* remove Ctl-C character from stream */
         if (fgets(PXS.cmd,CDSIZE,stdin)==(char *)NULL) {
            int myerr = errno;
            printf("Unable to read line! Errno = %d\n", myerr);
            fflush(stdout);
            }
         else {
            fwrite("fgets returned nonzero\n",23,1,stdout);
            fflush(stdout);
            PXS.flag1 = FALSE;
          /*sigaction(SIGINT,&onhndl,NULL); /* Restore signal handling */

            char *smatch_args[2] = {"QUIT","HX"};
            if (smatch(RK_NMERR, PXS.cmd, smatch_args, 2))
               abexit(666);  /* Exit if a quit command was entered. */
            }
         }
      if (!PXS.flag2) return NULL;
      PXS.flag1 = FALSE;
      rksleep(0,100000); /* pause for 1/10 sec, then iterate again */
      }
   }

/*--------------------------------------------------------------------*
*                                PXQ                                  *
*--------------------------------------------------------------------*/

int pxq(char *string, int maxstr) {

   if (*(PXS.cmd)!='\0') {
      strncpy(string, PXS.cmd, maxstr);
      PXS.flag1 = FALSE;
      PXS.cmd[0] = '\0';
      return strlen(string);
      }
   else return -1;
   } /* End pxq */

/*--------------------------------------------------------------------*
*                                PXON                                 *
*--------------------------------------------------------------------*/

int pxon(void) {

   /* Attempt to install SIGINT handler if it is not already */
   onhndl.sa_flags = 0x10000000;
   if (!PXS.flag2 && sigaction(SIGINT,&onhndl,NULL)>=0
         && pthread_create(&PXS.iothrd, NULL, pxthread, NULL)==0)
      PXS.flag2 = CDSIZE;
   else return -1;   /* Failure! */

   return 0;
   } /* End pxon */

/*--------------------------------------------------------------------*
*                                PXOFF                                *
*--------------------------------------------------------------------*/

void pxoff(void) {

   /* Perform default action on SIGINT */
   sigaction(SIGINT,&ofhndl,NULL);
   PXS.flag2 = 0;
   PXS.cmd[0] = '\0';

   } /* End pxoff */
