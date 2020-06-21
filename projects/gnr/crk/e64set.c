/* (c) Copyright 1999-2013, The Rockefeller University *11115* */
/* $Id: e64set.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*    e64set, e64push, e64pop, e64act, e64dec, e64dac, and e64test      *
*                                                                      *
*  Subroutine e64set presets the action to be taken when a routine     *
*  calls e64act(), usually because an arithmetic overflow error has    *
*  been detected.  e64push() does the same but saves the previous      *
*  settings on a push-down stack.  e64pop() restores the previous      *
*  settings in the reverse of the order stored.  The stack holds       *
*  up to E64_STKDPTH (currently 5) entries.                            *
*                                                                      *
*  V2 allows the e64act() call to specify which type of action is      *
*  desired, presuming the necessary pointer has been stored by a       *
*  previous call to e64set().  Thus, each call to e64set() stores      *
*  a pointer and also changes the default action for future calls      *
*  to e64act() for compatibility with V1, where always the action      *
*  specified by the most recent e64set() was performed.                *
*                                                                      *
*  V3 further allows the user to store a default 'ec' error code with  *
*  a call to e64dec().  Then when a user or crk routine calls e64dac() *
*  instead of e64act(), the stored error code is used.  The idea of    *
*  this is just to save passing the 'ec' code through large numbers    *
*  of arithmetic calls when it will only rarely be needed.  There is   *
*  a whole family of crk routines with names ending in 'd' instead of  *
*  'e', that use this route to report overflows.                       *
*                                                                      *
*  The crk 64-bit arithmetic routines with error checking typically    *
*  detect overflows on addition, subtraction, complementation, trun-   *
*  cation, and left-shifting.  All divide-by-zero errors terminate     *
*  the application immediately via abexit.                             *
*                                                                      *
*  Synopses: void e64set(unsigned int act, void *p)                    *
*            void e64push(unsigned int act, void *p)                   *
*            void e64pop(void)                                         *
*                                                                      *
*  Arguments:                                                          *
*     act   is a code defining the default action to be taken when     *
*           e64act() is called with the default code.  It is one of:   *
*           E64_ABEXIT   0    Terminate with abexit 69 (default)       *
*                             ('p' is ignored with this code).         *
*           E64_COUNT    1    Count the errors in long word at p[ec].  *
*           E64_FLAGS    2    Set bit (1<<ec) in long word at p.       *
*           E64_CALL     3    Call user-written routine                *
*                             void (*p)(char *fn-name, int ec).        *
*           where 'ec' is an error code provided by the caller of      *
*           the original arithmetic routine that finds the error.      *
*     p     is a pointer to an unsigned long or user-written routine   *
*           as specified by the 'act' argument.  The user is           *
*           responsible for seeing to it that sufficient storage is    *
*           allocated at p to hold the specified counts or flags.      *
*                                                                      *
*  Subroutine e64act() is called to report an overflow error.  It      *
*  can specify which 'act' action is to be taken, or that the last     *
*  e64set() or e64push() action (the current default) should be        *
*  taken.  e64dac() is the same as e64act() except the 'ec' argument   *
*  is omitted and instead the value stored by the most recent e64dec() *
*  call is used (default 0).                                           *
*                                                                      *
*  Synopsis: void e64act(void *fnm, int ec)                            *
*            void e64dec(int ec)                                       *
*            void e64dac(void *fnm)                                    *
*                                                                      *
*  Argument:                                                           *
*     fnm   is a literal string that gives the name of the calling     *
*           routine when the action is E64_ABEXIT.  When the action    *
*           is E64_CALL, 'fnm' can point to any information the user   *
*           wants to pass to the preset routine.  This argument is     *
*           not used when the action is E64_COUNT or E64_FLAGS.        *
*     ec    is an error code supplied by the caller of the routine     *
*           that finds the error.  If just an integer ('ec' < 2^24)    *
*           is coded, the default action specified by the most recent  *
*           call to e64set() or e64push() is performed with argument   *
*           'ec'.  Alternatively, one of the following macros may be   *
*           used to specify the action and the code in one int:        *
*                                                                      *
*  Action/code macros:                                                 *
*     EAabx(ec)   Perform E64_ABEXIT action with code 'ec'.            *
*     EAct(ec)    Perform E64_COUNT action with code 'ec'.             *
*     EAfl(ec)    Perform E64_FLAGS action with code 'ec'.             *
*     EAcb(ec)    Perform E64_CALL action with code 'ec'.              *
*                                                                      *
*  Subroutine e64test() can be called to force calls to abexitq() or   *
*  abexitmq() to return rather than exit.  These calls are used in     *
*  arithmentic routines that normally exit on certain errors (e.g.     *
*  divide by 0) but need to return when called from a test.            *
*                                                                      *
*  Synopsis: void e64stest(int i_am_test)                              *
*                                                                      *
*  Argument:                                                           *
*     i_am_test   is TRUE to force abexit[m]q() to return, FALSE       *
*           for normal behavior.  Of course any code that calls        *
*           abexit[m]q() must be prepared to handle a return.          *
*                                                                      *
*  Subroutine e64qtest() is used to query the current state of the     *
*  i_am_test switch.                                                   *
************************************************************************
*  V1A, 06/26/99, GNR - New routine                                    *
*  ==>, 05/10/08, GNR - Last date before committing to svn repository  *
*  V2A, 12/11/12, GNR - Add ability of e64act() to specify action      *
*  Rev, 06/09/13, GNR - Add e64stest(), e64qtest() calls               *
*  V3A, 10/21/13, GNR - Add e64dec, e64dac, preset error codes         *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "sysdef.h"
#include "bapkg.h"
#include "rkarith.h"
#include "rksubs.h"

/* Structure that saves e64set action--user may save/restore */
struct RKX_t {
   void *pcfr[E64_CALL];   /* Ptrs to count, flags, or routine */
   unsigned int  act;      /* Current default action code */
   };
struct {                   /* Global storage for e64set parms */
   struct RKX_t RKXi[E64_STKDPTH];
   int decec;              /* Default ec stored by e64dec() */
   int iestk;              /* Current position on stack */
   int itest;              /* State of i_am_test switch */
   } RKX;

void e64set(unsigned int act, void *p) {

   if (act > E64_CALL) abexit(10);
   if (act > 0) RKX.RKXi[RKX.iestk].pcfr[act-1] = p;
   RKX.RKXi[RKX.iestk].act = act;

   } /* End e64set() */

void e64push(unsigned int act, void *p) {

   if (++RKX.iestk >= E64_STKDPTH || act > E64_CALL) abexit(10);
   if (act > 0) RKX.RKXi[RKX.iestk].pcfr[act-1] = p;
   RKX.RKXi[RKX.iestk].act = act;

   } /* End e64push() */

void e64pop(void) {

   if (RKX.iestk > 0) RKX.iestk -= 1;

   } /* End e64pop() */

void e64dec(int ec) {

   RKX.decec = ec;

   } /* End e64dec() */

void e64act(void *fnm, unsigned int ec) {

   struct RKX_t *prkx = RKX.RKXi + RKX.iestk;
   unsigned int ia = ec >> EAshft;
   ec &= ((1 << EAshft) - 1);
   ia = (ia == 0) ? prkx->act : ia - 1;  /* Use default action */
   switch (ia) {

   case E64_COUNT:
      ++((long *)prkx->pcfr[E64_COUNT-1])[ec];
      break;
   case E64_FLAGS:
      *(long *)prkx->pcfr[E64_FLAGS-1] |= 1L << ec;
      break;
   case E64_CALL:
      ((void (*)(void *,int))prkx->pcfr[E64_CALL-1])(fnm, ec);
      break;
   /* N.B.  This case placed last because abexitm is declared
   *  "no_return" so compiler removes the "break" which causes
   *  failure if another case is below this one.  */
   case E64_ABEXIT:
      abexitm(69, ssprintf(NULL, "Error %d in %s.", ec, (char *)fnm));
      break;               /* (Avoid fall-through warning) */
      }

   } /* End e64act() */

void e64dac(void *fnm) {

   e64act(fnm, RKX.decec);

   } /* End e64dac() */

void e64stest(int i_am_test) {

   RKX.itest = i_am_test;

   } /* End e64stest() */

int e64qtest(void) {

   return RKX.itest;

   } /* End e64qtest() */
