/* (c) Copyright 1998-2008, The Rockefeller University *11115* */
/* $Id: rfqsame.c 61 2017-01-16 19:26:44Z  $ */
/***********************************************************************
*                  ROCKS Library - C Implementation                    *
*                               rfqsame                                *
*                                                                      *
*     This routine, rfqsame, determines whether two open RFdef blocks  *
*  point to the same file.  It is mainly intended for use by cryout()  *
*  to determine whether SPOUT should be automatically activated.  It   *
*  may not necessarily work with any two arbitrary files.              *
*                                                                      *
*     Note:  In DOS, the only way that was found to determine whether  *
*  stdout and stderr were the same, was to do an lseek to stdout, see  *
*  whether offset in stderr changes, then lseek stdout to previous     *
*  offset.  This code has been removed here because it is obsolete.    *
*                                                                      *
*  INPUT:  Pointers to two open RFdef blocks.                          *
*  OUTPUT: TRUE if the two RFdefs are the same, otherwise FALSE.       *
*                                                                      *
************************************************************************
*  V1A, 09/17/98, GNR - New routine                                    *
*  Rev, 05/08/99, GNR - Reposition stat.h to eliminate GCC error       *
*  Rev, 07/17/99, GNR - Expect stdin always to be read/write           *
*  Rev, 03/04/07, GNR - Work with revised rfdef                        *
*  ==>, 09/25/08, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#ifdef UNIX
#include <sys/stat.h>
#endif
#include "rfdef.h"

int rfqsame(struct RFdef *rfd1, struct RFdef *rfd2) {

#ifdef UNIX
/*---------------------------------------------------------------------*
*                   File checking for UNIX systems                     *
*---------------------------------------------------------------------*/

   struct stat stb1,stb2;

   /* Make sure both files are open */
   if (!(rfd1->iamopen & IAM_ANYOPEN) ||
       !(rfd2->iamopen & IAM_ANYOPEN)) abexit(52);
   /* Retrieve device and inode for both files */
   if (fstat(rfd1->frwd, &stb1) < 0) abexit(52);
   if (fstat(rfd2->frwd, &stb2) < 0) abexit(52);

   return (stb1.st_dev  == stb2.st_dev &&
           stb1.st_ino  == stb2.st_ino &&
           stb1.st_rdev == stb2.st_rdev);

#else
   return NO;        /* Safer bet, we don't know the answer */
#endif

   } /* End rfqsame() */
