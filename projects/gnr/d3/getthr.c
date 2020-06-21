/* (c) Copyright 2017, The Rockefeller University *11116* */
/* $Id: getthr.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               getthr                                 *
*                                                                      *
*  This is a kwjreg routine to read a threshold value or scale from a  *
*  control card when the binary scale depends on the context. It takes *
*  account of whether or not negative values are allowed and what kind *
*  of item is involved. These cases are coded in the first argument as *
*  a text string that can be passed from kwscan or inform, as follows: *
*  (1) The first character is 'V' if negatives are never allowed, 'C'  *
*  if negatives are not allowed in COMPAT=C mode, otherwise no code.   *
*  (2) The second character is:                                        *
*     'R' if the item is a threshold and the input is known to be a    *
*  repertoire type but the scale depends on COMPAT=C (i.e. GCONNs),    *
*     'S' if the item is a scale factor and the scale is S20 if in mV  *
*  range and input is a virtual cell, otherwise S24, or                *
*     'U' if the variable is a value type (B8IH),                      *
*     'X' if the item is a threshold and the scale depends on the      *
*  input source as well as the s(i) range.  With OLDRANGE, the scale   *
*  is always S14 and mV modifiers are not accepted.  With mV range, if *
*  the source (code stored in RP0->ksrctyp) is virtual input (IA, TV,  *
*  sense, etc.), the scale is S14 if the user enters a plain number,   *
*  but S7 if a "mV" qualifier is present.  If the source is REPSRC,    *
*  the scale is S7 with possible mV units modifier.                    *
*  (3) With 'S' or 'X', if the input type is not yet known, as indi-   *
*  cated by ksrctyp == BADSRC, the value is temporarily stored in the  *
*  RP0->GDfD struct (MASTER card) or RP0->LDfD (card after a CELLTYPE  *
*  but before a CONNTYPE) for later scaling and truncation.  The type  *
*  of the item is indicated by a letter code following 'S' or 'X' as   *
*  documented in the DEFTHR struct def in misc.h.  Values entered as   *
*  fractions go in ufrt, mV go in umVt, both are allowed in parens.    *
*                                                                      *
*  Design note:  getthr() currently puts everything in ix->Cn when     *
*  we have OLDRANGE, otherwise uses [LG]DfD system for items with      *
*  variable scales.  This means d3dflt2 etc. have to store defaults    *
*  in different places according to s(i) range.  If we add many more   *
*  of these, it will be better to always use [LG]DfD with new or old   *
*  range and eliminate the test where defaults are copied at g2conn,   *
*  but then getthr() will have a more complicated format selection     *
*  problem.                                                            *
*----------------------------------------------------------------------*
*                           chkscl, chkthr                             *
*                                                                      *
*  These routines have as argument a stored scale or threshold, shift  *
*  it with rounding by the difference between the stored scale and the *
*  second argument, check for overflow and return the shorter result.  *
*                                                                      *
************************************************************************
*  R77, 12/28/17, GNR - Break out from d3g2conn.c to this new file.    *
*  R77, 02/02/18, GNR - Expand to allow separate frac,mV entries.      *
*---------------------------------------------------------------------*/

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"

void getthr(char *pfmt, void *datitm) {      /* kwjreg J1 */

   char *pgf;
   int novck = 1;
   int oldbssel = bscomclr(RK_BSUNITS);
   /* Define names for the different format cases and the eqscan()
   *  format codes for each--the 'V' is skipped over if not needed */
   enum thrfmts { mVRange, FrRange, mVOpt,
      Scale24, Scale20, ValRange, UnkScale };
   static char *gtfmt[] = {  "VB7IH$mV", "VB14IH", "VB14IH$?7mV",
      "VB24IJ", "VB20IJ", "VB8IH", "VB" qqv(FBsc) "IW" };

   /* Check for negative test */
   if (pfmt[0] == 'V')
      novck = 0, ++pfmt;
   else if (pfmt[0] == 'C') {
      if (RP->compat & OLDRANGE) novck = 0;
      ++pfmt; }

   /* Select format string according to last code */
   switch (pfmt[0]) {
   case 'R':               /* Repertoire input test */
      pgf = (RP->compat & OLDRANGE) ? gtfmt[FrRange] : gtfmt[mVRange];
      break;
   case 'S':               /* Aij scale factor (ix->Cn.scl) */
      if (RP->compat & OLDRANGE) pgf = gtfmt[Scale24];
      else if (RP0->ksrctyp == BADSRC) {
         /* Locate temp storage for item */
         datitm = (RP0->notmaster ? RP0->LDfD.us.ascl :
            RP0->GDfD.us.ascl) + (pfmt[1] - 'A');
         pgf = gtfmt[UnkScale];
         }
      else pgf = (RP0->ksrctyp == REPSRC) ?
         gtfmt[Scale24] : gtfmt[Scale20];
      break;
   case 'U':
      pgf = gtfmt[ValRange];
      break;
   case 'X':               /* Input type may not be known yet */
      if (RP->compat & OLDRANGE) { pgf = gtfmt[FrRange]; break; }
      /* mV range so turn on RK_BSUNITS, make mV acceptable in inputs */
      bscomset(RK_BSUNITS);
      if (RP0->ksrctyp == BADSRC) {
         /* Item encountered before CONNTYPE card.  We now allow either
         *  (parens optional) or both scales (parens required).  Code
         *  will not flag two with or two without units as errors.  */
         char lea[SCANLEN1];
         int  gotunits;
         int  itmsleft = 2;
         do {
            scanck(lea, RK_REQFLD,
               ~(RK_COMMA|RK_LFTPAREN|RK_INPARENS|RK_RTPAREN));
            gotunits = cntrln(lea, SCANLEN);
            itmsleft = (!(RK.scancode & RK_INPARENS) ||
               (RK.scancode & RK_RTPAREN)) ? -1 : itmsleft - 1;
            if (gotunits) {
               datitm = (RP0->notmaster ? RP0->LDfD.umVt.amVthr :
                  RP0->GDfD.umVt.amVthr) + (pfmt[1] - 'A');
               pgf = gtfmt[mVRange]; }
            else {
               datitm = (RP0->notmaster ? RP0->LDfD.ufrt.afrthr :
                  RP0->GDfD.ufrt.afrthr) + (pfmt[1] - 'A');
               pgf = gtfmt[FrRange]; }
            scanagn();
            eqscan(datitm, pgf + novck, 0);
            } while (itmsleft > 0);
         if (itmsleft == 0) ermark(RK_TOOMANY);
         goto RestoreOldbssel;
         }
      else pgf = (RP0->ksrctyp == REPSRC) ?
         gtfmt[mVRange] : gtfmt[mVOpt];
      break;
   default:                /* Error if bad format passed */
      d3exit(pfmt, GETTHR_ERR, 0);
      } /* End format code switch */
   eqscan(datitm, pgf + novck, RK_EQCHK|RK_BEQCK);
RestoreOldbssel:
   bscompat(oldbssel);

   } /* End getthr() */

/*---------------------------------------------------------------------*
*                               chkscl                                 *
*---------------------------------------------------------------------*/

si32 chkscl(si64 tscl, int bscl, char *name) {

   int dscl;
   /* Special case:  SI64_SGN indicates default, pass it through */
   if (qsw(jrsw(tscl, SI64_SGN)) == 0) return SI32_SGN;
   if ((dscl = (int)FBsc - bscl)) tscl = jsrrsw(tscl, dscl);
   if (qsw(jrsl(abs64(tscl), SI32_MAX)) > 0) {
      char itmlim[8];
      int kovf = 1 << ((BITSPERUI32-1) - bscl);
      wbcdwt(&kovf, itmlim, 2<<RK_DS|RK_LFTJ|RK_IORF|RK_NINT|7);
      cryout(RK_E1, "0***MAGNITUDE OF SCALE FROM AN EARLIER "
         "CONTROL CARD EXCEEDS ", RK_LN2, itmlim, RK_CCL, NULL);
      }
   return swlo(tscl);
   }  /* End chkscl() */
