/* (c) Copyright 2017, The Rockefeller University *21114* */
/* $Id: deccol.c 31 2017-04-13 21:51:22Z  $ */
/***********************************************************************
*                              deccol()                                *
*                                                                      *
* SYNOPSIS:                                                            *
*     int deccol(bgrv *pbgr, const char *pcol);                        *
*                                                                      *
* DESCRIPTION:                                                         *
*  This functions decodes an ASCII drawing color from the set          *
*  accepted by the crk plot library into its red,green,blue            *
*  components, returned as an bgrv struct.                             *
*                                                                      *
* ARGUMENTS:                                                           *
*  pbgr        Ptr to an bgrv structure where result is returned.      *
*  pcol        Ptr to the ASCII color name to be decoded.              *
*                                                                      *
* PROTOTYPED IN:                                                       *
*  plots.h                                                             *
*                                                                      *
* NOTE:                                                                *
*  This routine will henceforth be accepted as the official definition *
*  of the color set accepted by this library.  The currently defined   *
*  named colors are given in the table below.                          *
*                                                                      *
* RETURN VALUES:                                                       *
*  0 if color decoding was successful, 1 if the pcol argument was      *
*  not an acceptable color name.                                       *
************************************************************************
*  Version 1, 03/31/17, GNR - New program                              *
*  ==>, 04/13/17, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <ctype.h>
#include <string.h>
#include "sysdef.h"
#include "plots.h"

#define MXLNAME   8        /* Max recognized part of color name */
#define DIG2BYT   4        /* Shift hex digit to byte */
#define NFIXNMS   (sizeof(cdefs)/sizeof(struct cdef))

#define D2B(d) ((d) >= 'A' ? (d) - 'A' + 10 : (d) - '0')

int deccol(bgrv *pbgr, const char *pcol) {

   struct cdef {
      char cname[MXLNAME];
      bgrv cvalu;
      };
   int i;
   char upnm[MXLNAME];

   /* Color defs are max 8 bits as used in 24-bit color specs */
   static struct cdef cdefs[] = {
      { "WHITE",    { 0xff, 0xff, 0xff } },
      { "BLACK",    {    0,    0,    0 } },
      { "BLUE",     { 0xff,    0,    0 } },
      { "CYAN",     { 0xff, 0xff,    0 } },
      { "MAGENTA",  { 0xff,    0, 0xff } },
      { "VIOLET",   { 0xff,    0, 0x80 } },
      { "ORANGE",   {    0, 0xa6, 0xff } },
      { "GREEN",    {    0, 0xff,    0 } },
      { "YELLOW",   {    0, 0xff, 0xff } },
      { "RED",      {    0,    0, 0xff } } };

/* Try to check the various possibilities in order of
*  descending usage frequency.  */

   for (i=0; i<MXLNAME; ++i) {
      upnm[i] = (char)toupper(pcol[i]);
      if (upnm[i] == '\0') break;
      }

   /* Check for my 12-bit color code */
   if (upnm[0] == 'X') {
      pbgr->bb = D2B(upnm[1]) << DIG2BYT;
      pbgr->gg = D2B(upnm[2]) << DIG2BYT;
      pbgr->rr = D2B(upnm[3]) << DIG2BYT;
      return 0;
      }

   if (upnm[0] == 'Z') {
      pbgr->bb = D2B(upnm[1]) << DIG2BYT | D2B(upnm[2]);
      pbgr->gg = D2B(upnm[3]) << DIG2BYT | D2B(upnm[4]);
      pbgr->rr = D2B(upnm[5]) << DIG2BYT | D2B(upnm[6]);
      return 0;
      }

   for (i=0; i<NFIXNMS; ++i) {
      if (!strncmp(pcol, cdefs[i].cname, MXLNAME)) {
         *pbgr = cdefs[i].cvalu;
          return 0; }
      }

   /* No match, error return */
   return 1;
   } /* End deccol() */






