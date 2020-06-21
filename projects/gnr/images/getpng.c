/* (c) Copyright 2017, The Rockefeller University *11114* */
/* $Id: getpng.c 33 2017-05-03 16:14:57Z  $ */
/***********************************************************************
*                              getpng.c                                *
*                                                                      *
*                  Routines for reading .png images                    *
*----------------------------------------------------------------------*
*                               getpng                                 *
*                                                                      *
*  Routine to read a png image file and return a pointer to the data.  *
*                                                                      *
*  Prototyped in:                                                      *
*                                                                      *
*  Synopsis:                                                           *
*                                                                      *
*  Arguments:                                                          *
*                                                                      *
*  Returns:                                                            *
*                                                                      *
*----------------------------------------------------------------------*
*                               freepng                                *
*                                                                      *
*  Routine to free memory allocated for an image read by getpng.       *
*                                                                      *

************************************************************************
*  V1A, 05/18/17, GNR - Based on example supplied with libpng          *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <png.h>
#include "sysdef.h"
#include "plots.h"


