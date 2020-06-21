/* (c) Copyright 1988-2008, The Rockefeller University *11115* */
/* $Id: bapkg.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*               BAPKG - Byte Array Manipulation Package                *
*                                                                      *
*  These routines are designed to provide C support for the routines   *
*  of the same names included in old FORTRAN programs.  They are the   *
*  same as the IBM assembler versions written by GNR except that all   *
*  arguments are required.  These routines will normally be replaced   *
*  in new code by the standard C library functions memset, etc.        *
*                                                                      *
*  Usage:                                                              *
*                                                                      *
*  void baclr(void *array, long bytlen)                                *
*     Zeros 'bytlen' bytes beginning at the location pointed           *
*     to by 'array'.                                                   *
*                                                                      *
*  void baset(void *array, long bytlen)                                *
*     Sets 'bytlen' bytes to all ones beginning at the location        *
*     pointed to by 'array'.                                           *
*                                                                      *
*  void bamove(void *array1, long idispl1, long bytlen,                *
*        void *array2, long idispl2)                                   *
*     Copies 'bytlen' bytes beginning 'idispl2' bytes from the         *
*     beginning of 'array2' to a location 'idispl1' bytes from the     *
*     begining of 'array1'.  Unlike the assembler version, 'idispl2'   *
*     is not optional.  The diplacements 'idispl1' and 'idispl2' are   *
*     allowed to be negative.  'bytlen' may be any positive number.    *
*                                                                      *
*  void bainit(void *array, long idispl1, long bytlen,                 *
*        void *value, long idispl2)                                    *
*     Copies the 8 bits found at a location 'idispl2' bytes beyond     *
*     'value' into 'bytlen' locations beginning 'idispl1' bytes        *
*     beyond the location pointed to by 'array'.  Unlike the           *
*     assembler version, the 'idispl2' argument is not optional.       *
*                                                                      *
************************************************************************
*  V1A, 06/28/88, SCD                                                  *
*  V1B, 01/27/89, JWT - Use void pointers                              *
*  Rev, 04/19/89, GNR - Use bapkg.h                                    *
*  Rev, 12/18/91, GNR - Add bitmcp, rewrite bitxxx routines            *
*  Rev, 02/26/92, GNR - Add bytnxr                                     *
*  Rev, 07/06/92, GNR - Change bytxxx args to void                     *
*  Rev, 08/08/92, GNR - Use shift,AND instead of divide,remainder      *
*  Rev, 03/28/98, GNR - Split into bapkg, bitpkg, bytpkg files         *
*  ==>, 07/27/02, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "bapkg.h"            /* Make sure prototypes get checked */

/*=====================================================================*
*                                baclr                                 *
*=====================================================================*/

void baclr(void *array, long bytlen) {
   register char *pa = (char *)array;
   char *paend = pa + bytlen;
   while (pa < paend) *pa++ = 0;
   }

/*=====================================================================*
*                                baset                                 *
*=====================================================================*/

void baset(void *array, long bytlen) {
   register char *pa = (char *)array;
   char *paend = pa + bytlen;
   while (pa < paend) *pa++ = (char)~0;
   }

/*=====================================================================*
*                               bamove                                 *
*=====================================================================*/

void bamove(void *array1, long idispl1, long bytlen, void *array2,
      long idispl2) {
   register char *pa1 = (char *)array1 + idispl1;
   register char *pa2 = (char *)array2 + idispl2;
   char *paend = pa1 + bytlen;
   while (pa1 < paend) *pa1++ = *pa2++;
   }

/*=====================================================================*
*                               bainit                                 *
*=====================================================================*/

void bainit(void *array1, long idispl1, long bytlen, void *value,
      long idispl2) {
   register char *pa = (char *)array1 + idispl1;
   char *paend = pa + bytlen;
   char v = *((char *)value + idispl2);
   while (pa < paend) *pa++ = v;
   }

