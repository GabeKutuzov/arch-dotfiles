/* (c) Copyright 1995, The Rockefeller University *11114* */
/***********************************************************************
*                               mat33.c                                *
*     Routines for basic operations on 3-vectors and 3x3 matrices      *
*                                                                      *
************************************************************************
*  V1A, 02/07/95, GNR - Initial version                                *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include "mat33.h"

/*---------------------------------------------------------------------*
*                                copy3                                 *
*                                                                      *
*  Copies a 3-vector to the second argument                            *
*---------------------------------------------------------------------*/

void copy3(float *v1, float *va) {

   va[0] = v1[0];
   va[1] = v1[1];
   va[2] = v1[2];
   } /* End copy3 */

/*---------------------------------------------------------------------*
*                                dot33                                 *
*                                                                      *
*  Performs dot product of two 3-vector arguments.                     *
*---------------------------------------------------------------------*/

float dot33(float *v1, float *v2) {

   return (v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2]);
   } /* End dot33 */

/*---------------------------------------------------------------------*
*                               cross33                                *
*                                                                      *
*  Performs cross product of two 3-vector arguments.                   *
*  Third argument is a pointer to the result vector.                   *
*---------------------------------------------------------------------*/

void cross33(float *v1, float *v2, float *va) {

   va[0] = v1[1]*v2[2] - v1[2]*v2[1];
   va[1] = v1[2]*v2[0] - v1[0]*v2[2];
   va[2] = v1[0]*v2[1] - v1[1]*v2[0];
   } /* End cross33 */

/*---------------------------------------------------------------------*
*                               triple                                 *
*                                                                      *
*  Computes triple product (determinant) of three 3-vector arguments.  *
*---------------------------------------------------------------------*/

float triple(float *v1, float *v2, float *v3) {

   return (v1[0]*v2[1]*v3[2] + v1[1]*v2[2]*v3[0] + v1[2]*v2[0]*v3[1] -
          (v1[0]*v2[2]*v3[1] + v1[1]*v2[0]*v3[2] + v1[2]*v2[1]*v3[0]));
   } /* End triple */

/*---------------------------------------------------------------------*
*                                len3                                  *
*                                                                      *
*  Computes length of a 3-vector.                                      *
*---------------------------------------------------------------------*/

float len3(float *v) {

   return (float)sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
   } /* End len3 */

/*---------------------------------------------------------------------*
*                                norm3                                 *
*                                                                      *
*  Normalizes a 3-vector.                                              *
*  Second argument is a pointer to the result vector, which may be     *
*     the same as the input vector.                                    *
*---------------------------------------------------------------------*/

void norm3(float *v, float *va) {

   double d = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
   if (d != 0.0) d = 1.0/d;
   va[0] = d*v[0];
   va[1] = d*v[1];
   va[2] = d*v[2];
   } /* End norm3 */
