/* (c) Copyright 2008, The Rockefeller University *11115* */
/* $Id: jaswt.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                                jaswt                                 *
*                                                                      *
*  This is a test program to try to see what is needed to use type     *
*  long long on the SGI.                                               *
*                                                                      *
***********************************************************************/

typedef long long si64;

si64 jasw(si64 x, si64 y) {

   return x + y;

   } /* End jaswt() */
