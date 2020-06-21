/***********************************************************************
*                              memtest.c                               *
*                                                                      *
*  This is the test program for the new shared memory allocation       *
*  library.  Similarly to CNS, it may be compiled into parallel and    *
*  serial versions, and both should be run to test the memory system.  *
*                                                                      *
*  Program can be run with command-line argument x to generate error   *
*  number x, where x is one of (31, 32, 33, 35, 36, 37, 38, 39).       *
*  Error 34 will show up as missing function in node library unless    *
*  makefile is incorrectly changed to compile allocp fns on comp       *
*  nodes.                                                              *
*                                                                      *
*  A second command-line argument, if present, will be passed to       *
*  aninit() as a debug parameter (use decimal value, enter first       *
*  argument as 0 for debug output without forced error).               *
*                                                                      *
*  Error codes in the range 360-379 are assigned to this program.      *
*                                                                      *
*  V1A, 09/11/99, GNR - New program                                    *
*  V2A, 07/01/01, GNR - Add chngpool tests                             *
*  V3A, 07/01/16, GNR - Modify to run in MPI environment               *
*  V3B, 09/02/18, GNR - Replace anread/anwrite w/blkbcst in errsynch   *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sysdef.h"
#ifdef UNIX
#include <signal.h>
#endif
#include "rocks.h"
#include "mpitools.h"
#include "memshare.h"
#include "mempools.h"
#include "memtest.h"
#include "mtnxdr.h"
#include "swap.h"

/* Global error count */
long nerr = 0;

/*=====================================================================*
*                         AUXILIARY ROUTINES                           *
*=====================================================================*/

#ifdef PAR
/* Convert union mtu3.  If the parent pointer points to the
*  union itself, we have a bare mtu3 rather than one embedded
*  in an mttsu.  In this case, always assume case 3.  */
void NXFuni_mtu3_u(struct NNSTR *pnn, void **ppobj, void *parent,
      int flags) {

   if (parent == *ppobj)
      nncom(pnn, ppobj, ttloc(IXuni_mtu3_3), flags);
   else switch (((mttsu *)parent)->tu) {
   case 1:
      nncom(pnn, ppobj, ttloc(IXuni_mtu3_1), flags);
      break;
   case 2:
      nncom(pnn, ppobj, ttloc(IXuni_mtu3_2), flags);
      break;
   case 3:
      nncom(pnn, ppobj, ttloc(IXuni_mtu3_3), flags);
      break;
      }
   } /* End NXFuni_mtu3_u() */

/*--------------------------------------------------------------------*/

/* Synchronize nodes on error count */
void static errsynch(int iex) {

   char ack[FMLSIZE];

   nerr = isum(nerr);

#ifdef PAR0
   lemfmi4(ack, nerr);
#endif
   blkbcst(ack, NULL, FMLSIZE, MEMTEST_ACK);
#ifdef PARn
   nerr = lemtoi4(ack);
   if (nerr > 0)
      exit(iex);
#endif
   } /* End errsynch() */

/*--------------------------------------------------------------------*/

/* Send a pointer as an intptr to a node */
#define putptr(p) ipbuf = (intptr)p, \
   nnput(&bcstr, &ipbuf, FMPSIZE)

/* Receive a host pointer at a node and translate it.
*  N.B.  This only works AFTER the membcst for that ptr!  */
#define getptr(str) (nnget(&str, &ipbuf, FMPSIZE), \
   xlatptr(&ipbuf))
#endif /* PAR */

/*--------------------------------------------------------------------*/

/* Check a character */
void static ckchr(char cval, char corr, char *id) {
   if (cval != corr) {
      dbgprt(ssprintf(NULL, "Value %1s reported for %s, "
         "correct is %1s", &cval, id, &corr));
      nerr += 1;
      }
   } /* End ckchr() */

/*--------------------------------------------------------------------*/

/* Check an integer */
void static ckint(int cval, int corr, char *id) {
   if (cval != corr) {
      dbgprt(ssprintf(NULL, "Value %d reported for %s, "
         "correct is %d", cval, id, corr));
      nerr += 1;
      }
   } /* End ckint() */

/*--------------------------------------------------------------------*/

/* Check a long */
void static cklng(long cval, long corr, char *id) {
   if (cval != corr) {
      dbgprt(ssprintf(NULL, "Value %ld reported for %s, "
         "correct is %ld", cval, id, corr));
      nerr += 1;
      }
   } /* End cklng() */

/*--------------------------------------------------------------------*/

/* Check a double */
void static ckdbl(double cval, double corr, char *id) {
   if (cval != corr) {
      dbgprt(ssprintf(NULL, "Value %x%x reported for %s, "
         "correct is %x%x", cval, id, corr));
      nerr += 1;
      }
   } /* End ckdbl() */

/*--------------------------------------------------------------------*/

/* Check a pointer */
void static ckptr(void *cval, void *corr, char *id) {
   if (cval != corr) {
      dbgprt(ssprintf(NULL, "Value %p reported for %s, "
         "correct is %p", cval, id, corr));
      nerr += 1;
      }
   } /* End ckptr() */

/*--------------------------------------------------------------------*/

/* Check a string */
void static ckstr(char *cval, char *corr, char *id) {
   if (strcmp(cval,corr) != 0) {
      dbgprt(ssprintf(NULL, "Value %s reported for %s, "
         "correct is %s", cval, id, corr));
      nerr += 1;
      }
   } /* End ckstr() */

/*--------------------------------------------------------------------*/

/* Check and return a block count */
static int ckblkct(memtype kmem, char *id) {
   struct mbbdlink *p,*p0;
   int mca,mcr,mct;        /* Actual, recorded, total mem blk counts */

   mca = 0;
   mcr = (int)MI.mchain[kmem].base.nlblks;
   p0 = &MI.mchain[kmem].base.link;
   p = &p0->pnmbd->link;
   if (p) for ( ; p!=p0; p=&p->pnmbd->link) ++mca;
   if (mca != mcr) {
      dbgprt(ssprintf(NULL, "%d base blocks recorded for %s, "
         "%d found", mcr, id, mca));
      nerr += 1;
      }
   mct = mca;

#ifdef PAR0
   mca = 0;
   mcr = (int)MI.mchain[kmem].mods.nlblks;
   p0 = &MI.mchain[kmem].mods.link;
   p = &p0->pnmbd->link;
   if (p) for ( ; p!=p0; p=&p->pnmbd->link) ++mca;
   if (mca != mcr) {
      dbgprt(ssprintf(NULL, "%d mods blocks recorded for %s, "
         "%d found", mcr, id, mca));
      nerr += 1;
      }
   mct += mca;
#endif
   return (long)mct;
   } /* End ckblkct() */

/*=====================================================================*
*                            MEMTEST MAIN                              *
*=====================================================================*/

int main(int argc, char *argv[]) {

   struct mblkdef *pa;  /* For snooping in control info */
   struct mts1 *p01,*p11,*p21,*p41,*pIII1,*pVII1;
   struct mts2 *p02,*p12,*p22,*p42,*pIII2,*pchg;
   union  mtu3 *p03;
   mttsu       *p13,*p23,*p43;
   char        *p3c1,*p3c2,*p3c3;
#ifndef PARn
   struct mts1 *p51;
   struct mts2 *p52;
   mttsu       *p53;
   char        *p6c1,*p6c2,*p6c3;
#endif
   void        *pfblk;
   size_t lc1=49,lc2=37,lc3=24;
   long   rghtlen[Npools],replen;
   long   oldlen1,oldlen2,midlen1,midlen2,newlen1,newlen2;
   long   tlen,tlen2,tlen3,tlen4;
   int    ie;        /* Code to generate a deliberate error */
   int    ip,kp;
   int    oldnb1,oldnb2,midnb1,midnb2,newnb1,newnb2;
#ifdef PAR
   struct NNSTR bcstr;
   intptr ipbuf;
#ifdef PAR0
   int        dbgarg;  /* First argument to aninit */
#endif
#endif /* PAR */                
   static char *mpnm[] = {
      "Static", "Dynamic", "Shared", "Private", "Host" };

/* Interpret command-line arguments */
   ie = (argc > 1) ? atoi(argv[1]) : 0;

/* Initialize node-node communications if parallel */

#ifdef PAR
#ifdef PARn
   aninit(ANI_DBMSG, 0, 0);
   dbgprt(ssprintf(NULL, "Node %d returned from aninit", NC.node));
#endif
#ifdef PAR0
   dbgarg = (argc > 2) ? atoi(argv[2]) : 0;
   aninit(ANI_DBMSG, (ui32)dbgarg, 0);
   dbgprt("Host returned from aninit");
#endif
#else  /* Serial */
   /* On serial machines, a negative coprocessor dimension will
   *  allow dual-mode library routines to tell what is going on.  */
   memset(&NC, 0, sizeof(struct NCINFO));
   NC.debug = argc > 2 ? atoi(argv[2]) : 0;
   NC.dim = -1;
#endif /* PAR */
   nxtupush(NXDRTT, NXDRUT);

/* Ignore signals that may be issued to this process unintentionally.
*  These include all signals that are generated from the keyboard,
*  which are always sent to all the processes in the current process
*  group, and the notorious SIGWINCH, which is sent by OpenWindows
*  whenever the window status changes.  */

#ifdef UNIX
   signal(SIGINT, SIG_IGN);
   signal(SIGTSTP, SIG_IGN);
   signal(SIGQUIT, SIG_IGN);
   signal(SIGWINCH, SIG_IGN);
#endif /* UNIX */


/*---------------------------------------------------------------------*
*  Stage I.  Allocate some memory blocks in each of the 5 pools        *
*  in some arbitrary order.  Put some data in each right away          *
*  to check for overwriting by later alloc calls.                      *
*---------------------------------------------------------------------*/

#ifndef PARn
   settit("      Parallel computer memory management test program");
   if (ie == 32) {
#if 1
      dbgprt("Do not try error 32 on a modern machine.");
#else
      /* Keep allocating until something gives */
      while (1) allocpmv(Host, 0x7fff, "Gen err 32");
#endif
      }
   else if (ie == 35)
      allocpmv(4095, IXstr_mts1, "Gen err 35");

   dbgprt("Entering Stage I setup");

   p01 = (struct mts1 *)allocpmv(Static, IXstr_mts1, "p01");
      p01->s1i1 = 11;
      p01->s1d1 = 11.1;
      p01->s1d2 = 11.2;
      p01->s1c1 = 'A';
   p02 = (struct mts2 *)allocpcv(Static, 2, IXstr_mts2, "p02");
      p02->s2c1 = 'B';
      p02->s2l1 = 258903411;
      p02->s2c2 = 'b';
      (p02+1)->s2c1 = 'C';
      (p02+1)->s2l1 = 258903412;
      (p02+1)->s2c2 = 'c';

   p11 = (struct mts1 *)allocpmv(Dynamic, IXstr_mts1, "p11");
      p11->s1i1 = 21;
      p11->s1d1 = 21.1;
      p11->s1d2 = 21.2;
      p11->s1c1 = 'D';

   if (ie == 38) {   /* Create a bad magic number */
      pa = (struct mblkdef *)((char *)p11 - LMBDUP);
      pa->info.magic = 847; }

   p12 = (struct mts2 *)allocpcv(Dynamic, 3, IXstr_mts2, "p12");
      p12->s2c1 = 'E';
      p12->s2l1 = 347826711;
      p12->s2c2 = 'e';
      (p12+1)->s2c1 = 'F';
      (p12+1)->s2l1 = 347826712;
      (p12+1)->s2c2 = 'f';
      (p12+2)->s2c1 = 'G';
      (p12+2)->s2l1 = 347826713;
      (p12+2)->s2c2 = 'g';
   p22 = (struct mts2 *)allocpcv(Shared, 1, IXstr_mts2, "p22");
      p22->s2c1 = 'I';
      p22->s2l1 = 482082432;
      p22->s2c2 = 'i';
   p23 = (mttsu *)allocpmv(Shared, IXmttsu, "p23");
      p23->tu = 2;
      p23->suu1.us2.s2c1 = 'J';
      p23->suu1.us2.s2l1 = 77856203;
      p23->suu1.us2.s2c2 = 'j';

   p3c1 = allocpmv(Shared, lc1, "p3c1");
      strcpy(p3c1,"Test string of 48+1 characters.23456789012345678");
   p3c3 = allocpmv(Shared, lc3, "p3c3");
      strcpy(p3c3,"Test string of 23+1 chs");

   /* Private and Host pools are not shared, just stick one datum in
   *  each to see that it they are NOT broadcast.  Addresses should
   *  exist on comp nodes for Private data, but not the others.  */
   p41 = (struct mts1 *)allocpmv(Private, IXstr_mts1, "p41");
      p41->s1i1 = 41;
   p42 = (struct mts2 *)allocpcv(Private, 2, IXstr_mts2, "p42");
   p43 = (mttsu *)allocpmv(Private, IXmttsu, "p43");
      p43->tu = 3;

   p51 = (struct mts1 *)allocpmv(Host, IXstr_mts1, "p51");
      p51->s1i1 = 52;
   p52 = (struct mts2 *)allocpcv(Host, 2, IXstr_mts2, "p52");
   p53 = (mttsu *)allocpmv(Host, IXmttsu, "p53");
      p53->tu = 3;

   p6c1 = allocpmv(Host, lc1, "p6c1");
      strcpy(p6c1,"Test string for host data");
   p6c2 = allocpcv(Host, 2, lc2, "p6c2");
      p6c2[0] = '\0';
   p6c3 = allocpmv(Host, lc3, "p6c3");
      p6c3[0] = '\0';

   p13 = (mttsu *)allocpmv(Dynamic, IXmttsu, "p13");
      p13->tu = 1;
      p13->suu1.us1.s1i1 = 311;
      p13->suu1.us1.s1d1 = 311.1;
      p13->suu1.us1.s1d2 = 311.2;
      p13->suu1.us1.s1c1 = 'H';
   p3c2 = allocpcv(Shared, 2, lc2, "p3c2");
      strcpy(p3c2,"Test string of 36+1 characters.23456");
      strcpy(p3c2+37,"ZYXabcdefghijklmnopqrstuvwxyz0123456");
   p21 = (struct mts1 *)allocpcv(Shared, 1, IXstr_mts1, "p21");
      p21->s1i1 = 31;
      p21->s1d1 = 31.1;
      p21->s1d2 = 31.2;
      p21->s1c1 = 'H';
   p03 = ( union mtu3 *)allocpmv(Static, IXuni_mtu3, "p03");
      p03->us3[0] = 30;
      p03->us3[4] = 34;

/* Link the test blocks together with some various pointers.
*  Mix it up.  */

   p01->pfs1 = p11;
   p01->pns2 = p02;
   p02->pns1 = p01;
   (p02+1)->pns1 = p41;
   p11->pfs1 = p01;
   p11->pns2 = p12;
   p12->pns1 = p01;
   (p12+1)->pns1 = p21;
   (p12+2)->pns1 = p11;
   p13->suu1.us1.pfs1 = p11;
   p13->suu1.us1.pns2 = p02;
   p21->pfs1 = NULL;
   p21->pns2 = p02;
   p22->pns1 = p41;
   p23->suu1.us2.pns1 = p01;
   p42->pns1 = p01;
   p52->pns1 = p11;
#endif /* !PARn */

   dbgprt("Entering Stage I membcst");
   pfblk = membcst(MPS_Static|MPS_Dynamic|MPS_Shared);

/* See that the correct length is reported for each pool */

   rghtlen[Static] = ALIGN_UP(sizeof(struct mts1)) +
      2*ALIGN_UP(sizeof(struct mts2)) +
        ALIGN_UP(sizeof(union mtu3)) + 3*LMBDUP;
   rghtlen[Dynamic] = ALIGN_UP(sizeof(struct mts1)) +
      3*ALIGN_UP(sizeof(struct mts2)) +
        ALIGN_UP(sizeof(mttsu)) + 3*LMBDUP;
   rghtlen[Shared] = ALIGN_UP(sizeof(struct mts1)) +
        ALIGN_UP(sizeof(struct mts2)) +
        ALIGN_UP(sizeof(mttsu)) + ALIGN_UP(lc1) +
        ALIGN_UP(2*lc2) + ALIGN_UP(lc3) + 6*LMBDUP;
   rghtlen[Private] = ALIGN_UP(sizeof(struct mts1)) +
      2*ALIGN_UP(sizeof(struct mts2)) +
        ALIGN_UP(sizeof(mttsu)) + 3*LMBDUP;
#ifdef PARn
   rghtlen[Host] = 0;
#else
   rghtlen[Host] = ALIGN_UP(sizeof(struct mts1)) +
      2*ALIGN_UP(sizeof(struct mts2)) +
        ALIGN_UP(sizeof(mttsu)) + ALIGN_UP(lc1) +
        ALIGN_UP(2*lc2) + ALIGN_UP(lc3) + 6*LMBDUP;
#endif

   for (ip=0,kp=1; ip<Npools; ++ip,kp+=kp) {
      replen = memsize(kp);
      if (replen != rghtlen[ip]) {
         dbgprt(ssprintf(NULL, "Length %ld reported for %s, "
            "correct is %ld", replen, mpnm[ip], rghtlen[ip]));
         nerr += 1;
         }
      }

#ifdef PAR
   errsynch(1);
#endif

#ifndef PARn
   if (nerr > 0)
      abexitm(361, "Test halted due to errors in Stage I");
   else
      cryout(RK_P1, "0Allocations and length tests OK.",
         RK_LN2+RK_FLUSH, NULL);
#endif

/*---------------------------------------------------------------------*
*  Stage II.  Check that the right data got broadcast to all           *
*  the nodes.  In the serial case, this is just a check that           *
*  data didn't get written over along the way.  If parallel,           *
*  must transmit the pointers separately for checking.                 *
*---------------------------------------------------------------------*/

#ifdef PAR
#ifdef PAR0
dbgprt("Entering Stage II, sending correct data");
   nnpcr(&bcstr, BCSTADDR, MEMTEST_PTRS);
   nnputi4(&bcstr, (long)ie);
   putptr(p01);
   putptr(p02);
   putptr(p03);
   putptr(p11);
   putptr(p12);
   putptr(p13);
   putptr(p21);
   putptr(p22);
   putptr(p23);
   putptr(p3c1);
   putptr(p3c2);
   putptr(p3c3);
   putptr(p41);
   putptr(p42);
   putptr(p43);
   nnpfl(&bcstr);
#else
dbgprt("Entering Stage II, receiving correct data");
   nngcr(&bcstr, BCSTADDR, MEMTEST_PTRS);
   ie = (int)nngeti4(&bcstr);
   p01 = (struct mts1 *)getptr(bcstr);
   p02 = (struct mts2 *)getptr(bcstr);
   p03 = ( union mtu3 *)getptr(bcstr);
   p11 = (struct mts1 *)getptr(bcstr);
   p12 = (struct mts2 *)getptr(bcstr);
   p13 =       (mttsu *)getptr(bcstr);
   p21 = (struct mts1 *)getptr(bcstr);
   p22 = (struct mts2 *)getptr(bcstr);
   p23 =       (mttsu *)getptr(bcstr);
   p3c1 =       (char *)getptr(bcstr);
   p3c2 =       (char *)getptr(bcstr);
   p3c3 =       (char *)getptr(bcstr);
   p41 = (struct mts1 *)getptr(bcstr);
   p42 = (struct mts2 *)getptr(bcstr);
   p43 =       (mttsu *)getptr(bcstr);
#endif
#endif /* PAR */

   if (pfblk != p01) {
      dbgprt(ssprintf(NULL, "Address %p reported for first block, "
         "correct is %p", pfblk, p01));
      nerr += 1;
      }

   ckint(p01->s1i1, 11, "p01->s1i1");
   ckdbl(p01->s1d1, 11.1, "p01->s1d1");
   ckptr(p01->pfs1, p11, "p01->pfs1");
   ckptr(p01->pns2, p02, "p01->pns2");
   ckdbl(p01->s1d2, 11.2, "p01->s1d2");
   ckchr(p01->s1c1, 'A', "p01->s1c1");

   ckchr(p02->s2c1, 'B', "p02->s2c1");
   cklng(p02->s2l1, 258903411, "p02->s2l1");
   ckptr(p02->pns1, p01, "p02->pns1");
   ckchr(p02->s2c2, 'b', "p02->s2c2");
   ckchr((p02+1)->s2c1, 'C', "(p02+1)->s2c1");
   cklng((p02+1)->s2l1, 258903412, "(p02+1)->s2l1");
   ckptr((p02+1)->pns1, p41, "(p02+1)->pns1");
   ckchr((p02+1)->s2c2, 'c', "(p02+1)->s2c2");

   cklng(p03->us3[0], 30, "p03->us3[0]");
   cklng(p03->us3[4], 34, "p03->us3[4]");

   ckint(p11->s1i1, 21, "p11->s1i1");
   ckdbl(p11->s1d1, 21.1, "p11->s1d1");
   ckptr(p11->pfs1, p01, "p11->pfs1");
   ckptr(p11->pns2, p12, "p11->pns2");
   ckdbl(p11->s1d2, 21.2, "p11->s1d2");
   ckchr(p11->s1c1, 'D', "p11->s1c1");

   ckchr(p12->s2c1, 'E', "p12->s2c1");
   cklng(p12->s2l1, 347826711, "p12->s2l1");
   ckptr(p12->pns1, p01, "p12->pns1");
   ckchr(p12->s2c2, 'e', "p12->s2c2");
   ckchr((p12+1)->s2c1, 'F', "(p12+1)->s2c1");
   cklng((p12+1)->s2l1, 347826712, "(p12+1)->s2l1");
   ckptr((p12+1)->pns1, p21, "(p12+1)->pns1");
   ckchr((p12+1)->s2c2, 'f', "(p12+1)->s2c2");
   ckchr((p12+2)->s2c1, 'G', "(p12+2)->s2c1");
   cklng((p12+2)->s2l1, 347826713, "(p12+2)->s2l1");
   ckptr((p12+2)->pns1, p11, "(p12+2)->pns1");
   ckchr((p12+2)->s2c2, 'g', "(p12+2)->s2c2");

   ckint(p13->tu, 1, "p13->tu");
   ckint(p13->suu1.us1.s1i1, 311, "p13->suu1.us1.s1i1");
   ckdbl(p13->suu1.us1.s1d1, 311.1, "p13->suu1.us1.s1d1");
   ckptr(p13->suu1.us1.pfs1, p11, "p13->suu1.us1.pfs1");
   ckptr(p13->suu1.us1.pns2, p02, "p13->suu1.us1.pns2");
   ckdbl(p13->suu1.us1.s1d2, 311.2, "p13->suu1.us1.s1d2");
   ckchr(p13->suu1.us1.s1c1, 'H', "p13->suu1.us1.s1c1");

   ckint(p21->s1i1, 31, "p21->s1i1");
   ckdbl(p21->s1d1, 31.1, "p21->s1d1");
   ckptr(p21->pfs1, NULL, "p21->pfs1");
   ckptr(p21->pns2, p02, "p21->pns2");
   ckdbl(p21->s1d2, 31.2, "p21->s1d2");
   ckchr(p21->s1c1, 'H', "p21->s1c1");

   ckchr(p22->s2c1, 'I', "p22->s2c1");
   cklng(p22->s2l1, 482082432, "p22->s2l1");
   ckptr(p22->pns1, p41, "p22->pns1");
   ckchr(p22->s2c2, 'i', "p22->s2c2");

   ckint(p23->tu, 2, "p23->tu");
   ckchr(p23->suu1.us2.s2c1, 'J', "p23->suu1.us2.s2c1");
   cklng(p23->suu1.us2.s2l1, 77856203, "p23->suu1.us2.s2l1");
   ckptr(p23->suu1.us2.pns1, p01, "p23->suu1.us2.pns1");
   ckchr(p23->suu1.us2.s2c2, 'j', "p23->suu1.us2.s2c2");

   ckstr(p3c1, "Test string of 48+1 characters.23456789012345678", "p3c1");
   ckstr(p3c2, "Test string of 36+1 characters.23456", "p3c2");
   ckstr(p3c2+37, "ZYXabcdefghijklmnopqrstuvwxyz0123456", "p3c2+37");
   ckstr(p3c3, "Test string of 23+1 chs", "p3c3");

   /* Following should not be correct on nodes */
#ifndef PARn
   ckint(p41->s1i1, 41, "p41->s1i1");
   ckptr(p42->pns1, p01, "p42->pns1");
   ckint(p43->tu, 3, "p43->tu");
   ckint(p51->s1i1, 52, "p51->s1i1");
   ckptr(p52->pns1, p11, "p52->pns1");
   ckint(p53->tu, 3, "p53->tu");
   ckstr(p6c1, "Test string for host data", "p6c1");
   ckchr(p6c2[0], 0, "p6c2");
   ckchr(p6c3[0], 0, "p6c3");
#endif /* !PARn */

#ifdef PAR
   errsynch(2);
#endif

#ifndef PARn
   if (nerr > 0)
      abexitm(362, "Test halted due to errors in Stage II");
   else
      cryout(RK_P1, "0Contents broadcast to all nodes OK.",
         RK_LN2+RK_FLUSH, NULL);

/*---------------------------------------------------------------------*
*  Stage III.  Add a couple of new blocks and see if everything        *
*  tracks.  Additions to Static data should cause an error.            *
*---------------------------------------------------------------------*/

   if (ie == 36)
      allocpmv(Static, IXstr_mts1, "Gen err 36");

   pIII1 = p01->pfs1 = allocpmv(Dynamic, IXstr_mts1, "pIII1");
   pIII2 = p01->pns2 = allocpmv(Shared, IXstr_mts2, "pIII2");

   pIII1->s1i1 = 107;
   pIII1->pfs1 = p11;
   pIII1->pns2 = p12;
   pIII2->s2l1 = 108;
   pIII2->pns1 = p11;

#endif /* !PARn */

   membcst(MPS_Static|MPS_Dynamic|MPS_Shared);

#ifdef PAR
#ifdef PAR0
   putptr(pIII1);
   putptr(pIII2);
   nnpfl(&bcstr);
#else
   pIII1 = (struct mts1 *)getptr(bcstr);
   pIII2 = (struct mts2 *)getptr(bcstr);
#endif
#endif /* PAR */

   ckptr(p01->pfs1, pIII1, "p01->pfs1==pIII1");
   ckptr(p01->pns2, pIII2, "p02->pns2==pIII2");
   ckint(pIII1->s1i1, 107, "pIII1->s1i1");
   ckptr(pIII1->pfs1, p11, "pIII1->pfs1");
   ckptr(pIII1->pns2, p12, "pIII1->pns2");
   cklng(pIII2->s2l1, 108, "pIII2->s2l1");
   ckptr(pIII2->pns1, p11, "pIII2->pns1");

#ifdef PAR
   errsynch(3);
#endif

#ifndef PARn
   if (nerr > 0)
      abexitm(363, "Test halted due to errors in Stage III");
   else
      cryout(RK_P1, "0New blocks and pointers added OK.",
         RK_LN2+RK_FLUSH, NULL);

/*---------------------------------------------------------------------*
*  Stage IV.  Expand one of the arrays with allocprv and               *
*  see whether the changes get propagated correctly.                   *
*  Make the array big enough to force a new origin.                    *
*---------------------------------------------------------------------*/

   pIII1->pns2 = p12 = (struct mts2 *)
         allocprv(p12, 1000, IXstr_mts2|MBT_Unint, "resize p12");
      (p12+999)->s2c1 = 'Z';
      (p12+999)->s2l1 = 643896203;
      (p12+999)->pns1 = p41;
      (p12+999)->s2c2 = 'z';
   /* If we don't update pointers to this block, should
   *  get error next time p11 block is broadcast.  */
   if (ie != 39)
      p11->pns2 = p12;

#endif /* !PARn */

   membcst(MPS_Dynamic);

#ifdef PAR
#ifdef PAR0
   putptr(p12);
   nnpfl(&bcstr);
#else
   p12 = (struct mts2 *)getptr(bcstr);
#endif
#endif /* PAR */

   ckchr(p12->s2c1, 'E', "p12->s2c1");
   cklng(p12->s2l1, 347826711, "p12->s2l1");
   ckptr(p12->pns1, p01, "p12->pns1");
   ckchr(p12->s2c2, 'e', "p12->s2c2");
   ckchr((p12+1)->s2c1, 'F', "(p12+1)->s2c1");
   cklng((p12+1)->s2l1, 347826712, "(p12+1)->s2l1");
   ckptr((p12+1)->pns1, p21, "(p12+1)->pns1");
   ckchr((p12+1)->s2c2, 'f', "(p12+1)->s2c2");
   ckchr((p12+2)->s2c1, 'G', "(p12+2)->s2c1");
   cklng((p12+2)->s2l1, 347826713, "(p12+2)->s2l1");
   ckptr((p12+2)->pns1, p11, "(p12+2)->pns1");
   ckchr((p12+2)->s2c2, 'g', "(p12+2)->s2c2");
   ckchr((p12+999)->s2c1, 'Z', "(p12+999)->s2c1");
   cklng((p12+999)->s2l1, 643896203, "(p12+999)->s2l1");
   ckptr((p12+999)->pns1, p41, "(p12+999)->pns1");
   ckchr((p12+999)->s2c2, 'z', "(p12+999)->s2c2");

   ckptr(pIII1->pns2, p12, "pIII1->pns2 rev.");
   if (ie != 39)
      ckptr(p11->pns2, p12, "p11->pns2 rev.");

#ifdef PAR
   errsynch(4);
#endif

#ifndef PARn
   if (nerr > 0)
      abexitm(364, "Test halted due to errors in Stage IV");
   else
      cryout(RK_P1, "0Block enlarged with allocprv OK.",
         RK_LN2+RK_FLUSH, NULL);

/*---------------------------------------------------------------------*
*  Stage V.  Use allocprv again to contract a block.  This time, the   *
*  pointers probably will not change--snoop in the control blocks to   *
*  see whether the change was performed correctly.                     *
*---------------------------------------------------------------------*/

   p11->pns2 = pIII1->pns2 = p12 = (struct mts2 *)
      allocprv(p12, 2, IXstr_mts2, "contract p12 block");

#endif /* !PARn */

   membcst(MPS_Dynamic);

#ifdef PAR
#ifdef PAR0
   putptr(p12);
   nnpfl(&bcstr);
#else
   p12 = (struct mts2 *)getptr(bcstr);
#endif
#endif /* PAR */

   pa = (struct mblkdef *)((char *)p12 - LMBDUP);

   ckchr(p12->s2c1, 'E', "p12->s2c1");
   cklng(p12->s2l1, 347826711, "p12->s2l1");
   ckptr(p12->pns1, p01, "p12->pns1");
   ckchr(p12->s2c2, 'e', "p12->s2c2");
   ckchr((p12+1)->s2c1, 'F', "(p12+1)->s2c1");
   cklng((p12+1)->s2l1, 347826712, "(p12+1)->s2l1");
   ckptr((p12+1)->pns1, p21, "(p12+1)->pns1");
   ckchr((p12+1)->s2c2, 'f', "(p12+1)->s2c2");

   ckptr(pIII1->pns2, p12, "pIII1->pns2 rev.");
   ckptr(p11->pns2, p12, "p11->pns2 rev.");

#ifndef PARn
   ip = pa->info.magic & MPMMask;
   kp = MPMAGIC;
   ckint(ip, kp, "magic number at contracted p12");
   ip = pa->info.magic & PoolMask;
   kp = (int)Dynamic;
   ckint(ip, kp, "pool type at contracted p12");
#endif
   ckint(pa->info.mbtype, IXstr_mts2, "mbtype at contracted p12");
#if ZSIZE == LSIZE
   cklng(pa->info.nmb, 2, "nmb at contracted p12");
#else
   ckint(pa->info.nmb, 2, "nmb at contracted p12");
#endif

#ifdef PAR
   errsynch(5);
#endif

#ifndef PARn
   if (nerr > 0)
      abexitm(365, "Test halted due to errors in Stage V");
   else
      cryout(RK_P1, "0Block contracted with allocprv OK.",
         RK_LN2+RK_FLUSH, NULL);

/*---------------------------------------------------------------------*
*  Stage VI.  Free a block and see if it disappears from the listing.  *
*---------------------------------------------------------------------*/

   if (ie == 31)  /* Create bad free call error */
      p12 = (struct mts2 *)64;
   freepv(p12, "p12");
   p11->pns2 = pIII1->pns2 = NULL;

   cryout(RK_P1,
      "0Next should follow dump of Dynamic region,", RK_LN2,
#ifdef PAR
      " first from host node, then from last PAR node,", RK_LN1,
#endif
      " (3 blocks in all--types 0x0, 0x18, 0x0):", RK_LN1+RK_FLUSH,
      NULL);

#endif /* !PARn */

   membcst(MPS_Dynamic);

#ifdef PARn
   if (NC.node == NC.tailnode)
#endif
      memdump(MPS_Dynamic);

/*---------------------------------------------------------------------*
*  Stage VII.  Change a couple of values in Shared data and broadcast. *
*  Use an undocumented bit in the the membcst() call to force a full   *
*  realloc as if memory had become fragmented, then check that all     *
*  values are still intact.  If ie == 37, force resend to fail.        *
*  Note that there must be an allocation change--the resend check      *
*  never happens if existing memory is just rebroadcast.               *
*---------------------------------------------------------------------*/

#ifndef PARn
   pVII1 = (struct mts1 *)allocpmv(Shared, IXstr_mts1, "pVII1");
      pVII1->s1i1 = 111;
      pVII1->s1d1 = 111.1;
      pVII1->pfs1 = p11;
      pVII1->pns2 = p22;
      pVII1->s1d2 = 111.2;
      pVII1->s1c1 = 'N';
   p21->s1i1 = 81;
   p21->s1d1 = 81.1;
   p21->s1d2 = 81.2;
   p21->s1c1 = 'K';
   p22->s2c1 = 'L';
   p22->s2l1 = 820824312;
   p22->s2c2 = 'l';
   p23->tu = 2;
   p23->suu1.us2.s2c1 = 'M';
   p23->suu1.us2.s2l1 = 17785203;
   p23->suu1.us2.s2c2 = 'm';
#endif

   membcst(MPS_Shared | ((ie == 37) ? (3<<Npools) : (1<<Npools)));

/* After this, all the pointers are likely to be different */
#ifdef PAR
#ifdef PAR0
   putptr(p01);
   putptr(p02);
   putptr(p03);
   putptr(p11);
   putptr(p13);
   putptr(p21);
   putptr(p22);
   putptr(p23);
   putptr(p3c1);
   putptr(p3c2);
   putptr(p3c3);
   putptr(p41);
   putptr(p42);
   putptr(p43);
   putptr(pIII1);
   putptr(pIII2);
   putptr(pVII1);
   nnpfl(&bcstr);
#else
   p01 =   (struct mts1 *)getptr(bcstr);
   p02 =   (struct mts2 *)getptr(bcstr);
   p03 =   ( union mtu3 *)getptr(bcstr);
   p11 =   (struct mts1 *)getptr(bcstr);
   p13 =         (mttsu *)getptr(bcstr);
   p21 =   (struct mts1 *)getptr(bcstr);
   p22 =   (struct mts2 *)getptr(bcstr);
   p23 =         (mttsu *)getptr(bcstr);
   p3c1 =         (char *)getptr(bcstr);
   p3c2 =         (char *)getptr(bcstr);
   p3c3 =         (char *)getptr(bcstr);
   p41 =   (struct mts1 *)getptr(bcstr);
   p42 =   (struct mts2 *)getptr(bcstr);
   p43 =         (mttsu *)getptr(bcstr);
   pIII1 = (struct mts1 *)getptr(bcstr);
   pIII2 = (struct mts2 *)getptr(bcstr);
   pVII1 = (struct mts1 *)getptr(bcstr);
#endif
#endif /* PAR */

   ckint(p01->s1i1, 11, "resent p01->s1i1");
   ckdbl(p01->s1d1, 11.1, "resent p01->s1d1");
   ckdbl(p01->s1d2, 11.2, "resent p01->s1d2");
   ckchr(p01->s1c1, 'A', "resent p01->s1c1");
   ckptr(p01->pfs1, pIII1, "resent p01->pfs1");
   ckptr(p01->pns2, pIII2, "resent p02->pns2");

   ckchr(p02->s2c1, 'B', "resent p02->s2c1");
   cklng(p02->s2l1, 258903411, "resent p02->s2l1");
   ckptr(p02->pns1, p01, "resent p02->pns1");
   ckchr(p02->s2c2, 'b', "resent p02->s2c2");
   ckchr((p02+1)->s2c1, 'C', "resent (p02+1)->s2c1");
   cklng((p02+1)->s2l1, 258903412, "resent (p02+1)->s2l1");
   ckptr((p02+1)->pns1, p41, "resent (p02+1)->pns1");
   ckchr((p02+1)->s2c2, 'c', "resent (p02+1)->s2c2");

   cklng(p03->us3[0], 30, "resent p03->us3[0]");
   cklng(p03->us3[4], 34, "resent p03->us3[4]");

   ckint(p11->s1i1, 21, "resent p11->s1i1");
   ckdbl(p11->s1d1, 21.1, "resent p11->s1d1");
   ckptr(p11->pfs1, p01, "resent p11->pfs1");
   ckptr(p11->pns2, NULL, "resent p11->pns2");
   ckdbl(p11->s1d2, 21.2, "resent p11->s1d2");
   ckchr(p11->s1c1, 'D', "resent p11->s1c1");

   ckint(p13->tu, 1, "resent p13->tu");
   ckint(p13->suu1.us1.s1i1, 311, "resent p13->suu1.us1.s1i1");
   ckdbl(p13->suu1.us1.s1d1, 311.1, "resent p13->suu1.us1.s1d1");
   ckptr(p13->suu1.us1.pfs1, p11, "resent p13->suu1.us1.pfs1");
   ckptr(p13->suu1.us1.pns2, p02, "resent p13->suu1.us1.pns2");
   ckdbl(p13->suu1.us1.s1d2, 311.2, "resent p13->suu1.us1.s1d2");
   ckchr(p13->suu1.us1.s1c1, 'H', "resent p13->suu1.us1.s1c1");

   ckint(p21->s1i1, 81, "resent p21->s1i1");
   ckdbl(p21->s1d1, 81.1, "resent p21->s1d1");
   ckptr(p21->pfs1, NULL, "resent p21->pfs1");
   ckptr(p21->pns2, p02, "resent p21->pns2");
   ckdbl(p21->s1d2, 81.2, "resent p21->s1d2");
   ckchr(p21->s1c1, 'K', "resent p21->s1c1");

   ckchr(p22->s2c1, 'L', "resent p22->s2c1");
   cklng(p22->s2l1, 820824312, "resent p22->s2l1");
   ckptr(p22->pns1, p41, "resent p22->pns1");
   ckchr(p22->s2c2, 'l', "resent p22->s2c2");

   ckint(p23->tu, 2, "resent p23->tu");
   ckchr(p23->suu1.us2.s2c1, 'M', "resent p23->suu1.us2.s2c1");
   cklng(p23->suu1.us2.s2l1, 17785203, "resent p23->suu1.us2.s2l1");
   ckptr(p23->suu1.us2.pns1, p01, "resent p23->suu1.us2.pns1");
   ckchr(p23->suu1.us2.s2c2, 'm', "resent p23->suu1.us2.s2c2");

   ckstr(p3c1, "Test string of 48+1 characters.23456789012345678", "resent p3c1");
   ckstr(p3c2, "Test string of 36+1 characters.23456", "resent p3c2");
   ckstr(p3c2+37, "ZYXabcdefghijklmnopqrstuvwxyz0123456", "resent p3c2+37");
   ckstr(p3c3, "Test string of 23+1 chs", "resent p3c3");

   ckint(pIII1->s1i1, 107, "resent pIII1->s1i1");
   ckptr(pIII1->pfs1, p11, "resent pIII1->pfs1");
   ckptr(pIII1->pns2, NULL, "resent pIII1->pns2");
   cklng(pIII2->s2l1, 108, "resent pIII2->s2l1");
   ckptr(pIII2->pns1, p11, "resent pIII2->pns1");

   ckint(pVII1->s1i1, 111, "pVII1->s1i1");
   ckdbl(pVII1->s1d1, 111.1, "pVII1->s1d1");
   ckptr(pVII1->pfs1, p11, "pVII1->mts1");
   ckptr(pVII1->pns2, p22, "pVII1->mts2");
   ckdbl(pVII1->s1d2, 111.2, "pVII1->s1d2");
   ckchr(pVII1->s1c1, 'N', "pVII1->s1c1");
#ifdef PAR
   errsynch(7);
#endif

#ifndef PARn
   if (nerr > 0)
      abexitm(367, "Test halted due to errors in Stage VII");
   else
      cryout(RK_P1, "0Broadcast with resend OK.",
         RK_LN2+RK_FLUSH, NULL);
#endif

/*---------------------------------------------------------------------*
*  Stage VIII.  Check various combinations of pool changes with other  *
*  operations.  N.B.  The pchg pointer is reused for new allocations.  *
*  This leads to several "memory leaks", which do no harm here.        *
*----------------------------------------------------------------------*
*  VIIIa:  Change the pool of an established block.                    *
*---------------------------------------------------------------------*/

/* (1) See that an established host block can move to a shared pool */

   tlen = ALIGN_UP(sizeof(struct mts2)) + LMBDUP;
#ifndef PARn
   pchg = (struct mts2 *)allocpmv(Host, IXstr_mts2, "pchg 8a");
      pchg->s2c1 = 'A', pchg->s2c2 = 'a';
      pchg->pns1 = NULL;
   oldlen1 = memsize(MPS_Host);
   oldnb1 = ckblkct(Host, "8a(1) init Host");
#endif
   newlen1 = memsize(MPS_Shared);
   newnb1 = ckblkct(Shared, "8a(1) init Shared");

#ifndef PARn
   chngpool(pchg, Shared);
#endif

   membcst(MPS_Shared);
#ifdef PAR     /* Send the pointer through */
#ifdef PAR0
   putptr(pchg);
   nnpfl(&bcstr);
#else
   pchg = (struct mts2 *)getptr(bcstr);
#endif
#endif /* PAR */

#ifndef PARn
   oldlen2 = memsize(MPS_Host);
   oldnb2 = ckblkct(Host, "8a(1) final Host");
   cklng(oldlen2, oldlen1 - tlen, "Host len 8a H->S");
   ckint(oldnb2, oldnb1 - 1, "Host nb 8a H->S");
   ckint((int)whatpool(pchg), (int)Shared, "pchg pool 8a H->S");
#endif
   newlen2 = memsize(MPS_Shared);
   newnb2 = ckblkct(Shared, "8a(1) final Shared");
   cklng(newlen2, newlen1 + tlen, "Shared len 8a H->S");
   ckint(newnb2, newnb1 + 1,"Shared nb 8a H->S");
   ckchr(pchg->s2c1, 'A', "pchg s2c1 8a H->S");
   ckchr(pchg->s2c2, 'a', "pchg s2c2 8a H->S");

/* (2) Now move it from Shared to Dynamic and test again.  (This
*  should not actually change the value of pchg on the node.)  */

   oldlen1 = newlen2;
   oldnb1 = newnb2;
   newlen1 = memsize(MPS_Dynamic);
   newnb1 = ckblkct(Dynamic, "8a(2) init Dynamic");
#ifndef PARn
   chngpool(pchg, Dynamic);
   pchg->s2c1 = 'B', pchg->s2c2 = 'b';
#endif

   membcst(MPS_Shared+MPS_Dynamic);

#ifndef PARn
   ckint((int)whatpool(pchg), (int)Dynamic, "pchg pool 8a S->D");
#endif
   oldlen2 = memsize(MPS_Shared);
   oldnb2 = ckblkct(Shared, "8a(2) final Shared");
   cklng(oldlen2, oldlen1 - tlen, "Shared len 8a S->D");
   ckint(oldnb2, oldnb1 - 1, "Shared nb 8a S->D");
   newlen2 = memsize(MPS_Dynamic);
   newnb2 = ckblkct(Dynamic, "8a(2) final Dynamic");
   cklng(newlen2, newlen1 + tlen, "Dynamic len 8a S->D");
   ckint(newnb2, newnb1 + 1,"Dynamic nb 8a S->D");
   ckchr(pchg->s2c1, 'B', "pchg s2c1 8a S->D");
   ckchr(pchg->s2c2, 'b', "pchg s2c2 8a S->D");

/* (3) Now move it from Dynamic back to Host and test again */

   oldlen1 = newlen2;
   oldnb1 = newnb2;
#ifndef PARn
   newlen1 = memsize(MPS_Host);
   newnb1 = ckblkct(Host, "8a(3) init Host");
   chngpool(pchg, Host);
   pchg->s2c1 = 'C', pchg->s2c2 = 'c';
#endif

   membcst(MPS_Shared+MPS_Dynamic);

   oldlen2 = memsize(MPS_Dynamic);
   oldnb2 = ckblkct(Dynamic, "8a(3) final Dynamic");
   cklng(oldlen2, oldlen1 - tlen, "Dynamic len 8a D->H");
   ckint(oldnb2, oldnb1 - 1, "Dynamic nb 8a D->H");
#ifndef PARn
   newlen2 = memsize(MPS_Host);
   newnb2 = ckblkct(Host, "8a(3) final Host");
   cklng(newlen2, newlen1 + tlen, "Host len 8a D->H");
   ckint(newnb2, newnb1 + 1,"Host nb 8a D->H");
   ckint((int)whatpool(pchg), (int)Host, "pchg pool 8a D->H");
   ckchr(pchg->s2c1, 'C', "pchg s2c1 8a D->H");
   ckchr(pchg->s2c2, 'c', "pchg s2c2 8a D->H");
#endif

#ifdef PAR
   errsynch(8);
#endif

#ifndef PARn
   if (nerr > 0)
      abexitm(368, "Test halted due to errors in Stage VIIIa");
#endif

/*---------------------------------------------------------------------*
*  VIIIb:  Change the pool of a newly added block.  (This is the same  *
*  as case VIIIa for a host block, so that test is not repeated here.) *
*---------------------------------------------------------------------*/

/* (1) See that an added Dynamic block can move to the Shared
*  pool before it is established as a base block.  */

#ifndef PARn
   pchg = (struct mts2 *)allocpmv(Dynamic, IXstr_mts2, "pchg 8b");
      pchg->s2c1 = 'D', pchg->s2c2 = 'd';
      pchg->pns1 = NULL;
#endif

   oldlen1 = memsize(MPS_Dynamic);
   oldnb1 = ckblkct(Dynamic, "8b(1) init Dynamic");
   newlen1 = memsize(MPS_Shared);
   newnb1 = ckblkct(Shared, "8b(1) init Shared");
#ifndef PARn
   chngpool(pchg, Shared);
#endif

   /* Don't broadcast Dynamic pool--
   *  block queue should update automatically.  */
   membcst(MPS_Shared);
#ifdef PAR     /* Send the pointer through */
#ifdef PAR0
   putptr(pchg);
   nnpfl(&bcstr);
#else
   pchg = (struct mts2 *)getptr(bcstr);
#endif
#endif /* PAR */

#ifndef PARn
   ckint((int)whatpool(pchg), (int)Shared, "pchg pool 8b D->S");
#endif
   oldlen2 = memsize(MPS_Dynamic);
   oldnb2 = ckblkct(Dynamic, "8b(1) final Dynamic");
   newlen2 = memsize(MPS_Shared);
   newnb2 = ckblkct(Shared, "8b(1) final Shared");
#ifdef PARn
   cklng(oldlen2, oldlen1, "Dynamic len 8b D->S");
   ckint(oldnb2, oldnb1, "Dynamic nb 8b D->S");
#else
   cklng(oldlen2, oldlen1 - tlen, "Dynamic len 8b D->S");
   ckint(oldnb2, oldnb1 - 1, "Dynamic nb 8b D->S");
#endif
   cklng(newlen2, newlen1 + tlen, "Shared len 8b D->S");
   ckint(newnb2, newnb1 + 1,"Shared nb 8b D->S");
   ckchr(pchg->s2c1, 'D', "pchg s2c1 8b D->S");
   ckchr(pchg->s2c2, 'd', "pchg s2c2 8b D->S");

/* (2) Now move it from Shared back to Host and test again */

   oldlen1 = newlen2;
   oldnb1 = newnb2;
#ifndef PARn
   newlen1 = memsize(MPS_Host);
   newnb1 = ckblkct(Host, "8b(2) init Host");

   chngpool(pchg, Host);
   pchg->s2c1 = 'E', pchg->s2c2 = 'e';
#endif

   membcst(MPS_Shared);

   oldlen2 = memsize(MPS_Shared);
   oldnb2 = ckblkct(Shared, "8b(2) final Shared");
   cklng(oldlen2, oldlen1 - tlen, "Shared len 8b S->H");
   ckint(oldnb2, oldnb1 - 1, "Shared nb 8b S->H");
#ifndef PARn
   newlen2 = memsize(MPS_Host);
   newnb2 = ckblkct(Host, "8b(2) final Host");
   cklng(newlen2, newlen1 + tlen, "Host len 8b S->H");
   ckint(newnb2, newnb1 + 1,"Host nb 8b S->H");
   ckint((int)whatpool(pchg), (int)Host, "pchg pool 8b S->H");
   ckchr(pchg->s2c1, 'E', "pchg s2c1 8b S->H");
   ckchr(pchg->s2c2, 'e', "pchg s2c2 8b S->H");
#endif

#ifdef PAR
   errsynch(8);
#endif

#ifndef PARn
   if (nerr > 0)
      abexitm(368, "Test halted due to errors in Stage VIIIb");
#endif

/*---------------------------------------------------------------------*
*  VIIIc:  Change the pool of a block that has just been reallocated.  *
*---------------------------------------------------------------------*/

/* (1) See that a reallocated host block can move to a shared pool */

   tlen2 = 2*ALIGN_UP(sizeof(struct mts2)) + LMBDUP;
   tlen3 = 3*ALIGN_UP(sizeof(struct mts2)) + LMBDUP;
   tlen4 = 4*ALIGN_UP(sizeof(struct mts2)) + LMBDUP;
#ifdef PAR
#define Node0Blks 2
#else
#define Node0Blks 1
#endif
#ifndef PARn
   pchg = (struct mts2 *)allocprv(pchg, 2, IXstr_mts2, "pchg 8c(1)");
      pchg->s2c1 = 'G', pchg->s2c2 = 'g';
      (pchg+1)->pns1 = NULL;
   oldlen1 = memsize(MPS_Host);
   oldnb1 = ckblkct(Host, "8c(1) init Host");
#endif

   newlen1 = memsize(MPS_Shared);
   newnb1 = ckblkct(Shared, "8c(1) init Shared");

#ifndef PARn
   chngpool(pchg, Shared);
#endif

   membcst(MPS_Shared);
#ifdef PAR     /* Send the pointer through */
#ifdef PAR0
   putptr(pchg);
   nnpfl(&bcstr);
#else
   pchg = (struct mts2 *)getptr(bcstr);
#endif
#endif /* PAR */

#ifndef PARn
   oldlen2 = memsize(MPS_Host);
   oldnb2 = ckblkct(Host, "8c(1) final Host");
   cklng(oldlen2, oldlen1 - tlen2, "Host len 8c H->S");
   ckint(oldnb2, oldnb1 - Node0Blks, "Host nb 8c H->S");
   ckint((int)whatpool(pchg), (int)Shared, "pchg pool 8c H->S");
#endif
   newlen2 = memsize(MPS_Shared);
   newnb2 = ckblkct(Shared, "8c(1) final Shared");
   cklng(newlen2, newlen1 + tlen2, "Shared len 8c H->S");
   ckint(newnb2, newnb1 + 1,"Shared nb 8c H->S");
   ckchr(pchg->s2c1, 'G', "pchg s2c1 8c H->S");
   ckchr(pchg->s2c2, 'g', "pchg s2c2 8c H->S");

/* (2) Now reallocate it again, then move it from Shared to Dynamic
*  and test again.  (This may change the value of pchg on the
*  node so send it again after the broadcast.)  */

#ifndef PARn
   pchg = (struct mts2 *)allocprv(pchg, 3, IXstr_mts2, "pchg 8c(2)");
      pchg->s2c1 = 'H', pchg->s2c2 = 'h';
      (pchg+2)->pns1 = NULL;
#endif
   oldlen1 = memsize(MPS_Shared);
   oldnb1 = ckblkct(Shared, "8c(2) init Shared");
   newlen1 = memsize(MPS_Dynamic);
   newnb1 = ckblkct(Dynamic, "8c(2) init Dynamic");
#ifndef PARn
   chngpool(pchg, Dynamic);
#endif

   membcst(MPS_Shared+MPS_Dynamic);
#ifdef PAR     /* Send the pointer through */
#ifdef PAR0
   putptr(pchg);
   nnpfl(&bcstr);
#else
   pchg = (struct mts2 *)getptr(bcstr);
#endif
#endif /* PAR */

   oldlen2 = memsize(MPS_Shared);
   oldnb2 = ckblkct(Shared, "8c(2) final Shared");
#ifdef PARn
   cklng(oldlen2, oldlen1 - tlen2, "Shared len 8c S->D");
   ckint(oldnb2, oldnb1 - 1, "Shared nb 8c S->D");
#else
   cklng(oldlen2, oldlen1 - tlen3, "Shared len 8c S->D");
   ckint(oldnb2, oldnb1 - Node0Blks, "Shared nb 8c S->D");
   ckint((int)whatpool(pchg), (int)Dynamic, "pchg pool 8c S->D");
#endif
   newlen2 = memsize(MPS_Dynamic);
   newnb2 = ckblkct(Dynamic, "8c(2) final Dynamic");
   cklng(newlen2, newlen1 + tlen3, "Dynamic len 8c S->D");
   ckint(newnb2, newnb1 + 1,"Dynamic nb 8c S->D");
   ckchr(pchg->s2c1, 'H', "pchg s2c1 8c S->D");
   ckchr(pchg->s2c2, 'h', "pchg s2c2 8c S->D");

/* (3) Now reallocate it yet again, then move it from Dynamic back
*  to Host and test again.  */

#ifndef PARn
   pchg = (struct mts2 *)allocprv(pchg, 4, IXstr_mts2, "pchg 8c(3)");
      pchg->s2c1 = 'I', pchg->s2c2 = 'i';
      (pchg+3)->pns1 = NULL;
#endif
   oldlen1 = memsize(MPS_Dynamic);
   oldnb1 = ckblkct(Dynamic, "8c(3) init Dynamic");
#ifndef PARn
   newlen1 = memsize(MPS_Host);
   newnb1 = ckblkct(Host, "8c(3) init Host");
   chngpool(pchg, Host);
#endif

   membcst(MPS_Dynamic);

   oldlen2 = memsize(MPS_Dynamic);
   oldnb2 = ckblkct(Dynamic, "8c(3) final Dynamic");
#ifdef PARn
   cklng(oldlen2, oldlen1 - tlen3, "Dynamic len 8c D->H");
   ckint(oldnb2, oldnb1 - 1, "Shared nb 8c D->H");
#else
   cklng(oldlen2, oldlen1 - tlen4, "Dynamic len 8c D->H");
   ckint(oldnb2, oldnb1 - Node0Blks, "Shared nb 8c D->H");
#endif
#ifndef PARn
   newlen2 = memsize(MPS_Host);
   newnb2 = ckblkct(Host, "8c(3) final Host");
   cklng(newlen2, newlen1 + tlen4, "Host len 8c D->H");
   ckint(newnb2, newnb1 + 1,"Host nb 8c D->H");
   ckint((int)whatpool(pchg), (int)Host, "pchg pool 8c D->H");
   ckchr(pchg->s2c1, 'I', "pchg s2c1 8c D->H");
   ckchr(pchg->s2c2, 'i', "pchg s2c2 8c D->H");
#endif

#ifdef PAR
   errsynch(8);
#endif

#ifndef PARn
   if (nerr > 0)
      abexitm(368, "Test halted due to errors in Stage VIIIc");
#endif

/*---------------------------------------------------------------------*
*  VIIId:  Change the pool of a block twice before broadcasting it.    *
*  There are four subcases:  Without and with intervening allocpr,     *
*  and with final target as Host or Shared.                            *
*---------------------------------------------------------------------*/

/* (1) Establish a Shared block, move it to Dynamic, then to Host */

#ifndef PARn
   pchg = (struct mts2 *)allocpmv(Shared, IXstr_mts2, "pchg 8d(1)");
      pchg->s2c1 = 'J', pchg->s2c2 = 'j';
      pchg->pns1 = NULL;
#endif
   membcst(MPS_Shared);    /* To base list on all nodes */

   oldlen1 = memsize(MPS_Shared);
   oldnb1 = ckblkct(Shared, "8d(1) init Shared");
   midlen1 = memsize(MPS_Dynamic);
   midnb1 = ckblkct(Dynamic, "8d(1) init Dynamic");
#ifndef PARn
   newlen1 = memsize(MPS_Host);
   newnb1 = ckblkct(Host, "8d(1) init Host");

   chngpool(pchg, Dynamic);
   chngpool(pchg, Host);
#endif

   membcst(MPS_Shared);
   /* Here there's no pointer to send--block is back on Host */

   oldlen2 = memsize(MPS_Shared);
   oldnb2 = ckblkct(Shared, "8d(1) final Shared");
   cklng(oldlen2, oldlen1 - tlen, "Shared len 8d(1)");
   ckint(oldnb2, oldnb1 - 1, "Shared nb 8d(1)");

   midlen2 = memsize(MPS_Dynamic);
   midnb2 = ckblkct(Dynamic, "8d(1) final Dynamic");
   cklng(midlen2, midlen1, "Dynamic len 8d(1)");
   ckint(midnb2, midnb1, "Dynamic nb 8d(1)");

#ifndef PARn
   newlen2 = memsize(MPS_Host);
   newnb2 = ckblkct(Host, "8d(1) final Host");
   cklng(newlen2, newlen1 + tlen, "Host len 8d(1)");
   ckint(newnb2, newnb1 + 1,"Host nb 8d(1)");
   ckint((int)whatpool(pchg), (int)Host, "pchg pool 8d(1)");
   ckchr(pchg->s2c1, 'J', "pchg s2c1 8d(1)");
   ckchr(pchg->s2c2, 'j', "pchg s2c2 8d(1)");
#endif

/* (2) Establish a Shared block, reallocate it,
*  then move it to Dynamic, finally to Host */

#ifndef PARn
   pchg = (struct mts2 *)allocpmv(Shared, IXstr_mts2, "pchg 8d(2)");
      pchg->s2c1 = 'K', pchg->s2c2 = 'k';
      pchg->pns1 = NULL;
#endif
   membcst(MPS_Shared);    /* To base list on all nodes */

   oldlen1 = memsize(MPS_Shared);
   oldnb1 = ckblkct(Shared, "8d(2) init Shared");
   midlen1 = memsize(MPS_Dynamic);
   midnb1 = ckblkct(Dynamic, "8d(2) init Dynamic");
#ifndef PARn
   newlen1 = memsize(MPS_Host);
   newnb1 = ckblkct(Host, "8d(2) init Host");

   pchg = (struct mts2 *)allocprv(pchg, 2, IXstr_mts2, "pchg 8d(2)");
      (pchg+1)->pns1 = NULL;
   chngpool(pchg, Dynamic);
   chngpool(pchg, Host);
#endif

   membcst(MPS_Shared);
   /* Here there's no pointer to send--block is back on Host */

   oldlen2 = memsize(MPS_Shared);
   oldnb2 = ckblkct(Shared, "8d(2) final Shared");
   cklng(oldlen2, oldlen1 - tlen, "Shared len 8d(2)");
   ckint(oldnb2, oldnb1 - 1, "Shared nb 8d(2)");

   midlen2 = memsize(MPS_Dynamic);
   midnb2 = ckblkct(Dynamic, "8d(2) final Dynamic");
   cklng(midlen2, midlen1, "Dynamic len 8d(2)");
   ckint(midnb2, midnb1, "Dynamic nb 8d(2)");

#ifndef PARn
   newlen2 = memsize(MPS_Host);
   newnb2 = ckblkct(Host, "8d(2) final Host");
   cklng(newlen2, newlen1 + tlen2, "Host len 8d(2)");
   ckint(newnb2, newnb1 + 1,"Host nb 8d(2)");
   ckint((int)whatpool(pchg), (int)Host, "pchg pool 8d(2)");
   ckchr(pchg->s2c1, 'K', "pchg s2c1 8d(2)");
   ckchr(pchg->s2c2, 'k', "pchg s2c2 8d(2)");
#endif

/* (3) Establish a Shared block, move it to Dynamic, then to Shared */

#ifndef PARn
   pchg = (struct mts2 *)allocpmv(Shared, IXstr_mts2, "pchg 8d(3)");
      pchg->s2c1 = 'L', pchg->s2c2 = 'l';
      pchg->pns1 = NULL;
#endif
   membcst(MPS_Shared);    /* To base list on all nodes */

   oldlen1 = memsize(MPS_Shared);
   oldnb1 = ckblkct(Shared, "8d(3) init Shared");
   midlen1 = memsize(MPS_Dynamic);
   midnb1 = ckblkct(Dynamic, "8d(3) init Dynamic");

#ifndef PARn
   chngpool(pchg, Dynamic);
   chngpool(pchg, Shared);
#endif

   membcst(MPS_Shared);
#ifdef PAR     /* Send the pointer through */
#ifdef PAR0
   putptr(pchg);
   nnpfl(&bcstr);
#else
   pchg = (struct mts2 *)getptr(bcstr);
#endif
#endif /* PAR */

#ifndef PARn
   ckint((int)whatpool(pchg), (int)Shared, "pchg pool 8d(3)");
#endif
   oldlen2 = memsize(MPS_Shared);
   oldnb2 = ckblkct(Shared, "8d(3) final Shared");
   cklng(oldlen2, oldlen1, "Shared len 8d(3)");
   ckint(oldnb2, oldnb1, "Shared nb 8d(3)");

   midlen2 = memsize(MPS_Dynamic);
   midnb2 = ckblkct(Dynamic, "8d(3) final Dynamic");
   cklng(midlen2, midlen1, "Dynamic len 8d(3)");
   ckint(midnb2, midnb1, "Dynamic nb 8d(3)");

   ckchr(pchg->s2c1, 'L', "pchg s2c1 8d(3)");
   ckchr(pchg->s2c2, 'l', "pchg s2c2 8d(3)");

/* (4) Establish a Shared block, reallocate it,
*  then move it to Dynamic, finally back to Shared again */

#ifndef PARn
   pchg = (struct mts2 *)allocpmv(Shared, IXstr_mts2, "pchg 8d(4)");
      pchg->s2c1 = 'M', pchg->s2c2 = 'm';
      pchg->pns1 = NULL;
#endif
   membcst(MPS_Shared);    /* To base list on all nodes */

   oldlen1 = memsize(MPS_Shared);
   oldnb1 = ckblkct(Shared, "8d(4) init Shared");
   midlen1 = memsize(MPS_Dynamic);
   midnb1 = ckblkct(Dynamic, "8d(4) init Dynamic");

#ifndef PARn
   pchg = (struct mts2 *)allocprv(pchg, 2, IXstr_mts2, "pchg 8d(4)");
      (pchg+1)->pns1 = NULL;
   chngpool(pchg, Dynamic);
   chngpool(pchg, Shared);
#endif

   membcst(MPS_Shared);
#ifdef PAR     /* Send the pointer through */
#ifdef PAR0
   putptr(pchg);
   nnpfl(&bcstr);
#else
   pchg = (struct mts2 *)getptr(bcstr);
#endif
#endif /* PAR */

#ifndef PARn
   ckint((int)whatpool(pchg), (int)Shared, "pchg pool 8d(4)");
#endif
   oldlen2 = memsize(MPS_Shared);
   oldnb2 = ckblkct(Shared, "8d(4) final Shared");
   cklng(oldlen2, oldlen1 - tlen + tlen2, "Shared len 8d(4)");
   ckint(oldnb2, oldnb1, "Shared nb 8d(4)");

   midlen2 = memsize(MPS_Dynamic);
   midnb2 = ckblkct(Dynamic, "8d(4) final Dynamic");
   cklng(midlen2, midlen1, "Dynamic len 8d(4)");
   ckint(midnb2, midnb1, "Dynamic nb 8d(4)");

   ckchr(pchg->s2c1, 'M', "pchg s2c1 8d(4)");
   ckchr(pchg->s2c2, 'm', "pchg s2c2 8d(4)");

#ifdef PAR
   errsynch(8);
#endif

#ifndef PARn
   if (nerr > 0)
      abexitm(368, "Test halted due to errors in Stage VIIId");
#endif

/*---------------------------------------------------------------------*
*  VIIIe:  Schedule a pool change, w/ or w/o an allocpr, then free the *
*  block without completing the broadcast (and thus the pool change).  *
*---------------------------------------------------------------------*/

/* (1) See that an established Shared block can be scheduled to
*  move to Dynamic, but then be deleted before the move is done.  */

#ifndef PARn
   pchg = (struct mts2 *)allocpmv(Shared, IXstr_mts2, "pchg 8e(1)");
      pchg->s2c1 = 'N', pchg->s2c2 = 'n';
      pchg->pns1 = NULL;
#endif

   membcst(MPS_Shared);

   oldlen1 = memsize(MPS_Shared);
   oldnb1 = ckblkct(Shared, "8e(1) init Shared");
   newlen1 = memsize(MPS_Dynamic);
   newnb1 = ckblkct(Dynamic, "8e(1) init Dynamic");

#ifndef PARn
   chngpool(pchg, Dynamic);
   freepv(pchg, "pchg 8e(1)");
#endif
   pchg = NULL;

   membcst(MPS_Shared+MPS_Dynamic);

   oldlen2 = memsize(MPS_Shared);
   oldnb2 = ckblkct(Shared, "8e(1) final Shared");
   cklng(oldlen2, oldlen1 - tlen, "Shared len 8e S->D->free");
   ckint(oldnb2, oldnb1 - 1, "Shared nb 8e S->D->free");
   newlen2 = memsize(MPS_Dynamic);
   newnb2 = ckblkct(Dynamic, "8e(1) final Dynamic");
   cklng(newlen2, newlen1, "Dynamic len 8e S->D->free");
   ckint(newnb2, newnb1,"Dynamic nb 8e S->D->free");
#ifndef PARn
   if (ie == 33)  /* Create bad whatpool error */
      kp = whatpool(pchg);
#endif

/* (2) See that an established Shared block can be scheduled to
*  move to Dynamic, be enlarged with an allocpr, but then be
*  deleted before the move/realloc is done.  */

#ifndef PARn
   pchg = (struct mts2 *)allocpmv(Shared, IXstr_mts2, "pchg 8e(2)");
      pchg->s2c1 = 'O', pchg->s2c2 = 'o';
      pchg->pns1 = NULL;
#endif

   membcst(MPS_Shared);

   oldlen1 = memsize(MPS_Shared);
   oldnb1 = ckblkct(Shared, "8e(2) init Shared");
   newlen1 = memsize(MPS_Dynamic);
   newnb1 = ckblkct(Dynamic, "8e(2) init Dynamic");

#ifndef PARn
   chngpool(pchg, Dynamic);
   pchg = (struct mts2 *)allocprv(pchg, 2, IXstr_mts2, "pchg 8e(2)");
      (pchg+1)->pns1 = NULL;
   freepv(pchg, "pchg 8e(2)");
#endif
   pchg = NULL;

   membcst(MPS_Shared+MPS_Dynamic);

   oldlen2 = memsize(MPS_Shared);
   oldnb2 = ckblkct(Shared, "8e(2) final Shared");
   cklng(oldlen2, oldlen1 - tlen, "Shared len 8e(2)");
   ckint(oldnb2, oldnb1 - 1, "Shared nb 8e(2)");
   newlen2 = memsize(MPS_Dynamic);
   newnb2 = ckblkct(Dynamic, "8e(2) final Dynamic");
   cklng(newlen2, newlen1, "Dynamic len 8e(2)");
   ckint(newnb2, newnb1,"Dynamic nb 8e(2)");

#ifdef PAR
   errsynch(8);
#endif

#ifndef PARn
   if (nerr > 0)
      abexitm(368, "Test halted due to errors in Stage VIIIe");
#endif

/*---------------------------------------------------------------------*
*  VIIIf:  Schedule a pool change, then do one or two reallocs on the  *
*  block before completing the broadcast (and thus the pool change).   *
*---------------------------------------------------------------------*/

/* (1) See that an established Shared block can be scheduled to
*  move to Dynamic, but then be expanded before the move is done.  */

#ifndef PARn
   pchg = (struct mts2 *)allocpmv(Shared, IXstr_mts2, "pchg 8f(1)");
      pchg->s2c1 = 'P', pchg->s2c2 = 'p';
      pchg->pns1 = NULL;
#endif

   membcst(MPS_Shared);

   oldlen1 = memsize(MPS_Shared);
   oldnb1 = ckblkct(Shared, "8f(1) init Shared");
   newlen1 = memsize(MPS_Dynamic);
   newnb1 = ckblkct(Dynamic, "8f(1) init Dynamic");

#ifndef PARn
   chngpool(pchg, Dynamic);
   pchg = (struct mts2 *)allocprv(pchg, 2, IXstr_mts2, "pchg 8f(1)");
      (pchg+1)->pns1 = NULL;
#endif

   membcst(MPS_Shared+MPS_Dynamic);
#ifdef PAR     /* Send the pointer through */
#ifdef PAR0
   putptr(pchg);
   nnpfl(&bcstr);
#else
   pchg = (struct mts2 *)getptr(bcstr);
#endif
#endif /* PAR */

   oldlen2 = memsize(MPS_Shared);
   oldnb2 = ckblkct(Shared, "8f(1) final Shared");
   cklng(oldlen2, oldlen1 - tlen, "Shared len 8f(1)");
   ckint(oldnb2, oldnb1 - 1, "Shared nb 8f(1)");
   newlen2 = memsize(MPS_Dynamic);
   newnb2 = ckblkct(Dynamic, "8f(1) final Dynamic");
   cklng(newlen2, newlen1 + tlen2, "Dynamic len 8f(1)");
   ckint(newnb2, newnb1 + 1, "Dynamic nb 8f(1)");

/* (2) See that an established Shared block can be scheduled to
*  move to Dynamic, be enlarged with an allocpr, then be enlarged
*  again so allocpr sees a combo change-pool-and-locate block.  */

#ifndef PARn
   pchg = (struct mts2 *)allocpmv(Shared, IXstr_mts2, "pchg 8f(2)");
      pchg->s2c1 = 'Q', pchg->s2c2 = 'q';
      pchg->pns1 = NULL;
#endif

   membcst(MPS_Shared);

   oldlen1 = memsize(MPS_Shared);
   oldnb1 = ckblkct(Shared, "8f(2) init Shared");
   newlen1 = memsize(MPS_Dynamic);
   newnb1 = ckblkct(Dynamic, "8f(2) init Dynamic");

#ifndef PARn
   chngpool(pchg, Dynamic);
   pchg = (struct mts2 *)allocprv(pchg, 2, IXstr_mts2, "pchg 8f(2.1)");
      (pchg+1)->pns1 = NULL;
   pchg = (struct mts2 *)allocprv(pchg, 3, IXstr_mts2, "pchg 8f(2.2)");
      (pchg+2)->pns1 = NULL;
#endif

   membcst(MPS_Shared+MPS_Dynamic);
#ifdef PAR     /* Send the pointer through */
#ifdef PAR0
   putptr(pchg);
   nnpfl(&bcstr);
#else
   pchg = (struct mts2 *)getptr(bcstr);
#endif
#endif /* PAR */

   oldlen2 = memsize(MPS_Shared);
   oldnb2 = ckblkct(Shared, "8f(2) final Shared");
   cklng(oldlen2, oldlen1 - tlen, "Shared len 8f(2)");
   ckint(oldnb2, oldnb1 - 1, "Shared nb 8f(2)");
   newlen2 = memsize(MPS_Dynamic);
   newnb2 = ckblkct(Dynamic, "8f(2) final Dynamic");
   cklng(newlen2, newlen1 + tlen3, "Dynamic len 8f(2)");
   ckint(newnb2, newnb1 + 1,"Dynamic nb 8f(2)");

#ifdef PAR
   errsynch(8);
#endif

#ifndef PARn
   if (nerr > 0)
      abexitm(368, "Test halted due to errors in Stage VIIIf");
   if (nerr == 0)
      cryout(RK_P1, "0Pool change tests OK.",
         RK_LN2+RK_FLUSH, NULL);
#endif

/*---------------------------------------------------------------------*
*                       End memtest.  Clean Up                         *
*---------------------------------------------------------------------*/

#ifdef PAR
#ifdef PAR0
   nnpcl(&bcstr);
#else
   nngcl(&bcstr);
#endif
   appexit(NULL, 0, 0);
#endif /* PAR */

   return 0;
   } /* End memtest() */

