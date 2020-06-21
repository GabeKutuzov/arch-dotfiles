/* (c) Copyright 2007, The Rockefeller University *11114* */
/* $Id: sjrbar1.c 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                               sjrbar1                                *
*                                                                      *
*  This is a user-written "getsjr" routine for use when called from    *
*  CNS via the UPROG option on the CONNTYPE card.  It picks up a       *
*  colored pixel from a virtual camera, subtracts the mean value       *
*  of that pixel as passed in the third argument, and squares the      *
*  result.  Calling arguments are arranged in CNS sjbr routine such    *
*  that this routine needs no knowledge of CNS data structures or      *
*  headers.  Things like the left shift are just hard coded.           *
*                                                                      *
*  This routine assumes 24-bit color.  For other cases, make a         *
*  different version and change the '1' in the name to another         *
*  number.  Note that 'csel' is 1 for blue, 2 for green, 3 for red.    *
*                                                                      *
*  Note that the pixel is shifted left 6 bits to yield a (S14) value.  *
*                                                                      *
*  V1A, 10/25/07, GNR - New routine                                    *
*  ==>, 10/25/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

typedef unsigned char byte;

long sjrbar1(byte *image, long Lij, long rbar, int csel) {
   
   byte *pS = image + Lij + Lij + Lij;
   long tsj = ((long)pS[3-csel] << 6) - rbar;

   return (tsj * tsj >> 14);

   } /* End sjrbar1() */
