/* (c) Copyright 2011, The Rockefeller University *11115* */
/* $Id: rkprintf.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                       RKPRINTF.H Header File                         *
*                              11/26/11                                *
*                                                                      *
*             Common variables and function declarations               *
*       private to ROCKS implementation of C library functions         *
*  printf(), fprintf(), sprintf(), snprintf(), and added rfprintf()    *
*                                                                      *
*  Note:  Caller should use prototypes of printf(), etc. from stdio.h. *
************************************************************************
*  V1A, 11/26/11, GNR - New header file                                *
*  ==>, 11/27/11, GNR - Last date before committing to svn repository  *
***********************************************************************/

#ifndef RKPRINTF_HDR_INCLUDED
#define RKPRINTF_HDR_INCLUDED

#define OMIT_ROCKV

#include "stddef.h"
#include "stdarg.h"
#include "stdio.h"
#include <sysdef.h>
#include <rocks.h>
#include <rfdef.h>

#define LPFLINE 408           /* Length of line buffer -- good if
                              *  no less than SBHDCH in rockv.h  */
#define MXLSPFO 255           /* Maximum allowed width of sprintf()
                              *  output (arbitrary safety limit) */
struct RKPFC {
   const char *fptr;          /* Format pointer.  Always points
                              *  to next unused format code.  */
   char *line;                /* Ptr to line start ('line' arg) */
   char *pdat;                /* Next open space in output line */
   char *top;                 /* End of buffer */
   void (*outcb)(long lout);  /* Output callback */
   char *pfline;              /* Permanent shared line buffer */
   union {
      FILE *fpfile;           /* File for fprintf() */
      rkfd *rffile;           /* File for rfprintf() */
      } uf;
   va_list lptr;              /* List pointer */
   va_list bklp;              /* Bracket initial list pointer */
   long totl;                 /* Total chars written */
   int  dig;                  /* Value of format digit string */
   int  icol;                 /* Indent column */
   int  ccode;                /* cryout() control bits */
   int  cnln;                 /* Number of LFs in line */
   int  cprty;                /* cryout() priority and error level */
   int  cspt;                 /* cryout() spout number */
   ui16 pfflgs;               /* Immediate and global flags */
#define  GOTWID   0x0001         /* Width was specified */
#define  GOTDEC   0x0002         /* Got decimal ("precision") */
#define  GOTAUTO  0x0004         /* Got auto decimal */
#define  GOTIADV  0x0008         /* Got '^' iadv code */
#define  SKIPITEM 0x0010         /* Skip item */
#define  SKIPQABL 0x0020         /* If skipping, write blanks */
#define  PTRITEM  0x0040         /* Argument is a pointer */
#define  NOWIDCK  0x0080         /* No width check */
#define  ALTFORM  0x0100         /* Alternate format flag */
#define  LISTSTEP 0x0200         /* Repeat items are separate */
#define  IMMFLGS  0x03ff         /* Flags for one conversion */
#define  INBKTS   0x0400         /* Currently in brackets */
#define  INPARENS 0x0800         /* Currently in parens */
#define  INDEXING 0x1000         /* Now doing %nZ indexing */
#define  NULLTERM 0x2000         /* Include null terminator */
#define  ENDITALL 0x4000         /* End signal to outcb() */
   } ;

#ifdef __cplusplus
extern "C" {
#endif
long rfprintf(rkfd *rkf, const char *fmt, ...);
long rkprintf(void (*outcb)(long lout),
   char *line, size_t llen, const char *fmt, ... );
#ifdef __cplusplus
}
#endif

#endif /* defined RKPRINTF_HDR_INCLUDED */
