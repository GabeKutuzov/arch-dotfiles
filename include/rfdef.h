/* (c) Copyright 1992-2017, The Rockefeller University *11115* */
/* $Id: rfdef.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                          ROCKS I/O Library                           *
*                          RFDEF Header File                           *
*  Defines structures and constants used to manipulate ROCKS files.    *
*                                                                      *
*  See rfalloc.c for discussion of rationale of these routines.        *
*----------------------------------------------------------------------*
*  V1A, 02/29/92, GNR - Derived from filedef.h with buffering added    *
*  Rev, 03/15/95, GNR - Add READWRITE code                             *
*  Rev, 01/30/97, GNR - Change all 'char' arguments to int type        *
*  Rev, 05/22/97, GNR - Make chars into schrs for SGI                  *
*  Rev, 08/30/97, GNR - Change RF type to FILE* on UNIX systems,       *
*                       always include buffering and seeking data      *
*  Rev, 09/13/97, GNR - Add rfflush(), rftell(), userdata              *
*  Rev, 08/15/98, GNR - Add rfgets(), codes for SysIn, SysOut, SysErr  *
*  Rev, 06/02/99, GNR - Add rf[r|w][i2|i4|r4|r8] routines, RF_RDWR     *
*  Rev, 09/04/99, GNR - Remove NCUBE support, add rf[rw]i8             *
*  Rev, 03/01/07, GNR - Add support for socket buffering               *
*  Rev, 12/13/07, GNR - Add C++ wrapper                                *
*  ==>, 12/13/07, GNR - Last mods before committing to svn repository  *
*  Rev, 09/23/08, GNR - Longs to size_t where appropriate              *
*  Rev, 11/26/11, GNR - typedef the RFdef structure                    *
*  Rev, 04/24/14, GNR - Add accmeth ON_LINE switch                     *
*  Rev, 01/11/17, GNR - Add accmeth MK_TEMP switch                     *
***********************************************************************/

#ifndef RFDEF_INCLUDED
#define RFDEF_INCLUDED

#include <stdlib.h>
#if defined (RFM_FOPEN) || defined (RFM_FDOPEN)
#include <stdio.h>
#endif

#define DFLT_BLKSIZE 4096

#ifndef SEEK_SET
#define SEEK_SET (0)       /* In case not in stdio.h */
#define SEEK_CUR (1)
#define SEEK_END (2)
#endif

typedef struct RFdef {
   struct RFdef *nf;       /* Pointer to next file in linked list     */
   char *fname;            /* External file or host name--
                              Can be empty to specify current host    */
#if defined (RFM_FOPEN) || defined (RFM_FDOPEN)
   FILE *pfil;             /* fread/fwrite file descriptor            */
#endif
   int  frwd;              /* read/write file descriptor              */
   char *buff;             /* Ptr to start of buffer                  */
   char *bptr;             /* Ptr to current buffer location          */
   size_t aoff;            /* Keep track of absolute file offset      */
   long lbsr;              /* Length of buffer space remaining        */
   long userdata;          /* User can put anything they want here    */
   schr inout;             /* File access mode:
                              <= 0 read only, 1 write, 2 readwrite    */
   schr fmt;               /* File format:
                              <= 0 binary, > 0 text                   */
   schr accmeth;           /* Access method: = 0 sequential,
                              +1 initiator, +2 listener, +4 direct,
                              +8 online, +16 make temp file           */
   schr append;            /* Position w/in file after opening:
                              <= 0 top, > 0 bottom                    */
   schr look;              /* Look-ahead read flag:
                              <  0 no look-ahead, >= 0 yes            */
   schr norew;             /* File position after closing:
                              <= 0 rewind, > 0 no rewind              */
   schr retbuf;            /* Buffer retention after closing:
                              <= 0 release, > 0 retain                */
   char iamopen;           /* 1 if listening, 2 if open, 4 if fopen   */
   size_t blksize;         /* Suggested block and buffer size:
                              <= 0 for default                        */
   union {                 /* Stuff for different access methods      */
      struct {
         size_t lrecl;        /* Logical record length                */
         long numrec;         /* Number of records expected:
                                 <= 0 for default                     */
         } DASD;           /* IBM DASD -- not used much here          */
      struct {
         int  port;           /* TCP/IP port number                   */
         int  isfd;           /* Initial socket file descriptor       */
         } SOCK;           /* Internet socket                         */
      } URF;
   int  rferrno;           /* Error number if 0 returned              */
   } rkfd;

/* N.B.  If "blksize" or "numrec" fields are not explicitly set by the
*  user, they will be set by host OS (if implemented)... some OS (e.g.
*  IBM) may REQUIRE a "lrecl" value (which can also be specified on a
*  DD or FILEDEF statement).  */

/* Values for RFdef parameters.  Some of these repeat values available
*  in various operating systems, but our values will be same for all. */

#define SAME 0             /* Specifies no change in corresponding
                              field of RFdef record */
#define READ         -1
#define WRITE         1    /* Constants */
#define READWRITE     2
#define BINARY       -1
#define TEXT          1       /*    for    */
#define SysIn         5
#define SysOut    (SysIn+1)
#define SysErr    (SysIn+2)
#define ACC_DEFER    -1
#define SEQUENTIAL    0          /*   file    */
#define DIRECT        1
#define INITIATOR     2
#define LISTENER      4
#define ON_LINE       8
#define MK_TEMP      16
#define TOP          -1             /*  settings */
#define BOTTOM        1
#define NO_LOOKAHEAD -1
#define LOOKAHEAD     1                /*  defined  */
#define REWIND       -1
#define NO_REWIND     1
#define RELEASE_BUFF -1                   /*   above   */
#define RETAIN_BUFF   1
#define IAM_LISTENING 1    /* File is listening */
#define IAM_OPEN      2    /* File opened via open() */
#define IAM_FOPEN     4    /* File opened via fopen() or fdopen() */
#define IAM_DLOPEN    8    /* File opened via dlopen() */
#define IAM_SOCKDUP  16    /* File is socket duplicate */
#define IAM_ANYOPEN  14    /* IAM_OPEN | IAM_FOPEN | IAM_DLOPEN */
#define IGNORE        0    /* Don't care value */

#define NO_ABORT     -1    /* Run-time file errors not fatal */
#define ABORT         0    /* Run-time file errors fatal */
#define NOMSG_ABORT   1    /* Errors are fatal, don't try to print */

#define SEEKABS       0    /* Seek absolute */
#define SEEKREL       1    /* Seek relative */
#define SEEKEND       2    /* Seek from end of file */
#define ATEOF        -1    /* rfread reached end of file */

/* Prototypes for ROCKS file functions */

#ifdef __cplusplus
extern "C" {
#endif
struct RFdef *rfallo(char *fname1, int inout1, int fmt1,
   int accmeth1, int append1, int look1, int norew1,
   int retbuf1, size_t blksize1, size_t lrecl1, long nrp1, int ierr);
struct RFdef *rfdups(struct RFdef *fd, size_t blksize1, int ierr);
int rfopen(struct RFdef *fd, char *fname1, int inout1, int fmt1,
   int accmeth1, int append1, int look1, int norew1,
   int retbuf1, size_t blksize1, size_t lrecl1, long nrp1, int ierr);
int rfqsame(struct RFdef *rfd1, struct RFdef *rfd2);
/*  N.B.  Throughout this package, wherever a return value has been
*  specified as "negative if error", it is declared "long" rather
*  than "size_t" in case "size_t" is an unsigned type.  This
*  restricts lengths to 2**31 in 32-bit systems and 2**63 in
*  64-bit systems--but the 2**31 case was true from the start.  */
long rfgetb(struct RFdef *fd, int ierr);
long rfgets(struct RFdef *fd, char *item, size_t length, int ierr);
long rfread(struct RFdef *fd, void *item, size_t length, int ierr);
long rfseek(struct RFdef *fd, size_t offset, int kseek, int ierr);
size_t rftell(struct RFdef *fd);
long rfwrite(struct RFdef *fd, void *item, size_t length, int ierr);
long rfflush(struct RFdef *fd, int ierr);
int  rfclose(struct RFdef *fd, int norew1, int retbuf1, int ierr);
/* N.B.  Functions in the following class do not support int or long
*  arguments, because their lengths may differ on different machines.
*  Use the 32- or 64-bit versions according to the magnitude of the
*  variable and coerce to/from a different size if needed at run time.
*  si64 != ui64 because implemented as structs in 32-bit world.  */
/* Routines to read one item from a big-endian file, abort on error */
short  rfri2(struct RFdef *fd);
si32   rfri4(struct RFdef *fd);
si64   rfri8(struct RFdef *fd);
ui64   rfru8(struct RFdef *fd);
float  rfrr4(struct RFdef *fd);
double rfrr8(struct RFdef *fd);
/* Routines to write one item to a big-endian file, abort on error */
void rfwi2(struct RFdef *fd, short i2);
void rfwi4(struct RFdef *fd, si32 i4);
void rfwi8(struct RFdef *fd, si64 i8);
void rfwu8(struct RFdef *fd, ui64 u8);
void rfwr4(struct RFdef *fd, float r4);
void rfwr8(struct RFdef *fd, double r8);
#ifdef __cplusplus
}
#endif
#endif                     /* End ifndef RFDEF_INCLUDED */

