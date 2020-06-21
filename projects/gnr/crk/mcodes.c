/* (c) Copyright 1989-2009, The Rockefeller University *11116* */
/* $Id: mcodes.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                        MCODES, MCODOP[LIHC]                          *
*                         Match option codes                           *
*                                                                      *
*     Routine mcodes, which is part of the ROCKS library, is used to   *
*  initialize a flag word according to a string of one-letter option   *
*  codes entered by a user.  The 'keys' argument specifies a list of   *
*  possible option codes.  These codes map onto bits in the flag word  *
*  in order from right to left.  Each character in the 'data' string   *
*  is compared with all of the characters in 'keys'.  When the n'th    *
*  code from the right of 'keys' is found in 'data', the n'th bit from *
*  the right of 'flagword' is selected.  The use of the selected bits  *
*  depends on the first character in the data as follows:              *
*     +        OR the result with any existing codes in 'flagword',    *
*     -        Clear bits in 'flagword' corresponding to given codes,  *
*   other      Replace 'flagword' with the selected bits.              *
*  If the data string is literally "0", then 'flagword' is set to 0    *
*  unless "0" is itself a recognized option code.  If any character    *
*  in the data is not matched, an error message is generated,          *
*  RK.iexit is set nonzero, and mcodes returns an error code of 1.     *
*                                                                      *
*  Usage: int mcodes(char *data, char *keys, ui32 *flagword)           *
*                                                                      *
*  Arguments:                                                          *
*     'data' is the character string to be interpreted (max 32 chars). *
*                                                                      *
*     'keys' is a string that specifies the codes to be recognized in  *
*        the data (max 32 chars). Blanks may be used to skip over bits *
*        that are not to be set in 'flagword' by any code.  While any  *
*        characters other than '+' and '-' may occur in 'keys', in     *
*        general only letters and numbers should be used to avoid      *
*        complications with scan().                                    *
*                                                                      *
*     'flagword' is the control word to be initialized by mcodes.      *
*        If a NULL pointer is passed, the only action is to store      *
*        the key information for subsequent calls to mcodop().         *
*                                                                      *
*  Return values: 0 if matching was successful, otherwise 1.           *
*                                                                      *
*  To apply the same transformation to additional flagwords, or to     *
*  flagwords of type other than ui32, the mcodop family of routines    *
*  can be used.                                                        *
*                                                                      *
*  Usage: void mcodopl(ui32 *flagword)                                 *
*         void mcodopi(unsigned int   *flagword)                       *
*         void mcodoph(unsigned short *flagword)                       *
*         void mcodopc(unsigned char  *flagword)                       *
*                                                                      *
*  Arguments:                                                          *
*     'flagword' is a control word to be modified in the same way      *
*        as the 'flagword' argument to the last mcodes() call.         *
*                                                                      *
*  To handle special situations, the selected bits are left in         *
*  RK.mcbits and a code indicating the selected action is left         *
*  in RK.mckpm.  Values of this code are defined in rkxtra.h.          *
*                                                                      *
************************************************************************
*  Rev, 04/19/89, GNR - Prototype moved to rkxtra.h                    *
*  Rev, 05/21/95, GNR - Always accept key '0' alone--returns 0         *
*  Rev, 11/08/97, GNR - Add plus,minus convention, save RK.mcbits      *
*  Rev, 10/17/01, GNR - Expand mcodop() to four variants               *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
*  Rev, 08/11/09, GNR - Change type of flagword from long to ui32      *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

/*=====================================================================*
*                               mcodop                                 *
*=====================================================================*/

void mcodopl(ui32 *lflag) {

   switch (RK.mckpm) {
   case RK_MCSET:  *lflag = RK.mcbits; break;
   case RK_MCOR:   *lflag |= RK.mcbits; break;
   case RK_MCCLR:  *lflag &= ~RK.mcbits; break;
   case RK_MCZERO: *lflag = 0; break;
      }

   } /* End mcodopl() */

void mcodopi(unsigned int *iflag) {

   ui32 lflag = (ui32)(*iflag);
   mcodopl(&lflag);
   *iflag = (unsigned int)lflag;

   } /* End mcodopi() */

void mcodoph(unsigned short *hflag) {

   ui32 lflag = (ui32)(*hflag);
   mcodopl(&lflag);
   *hflag = (unsigned short)lflag;

   } /* End mcodoph() */

void mcodopc(unsigned char *cflag) {

   ui32 lflag = (ui32)(*cflag);
   mcodopl(&lflag);
   *cflag = (unsigned char)lflag;

   } /* End mcodopc() */


/*=====================================================================*
*                               mcodes                                 *
*=====================================================================*/

int mcodes(char *data, char *keys, ui32 *flagword) {

   ui32 result = 0;
   char unmatched[33]; char *nextbad = unmatched; int numbad = 0;
   int kdata = strcmp(data,"0"); /* Compare before incrementing data */
   int kpm = RK_MCSET;           /* Flag for leading plus or minus */
   int lkeys = strlen(keys);

   if (lkeys > BITSPERUI32) abexit(25);

   if (data[0] == '+')      { kpm = RK_MCOR;  ++data; }
   else if (data[0] == '-') { kpm = RK_MCCLR; ++data; }

   while (*data) {            /* Inspect each data character */
      char *k = keys+lkeys;
      register ui32 keybit = 1;
      while (k > keys) {      /* Match with all keys */
         if (*data == *--k) { result |= keybit; goto keyfound; }
         keybit += keybit; }
      /* Key not matched.  If key is a lonesome '0', return 0.
      *  Otherwise, record bad character for error message.  */
      if (kdata == 0) kpm = RK_MCZERO;
      else if (++numbad < sizeof unmatched) *nextbad++ = *data;
keyfound: data++;
      } /* End loop over data string */

   RK.mcbits = result;        /* Save results */
   RK.mckpm  = kpm;
   if (flagword)
      mcodopl(flagword);      /* Perform flagword update */

/* If any characters not matched, generate error message */

   if (numbad == 0) return 0;
   *nextbad = '\0';
   ermark(RK_MARKFLD);
   cryout(RK_P1," ***Option codes \"",RK_LN1+18,unmatched,
      RK_CCL+numbad,"\" not recognized.",RK_CCL+17,NULL);
   RK.iexit |= 1;
   return 1;

   } /* End mcodes */
