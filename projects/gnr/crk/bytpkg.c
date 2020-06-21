/* (c) Copyright 1998-2008, The Rockefeller University *11115* */
/* $Id: bytpkg.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*              BYTPKG - Byte Array Manipulation Package                *
*                                                                      *
*  These routines are designed to provide C support for the routines   *
*  of the same names included in old FORTRAN programs. They work the   *
*  same as the IBM assembler versions written by GNR. These routines   *
*  will normally be replaced in new programs by inline C code.         *
*                                                                      *
*  Usage:                                                              *
*                                                                      *
*  void bytmov(void *array1, long bytlen, void *array2)                *
*     Copies 'bytlen' bytes from 'array2' to 'array1'.                 *
*                                                                      *
*  void bytior(void *array1, long bytlen, void *array2)                *
*     Computes the logical 'OR' of 'bytlen' bytes of 'array2' with     *
*     'array1', leaving the result in 'array1'.                        *
*                                                                      *
*  void bytand(void *array1, long bytlen, void *array2)                *
*     Computes the logical 'AND' of 'bytlen' bytes of 'array2' with    *
*     'array1', leaving the result in 'array1'.                        *
*                                                                      *
*  void bytxor(void *array1, long bytlen, void *array2)                *
*     Computes the logical 'XOR' of 'bytlen' bytes of 'array2' with    *
*     'array1', leaving the result in 'array1'.                        *
*                                                                      *
*  void bytnxr(void *array1, long bytlen, void *array2)                *
*     Computes the complement of the logical 'XOR' of 'bytlen'         *
*     bytes of 'array2' with 'array1', leaving the result in 'array1'. *
*                                                                      *
************************************************************************
*  V1A, 03/28/98, GNR - Broken out from original bapkg.c               *
*  ==>, 03/01/06, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stdlib.h>
#include "sysdef.h"
#include "bapkg.h"            /* Make sure prototypes get checked */

/*=====================================================================*
*                               bytmov                                 *
*=====================================================================*/

void bytmov(void *array1, long bytlen, void *array2) {
   register char *pa1 = (char *)array1;
   register char *pa2 = (char *)array2;
   char *paend = pa1 + bytlen;
   while (pa1 < paend) *pa1++ = *pa2++;
   }

/*=====================================================================*
*                               bytior                                 *
*=====================================================================*/

void bytior(void *array1, long bytlen, void *array2) {
   register char *pa1 = (char *)array1;
   register char *pa2 = (char *)array2;
   char *paend = pa1 + bytlen;
   while (pa1 < paend) *pa1++ |= *pa2++;
   }

/*=====================================================================*
*                               bytand                                 *
*=====================================================================*/

void bytand(void *array1, long bytlen, void *array2) {
   register char *pa1 = (char *)array1;
   register char *pa2 = (char *)array2;
   char *paend = pa1 + bytlen;
   while (pa1 < paend) *pa1++ &= *pa2++;
   }

/*=====================================================================*
*                               bytxor                                 *
*=====================================================================*/

void bytxor(void *array1, long bytlen, void *array2) {
   register char *pa1 = (char *)array1;
   register char *pa2 = (char *)array2;
   char *paend = pa1 + bytlen;
   while (pa1 < paend) *pa1++ ^= *pa2++;
   }

/*=====================================================================*
*                               bytnxr                                 *
*=====================================================================*/

void bytnxr(void *array1, long bytlen, void *array2) {
   register char *pa1 = (char *)array1;
   register char *pa2 = (char *)array2;
   char *paend = pa1 + bytlen;
   while (pa1 < paend) *pa1++ ^= ~*pa2++;
   }

