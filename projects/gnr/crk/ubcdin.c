/* (c) Copyright 1966-2009, The Rockefeller University *11115* */
/* $Id: ubcdin.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                              ubcdin.c                                *
*                                                                      *
*  ROCKS unsigned integer numerical input conversion routines          *
*  Adapted from IBM Assembler versions by G.N. Reeke                   *
*  (This is the obsolete version, replaced by wbcdin(), but retained   *
*  for compatibility with older, pre-64-bit applications.  It will     *
*  compile and run in a 64-bit system, but only input 32-bit values.)  *
*                                                                      *
************************************************************************
*  V00, xx/xx/66, GNR - IBM 360 Assembler version                      *
*  V1A, 10/17/88, GNR - Convert from my IBM Assembler version          *
*  V1B, 01/08/89, GNR - Add RK_BYTE bit to IBCDIN call                 *
*  V2A, 02/12/89, GNR - Separate from BCDOUT, IBCDWT                   *
*  Rev, 04/19/89, GNR - Incorporate rkxtra.h                           *
*  Rev, 01/16/92, GNR - Correct GFP bug when scale = 31                *
*  Rev, 01/25/97, GNR - Add RK_ZTST and DECIN, UBCDIN functions        *
*  V2B, 01/25/03, GNR - Add forced decimal scaling                     *
*  V2C, 03/08/08, GNR - More detailed error messages                   *
*  ==>, 03/16/08, GNR - Last date before committing to svn repository  *
*  Rev, 02/16/09, GNR - Perform value adjustment before overflow test  *
*  Rev, 05/25/09, GNR - Split out from bcdin.c                         *
***********************************************************************/

/*---------------------------------------------------------------------*
*        Usage:                                                        *
*---------------------------------------------------------------------*/

/*
   For characters to unsigned long integer (or general fixed point):
         ui32 ubcdin(ui32 ic, char *field)

   'ic' is the processing code described below.
   'field' is an array containing the string to be converted.

   The operation code 'ic' is the sum of four quantities:
         ic = scale + dec + op + width - 1

      'scale' is required only for noninteger fixed-point variables.
         The scale code is (s << RK_BS), where RK_BS is 24 and 's'
         is the number of bits to the right of the binary point in
         the result (maximum, 31).

      'dec' specifies decimal scaling. If the RK_DSF bit (=0x00800000)
         is set, scaling is forced, i.e. the value entered by the user
         is always multipled by the scale.  If RK_DSF is not set, sca-
         ling only occurs when no decimal point is found in the input.
         'dec' is equal to (d << RK_DS), where RK_DS is 16.  The scale
         is (10**(-d)) with (-64 < d <= 63) (i.e. 'dec' on input is 7
         bits including the 3 bits used for the exponent width by the
         output routines--but note that if the magnitude of the net of
         'd' and any input exponent or decimal is too large the result
         may report an out-of-range or zero error).  Thus, positive
         values of 'd' correspond to the number of places to the right
         of the inserted decimal when no decimal is found in the input.
         Users normally expect 'd' to be 0.

      'op' specifies the operation to be performed.  It is made
         up by adding the desired codes from the following table.
         The argument types and format codes are independent.

         Hexadecimal Defined
           Value      Name    Function
         ----------  -------  ------------------------------------------
         0x00008000  RK_GFP   General Fixed Point.  Result is a fixed
                              point value with 'scale' fraction bits.
         0x00004000  RK_ZTST  Zero Test.  Generate an error if the
                              numerical value of the result is zero.
         0x00002000  RK_CTST  Character Test.  Generate an error if non-
                              numeric characters found in input string.
         0x00001000  RK_QPOS  Query Positive.  Generate an error if the
                              numerical value of the result is negative.
         0x00000800  RK_INT   Integer (UBCDIN only).  This bit has no
                              function, but is retained for compati-
                              bility with the FORTRAN version.
         0x00000600  RK_IORF  I (integer) format.  Exponential format
                              is also accepted when this code is used.
         0x00000400  RK_OCTF  O (octal) conversion.  Octal input
                              conversion is not currently available.
         0x00000200  RK_HEXF  H (hexadecimal) conversion.
         0x00000100  RK_SNGL  Single precision.  Checks whether over-
                              flow would occur if result truncated by
                              caller to a short int.  If input format
                              is hexadecimal, generates an error if
                              more digits are found than needed to
                              represent a short integer.
         0x00000080  RK_BYTE  Byte precision.   Checks whether over-
                              flow would occur if result truncated by
                              caller to a byte.

      'width' is the number of characters in the input field to be
         examined, less one.  (The maximum width is RK_MXWD+1 = 32.)

   Because portability is the goal, accuracy will be no better
   than can be achieved with ordinary double operations.
*/

/*---------------------------------------------------------------------*
*        Error Procedures:                                             *
*---------------------------------------------------------------------*/

/*
      1) All control parameters are treated modulo their maximum
         allowed values.

      2) Input errors generate calls to ivckmsg.  This calls ermark
         to mark the error if input was scanned, then calls cryout to
         print an appropriate message.  If an input value overflows,
         it is set to the largest value that can be expressed in the
         requested numeric type.  A special dispensation is made for
         short and byte types:  If the overflow is exactly to the next
         higher integer, it is set to the maximum value but no error
         is flagged.  If an input value is zero and the RK_ZTST bit is
         set, the value is set to a small positive value, possibly
         preventing a divide-by-zero error in the caller.
*/

/*---------------------------------------------------------------------*
*        Restrictions:                                                 *
*---------------------------------------------------------------------*/

/*
      1) Exponents larger than EXP_SIZE can not be handled.

      2) Program assumes 8 bits per byte.

      3) Floating and integer variables must be stored
         with same byte order.

      4) The representations of the characters '0' through
         '9' and 'A' through 'F' must be consecutive codes.
*/

/*---------------------------------------------------------------------*
*        Global definitions:                                           *
*---------------------------------------------------------------------*/

#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "rockv.h"
#include "rkxtra.h"

extern ui16 RK_inflags;       /* Various flags */


/*=====================================================================*
*                                                                      *
*                           Function UBCDIN                            *
*                                                                      *
*  This version of UBCDIN has been implemented as a call to DECIN      *
*  followed by scaling and fixing--this gives the easiest portable     *
*  way to get general fixed-point numbers, although not the fastest.   *
*=====================================================================*/

ui32 ubcdin(ui32 ic, char *field) {

   ui32 uval;

   RK_Inflags = 0;

/* Handle hexadecimal input */

   if ((ic & RK_IORF) == RK_HEXF) {
      if (ic & RK_BYTE) {
         byte bval;
         hexin(ic, field, &bval, sizeof(byte));
         uval = (ui32)bval; }
      else if (ic & RK_SNGL) {
         ui16 sval;
         hexin(ic, field, (byte *)&sval, sizeof(ui16));
         uval = (ui32)sval; }
      else
         hexin(ic, field, (byte *)&uval, sizeof(ui32));
      }

/* Decimal input.  First set up values needed for overflow tests */

   else {
      double value,tval,tval1;
      int iint;

      if      (ic & RK_BYTE) iint = BITSPERBYTE,
         tval = (double)BYTE_MAX;
      else if (ic & RK_SNGL) iint = (HSIZE*BITSPERBYTE),
         tval = (double)UI16_MAX;
      else                   iint = BITSPERUI32,
         tval = (double)UI32_MAX;

      /* Use decin to get double precision value,
      *  but force error if value is negative.  */
      value = bcdin((ic|RK_QPOS), field);

/* Deal with fixed-point scaling */

      if (ic & RK_GFP) {      /* Scale general fixed point */
         register int bscl = (int)(ic >> RK_BS) & 31;
         value = value * (double)(1UL<<bscl) + 0.5;
         iint -= bscl;
         }
      else                    /* Integer: Give error if frac */
         if (RK_Inflags & NONZERO_FRAC) ivckmsg0(field, IVCK_INTEGER);

/* Test for overflow against next larger integer, so round-off
*  fraction that will be dropped later will not interfere.  */

      tval1 = tval + 1.0;
      if (value >= tval1) {
         if ((value > tval1) || ((ic & (RK_BYTE|RK_SNGL)) == 0))
            ivckmsg2(field, iint);
         uval = (ui32)tval; }
      else
         uval = (ui32)value;

      } /* End non-hexadecimal input processing */

/* If a forbidden zero value was found, generate an error
*  message and replace it with a value of one.  */

   if (ic & RK_ZTST && uval == 0) {
      ivckmsg0(field, IVCK_APPNZ);
      uval = 1;
      }

   return uval;
   } /* End ubcdin() */
