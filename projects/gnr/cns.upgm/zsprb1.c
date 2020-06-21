/* (c) Copyright 2001, The Rockefeller University *11113* */
/* $Id: zsprb1.c 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                               zsprb1                                 *
*                                                                      *
*  This is a test program for the SPRB user exit from CNS.  It uses    *
*  the parameters to calculate an arbitrary, but easily checked,       *
*  value for the probe level.                                          *
*                                                                      *
*  ==>, 02/17/05, GNR - Last mod before committing to svn repository   *
***********************************************************************/

float zsprb1(long iseq, long icell, long series, long trial,
      long cycle, float sprb) {

   return (sprb + 0.1 * (float)((icell + trial)%10));

   } /* End zsprb1() */
