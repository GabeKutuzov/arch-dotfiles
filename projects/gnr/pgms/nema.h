/* (c) Copyright 1995, The Rockefeller University *11113* */
/***********************************************************************
*                               NEMA.h                                 *
*                                                                      *
*  This file contains prototypes for functions that either read or     *
*  generate parts of an ACR_NEMA image file.  The actual details of    *
*  what is in an ACR_NEMA file are in NEMA.HDR.  This header allows    *
*  those details to be hidden from client programs.                    *
*                                                                      *
*  Initial version, 01/26/95, G.N. Reeke                               *
***********************************************************************/

int makeNEMA(int nfd, char *vers, char *inst, char *dept, char *note,
   long study, long series, long idim, long loden, long hiden,
   long ncol, long nrow, long nsec, double xpix, double ypix,
   double zpix);
