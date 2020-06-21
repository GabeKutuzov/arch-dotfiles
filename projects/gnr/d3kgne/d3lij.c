/* (c) Copyright 1991-2010, Neurosciences Research Foundation, Inc. */
/* $Id: d3lij.c 46 2011-05-20 20:14:09Z  $ */
/***********************************************************************
*                             CNS Program                              *
*               d3lij - Connection Generation Routines                 *
*                                                                      *
*  Note:  These routines are designed to make Lij values accessible    *
*  at any point in CNS without regard to whether or not OPTIMIZE       *
*  STORAGE is in effect.  This makes CNS easier to understand and      *
*  maintain, at a cost of some inner loop calls and redundant bits     *
*  of arithmetic.                                                      *
*                                                                      *
*  None of these routines should be called on a PAR0 node--this file   *
*  should not even be compiled on a PAR0 node, hence no #ifdefs.       *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  int Lija.lijbr(struct CONNTYPE *ix)                                 *
*                                                                      *
*  This routine returns YES if a connection was generated or NO if     *
*     there are no more connections of the current type for the        *
*     current cell (should happen only when generating).               *
*  The actual value of Lij is returned in the Lija structure.          *
*  The caller is expected to maintain Lija.isyn as a loop over nuk     *
*     synapses, i.e., the number actually used.  The d3lij routine     *
*     then sets Lija.jsyn, the actual connection number, which may     *
*     increment more than once per value of isyn when connections      *
*     are skipped (due to being out-of-bounds).                        *
*  In V8A, a "quick and dirty" implementation of self-avoidance is     *
*     included that may perform tests when layers are not the same,    *
*     and that just generates an "out-of-bounds" rather than an        *
*     alternate connection when self is hit.  Until this is fixed,     *
*     rule A has been made illegal with V.                             *
*  In V8D, this was fixed to the extent that the test is always        *
*     omitted when the layers are not the same.                        *
*  Prerequisites are that d3kij() must have been called to set up      *
*     for the particular kind of generation wanted (initial, normal    *
*     access, or regenerating), d3kiji() must have been called to set  *
*     up for the particular cell type and cell number to be accessed,  *
*     and d3kijx() and d3lijx() must have been called to set up for    *
*     the particular connection type to be accessed, in that order.    *
*     The caller must maintain the synapse index (isyn) and increment  *
*     the data pointer (psyn) in the Lija structure, as these are      *
*     shared among all d3lij, d3cij, etc. calls.                       *
*  In KMEM_GO mode, Lij values are looked up in memory if OPTIMIZE     *
*     SPEED is in effect or regenerated from input parameters if       *
*     OPTIMIZE STORAGE is in effect.  Otherwise they are generated.    *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  void d3lijx(struct CONNTYPE *ix)                                    *
*                                                                      *
*  This routine must be called to initialize the Lija array for a      *
*     particular connection type after the celltype and cell have      *
*     been established by d3kiji().  In KMEM_GO mode, the value of     *
*     NUK stored by d3genr() can be used to control a subsequent       *
*     loop over the connections.                                       *
*                                                                      *
*----------------------------------------------------------------------*
*  V4A, 10/24/88  Translated to C from version V3F                     *
*  V5C, 02/20/92, GNR - Allow KGEN=E with various third letter types   *
*  V5E, 07/25/92, GNR - Make KGEN=C consistent in serial and parallel  *
*  Rev, 08/06/92, GNR - Force Lij generation if KGEN=L                 *
*  Rev, 08/15/92, GNR - Remove udevskip from lb1j done in d3go on SKIP *
*  Rev, 08/20/92, GNR - Reflect removal of VG bounds checking in d3go  *
*  Rev, 10/11/92, GRN - Add KGEN=H ("Hypergroup") routine              *
*  V6A, 03/03/93, GNR - Add subarbors, eliminate lbsrcla, fix lb1tia   *
*  V7B, 06/23/94, GNR - Assign lijomit if KGNHV, introduce isyn,isub   *
*  V7C, 01/30/95, GNR - Add KGEN=Q (sQuare annulus) routine            *
*  V8A, 04/15/95, GNR - Merge kchng,flags into WDWDEF, add KGNSA       *
*  Rev, 04/26/97, GNR - Set to always condense connection data         *
*  Rev, 07/30/00, GNR - Correct xia bug, use xoff,yoff with X,J rules  *
*  V8D, 07/01/06, GNR - Major rewrite to make transparent to user code *
*  ==>, 10/31/07, GNR - Last mod before committing to svn repository   *
*  Rev, 06/06/08, GNR - Add SUBARBOR options L and 3                   *
*  Rev, 06/23/08, GNR - Revise IA bounds-checking rules, add jsyn      *
*  V8E, 02/25/09, GNR - Correct jsyn for KGEN=Q, isas for getlij       *
*  V8F, 06/17/10, GNR - Correct rowe test in lbqb, add lbsai           *
*  V8H, 10/24/10, GNR - Add xn (once per conntype) variable allocation *
***********************************************************************/

#define LIJTYPE struct LIJARG

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "rkarith.h"

extern struct LIJARG Lija;

/*---------------------------------------------------------------------*
*               Prototypes for Lij generating routines                 *
*---------------------------------------------------------------------*/

/* Partitioned connection types */
int lb1fp(struct CONNTYPE *ix);
int lb1fpia(struct CONNTYPE *ix);
int lb1fpvg(struct CONNTYPE *ix);
int lb1gp(struct CONNTYPE *ix);
int lb1hp(struct CONNTYPE *ix);
int lb1jp(struct CONNTYPE *ix);
int lb1op(struct CONNTYPE *ix);
int lb1up(struct CONNTYPE *ix);
int lb1upia(struct CONNTYPE *ix);
int lb1upvg(struct CONNTYPE *ix);

/* Normal connection types--first connection */
int lb1f(struct CONNTYPE *ix);
int lb1fia(struct CONNTYPE *ix);
int lb1fvg(struct CONNTYPE *ix);
int lb1g(struct CONNTYPE *ix);
int lb1h(struct CONNTYPE *ix);
int lb1j(struct CONNTYPE *ix);
int lb1n(struct CONNTYPE *ix);
int lb1o(struct CONNTYPE *ix);
int lb1t(struct CONNTYPE *ix);
int lb1tia(struct CONNTYPE *ix);
int lb1tvg(struct CONNTYPE *ix);
int lb1u(struct CONNTYPE *ix);
int lb1uia(struct CONNTYPE *ix);
int lb1uvg(struct CONNTYPE *ix);
int lb1x(struct CONNTYPE *ix);
int lb1xia(struct CONNTYPE *ix);

/* Normal connection types--subsequent connections */
int lb2a(struct CONNTYPE *ix);
int lb2aia(struct CONNTYPE *ix);
int lb2b(struct CONNTYPE *ix);
int lb2bia(struct CONNTYPE *ix);
int lbqb(struct CONNTYPE *ix);
int lb2c(struct CONNTYPE *ix);
int lb2cia(struct CONNTYPE *ix);
int lb2cvg(struct CONNTYPE *ix);
int lb2d(struct CONNTYPE *ix);
int lb2dia(struct CONNTYPE *ix);
int lb2q(struct CONNTYPE *ix);
int lb2qia(struct CONNTYPE *ix);
int lbsa(struct CONNTYPE *ix);
int lbsaia(struct CONNTYPE *ix);
int lbsai(struct CONNTYPE *ix);
int lbsal(struct CONNTYPE *ix);

/* Special cases */
int d3elij(struct CONNTYPE *ix);
int firstLij(struct CONNTYPE *ix);
int getlij(struct CONNTYPE *ix);
int lijomit(struct CONNTYPE *ix);
void lijxyr(struct CONNTYPE *ix);
int otherLij(struct CONNTYPE *ix);


/*=====================================================================*
*                               d3lijx                                 *
*                                                                      *
*  Initialize for Lij generation for a particular connection type.     *
*=====================================================================*/

void d3lijx(struct CONNTYPE *ix) {

   if (Lija.nLij >= 0) return;      /* Already called once */

   Lija.jsyn   = -1;                /* Increments on access */
   Lija.nLij   = 0;
   Lija.lijbr  = ix->cnflgs & GETLIJ ? ix->lijbr1 : firstLij;
   Lija.isas   = 0;
   Lija.nsas   = ix->nsa;
   Lija.jsa    = 0;
   Lija.lb2s   = ix->Cn.saopt & SAOPTL ? ix->nsa : 1;

/* When Cij values are stored in the prd data, a skip-connection
*  list is generated and stored in descending order starting at
*  the top of the space allocated for Cij.  */

   Lija.pskpl = cnexists(ix,CIJ) ? Lija.pxnd + ix->lct - ix->lc : NULL;

/* If call is from a program that consumes rather than generates
*  connections, retrieve Lija.nuk and calculate maximum number
*  skipped from nuk.  Then, if a skip list has been stored, pick
*  up the value of the first index skipped, otherwise set to -1.  */

   Lija.skipndx = -1;
   if (Lija.kcall == KMEM_GO) {
      rd_type *pnuk = Lija.pxnd + ix->xnoff[NUK];
      d3gtln(Lija.nuk, pnuk, ix->nuklen);
      if (Lija.pskpl && (Lija.nskipr = ix->nc - Lija.nuk) > 0)
         d3gtln(Lija.skipndx, Lija.pskpl, ix->nuklen);
      } /* End skip list setup */

/* If doing self-avoidance, set satest to current cell number,
*  else to -1.  This saves one test in each generating routine.
*  (d3tchk() turns off KGNSA bit if the layers are not the same.)  */

   Lija.satest = (ix->kgen & KGNSA) ? Lija.kcell : -1;

/* These variables are only used if generating rather than
*  fetching Lij, but it's probably faster to skip the 'if' and
*  just copy them whether needed or not.  */

   Lija.wlseed = ix->clseed;
   Lija.boxe   = 0;                 /* For KGNPT only */

   } /* End d3lijx() */


/***********************************************************************
*                                                                      *
*                Actual connection generating routines                 *
*                                                                      *
*     The appropriate lb1 (primary Lij) routine is entered via first-  *
*  Lij() to generate the first connection to each cell, and then the   *
*  lb2 or lbq secondary Lij routine is entered via otherLij() for each *
*  subsequent connection.  This allows for the different handling of   *
*  the skip list on first and subsequent calls to the same generating  *
*  routine in independent and partitioned cases, and incidentally      *
*  allows the skip list to be handled in one place rather than in      *
*  each generating routine.  If subarbors are being generated, the     *
*  subarbor routine replaces the secondary routine in lijbr2, and the  *
*  secondary routine is called from there via lijbr3.                  *
*                                                                      *
*     All these routines return YES if a connection was generated or   *
*  NO if there are no more connections of the current type for the     *
*  current cell.  The actual value of Lij is returned in Lija.lijval.  *
*  If a connection is skipped due to being out-of-bounds, Lija.jsyn    *
*  is incremented, a skip list entry is stored (except in the getlij   *
*  case), and another connection is generated, so the caller never     *
*  knows about the skip unless checking Lija.jsyn.                     *
*                                                                      *
*     All Lij routines must do their own bounds checking.  In the      *
*  case of input from the input array, the check must be done against  *
*  the size of the viewing window (KGNST case) or the whole IA (all    *
*  other cases).  Then it should not be necessary to check again when  *
*  the window offset is added, typically in a d3sj() routine, as long  *
*  as the window remains inside the IA.  Primary Lij routines must     *
*  guarantee that their output is in bounds (or must exit with the     *
*  return value NO but store the starting values in fstx,fsty)         *
*  unless used with one of the secondary routines that allow for       *
*  this--currently B, C, D, and Q.                                     *
*                                                                      *
*     Currently all primary routines (except partitioned) must store   *
*  fstx, fsty, and fstr for possible use by subarbor or secondary Lij  *
*  routines.  Although these are supposed to be 'first' values, it is  *
*  OK to reset them every time the lb1 routine is called, because it   *
*  will be called more than once per subarbor only in the case KGEN=I, *
*  and in that case nobody else will look at the 'first' values.       *
*  Routines that use these variables in their own operations take      *
*  advantage of this and may set them each time called.  Other         *
*  routines call lijxyr() to set them.  This is done only on the       *
*  first call in a subarbor, which saves a little time when KGEN=I.    *
*                                                                      *
*     d3genr() condenses all stored connection data such that skipped  *
*  connections are not stored and the unused space is at the end of    *
*  the data.  If Cij are stored, a list of skipped connection indexes  *
*  is stored in the connection data.  The number of connections actu-  *
*  ally generated is stored in nuklen bytes at pnuk = Lija.pxnd +      *
*  xnoff[NUK].                                                         *
*                                                                      *
*  Lij storage was revised in V3F to allow Lij to have 1-4 bytes as    *
*  needed.  The external Lij connection list (CONNLIST) file format    *
*  has been revised (02/20/92) to allow a full 4-byte Lij, which is    *
*  truncated to lijlen bytes when read.  Note that lijlen clicks up    *
*  one byte when srcnelt just equals a power of 256, even though cell  *
*  numbers start at 0, because the code Lij = all 1's is reserved for  *
*  use as the skip flag and cannot represent a source cell.            *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                  V8D, 08/01/06, Compatibility Note                   *
*                                                                      *
*  Code for partitioned types was modified to eliminate assumption     *
*  that nc < S16 and chsiz < S16.  Results will not match old code.    *
*  With this in mind, no correction was made for change in direction   *
*  of isyn counting, which will also cause results to differ.  A bug   *
*  using srcngx instead of nux to calculate x,y coords for KGNTP was   *
*  also fixed.                                                         *
*                                                                      *
*  Code for type C secondary connections was also modified to          *
*  eliminate S16 assumptions.  Results will not match old code.        *
*  The following translation note dated 11/20/88 is therefore moot:    *
*     "It was not practical in all cases to exactly duplicate the      *
*  arithmetic carried out in the IBM assembler versions of the Lij     *
*  generating routines.  Routines that may perform differently than    *
*  their IBM assembler predecessors are:  lb2cia."                     *
*                                                                      *
*  Under the new IA bounds-checking rules, some results may differ     *
***********************************************************************/

/*---------------------------------------------------------------------*
*  Lij routine for calling first Lij routine and then setting otherLij *
*     to call the secondary Lij routine for subsequent calls.          *
*  V7C, 01/31/95, GNR - Revised to execute lijbr2 routine right away   *
*     if code 'Q' is enabled.  To get rid of the extra 'if' in this    *
*     code, it would be necessary to make another version and put its  *
*     address in lijbr at start of each cell in d3genr/d3go, hence     *
*     utilizing yet another lijbr ptr in the conntype block (not done).*
*  V8D, 08/27/06, GNR - Generate skip list here rather than in d3genr  *
*     or d3regenr.  If connection is out-of-bounds, store isyn in skip *
*     list, increment jsyn, and repeat until ix->nc count fulfilled.   *
*  Rev, 09/29/06, GNR - Add nLij counter so user code, cijbr() code,   *
*     and sjbr() code can all call lijbr() and get just one execution. *
*  V8G, 10/18/10, GNR - Correct bug in earlier KGNAN bug fix.          *
*---------------------------------------------------------------------*/


int firstLij(struct CONNTYPE *ix) {

   /* Oops, user didn't initialize */
   if (Lija.nLij < 0) d3lijx(ix);

   /* Set jsyn to first conn--getLij,lb2q will increment from -1 */
   Lija.jsyn = 0;
   Lija.lijrc = ix->lijbr1(ix);  /* Call first routine */
   Lija.svlseed = Lija.wlseed;   /* Save restart seed  */
   Lija.lijbr = otherLij;        /* Set for second routine  */
   if (ix->kgen & KGNAN) {       /* (Types that don't use first Lij) */
      Lija.jsyn = -1;
      Lija.lijrc = otherLij(ix);
      }
   else {
      if (!Lija.lijrc) {
         if (Lija.pskpl) {       /* Save indices of skipped item(s) */
            long jsyn;           /* Loop is really for NOPSAL */
            for (jsyn=0; jsyn<Lija.lb2s; ++jsyn) {
               d3ptln(jsyn, Lija.pskpl, ix->nuklen);
               Lija.pskpl -= ix->nuklen; }
            }
         Lija.jsyn += Lija.lb2s - 1;
         Lija.lijrc = otherLij(ix);
         }
      else
         Lija.nLij = 1;
      }

   return Lija.lijrc;

   } /* End firstLij() */

/* Note:  otherLij() should always be entered with Lija.jsyn equal
*  to the number of the connection previously processed, as it
*  will be incremented here before calling the actual generating
*  routine.  That is the reason for adding Lija.lb2s in firstLij()
*  when the first connection (or subarbor) gets skipped.  */

int otherLij(struct CONNTYPE *ix) {

   /* Return right away if lijbr() already executed for this conn */
   if (Lija.nLij > Lija.isyn) return Lija.lijrc;
   ++Lija.nLij;

   while (++Lija.jsyn < ix->nc) {
      if ((Lija.lijrc = ix->lijbr2(ix)))  /* Assignment intended */
         return Lija.lijrc;
      if (Lija.pskpl) {          /* Save index of skipped item */
         d3ptln(Lija.jsyn, Lija.pskpl, ix->nuklen);
         Lija.pskpl -= ix->nuklen; }
      } /* End loop over out-of-bound connections */

   return (Lija.lijrc = NO);

   } /* End otherLij() */


/*---------------------------------------------------------------------*
*     Lij routine used when Lij's are not stored (hand vision).        *
*---------------------------------------------------------------------*/

int lijomit(struct CONNTYPE *ix) {

   ++Lija.jsyn;
   Lija.lijval = 0;
   return YES;

   } /* End lijomit() */


/*---------------------------------------------------------------------*
*     getlij - Pick up Lij from the data structure                     *
*                                                                      *
*     This routine is used when in KMEM_GO mode and Lij values were    *
*     prestored (the usual case with OPTIMIZE SPEED and always with    *
*     external connections regardless of OPTIMIZE setting.             *
*                                                                      *
*     It is no longer necessary to update wlseed when skipping the     *
*     rest of the connections for this cell, because it will be        *
*     initialized from ix->clseed when d3lijx is next called.          *
*---------------------------------------------------------------------*/

int getlij(struct CONNTYPE *ix) {

   /* Return right away if lijbr() already executed for this conn */
   if (Lija.nLij > Lija.isyn) return Lija.lijrc;
   ++Lija.nLij;

   /* If this was a skipped connection, skip it again now.
   *  Note:  If there is no skip list, Lija.skipndx is -1 and this
   *    'while' is skipped--no need to test existence of pskpl.  */
   while (++Lija.jsyn == Lija.skipndx) {
      if (--Lija.nskipr <= 0)
         Lija.skipndx = -1;
      else {
         Lija.pskpl -= ix->nuklen;
         d3gtln(Lija.skipndx, Lija.pskpl, ix->nuklen);
         }
      }

   /* Do subarbor bookkeeping */
   while (Lija.jsyn >= Lija.nsas) {
      Lija.isas = Lija.nsas, Lija.nsas += ix->nsa;
      }

   d3gtln(Lija.lijval, Lija.psyn+ix->cnoff[LIJ], ix->lijlen);

   return (Lija.lijrc = (Lija.lijval != ix->lijlim));

   } /* End getlij() */


/*---------------------------------------------------------------------*
*     lb1f - Floating map from a repertoire - new in V2I               *
*                                                                      *
*     Maps uniformly from a specified rectangular region in the        *
*        source.  Sort of a T-U hybrid.                                *
*     Since this routine can generate connections that are out of      *
*        bounds, it currently can not be used with 'A'.                *
*---------------------------------------------------------------------*/

int lb1f(struct CONNTYPE *ix) {

   ui32 low32a,low32b;

   Lija.fstr = jm64sb(udev(&Lija.wlseed),ix->srcnel,1,&low32a);
   Lija.fstx = ix->xoff + jm64nb(low32a>>1,ix->ul1.s.sfx,&low32b);
   Lija.fsty = ix->yoff + jm64nh(low32b>>1,ix->ul1.s.sfy);

   /* Skip the connection if X or Y out of bounds */
   if ((unsigned long)Lija.fstx >= ix->iawxsz) return NO;
   if ((unsigned long)Lija.fsty >= ix->iawysz) return NO;

   Lija.lijval = (Lija.fsty*ix->srcngx + Lija.fstx)*ix->srcnel +
      Lija.fstr;

   return (Lija.lijval != Lija.satest);

   } /* End lb1f() */


/*---------------------------------------------------------------------*
*     lb1fia - Floating map from input array - new in V2I              *
*---------------------------------------------------------------------*/

int lb1fia(struct CONNTYPE *ix) {

   ui32 low32a;

   Lija.fstx = ix->xoff +
      jm64nb(udev(&Lija.wlseed),ix->ul1.s.sfx,&low32a);
   Lija.fsty = ix->yoff +
      jm64nh((long)(low32a>>1),ix->ul1.s.sfy);
   Lija.fstr = 0;

   /* Unsigned test should reject negatives */
   if ((unsigned long)Lija.fstx >= ix->iawxsz) return NO;
   if ((unsigned long)Lija.fsty >= ix->iawysz) return NO;

   Lija.lijval = (Lija.fstx << RP->ky1) + Lija.fsty;

   return YES;

   } /* End lb1fia() */


/*---------------------------------------------------------------------*
*     lb1fvg - Floating map from a virtual repertoire, or TV           *
*                                                                      *
*     Since this routine can generate connections that are out of      *
*        bounds, it currently can not be used with 'A'.                *
*---------------------------------------------------------------------*/

int lb1fvg(struct CONNTYPE *ix) {

   ui32 low32a;

   Lija.fstx = ix->xoff +
      jm64nb(udev(&Lija.wlseed),ix->ul1.s.sfx,&low32a);
   Lija.fsty = ix->yoff +
      jm64nh((long)(low32a>>1),ix->ul1.s.sfy);
   Lija.fstr = 0;

   /* Skip the connection if X or Y out of bounds */
   if ((unsigned long)Lija.fstx >= ix->iawxsz) return NO;
   if ((unsigned long)Lija.fsty >= ix->iawysz) return NO;

   Lija.lijval = Lija.fstx + Lija.fsty*ix->srcngx;

   return YES;

   } /* End lb1fvg() */


/*---------------------------------------------------------------------*
*     lb1fp - Partitioned floating from a repertoire - new in V2I      *
*                                                                      *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1fp(struct CONNTYPE *ix) {

   ldiv_t qrmp,qrmq;
   long wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   qrmp  = ldiv(wy, ix->srcnel);
   Lija.lijval = qrmp.rem;
   qrmq  = ldiv(qrmp.quot, ix->nux);
   wx = qrmq.rem  + ix->xoff;
   wy = qrmq.quot + ix->yoff;

   /* Skip the connection if X or Y out of bounds */
   if ((unsigned long)wx >= ix->iawxsz) return NO;
   if ((unsigned long)wy >= ix->iawysz) return NO;

   Lija.lijval += (wy*ix->srcngx + wx)*ix->srcnel;

   return (Lija.lijval != Lija.satest);

   } /* End lb1fp() */


/*---------------------------------------------------------------------*
*     lb1fpia - Partitioned floating from input array - new in V2I     *
*                                                                      *
*     No need to save fstxy--this routine is its own lb2 routine       *
*---------------------------------------------------------------------*/

int lb1fpia(struct CONNTYPE *ix) {

   ldiv_t qrm;
   long wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   qrm = ldiv(wy, ix->nux);
   wx = qrm.rem;
   wy = qrm.quot;

   /* Unsigned test should reject negatives */
   if ((unsigned long)wx >= ix->iawxsz) return NO;
   if ((unsigned long)wy >= ix->iawysz) return NO;

   Lija.lijval = (wx << RP->ky1) + wy;

   return YES;

   } /* End lb1fpia() */


/*---------------------------------------------------------------------*
*     lb1fpvg - Partitioned floating from virtual repertoire or TV     *
*                                                                      *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1fpvg(struct CONNTYPE *ix) {

   ldiv_t qrm;
   long wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   qrm = ldiv(wy, ix->nux);
   wx = qrm.rem  + ix->xoff;
   wy = qrm.quot + ix->yoff;

   /* Skip the connection if X or Y out of bounds */
   if ((unsigned long)wx >= ix->iawxsz) return NO;
   if ((unsigned long)wy >= ix->iawysz) return NO;

   Lija.lijval = wy*ix->srcngx + wx;

   return YES;

   } /* End lb1fpvg() */


/*---------------------------------------------------------------------*
*     lb1g - Random from same group as target cell is in               *
*                                                                      *
*     (Note: this is not necessarily in the same layer.                *
*        wgrpad is incremented at d3kiji for new groups.)              *
*---------------------------------------------------------------------*/

int lb1g(struct CONNTYPE *ix) {

   Lija.lijval = jm64nh(udev(&Lija.wlseed),ix->ul1.g.srcnel2)
                 + ix->ul1.g.wgrpad;
   if (Lija.jsyn == Lija.isas) lijxyr(ix);   /* Decode coords */

   return (Lija.lijval != Lija.satest);

   } /* End lb1g() */


/*---------------------------------------------------------------------*
*     lb1gp - Partitioned from same group as target cell is in         *
*                                                                      *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1gp(struct CONNTYPE *ix) {

   long wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   Lija.lijval = ix->ul1.g.wgrpad + wy;

   return (Lija.lijval != Lija.satest);

   } /* End lb1gp() */


/*---------------------------------------------------------------------*
*     lb1h - Random from same "hypergroup" as target cell is in        *
*                                                                      *
*     (A "hypergroup" is the area defined by ngridx,ngridy.)           *
*     Currently lb1hia, lb1hpia have not been implemented (IA needs    *
*        new grid params to do it and hypergroups do not exist with    *
*        virtual group inputs.)                                        *
*     Note: input is not necessarily from the same layer.              *
*        wgrpad is incremented at d3kiji for new hypergroups.          *
*---------------------------------------------------------------------*/

int lb1h(struct CONNTYPE *ix) {

   ldiv_t qrm;
   long randcell = jm64nh(udev(&Lija.wlseed),ix->ul1.g.srcnel2);

   qrm = ldiv(randcell, ix->ul1.g.gridrow);
   Lija.lijval = qrm.quot*ix->ul1.g.srcnel1 + qrm.rem +
      ix->ul1.g.wgrpad;
   if (Lija.jsyn == Lija.isas) lijxyr(ix);   /* Decode coords */

   return (Lija.lijval != Lija.satest);

   } /* End lb1h() */


/*---------------------------------------------------------------------*
*     lb1hp - Partitioned from same "hypergroup" as target             *
*                                                                      *
*     This type new in V8D.                                            *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1hp(struct CONNTYPE *ix) {

   ldiv_t qrm;
   long wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   qrm  = ldiv(wy, ix->ul1.g.gridrow);
   Lija.lijval = qrm.quot*ix->ul1.g.srcnel1 + qrm.rem +
      ix->ul1.g.wgrpad;

   return (Lija.lijval != Lija.satest);

   } /* End lb1hp() */


/*---------------------------------------------------------------------*
*     lb1j - Joint-style distribution from a repertoire                *
*                                                                      *
*     Choose randomly from lsg cells starting at loff, then            *
*        increment loff by nux+nuy*nel*ngx in wgrpad.                  *
*     This code used for input from REPERTOIRE or VG or TV.            *
*     V8D, 08/27/06, GNR - If out-of-bounds, just skip the one         *
*        connection, it might come in again on next KGNIN call.        *
*---------------------------------------------------------------------*/

int lb1j(struct CONNTYPE *ix) {

   Lija.lijval = jm64nh(udev(&Lija.wlseed),ix->lsg) + ix->ul1.g.wgrpad;

   if (Lija.jsyn == Lija.isas) lijxyr(ix);   /* Decode coords */

   /* Check for out of bounds */
   if ((unsigned long)(Lija.lijval) >= ix->srcnelt) return NO;

   return (Lija.lijval != Lija.satest);

   } /* End lb1j() */


/*---------------------------------------------------------------------*
*     lb1jp - Partitioned from lsg cells starting at loff              *
*                                                                      *
*     This code used for input from REPERTOIRE or VG or TV.            *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1jp(struct CONNTYPE *ix) {

   long wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   Lija.lijval = ix->ul1.g.wgrpad + wy;

   /* Check for out of bounds */
   if ((unsigned long)(Lija.lijval) >= ix->srcnelt) return NO;

   return (Lija.lijval != Lija.satest);

   } /* End lb1jp() */


/*---------------------------------------------------------------------*
*     lb1n - Normal distribution around the target cell                *
*                                                                      *
*     (We want a true 2d distribution--coding postponed)               *
*---------------------------------------------------------------------*/

int lb1n(struct CONNTYPE *ix) {

   /* *** Temporary error exit *** */
   d3exit(LIJGEN_ERR, fmturlnm(Lija.kl), (long)ix->ict);

   /* Logic never gets here, include return to get rid of warning */
   return NO;

   } /* End lb1n() */


/*---------------------------------------------------------------------*
*     lb1o - Uniformly chosen from groups other than current group     *
*---------------------------------------------------------------------*/

int lb1o(struct CONNTYPE *ix) {

   Lija.lijval = jm64nh(udev(&Lija.wlseed),ix->ul1.g.srcnel2) +
                 ix->ul1.g.wgrpad + ix->ul1.g.ogoff;

   /* If wrapped around, adjust base */
   if (Lija.lijval >= ix->srcnelt) Lija.lijval -= ix->srcnelt;

   if (Lija.jsyn == Lija.isas) lijxyr(ix);   /* Decode coords */

   return YES;

   } /* End lb1o() */


/*---------------------------------------------------------------------*
*     lb1op - Partitioned from groups other than target group          *
*                                                                      *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1op(struct CONNTYPE *ix) {

   long wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   Lija.lijval = ix->ul1.g.wgrpad + ix->ul1.g.ogoff + wy;

   /* If wrapped around, adjust base */
   if (Lija.lijval >= ix->srcnelt) Lija.lijval -= ix->srcnelt;

   return YES;

   } /* End lb1op() */


/*---------------------------------------------------------------------*
*     lb1t - Topographic map from a REPERTOIRE or VG - new in V1K      *
*                                                                      *
*     Maps each target group onto a region of proportional             *
*        fractional size in the source.  A group in the region is      *
*        chosen with probability proportional to the fraction of       *
*        its area that is in the region.  Then a cell is chosen at     *
*        random from within the chosen group.  This algorithm          *
*        maintains an ordering relationship from group to group        *
*        but not from cell to cell within a group.  It guarantees      *
*        that every source cell has equal probability of being         *
*        used overall.                                                 *
*     V3A, 08/07/87, GNR - Modify to add offset, bounds checking.      *
*        Store X,Y coords for lb2 routines.  Correct so all            *
*        cells in a source group are positionally equivalent.          *
*        The sequence of multiplies and divides has been arranged      *
*        to minimize loss of random bits on the right.                 *
*     Since this routine can generate connections that are out of      *
*        bounds, it currently cannot be used with 'A'.                 *
*---------------------------------------------------------------------*/

int lb1t(struct CONNTYPE *ix) {

   unsigned long low32a, low32b;

   Lija.lijval = Lija.fstr =
      jm64sb(udev(&Lija.wlseed),ix->srcnel,1,&low32a);
   Lija.fstx = ix->xoff +
      jm64nb(ds64nq(Lija.groupx,low32a,-1,ix->ul1.s.fstngx),
             ix->ul1.s.sfx,&low32b);
   Lija.fsty = ix->yoff +
      jm64nh(ds64nq(Lija.groupy,low32b,-1,ix->ul1.s.fstngy),
             ix->ul1.s.sfy);

   /* Skip the connection if X or Y out of bounds */
   if ((unsigned long)Lija.fstx >= ix->iawxsz) return NO;
   if ((unsigned long)Lija.fsty >= ix->iawysz) return NO;

   Lija.lijval += ix->srcnel * (Lija.fsty*ix->srcngx + Lija.fstx);

   return (Lija.lijval != Lija.satest);

   } /* End lb1t() */


/*---------------------------------------------------------------------*
*     lb1tia - Topographic map from the input array                    *
*                                                                      *
*     Rewritten in V2C to permit nonintegral mappings via random       *
*        selection from a box as in lb1t.  Earlier versions were       *
*        fully deterministic, with equal steps (unfortunate side       *
*        effect--this version is slower).                              *
*     This routine can generate connections that are out-of-bounds     *
*        given some combinations of the xoff,yoff,nux,nuy parameters.  *
*        These conns can be stored as origin points for lb2 routines,  *
*        but under the new rules, they must be rejected here.          *
*     Since this routine can generate connections that are out of      *
*        bounds, it currently cannot be used with 'A'.                 *
*     Bug fix in V6A--calculate new x,y coords on first cell on node,  *
*        even if it is not first cell in group.                        *
*---------------------------------------------------------------------*/

int lb1tia(struct CONNTYPE *ix) {

   /* fstnew is TRUE if a 1:1 mapping and this is first connection of
   *  first cell on this node, or first connection of first cell in a
   *  new group, or not a 1:1 mapping and this is any connection.  */
   if (ix->ul1.s.fstnew) {
      /* Calculate location of source for this cell or group */
      unsigned long low32a;
      /* Note shift of udev result: sign bit is filled in */
      Lija.fstx = ix->xoff + jm64nb(ds64nq(Lija.groupx,
         ((unsigned long)udev(&Lija.wlseed))<<1,
         -1,ix->ul1.s.fstngx),ix->ul1.s.sfx,&low32a);
      Lija.fsty = ix->yoff + jm64nh(ds64nq(Lija.groupy,
            low32a,-1,ix->ul1.s.fstngy),ix->ul1.s.sfy);
      /* If in a 1:1 mapping, save coordinates and set flag to reuse */
      if (ix->cnflgs & STM11) {
         ix->ul1.s.fstsvx = Lija.fstx; ix->ul1.s.fstsvy = Lija.fsty;
         ix->ul1.s.fstnew = FALSE;
         }
      } /* End new source */

   else {
      /* Reuse same source cell if 1:1 mapping and not a new target
      *  group.  Note that this code does not advance wlseed, there-
      *  fore udevskip for first cell on node must compensate.  */
      Lija.fstx = ix->ul1.s.fstsvx; Lija.fsty = ix->ul1.s.fstsvy;
      }
   Lija.fstr = 0;

   /* Unsigned test should reject negatives */
   if ((unsigned long)Lija.fstx >= ix->iawxsz) return NO;
   if ((unsigned long)Lija.fsty >= ix->iawysz) return NO;

   Lija.lijval = (Lija.fstx << RP->ky1) + Lija.fsty;
   return YES;

   } /* End lb1tia() */


/*---------------------------------------------------------------------*
*     lb1tvg - Topographic map from virtual groups or TV               *
*                                                                      *
*     Since this routine can generate connections that are out of      *
*        bounds, it currently can not be used with 'A'.                *
*---------------------------------------------------------------------*/

int lb1tvg(struct CONNTYPE *ix) {

   unsigned long low32a;

   Lija.fstx = ix->xoff +
      jm64nb(ds64nq(Lija.groupx,
         ((unsigned long)udev(&Lija.wlseed))<<1, -1,
         ix->ul1.s.fstngx),ix->ul1.s.sfx,&low32a);
   Lija.fsty = ix->yoff + jm64nh(ds64nq(Lija.groupy,
      low32a,-1,ix->ul1.s.fstngy),ix->ul1.s.sfy);
   Lija.fstr = 0;

   /* Skip the connection if X or Y out of bounds */
   if ((unsigned long)Lija.fstx >= ix->iawxsz) return NO;
   if ((unsigned long)Lija.fsty >= ix->iawysz) return NO;

   Lija.lijval = Lija.fsty*ix->srcngx + Lija.fstx;
   return YES;

   } /* End lb1tvg() */


/*---------------------------------------------------------------------*
*     lb1u - Uniform random from anywhere in the layer                 *
*---------------------------------------------------------------------*/

int lb1u(struct CONNTYPE *ix) {

   Lija.lijval = jm64nh(udev(&Lija.wlseed),ix->ul1.g.srcnel2);

   if (Lija.jsyn == Lija.isas) lijxyr(ix);   /* Decode coords */

   return (Lija.lijval != Lija.satest);

   } /* End lb1u() */


/*---------------------------------------------------------------------*
*     lb1uia - Uniform input from anywhere in Input Array              *
*---------------------------------------------------------------------*/

int lb1uia(struct CONNTYPE *ix) {

   long work = jm64nh(udev(&Lija.wlseed),ix->ul1.g.srcnel2);

   if (Lija.isyn == 0) {
      Lija.fstx = work >> RP->ky;
      Lija.fsty = work & RP->ymask;
      Lija.fstr = 0;
      }
   Lija.lijval = (work & -RP->nsy) + work;

   return YES;

   } /* End lb1uia() */


/*---------------------------------------------------------------------*
*     lb1uvg - Uniform input from virtual groups or TV                 *
*---------------------------------------------------------------------*/

int lb1uvg(struct CONNTYPE *ix) {

   Lija.lijval = jm64nh(udev(&Lija.wlseed),ix->ul1.g.srcnel2);

   if (Lija.jsyn == Lija.isas) lijxyr(ix);   /* Decode coords */

   return YES;

   } /* End lb1uvg() */


/*---------------------------------------------------------------------*
*     lb1up - Partitioned from entire layer                            *
*                                                                      *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1up(struct CONNTYPE *ix) {

   long wx;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   Lija.lijval = Lija.boxe +
      jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   return (Lija.lijval != Lija.satest);

   } /* End lb1up() */


/*---------------------------------------------------------------------*
*     lb1upia - Partitioned uniform input from Input Array             *
*                                                                      *
*     No need to save fstx,fsty--this routine is its own lb2 routine   *
*---------------------------------------------------------------------*/

int lb1upia(struct CONNTYPE *ix) {

   long wx,wy;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   wy = Lija.boxe + jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   Lija.lijval = (wy & -RP->nsy) + wy;

   return YES;

   } /* End lb1upia() */


/*---------------------------------------------------------------------*
*     lb1upvg - Partitioned input from virtual groups or TV            *
*                                                                      *
*     No need to save fstx,fsty--this routine is its own lb2 routine.  *
*     Routine is same as lb1up except no self-avoidance test.          *
*---------------------------------------------------------------------*/

int lb1upvg(struct CONNTYPE *ix) {

   long wx;

   wx = dm64nq(ix->ul2.p.srcsiz, Lija.isyn+1, ix->nc);
   Lija.lijval = Lija.boxe +
      jm64nh((wx-Lija.boxe)<<1, udev(&Lija.wlseed));
   Lija.boxe = wx;         /* Start at top of box next time */

   return YES;

   } /* End lb1upvg() */


/*---------------------------------------------------------------------*
*     lb1x - Systematic input from a repertoire or virtual groups      *
*                                                                      *
*     Contrary to general policy, this does use wrap-around because    *
*        user controls incrementing, may want it.                      *
*---------------------------------------------------------------------*/

int lb1x(struct CONNTYPE *ix) {

   Lija.lijval = ix->ul1.x.cwrkoff;
   ix->ul1.x.cwrkoff += ix->ul1.x.incoff;

   /* If wrapped around, adjust base */
   if (ix->ul1.x.cwrkoff >= ix->srcnelt)
      ix->ul1.x.cwrkoff -= ix->srcnelt;

   if (Lija.jsyn == Lija.isas) lijxyr(ix);   /* Decode coords */

   return (Lija.lijval != Lija.satest);  /* Always TRUE if VG input */

   } /* End lb1x() */


/*---------------------------------------------------------------------*
*     lb1xia - Systematic from input array                             *
*                                                                      *
*     Similar to lb1tia, but user controls offset directly.            *
*     Wrap-around is permitted, see note for lb1x.                     *
*---------------------------------------------------------------------*/

int lb1xia(struct CONNTYPE *ix) {

   long work;

   Lija.lijval = ix->ul1.x.cwrkoff;
   work = Lija.lijval + ix->ul1.x.incoff + RP->xymaskc;
   if (work >= 0) work++;           /* X overflow, bump Y */
   ix->ul1.x.cwrkoff = work & RP->xymask;
   if (Lija.isyn == 0) {
      Lija.fstx = Lija.lijval >> RP->ky1;
      Lija.fsty = Lija.lijval & RP->ymask;
      Lija.fstr = 0;
      }

   return YES;

   } /* End lb1xia() */


/***********************************************************************
*     lijxyr - Decode first Lij into x,y,r coords for subarbor and lb2 *
***********************************************************************/

void lijxyr(struct CONNTYPE *ix) {

   ldiv_t qrm;
   long work = Lija.lijval/ix->srcnel;
   qrm = ldiv(work, ix->srcngx);
   Lija.fstr = Lija.lijval - work*ix->srcnel;
   Lija.fstx = qrm.rem;
   Lija.fsty = qrm.quot;

   } /* End lijxyr() */


/***********************************************************************
*                                                                      *
*     Secondary Lij routines - Generation of connections after the     *
*                                first                                 *
*                                                                      *
*     lb2xx    Generate connections for rule xx.  For rule 'B', lb2b   *
*              assumes the first point (in lijval) is in bounds.       *
*     lbqb     For rule 'B', works from the coords in fstxy instead,   *
*              allowing first point to be out-of-bounds while          *
*              subsequent points can bleed back into bounds.           *
*                                                                      *
*     all lb2 routines must check for connections out-of-bounds        *
*                                                                      *
***********************************************************************/

/*---------------------------------------------------------------------*
*     lb2a - Adjacent cell in a regular or virtual repertoire or TV    *
*                                                                      *
*        lb2a has been defined to give helical boundary conditions.    *
*     Since this code cannot be guaranteed if the first connection     *
*     is out of bounds, code 'A' is illegal with any first-connection  *
*     code that can generate such connections (currently F,S,T).       *
*     This state of affairs must be reviewed when a user-selectable    *
*     boundary condition option is added.                              *
*                                                                      *
*     Rev, 12/10/87, GNR - Use true mod function so stride<0 is OK     *
*     V3F, 09/02/88, GNR - Use this code for VG input--                *
*---------------------------------------------------------------------*/

int lb2a(struct CONNTYPE *ix) {

   Lija.lijval = (Lija.lijval+ix->ul2.a.incr)%ix->srcnelt;
   /* Make sure it's positive */
   if (Lija.lijval < 0) Lija.lijval += ix->srcnelt;

   return YES;

   } /* End lb2a() */


/*---------------------------------------------------------------------*
*     lb2aia - Adjacent cell in input array                            *
*                                                                      *
*     Uses helical boundary conditions--when goes out-of-bounds        *
*        to right, moves to corresponding point in next row--          *
*        when goes out at bottom, moves up and over one column.        *
*     Test X overflow first, adjustment can cause y overflow.          *
*     (Stride is <= IA size, so can only go over by one unit.)         *
*                                                                      *
*     *************************************************************    *
*     ****  This code is not correct if an offset is added in  ****    *
*     ****  d3go.  To avoid this, 'A' is made illegal with 'S' ****    *
*     *************************************************************    *
*                                                                      *
*---------------------------------------------------------------------*/

int lb2aia(struct CONNTYPE *ix) {

   /* If out at the right, increment X */
   if ((Lija.lijval += ix->ul2.a.incr) > ix->ul2.a.lim)
      Lija.lijval -= ix->ul2.a.lim;
   /* Clean up coords */
   Lija.lijval = (Lija.lijval + RP->nsy) & RP->xymask;

   return YES;

   } /* End lb2aia() */


/*---------------------------------------------------------------------*
*     lb2b - 'Box' distribution in a cell layer or VT or TV            *
*          = rectangle with upper left corner at starting point        *
*                                                                      *
*     Case I (old style) version--the first connection must be         *
*        in bounds, otherwise lbqb should be called instead.           *
*     As per bounds checking rules in d3go, bounds are checked         *
*        here for repertoire input.  Any skips must be inserted        *
*        in correct order, as lb2b may be used with 'L' or 'M'         *
*        Cij generating codes.                                         *
*---------------------------------------------------------------------*/

int lb2b(struct CONNTYPE *ix) {

   long work;   /* Scratch space */

   if (Lija.jsyn == Lija.lb2s) {
      /* Here if first call to routine in any subarbor.  First Lij is
      *  supposed to be in bounds or lbqb should have been called, but
      *  this is tested here "just in case".  This code rewritten,
      *  08/09/06, to give development error instead of nonexistent
      *  old LIJ_SKIP_TYP, rename stlim,rowlim-->rowe,boxe.  */
      if ((unsigned long)Lija.lijval >= (unsigned long)ix->srcnelt)
         d3exit(LIJBND_ERR, fmturlnm(Lija.kl), (long)ix->ict);
      Lija.rowe = Lija.lijval + ix->ul2.b.rslen;
      work = ix->ul2.b.rrlen - (Lija.lijval%ix->ul2.b.rrlen);
      Lija.boxe = Lija.lijval + min(work, ix->ul2.b.rslen);
      }

   /* Advance horizontally, check for end of row or box */
   if ((Lija.lijval += ix->stride) >= Lija.rowe) {
      /* New row--undo the X motion.  Off the end of the world? */
      if ((Lija.lijval += ix->ul2.b.rrlen - ix->ul2.b.rslen) >=
            ix->srcnelt) {
         /* Force otherLij to skip rest of conntype */
         Lija.jsyn = ix->nc; return NO; }
      Lija.boxe += ix->ul2.b.rrlen;
      Lija.rowe += ix->ul2.b.rrlen;
      }
   else if ((unsigned long)Lija.lijval >= Lija.boxe) return NO;

   return (Lija.lijval != Lija.satest);

   } /* End lb2b() */


/*---------------------------------------------------------------------*
*     lb2bia - 'Box' distribution in input array                       *
*                                                                      *
*     Rev, 06/23/08, GNR - Work with x,y coords, add bounds check per  *
*                          new rule, using lbqb code as a guide        *
*---------------------------------------------------------------------*/

int lb2bia(struct CONNTYPE *ix) {

   if (Lija.jsyn == Lija.lb2s) {
      /* Here if first call to routine in any subarbor */
      if (Lija.fstx >= (signed long)ix->iawxsz ||
            Lija.fsty >= (signed long)ix->iawysz ||
            Lija.fstx + ix->ul2.b.rslen <= 0 ||
            Lija.fsty + ix->nry <= 0) {
         /* Force otherLij to skip rest of conntype */
         Lija.jsyn = ix->nc; return NO; }
      Lija.xcur = Lija.fstx;
      Lija.ycur = Lija.fsty;
      Lija.rowe = Lija.fstx + ix->ul2.b.rslen;
      Lija.boxe = Lija.fsty + ix->nry;
      }

   if ((Lija.xcur += ix->stride) >= Lija.rowe) {
      Lija.xcur = Lija.fstx;
      if (++Lija.ycur >= Lija.boxe) {
         /* Should be impossible to get here if nc count is OK */
         d3exit(LIJBND_ERR, fmturlnm(Lija.kl), (long)ix->ict);
         }
      }

   if ((unsigned long)Lija.xcur >= ix->iawxsz ||
       (unsigned long)Lija.ycur >= ix->iawysz) return NO;

   Lija.lijval = Lija.xcur << RP->ky1 | Lija.ycur;
   return YES;

   } /* End lb2bia() */


/*---------------------------------------------------------------------*
*     lbqb - 'Box' distribution in a cell layer or VT or TV            *
*                                                                      *
*     Case II (new style) version--first connection may be out         *
*        of bounds but coords will be stored in fstx,fsty,fstr.        *
*        The bounds tests are considerably more complicated            *
*        than for lb2b because it is not enough to know that           *
*        lijval has strayed above or below the repertoire limits.      *
*     As per bounds checking rules in d3go, bounds are checked         *
*        here for repertoire or virtual cell input.  Any skips         *
*        must be inserted in correct order, as lbqb may be used        *
*        with 'L' or 'M' Cij generating codes.                         *
*---------------------------------------------------------------------*/

int lbqb(struct CONNTYPE *ix) {

   long work,work2;                 /* Scratch space */

   if (Lija.jsyn == Lija.lb2s) {
      /* Here if first call to routine in any subarbor */
      if ((Lija.fstx >= ix->srcngx) || (Lija.fsty >= ix->srcngy)) {
         /* Force otherLij to skip rest of conntype */
         Lija.jsyn = ix->nc; return NO; }

      work = Lija.fsty*ix->ul2.b.rrlen;
      if (work <= ix->ul2.b.rtlen) {
         Lija.jsyn = ix->nc; return NO; }

      Lija.lijval = Lija.fstr + Lija.fstx*ix->srcnel;
      if ((work2 = Lija.lijval + ix->ul2.b.rslen) <= 0) {
         Lija.jsyn = ix->nc; return NO; }

      Lija.lijval += work;       /* What Lij would be after lijbr1 */
      Lija.rowe = work + work2;     /* Save row loop limit */
      Lija.boxe = work;             /* Save left row boundary */
      } /* End if first call */

   /* Rev, 06/16/10, GNR - '>' here corrected to '>=' */
   if ((Lija.lijval += ix->stride) >= Lija.rowe) {
      /* Undo X motion and advance one along Y.  If runs off the
      *  bottom of the source array, skip rest of conntype.  */
      if ((Lija.lijval += ix->ul2.b.rrlen - ix->ul2.b.rslen) >=
            ix->srcnelt) {
         Lija.jsyn = ix->nc; return NO; }
      Lija.rowe += ix->ul2.b.rrlen;
      Lija.boxe += ix->ul2.b.rrlen;
      }

   /* Skip if still below start of source array */
   if (Lija.lijval < 0 || Lija.lijval < Lija.boxe) return NO;

   /* If past end of current row, omit */
   if ((Lija.boxe + ix->ul2.b.rrlen) <= Lija.lijval) return NO;

   return (Lija.lijval != Lija.satest);

   } /* End lbqb() */


/*---------------------------------------------------------------------*
*     lb2c - 'Crow's foot' distribution within a normal repertoire.    *
*                                                                      *
*     Distribution is centered on the cell selected first.             *
*     Any connections generated out-of-bounds are rejected.            *
*     Doesn't make sense to use with subarbors, so not coded.          *
*     (New version in V1K--selects a group in a square of size         *
*        nrx*nry and then selects any cell in that group)              *
*     (Changed 11/25/86 from upper-left to centered for OLS.  This     *
*        will not cause incompat with old decks because C didn't       *
*        exist before V1K--but real problem is to fix B)               *
*     (Changed 01/12/87 add rounding so result is symmetric)           *
*     (Changed 08/13/06 to remove assumption of nel < S16 and          *
*        ngrp < S15--results will differ from old runs.)               *
*     Same routine is now used for Case I and Case II                  *
*---------------------------------------------------------------------*/

int lb2c(struct CONNTYPE *ix) {

   long wx,wy;                /* Scratch space */
   ui32 low32a;

   /* Make a random number that's negative half the time */
   wx = jm64nb(udev(&Lija.wlseed)<<1,ix->nrx,&low32a);
   if ((long)low32a < 0) wx++;
   wx += Lija.fstx;
   /* Return NO if X out of range -- high or negative */
   if ((unsigned long)wx >= ix->iawxsz) {
      udev(&Lija.wlseed);     /* Make deterministic */
      return NO; }

   /* Make another random number that's negative half the time */
   wy = jm64nb(udev(&Lija.wlseed)<<1,ix->nry,&low32a);
   if ((long)low32a < 0) wy++;
   wy += Lija.fsty;
   /* Return NO if Y out of range -- high or negative */
   if ((unsigned long)wy >= ix->iawysz) return NO;

   Lija.lijval = ix->srcnel*(wy*ix->srcngx + wx) +
      uwhi(jmuw(low32a, (ui32)ix->srcnel));

   return (Lija.lijval != Lija.satest);

   } /* End lb2c() */


/*---------------------------------------------------------------------*
*     lb2cia - 'Crow's foot' distribution in input array.              *
*                                                                      *
*     Selects points uniformly distributed in an nrx*nry box centered  *
*        on the pixel chosen by the lb1 routine.  Revised in V2C to    *
*        have center of distribution, not corner, on first point       *
*        chosen.  This in turn requires rounding.                      *
*     The following comments are obsolete in light of the new bounds   *
*        checking rules:                                               *
*        (See design notes, p. 33, for an analysis showing that all    *
*        out-of-bounds points can be detected by d3go provided we      *
*        require nry <= nsy; ht <= nsy.)                               *
*        (There was a choice to be made, whether connections outside   *
*        the scan window but inside the input array should be rejected.*
*        In this code such connections are considered valid on grounds *
*        (1) probably more realistic biologically, (2) anyway, user    *
*        can get rid of them by using smaller nux, nuy, (3) allows     *
*        bia,bwa and cia,cwa to be the same routines.  Leading 1's in  *
*        negative Y's are kept--automatically corrects X in d3go.)     *
*     Currently using same routine for case I and case II.             *
*     Rev, 06/23/08, GNR - Restore bounds checking via iawxsz,iawysz   *
*---------------------------------------------------------------------*/

int lb2cia(struct CONNTYPE *ix) {

   long wx,wy;                /* Scratch space */
   ui32 low32x,low32y;

   wx = Lija.fstx + jm64nb(udev(&Lija.wlseed)<<1,ix->nrx,&low32x);
   if ((long)low32x < 0) wx++;
   if ((unsigned long)wx >= ix->iawxsz) return NO;

   /* Use the low order half for Y */
   wy = Lija.fsty + jm64nb(low32x,ix->nry,&low32y);
   if ((long)low32y < 0) wy++;
   if ((unsigned long)wy >= ix->iawysz) return NO;

   Lija.lijval = wx << RP->ky1 | wy;
   return YES;

   } /* End lb2cia() */


/*---------------------------------------------------------------------*
*     lb2cvg - 'Crow's foot' distribution within virtual touch or TV   *
*                                                                      *
*     Distribution is centered on the cell selected first.             *
*        Any connections generated out-of-bounds are rejected.         *
*        VT is treated as having special case--one cell/group.         *
*     Same routine is now used for Case I and Case II.                 *
*---------------------------------------------------------------------*/

int lb2cvg(struct CONNTYPE *ix) {

   long wx,wy;                /* Scratch space */
   ui32 low32a;

   /* Make a random number that is negative half the time */
   wx = jm64nb(udev(&Lija.wlseed)<<1,ix->nrx,&low32a);
   if ((long)low32a < 0) wx++;
   wx += Lija.fstx;
   /* Return NO if X out of range -- high or negative */
   if ((unsigned long)wx >= ix->iawxsz) {
      udev(&Lija.wlseed);     /* Make deterministic */
      return NO; }

   /* Make another random number that's negative half the time */
   wy = jm64nb(udev(&Lija.wlseed)<<1,ix->nry,&low32a);
   if ((long)low32a < 0) wy++;
   wy += Lija.fsty;
   /* Return NO if Y out of range -- high or negative */
   if ((unsigned long)wy >= ix->iawysz) return NO;

   Lija.lijval = wx + wy*ix->srcngx;

   return YES;

   } /* End lb2cvg() */


/*---------------------------------------------------------------------*
*     lb2d - 'Diagonal' distribution in a cell layer or VT or TV       *
*                                                                      *
*     Class I and Class II versions are now identical.  First lijval   *
*        must be reconstructed from coords, as may be out-of-bounds.   *
*     As per bounds checking rules in d3go, bounds are checked         *
*        here for repertoire or VG input.  Any skips must be in        *
*        correct order, as lb2d may be used with 'L' or 'M'.           *
*     There is no Y loop, as the caller counts total connections.      *
*---------------------------------------------------------------------*/

int lb2d(struct CONNTYPE *ix) {

   if (Lija.jsyn == Lija.lb2s) {
      /* Here if first call to routine in any subarbor.
      *  Save current coords (x in cells, y in groups).
      *  Don't need to set lijval--lbsa does that for us.  */
      Lija.xcur = Lija.rowx = Lija.fstx*ix->srcnel + Lija.fstr;
      Lija.ycur = Lija.rowy = Lija.fsty;
      Lija.rowct = ix->nrx;      /* Start counter */
      }

   if (!--Lija.rowct) {
      /* New row--update the row start coords */
      Lija.rowct = ix->nrx;
      Lija.lijval += ix->ul2.d.yincr;
      Lija.xcur = Lija.rowx += ix->nhy;
      Lija.ycur = Lija.rowy += ix->nvy;
      }
   else {
      /* Update the current coords */
      Lija.lijval += ix->ul2.d.xincr;
      Lija.xcur += ix->nhx;
      Lija.ycur += ix->nvx;
      }

   if ((unsigned long)Lija.xcur >= ix->ul2.d.rowsz) return NO;
   if ((unsigned long)Lija.ycur >= ix->iawysz)      return NO;

   return (Lija.lijval != Lija.satest);

   } /* End lb2d() */


/*---------------------------------------------------------------------*
*     lb2dia - 'Diagonal' distribution in input array                  *
*                                                                      *
*     When dxincr,dyincr are prepared, leading 1's in negative         *
*        numbers must be kept so Y overflow corrects X etc.            *
*     Rev, 06/23/08, GNR - Restore bounds checking via xymaskc         *
*---------------------------------------------------------------------*/

int lb2dia(struct CONNTYPE *ix) {

   /* Initialize rowct on first call to routine in subarbor */
   if (Lija.jsyn == Lija.lb2s)
      Lija.rowct = ix->nrx;

   if (!--Lija.rowct) {
      /* New row--restart the row counter */
      Lija.rowct = ix->nrx;
      Lija.lijval += ix->ul2.d.yincr;
      }
   else
      Lija.lijval += ix->ul2.d.xincr;

   return (Lija.lijval & RP->xymaskc ? NO : YES);

   } /* End lb2dia() */


/*---------------------------------------------------------------------*
*     lb2q -  Square annular connection distribution.                  *
*                                                                      *
*     Selects with uniform probability from within an annulus of       *
*        inner dimensions nhx * nhy groups and outer dimensions        *
*        nrx * nry groups centered on the cell selected first.         *
*        A cell is chosen at random from the selected group.           *
*        Any connections generated out-of-bounds are rejected.         *
*     Similarly to the lb2c routines, no provision is made to clone    *
*        Lij selections across subarbors.                              *
*     N.B.  This code is free of the ancient assumption that there     *
*        are never more than 32767 cells in a group nor > 32767 groups *
*        in a repertoire at a cost of one multiply.  It consumes       *
*        exactly one random number per call, no discards.              *
*     N.B.  To save memory, this code also handles the VG case.  There *
*        are two avoidable, but harmless, mults. by srcnel at the end. *
*---------------------------------------------------------------------*/

int lb2q(struct CONNTYPE *ix) {

   long wg,wx,wy;                /* Group, x coord, y coord */
   ui32 low32a;                  /* Cell selector */

   /* Get a random number and separate into group and cell selectors */
   wg = jm64nb(udev(&Lija.wlseed),ix->ul2.q.qatot,&low32a);

   if ((wx = wg - ix->ul2.q.qhalr) >= 0) {

      /* Selected group is in top or bottom block.  Calculate y coord
      *  according to sign of low-order random bits.  Check bounds.  */
      ldiv_t qrm = ldiv(wx, ix->nrx);
      register long row = ix->ul2.q.qhnry - qrm.quot;
      if (low32a & MINUS0) wy = Lija.fsty - row;
      else                 wy = Lija.fsty + row;
      if ((unsigned long)wy >= ix->iawysz) return NO;

      /* Selected y is acceptable.  Now calculate x coord and check. */
      wx = Lija.fstx + qrm.rem - ix->ul2.q.qhnrx;
      if ((unsigned long)wx >= ix->iawxsz) return NO;
      } /* End top-bottom block */

   else {

      /* Selected group is in left or right block.  Calculate x coord
      *  according to sign of low-order random bits.  Check bounds.  */
      ldiv_t qrm = ldiv(wg,(long)ix->nhy);
      register long col = ix->ul2.q.qhnrx - qrm.quot;
      if (low32a & MINUS0) wx = Lija.fstx - col;
      else                 wx = Lija.fstx + col;
      if ((unsigned long)wx >= ix->iawxsz) return NO;

      /* Selected x is acceptable.  Now calculate y coord and check. */
      wy = Lija.fsty + qrm.rem - ix->ul2.q.qhnhy;
      if ((unsigned long)wy >= ix->iawysz) return NO;
      } /* End left-right block */

   Lija.lijval = ix->srcnel*(wx + ix->srcngx*wy) +
      jm64nh(ix->ul2.q.qsrcnel2,(low32a & LONG_MAX));

   return YES;

   } /* End lb2q() */


/*---------------------------------------------------------------------*
*     lb2qia - Square annular connection selection from input array.   *
*                                                                      *
*     Selects with uniform probability from within an annulus of       *
*        inner dimensions nhx * nhy pixels and outer dimensions        *
*        nrx * nry pixels centered on the pixel selected first.        *
*     Similarly to the lb2c routines, no provision is made to clone    *
*        Lij selections across subarbors.                              *
*     Rev, 06/23/08, GNR - Restore bounds checking via iawxsz,iawysz   *
*---------------------------------------------------------------------*/

int lb2qia(struct CONNTYPE *ix) {

   long wp,wx,wy;                /* pixel, x coord, y coord */
   unsigned long low32a;         /* Side selector */

   /* Get a random number and separate into pixel and side selectors */
   wp = jm64nb(udev(&Lija.wlseed),ix->ul2.q.qatot,&low32a);

   if ((wx = wp - ix->ul2.q.qhalr) >= 0) {

      /* Selected pixel is in top or bottom block.  Calculate y
      *  coord according to sign of low-order random bits.  */
      ldiv_t qrm = ldiv(wx, ix->nrx);
      register long row = ix->ul2.q.qhnry - qrm.quot;
      if (low32a & MINUS0) wy = Lija.fsty - row;
      else                 wy = Lija.fsty + row;

      /* Calculate x coord */
      wx = Lija.fstx + qrm.rem - ix->ul2.q.qhnrx;
      } /* End top-bottom block */

   else {

      /* Selected pixel is in left or right block.  Calculate x
      *  coord according to sign of low-order random bits.  */
      ldiv_t qrm = ldiv(wp, (long)ix->nhy);
      register long col = ix->ul2.q.qhnrx - qrm.quot;
      if (low32a & MINUS0) wx = Lija.fstx - col;
      else                 wx = Lija.fstx + col;

      /* Calculate y coord */
      wy = Lija.fsty + qrm.rem - ix->ul2.q.qhnhy;
      } /* End left-right block */

   if ((unsigned long)wx >= ix->iawxsz) return NO;
   if ((unsigned long)wy >= ix->iawysz) return NO;

   Lija.lijval = (wx << RP->ky1) + wy;
   return YES;

   } /* End lb2qia() */


/*---------------------------------------------------------------------*
*     lbsa - Subarbor generation routine--repertoire or VG source      *
*                                                                      *
*     When connection number is not a multiple of ix->nsa, executes    *
*        normal lb2 routine via pointer in lijbr3.  Otherwise,         *
*        advances to next subarbor, modifying lijval, fstx, fsty, and  *
*        fstr so that subsequent lb2 generation will proceed from the  *
*        new origin.                                                   *
*---------------------------------------------------------------------*/

int lbsa(struct CONNTYPE *ix) {

   /* If not at end of subarbor, execute lijbr3 routine */
   if (Lija.jsyn < Lija.nsas) return ix->lijbr3(ix);

   Lija.lb2s += ix->nsa;
   Lija.isas = Lija.nsas;
   Lija.nsas += ix->nsa;

   /* Time to advance.  Works like lbqb, except box width is
   *  in nsax and design optimizes memory rather than speed.  */

   Lija.wlseed = Lija.svlseed;      /* Reuse seed sequence */
   if (++Lija.jsa < ix->nsax)
      ++Lija.fstx;                  /* Increment along row */
   else {
      ++Lija.fsty;                  /* Increment to next row */
      Lija.fstx -= (ix->nsax - 1);  /* Restart row position */
      Lija.jsa = 0;                 /* Restart row counter */
      }

   /* Check for out of bounds, then convert coords to Lij */
   if ((unsigned long)Lija.fstx >= ix->iawxsz) return NO;
   if ((unsigned long)Lija.fsty >= ix->iawysz) return NO;
   Lija.lijval = Lija.fstr + ix->srcnel *
      (Lija.fstx + ix->srcngx * Lija.fsty);

   return YES;

   } /* End lbsa() */


/*---------------------------------------------------------------------*
*     lbsaia - Subarbor generation routine--input array source         *
*                                                                      *
*     When connection number is not a multiple of ix->nsa, executes    *
*        normal lb2 routine via pointer in lijbr3.  Otherwise,         *
*        advances to next subarbor, modifying lijval, fstx, fsty, and  *
*        fstr so that subsequent lb2 generation will proceed from the  *
*        new origin.                                                   *
*     Per usual IA input conventions, no bounds checking.              *
*---------------------------------------------------------------------*/

int lbsaia(struct CONNTYPE *ix) {

   /* If not at end of subarbor, execute lijbr3 routine */
   if (Lija.jsyn < Lija.nsas) return ix->lijbr3(ix);

   Lija.lb2s += ix->nsa;
   Lija.isas = Lija.nsas;
   Lija.nsas += ix->nsa;

   /* Time to advance.  Works like lbqb, except box width is in
   *  nsax and design optimizes memory rather than speed.  */

   Lija.wlseed = Lija.svlseed;      /* Reuse seed sequence */
   if (++Lija.jsa < ix->nsax)
      ++Lija.fstx;                  /* Increment along row */
   else {
      ++Lija.fsty;                  /* Increment to next row */
      Lija.fstx -= (ix->nsax - 1);  /* Restart row position */
      Lija.jsa = 0;                 /* Restart row counter */
      }

   Lija.lijval = ((Lija.fstx << RP->ky1) + Lija.fsty) & RP->xymask;

   return YES;

   } /* End lbsaia() */


/*---------------------------------------------------------------------*
*            lbsai - Subarbor generation routine--Option I             *
*                                                                      *
*     Treats each subarbor as an independent arbor, running the lb1    *
*        routine on the first connection and the normal lb2 routine    *
*        via the pointer in lijbr3 for the rest.                       *
*---------------------------------------------------------------------*/

int lbsai(struct CONNTYPE *ix) {

   /* If not at end of subarbor, execute lijbr3 routine */
   if (Lija.jsyn < Lija.nsas) return ix->lijbr3(ix);

   /* New subarbor.  Update synapse count, run lijbr1 routine */
   Lija.lb2s += ix->nsa;
   Lija.isas = Lija.nsas;
   Lija.nsas += ix->nsa;

   return ix->lijbr1(ix);

   } /* End lbsai() */


/*---------------------------------------------------------------------*
*            lbsal - Subarbor generation routine--Option L             *
*                                                                      *
*     When connection number is not a multiple of ix->nsa, repeats     *
*     same Lij obtained last time.  Otherwise, performs normal lb2     *
*     routine.  This is basically intended to allow 3 consecutive      *
*     connections to sample three colors from the same input, but      *
*     there may be other uses, so it is allowed with REPSRC input.     *
*---------------------------------------------------------------------*/

int lbsal(struct CONNTYPE *ix) {

   /* If not at end of subarbor, return previous result */
   if (Lija.jsyn < Lija.nsas) return Lija.lijrc;

   /* Advance to next subarbor, but do not advance Lija.lb2s,
   *  because lb2 routines are cycling just once in this case.  */
   Lija.isas = Lija.nsas;
   Lija.nsas += ix->nsa;

   /* Now execute normal lb2 routine */
   return ix->lijbr3(ix);

   } /* End lbsal() */

