/* (c) Copyright 2011, The Rockefeller University *11116* */
/* $Id: bbdcexsi.c 28 2017-01-13 20:35:15Z  $ */
/***********************************************************************
*                      BBD (Client Side) Package                       *
*                             bbdcexsi.c                               *
*                     Provide extra server input                       *
*                                                                      *
*  The purpose of this routine is to provide a text string that will   *
*  be inserted in the input to a server (typically CNS) after the      *
*  BBDHOST card created by bbdcminp and before the contents of the     *
*  specified control file.  It is assumed that this string will        *
*  contain a ROCKS EXECUTE card that will provide values for variable  *
*  symbols contained in the control file.  For example, it can be      *
*  used to pass a run number to CNS so that the CNS output files       *
*  automatically carry the same run number as the client.  bbdcexsi()  *
*  must be called by the client before bbdcinit() is called.  (It is   *
*  a separate program in order to avoid changing the bbdcinit() call   *
*  for existing programs.)                                             *
*                                                                      *
*  Synopsis:                                                           *
*  void bbdcexsi(const char *exstring)                                 *
*                                                                      *
*  Argument:                                                           *
*     exstring A string containing valid control information for the   *
*              server.  It should end with a newline ('\n') character. *
*              It is copied, so it need not be static.                 *
*                                                                      *
*  Error Handling:                                                     *
*     All errors are terminal and result in a call to abexit() with    *
*     a suitable message.  There is nothing useful the caller can do.  *
************************************************************************
*  V1A, 01/17/11, GNR - New program                                    *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sysdef.h"
#include "bbd.h"

struct BBDComData BBDcd;               /* Common BBD data struct */

void bbdcexsi(const char *exstring) {

   size_t lexsi;                       /* Length of exsi string */

   if (BBDcd.exsi || BBDcd.ppfeff)
      abexitm(BBDcErrExsi, "bbdcexsi() called too late or twice");

   if ((lexsi = strlen(exstring)) == 0) return;    /* JIC */
   BBDcd.exsi = (char *)malloc(lexsi);
   strcpy(BBDcd.exsi, exstring);

   } /* End bbdcexsi() */
