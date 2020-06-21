/* (c) Copyright 2003-2017, The Rockefeller University *11115* */
/* $Id: muscan.c 66 2018-03-15 19:01:23Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                         MUSCAN and BSCOMPAT                          *
*                                                                      *
************************************************************************
*                                                                      *
*                    BSCOMPAT, BSCOMCLR, BSCOMSET                      *
*                                                                      *
*  These three functions bzw set the RK.bssel binary scale selection   *
*  byte from the argument, clear specified bits in RK.bssel, or set    *
*  specified bits.  All three return the previous setting (so the user *
*  can restore it later if desired).  Allowed bits have names of form  *
*  "RK_BSxxxxx" and are defined in rocks.h.  RK_BSUNITS modifies       *
*  muscan() function as described below.  The other 3 bits cause       *
*  inform() and kwscan(), when a 'B' binary scale code is followed by  *
*  two values separated by a '/', '|', or '?', bzw, to use the second  *
*  scale rather than the normal first scale.  This is provided as a    *
*  convenience to facilitate implementation of variables with multiple *
*  possible binary scales in applications.  These routines are placed  *
*  in this source file because of the inform/kwscan commonality.       *
*                                                                      *
************************************************************************
*                                                                      *
*                               MUSCAN                                 *
*                                                                      *
*  This function is used by inform() and kwscan() to process metric    *
*  unit specifiers in scanned input.  All results are returned via     *
*  argument pointers.  If there is no units specifier, or the data     *
*  field is a pure number, returns without doing anything.             *
*                                                                      *
*  Arguments:                                                          *
*     ppfmt    Ptr to ptr to format specifier.  The pointer in the     *
*              caller is advanced to the next comma after the units    *
*              specifier (if there is one).                            *
*     pdat     Ptr to input data field (scanned or fixed format).      *
*     pcc      Ptr to cc field that will be used to call [iw]bcdin().  *
*              If there is a units override in the data, the default   *
*              decimal field is replaced by the correct scale factor   *
*              and the RK_CTST bit is turned off so [wi]bcdin will not *
*              report a bad character in the data field.  The length   *
*              field is used to fence the input data scan.             *
*                                                                      *
*  Action:                                                             *
*     A units specification in an inform or kwscan format consists of  *
*  a '$' followed by, optionally, a '?', a binary scale modifier of    *
*  the form [+]q, a default units multiplier (a,f,p,u,m,c,d,-,M,K,G,T, *
*  where a=atto, f=femto, etc. and '-' is required for unmodified base *
*  units), followed by a literal string to specify the name of the     *
*  units to be entered. '?' means a units spec in the user data is     *
*  acceptable only if RK.bssel bit RK_BSUNITS is set.  'q' if entered  *
*  is an integer 1-9 that is subtracted from (added to if preceded by  *
*  '+') the binary scale in the 'pcc' code whenever the user data has  *
*  a units spec.  Two unit specs can be separated by a '/', e.g.       *
*  'cm/-sec'.  In that case, both units in the quotient can be scaled  *
*  individually.  The user units spec must match the muscan call       *
*  template or an error is generated.                                  *
*     For example, "$mV" would indicate that a number of millivolts    *
*  is to be entered.  The user can then enter a bare number, which     *
*  is scaled by any default decimal, or a number followed by any mul-  *
*  tiplier and a 'V', in which case the input is scaled appropriately, *
*  e.g. "4V" would be read in as 4000mV.  If the unit name ends with   *
*  a digit (2,3,etc.) it indicates that the units are taken to that    *
*  power, and the input is scaled accordingly, e.g. if the units are   *
*  "$cm2" an input of "30mm2" would be read as 0.3cm2.                 *
*     Second example:  "$7mV" would indicate as before that a bare     *
*  number entered is left alone, but if a number followed by 'mV'      *
*  is entered, the binary scale is reduced by 7 and any implied        *
*  decimal units multipliers are also applied as described above.      *
*                                                                      *
*  Errors:                                                             *
*     abexit(21)  The format specification starts with an unknown      *
*              metric multiplier letter (abexit, no message).          *
*     abexit(119) The binary scale modifier caused the input scale     *
*              to be outside the range 0 - 63 (abexit, no message).    *
*     RK_UNITERR  The user data has an units specifier that does       *
*              not match the one required by the application.          *
*                                                                      *
************************************************************************
*  V1A, 01/30/03, GNR - New program                                    *
*  Rev, 03/31/03, GNR - Allow divisive units with no numerator         *
*  ==>, 05/22/08, GNR - Last date before committing to svn repository  *
*  Rev, 06/13/09, GNR - Bug fix -- must decr. psfx if units match      *
*  Rev, 08/11/09, GNR - Change type of pcc to *ui32 for PCLUX64        *
*  R66, 12/22/17, GNR - Add '?' and binary scale spec to '$' codes     *
***********************************************************************/

#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"
#include "rkxtra.h"

/*=====================================================================*
*                    BSCOMPAT, BSCOMCLR, BSCOMSET                      *
*=====================================================================*/

int bscompat(int kbs) {
   int oldbssel = (int)RK.bssel;
   RK.bssel = (byte)kbs;
   return oldbssel;
   } /* End bscompat() */

int bscomclr(int kbs) {
   int oldbssel = (int)RK.bssel;
   RK.bssel &= ~(byte)kbs;
   return oldbssel;
   } /* End bscomclr() */

int bscomset(int kbs) {
   int oldbssel = (int)RK.bssel;
   RK.bssel |= (byte)kbs;
   return oldbssel;
   } /* End bscomset() */


/*=====================================================================*
*                               MUSCAN                                 *
*=====================================================================*/

void muscan(char **ppfmt, char *pdat, ui32 *pcc) {

   static char mupfx[] = "afpnu\xE6mcd-DHKMGTPE";
   static schr muscl[] = { -18, -15, -12, -9, -6, -6, -3, -2, -1, 0,
      1, 2, 3, 6, 9, 12, 15, 18 };

/* If not at a '$' in the format, there is nothing to do */

   if (**ppfmt == '$') {

      char *pfmt = *ppfmt + 1;   /* Point to char after '$' */
      char *psfx = pdat;         /* Ptr to data unit suffix */
      char *pmpd,*pmpf;          /* Ptrs to multipler prfxs */
      char *pune;                /* Ptr to end of unit name */
      ui32 ds,os = 0;            /* Data, overall scales    */
      int  dbs = 0;              /* Change in binary scale  */
      int  power;                /* Power unit is taken to  */
      int  atslash;              /* Scan is at a slash      */

/* Find end of unit spec and update caller's format ptr */

      for (pune=pfmt;
         *pune && *pune != ',' && *pune != ')'; ++pune) ;
      *ppfmt = pune;

/* If the input item doesn't have a units specifier, there is
*  nothing further to do.  Test with cntrln() because we might
*  be in unterminated data */

      if (cntrln(pdat, (*pcc&RK_MXWD)+1) == NUMBRET) return;

/* Error if there is a unit specifier in the input item, but muscan
*  was called with '?' and RK.bssel RK_BSUNITS bit is off.  */

      if (*pfmt == '?') {
         if (!(RK.bssel & RK_BSUNITS)) goto muscan_error;
         ++pfmt; }

/* If there is a binary scale modifier, store it and advance
*  the format pointer past it.  */

      if (*pfmt == '+')
         { dbs = (pfmt[1] - '0') << RK_BS; pfmt += 2; }
      else if (isdigit((int)*pfmt))
         { dbs = ('0' - pfmt[0]) << RK_BS; pfmt += 1; }

/* Find end of user's data field.  Use first NUL from the left
*  or first whitespace that follows a non-whitespace--don't
*  assume rightmost non-NUL character is the right one.  */

      {  char *pdte = pdat + (*pcc & RK_MXWD);  /* Data end */
         int kc, klc = FALSE;
         while (psfx <= pdte && *psfx) {
            kc = isspace(*psfx);
            if (klc) { if (kc) break; }
            else { if (!kc) klc = TRUE; }
            ++psfx;
            }
         }

/* Scan user data from right for matching unit specifiers */

      do {
         power = isdigit(*(pune-1)) ? *(pune-1) - '0' : 1;
         while (pune > pfmt && *(pune-1) != '/' &&
            psfx > pdat && *--psfx == *--pune) ;
         /* Note: The next 2 tests look redundant with the ones
         *  in the 'while' above, but they are not, because the
         *  pointers may or may not have decremented.  */
         if (atslash = *(pune-1) == '/')  /* Assignment intended */
            power = -power;
         /* Error if mismatch or ran out of user data while matching */
         else if (pune > pfmt) goto muscan_error;
         if (*pune == *psfx) {   /* Unit names match */
            /* Decrease psfx if not atslash so correct length
            *  will be stored into pcc below  */
            if (!atslash) --psfx; }
         else {                  /* Unit names do not match */
            /* Pick up multiplier in format--error if not found */
            /* Assignment intended in next line */
            if (!(pmpf = strchr(mupfx, (int)*pune))) abexit(21);
            ds = muscl[pmpf-mupfx];
            /* Pick up multiplier in data--OK if not there */
            if (atslash && *psfx == '/')
               ;
            /* Assignment intended in next line */
            else if (pmpd = strchr(mupfx, (int)*psfx))
               ds -= muscl[pmpd-mupfx], --psfx;
            else if (*psfx != '.' && !isdigit((int)*psfx))
               goto muscan_error;
            os += power*ds;
            }
         --pune;
         } while (atslash);

      /* Force subsequent [i]bcdin call to use our scale and
      *  shortened width so units are not taken as data.  */
      dbs += (int)(*pcc & (RK_FORCE-RK_BSCL));
      if (dbs < 0 || dbs >= RK_FORCE) abexit(119);
      *pcc = *pcc & ~((RK_FORCE-RK_D)|RK_MXWD) | RK_DSF | (ui32)dbs |
         (os << RK_DS & (RK_DSF-RK_D)) | (psfx - pdat);

      } /* End dollar sign found */

   return;

/* Here on various user errors.  Turn off RK_CTST to
*  prevent redundant error from [iuw]bcdin() */

muscan_error:
   ermark(RK_MULTERR);
   *pcc &= ~RK_CTST;
   return;

   } /* End muscan() */

