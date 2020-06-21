/* (c) Copyright 1995, The Rockefeller University *11113* */
/***********************************************************************
*                               mat33.h                                *
*                                                                      *
*  This file contains prototypes for the functions implemented in file *
*  mat33.c.  These functions manipulate 3-vectors and 3x3 matrices.    *
*                                                                      *
*  Initial version, 02/08/95, G.N. Reeke                               *
***********************************************************************/

void copy3(float *v1, float *va);               /* Copy vector */
float dot33(float *v1, float *v2);              /* Dot product */
void cross33(float *v1, float *v2, float *va);  /* Cross product */
float triple(float *v1, float *v2, float *v3);  /* Triple product */
float len3(float *v);                           /* Vector length */
void norm3(float *v, float *va);                /* Make unit vector */

