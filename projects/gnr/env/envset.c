/* (c) Copyright 1988-2013, The Rockefeller University *21116* */
/* $Id: envset.c 23 2017-01-16 19:28:30Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                              ENVSET.C                                *
*                Initialization for environment module                 *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  (1) Once-only initialization to set up environment data structure:  *
*                                                                      *
*     Synopsis: struct ENVDEF *envset(struct RFdef *libfile,           *
*        struct RFdef *repfile, struct RFdef *pixfile, ARM *parm1,     *
*        WINDOW *pwdw1, int kx, int ky, int mxobj, int mxref,          *
*        int kcolor, int kesave)                                       *
*                                                                      *
*     Arguments:                                                       *
*       libfile: Ptr to shape library file                             *
*       repfile: Ptr to replay activity records file                   *
*       pixfile: Ptr to pixel format stimuli file                      *
*         parm1: Ptr to arm list                                       *
*         pwdw1: Ptr to window list                                    *
*         kx,ky: Log(2) (X,Y) input array dimension (max 15)           *
*         mxobj: Max simultaneous objects (max SHRT_MAX)               *
*         mxref: Obsolete max reference number--ignored                *
*        kcolor: 0 (ENV_MONO), environement is monochrome,             *
*                1 (ENV_COLOR), environment is colored,                *
*                2 (ENV_ORCOL), environment objects OR colors together *
*        kesave: 0 (ENV_SAVOBJ), save only objects in replay file,     *
*                1 (ENV_SAVALL), save full input array in replay file  *
*     Value returned: Pointer to initialized environment record        *
*                  (NULL on error)                                     *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  (2) Set object and graphics options (may be called at any time):    *
*                                                                      *
*     Synopsis: int envopt(struct ENVDEF *Env, double epr, double      *
*        eslht, int ept, int eilo, int eihi, int kux, int kuy,         *
*        int egrid, char *ecolgrd, char *ecolstm, int kpixel,          *
*        int kfill, int kretrace, int komit, int kobjlist)             *
*                                                                      *
*     Arguments:                                                       *
*           Env: Pointer (returned by envset) to environment data      *
*           epr: Radius of pixel in environment plot                   *
*         eslht: Letter height in environment plot                     *
*           ept: Threshold density for plotting a pixel (S8)           *
*     eilo,eihi: Values assigned to low and high densities for         *
*                objects with just binary densities (S8)               *
*       kux,kuy: Default used (X,Y) size of stimuli (max SHRT_MAX)     *
*         egrid: Plot grid intervals (=0, no grids)                    *
*       ecolgrd: Grid drawing color (max ECOLMAXCH characters)         *
*       ecolstm: Mono stimulus drawing color (max ECOLMAXCH chars)     *
*        kpixel: 0 (ENV_CIRCLE), draw pixels as circles,               *
*                1 (ENV_SQUARE), draw pixels as squares                *
*         kfill: 0 (FALSE), do not fill in pixel shapes,               *
*                1 (TRUE), do fill in pixel shapes                     *
*      kretrace: 0 (FALSE), do not retrace when drawing pixels,        *
*                1 (TRUE), do retrace pixels                           *
*         komit: 0 (FALSE), add titles to environment plots,           *
*                1 (TRUE), omit titles from environment plots          *
*      kobjlist: 0 (FALSE), omit printing object list                  *
*                1 (TRUE), print object list on each envcyc call       *
*                2, print and spout object list                        *
*     Note:                                                            *
*        Any of the color arguments may be a NULL pointer to indicate  *
*        that the current color should not be changed.  Any of the     *
*        remaining arguments may be set negative to indicate that the  *
*        current value of that argument should not be changed.         *
*     Note:                                                            *
*        The sole reason that the epr and eslht args are double rather *
*        than float is so this function can remain unprototyped under  *
*        the UCC compiler to save formal symbol table space.           *
*     Value returned:                                                  *
*        0 (ENV_NORMAL) if options were stored successfully            *
*       -1 (ENV_FATAL)  if there was an error                          *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  (3) To signal to the env package that a new trial series has begun: *
*                                                                      *
*     Synopsis: void envser(struct ENVDEF Eptr)                        *
*                                                                      *
*     This routine causes envcyc to reuse stimulus sequences as        *
*     requested.                                                       *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  Format of shape library (libfile) is:                               *
*      Record 1:  library header (16 bytes)                            *
*        char *: name and version of making program (12 chars)         *
*          ui32: number of shapes in library                           *
*  Records 2-ff: index records (8-bytes per record)                    *
*        char *: shape name (4 chars)                                  *
*          ui32: starting record number for that shape                 *
*                                                                      *
*  Remaining records: DSLB shape information, envshp format            *
*                     (each padded to 80 bytes)                        *
*                                                                      *
*  NOTE: Pixel file header info is by definition big-endian            *
*        regardless of machine on which env is implemented.            *
*        Swapping is done accordingly when file is made or read.       *
*        At present time, header text in this file is EBCDIC.          *
*        Plan is to change it to ASCII when file preparation is        *
*        first implemented on an ASCII machine.                        *
*                                                                      *
*----------------------------------------------------------------------*
*  V4A, 12/05/88, JWT - Converted FORTRAN to C                         *
*  Rev, 05/15/89, JWT - Initial field values added to param list,      *
*                       environment record is now function output      *
*                       (formerly input)                               *
*  Rev, 03/28/90, JWT - Added reference # to object lookup table       *
*  Rev, 04/04/90, JWT - Removed hex stimulus file pointer,             *
*                       added support for pixel file objects           *
*  Rev, 03/04/92, GNR - Use rf io routines                             *
*  Rev, 05/11/92, GNR - Add eslht default, use external swap function  *
*  Rev, 02/10/97, GNR - Make library file always big-endian            *
*  Rev, 04/15/97, GNR - Add envopt(), allowing ENVDEF to be hidden     *
*  Rev, 04/02/98, GNR - Change eilo dflt to 0, ecolstm to RGB if !B&W  *
*  Rev, 05/22/99, GNR - Use new swapping scheme                        *
*  Rev, 08/30/01, GNR - Automatic update of mxref per input found      *
*  Rev, 02/22/03, GNR - Change s_type to stim_type                     *
*  ==>, 12/16/07, GNR - Last mod before committing to svn repository   *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
*  Rev, 11/28/10, GNR - Change COLMAXCH to ECOLMAXCH                   *
*  Rev, 10/26/13, GNR - Change longs to ui32 in libfile                *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include "sysdef.h"
#include "swap.h"
#include "envglob.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#include "rocks.h"
#include "rkxtra.h"

#define LIBMAKER "D3LIBPRP" /* Name of shape library make utility */
#define PIXMAKER "D3PXLPRP" /* Name of pixel file make utility */
#define MINVERS  "V2A"      /* Min compatible pixel file make version */
#define VERSIZE 12          /* Number of chars in maker version name */

static jmp_buf errjmp;     /* Save program state for error return */

/*---------------------------------------------------------------------*
*                               eckarg                                 *
*                                                                      *
*  This little routine is used to check that calling arguments are     *
*  within the ranges required by env program restrictions.             *
*---------------------------------------------------------------------*/

static void eckarg(int arg, int minarg, int maxarg, char *argname) {

   if (arg >= minarg && arg <= maxarg) return;
   cryout(RK_P1, "0***ENVIRONMENT: ", RK_LN2, argname, RK_CCL,
      " IS OUT OF REQUIRED RANGE.", RK_CCL, NULL);
   longjmp(errjmp, ENV_FATAL);

   } /* End eckarg */

/*---------------------------------------------------------------------*
*                               envset                                 *
*---------------------------------------------------------------------*/

struct ENVDEF *envset(struct RFdef *libfile, struct RFdef *repfile,
   struct RFdef *pixfile, ARM *parm1, WINDOW *pwdw1, int kx, int ky,
   int mxobj, int mxref, int kcolor, int kesave) {

/* Lookup table for rudimentary EBCDIC to ASCII char conversion */
   static byte ebcdic_to_ascii[256] = {
        0,   1,   2,   3,   0,'\t',   0, 127,
        0,   0,   0,  11,'\f','\r',  14,  15,
       16,  17,  18,  19,   0,   0,'\b',   0,
       24,  25,   0,   0,   0,   0,   0,   0,
        0,   0,  28,   0,   0,'\n',   0,  27,
        0,   0,   0,   0, 245,   5,   6,   7,
        0,   0,  22,   0,   0,   0,   0,   4,
        0,   0,   0,   0,  20,  21,   0,  26,
      ' ',   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0, '.',  60, '(', '+',   0,
      '&',   0,   0,   0,   0,   0,   0,   0,
        0,   0, '!', '$', '*', ')', ';',   0,
      '-', '/',   0,   0,   0,   0,   0,   0,
        0,   0, 124, ',', '%', '_', '>', '?',
        0,   0,   0,   0,   0,   0,   0,   0,
        0, '`', ':', '#', '@',   0, '=', '"',
        0, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
      'h', 'i',   0,   0,   0,   0,   0,   0,
        0, 'j', 'k', 'l', 'm', 'n', 'o', 'p',
      'q', 'r',   0,   0,   0,   0,   0,   0,
        0, '~', 's', 't', 'u', 'v', 'w', 'x',
      'y', 'z',   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
      '{', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
      'H', 'I',   0,   0,   0,   0,   0,   0,
      '}', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
      'Q', 'R',   0,   0,   0,   0,   0,   0,
      '\\',  0, 'S', 'T', 'U', 'V', 'W', 'X',
      'Y', 'Z',   0,   0,   0,   0,   0,   0,
      '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9',   0,   0,   0,   0,   0,   0
      } ;

/* Default plotting colors */
   static char *dcolgrd = "CYAN";
   static char *dcolstm = "YELLOW";
   static char *dcolrgb = "RGB";

   struct ENVDEF *Env;     /* Pointer to new ENVDEF block */
   long llx,lly;
   int i;
   char fvers[VERSIZE];    /* Buffer for reading file header info */

/* Allocate ENVDEF block */

   Env = (struct ENVDEF *)callocv(1, sizeof(struct ENVDEF),
      "ENVIRONMENT DATA BLOCK");

/* All error returns executed here.  Since our caller is now going
*  to terminate with an error condition, there isn't much point to
*  free allocated memory now, but this would be the place to do it.  */

   if (setjmp(errjmp)) {
      RK.iexit |= ENV_ERR;
      return NULL;
      }

/* Check that parameters are within required ranges */

   eckarg(kx, 0, MAXENV, "IA X SIZE");
   eckarg(ky, 0, MAXENV, "IA Y SIZE");
   eckarg(mxobj, 0, SHRT_MAX, "MXOBJ");
   eckarg(kcolor, ENV_MONO, ENV_ORCOL, "ENV COLOR OPTION");
   eckarg(kesave, ENV_SAVOBJ, ENV_SAVALL, "ENV SAVE OPTION");

/* Calculate size of input array */

   llx = 1<<kx, lly = 1<<ky;

/* Read shape library header info if present */

   if (libfile) {

      rfopen(libfile, NULL, READ, BINARY, DIRECT,
            TOP, NO_LOOKAHEAD, REWIND, RELEASE_BUFF, IGNORE,
            LIB_REC_SIZE, IGNORE, ABORT);
      rfread(libfile, fvers, VERSIZE, ABORT);
      if (strncmp(fvers, LIBMAKER, strlen(LIBMAKER))) {
         cryout(RK_P1, "0***ENVSET: SHAPE LIBRARY VERSION ERROR",
            RK_LN2, NULL);
         longjmp(errjmp, ENV_FATAL);
         }
      Env->libnum = (long)rfri4(libfile);

      /* Allocate space for library index */
      Env->plibid = (struct LIBINDEF *)mallocv(
         Env->libnum * sizeof(struct LIBINDEF), "ENVSET: lib");

      /* Read id names and record numbers into library index */
      rfseek(libfile, LIB_REC_SIZE, SEEKABS, ABORT);
      rfread(libfile, Env->plibid,
         sizeof(struct LIBINDEF)*Env->libnum, ABORT);
#if BYTE_ORDRE < 0
      {  struct LIBINDEF *pindex = Env->plibid;
         struct LIBINDEF *pindexe = Env->plibid + Env->libnum;
         for ( ; pindex<pindexe; pindex++) {
            char *pswp = (char *)&pindex->recnum;
            pindex->recnum = bemtoi4(pswp); }
         } /* End pindex local scope */
#endif
      } /* End library file processing */

/* Read pixel file header info (must be version 2A or later) */

   if (pixfile) {
      unsigned long pixlx,pixly; /* x,y size buffers */
      byte vbuff[VERSIZE];

      rfopen(pixfile, NULL, READ, BINARY, DIRECT,
            TOP, NO_LOOKAHEAD, REWIND, RELEASE_BUFF, IGNORE,
            LIB_REC_SIZE, IGNORE, ABORT);
      rfread(pixfile, vbuff, VERSIZE, ABORT);
      for (i=0; i<VERSIZE; i++) fvers[i] = ebcdic_to_ascii[vbuff[i]];
      if (strncmp(fvers, PIXMAKER, strlen(PIXMAKER)) ||
            strncmp(fvers+strlen(PIXMAKER)+1, MINVERS,
            strlen(MINVERS)) < 0) {
         cryout(RK_P1,"0***ENVSET: PIXEL FILE VERSION ERROR",
            RK_LN2, NULL);
         longjmp(errjmp, ENV_FATAL);
         }

      /* Read number, x and y size of pixel objects */
      Env->pixnum = (long)rfri4(pixfile);
      pixlx = (unsigned long)rfri4(pixfile);
      pixly = (unsigned long)rfri4(pixfile);
      eckarg(pixlx, llx, llx, "PIXEL IMAGE X SIZE");
      eckarg(pixly, lly, lly, "PIXEL IMAGE Y SIZE");
      } /* End pixel file processing */

/* Initialize ENVDEF block */

   Env->pshpfill = &Env->pexshp;
   Env->parm1 = parm1;
   Env->pwdw1 = pwdw1;
   Env->libfile = libfile;
   Env->repfile = repfile;
   Env->pixfile = pixfile;
   Env->epr = 1.0;
   Env->eslht = 0.22;
   Env->iseed1 = 3950291;
   Env->iseed2 = 4756103;
   Env->ekx = kx;
   Env->eky = ky;
   Env->eux = 12;
   Env->euy = 10;
   Env->mxobj = (short)mxobj;
   strncpy(Env->ecolgrd, dcolgrd, ECOLMAXCH);
   strncpy(Env->ecolstm, kcolor ? dcolrgb : dcolstm, ECOLMAXCH);
   Env->eilo = 0;               /* Background pixel */
   Env->eihi = 240;             /* Foreground pixel */
   Env->kcolor = (char)kcolor;
   Env->kesave = (char)kesave;

/* Set constants relating to input array size */
   Env->lx = llx;
   Env->ly = lly;
   Env->lxsub1 = llx - 1;
   Env->lysub1 = lly - 1;
   Env->lxs16 = (si32)llx<<16;
   Env->lys16 = (si32)lly<<16;
   Env->lxs16m1 = Env->lxs16 - 1;
   Env->lys16m1 = Env->lys16 - 1;
   Env->lxt2m1 = (long)Env->lxs16 + (long)Env->lxs16m1;
   Env->lyt2m1 = (long)Env->lys16 + (long)Env->lys16m1;
   Env->lymul2 = lly<<1;
   Env->ltotal = Env->lx*Env->lymul2;
   Env->rlx = (float)llx;
   Env->rly = (float)lly;

   return Env;

   } /* End envset() */

/*---------------------------------------------------------------------*
*                               envopt                                 *
*---------------------------------------------------------------------*/

int envopt(struct ENVDEF *Env, double epr, double eslht, int ept,
      int eilo, int eihi, int kux, int kuy, int egrid,
      char *ecolgrd, char *ecolstm, int kpixel, int kfill,
      int kretrace, int komit, int kobjlist) {

/* All error returns executed here */

   if (setjmp(errjmp)) {
      RK.iexit |= ENV_ERR;
      return ENV_FATAL;
      }

   if (eslht >= 0.0) Env->eslht = (float)eslht;
   if (epr >= 0.0)   Env->epr  = (float)epr;
   if (ept >= 0)     Env->ept  = ept;
   if (eilo >= 0)    Env->eilo = (stim_type)min(eilo, (BYTE_MAX<<1)+1);
   if (eihi >= 0)    Env->eihi = (stim_type)min(eihi, (BYTE_MAX<<1)+1);
   if (kux >= 0) {
      eckarg(kux, 0, Env->lx, "KUX");
      Env->eux = (short)kux; }
   if (kuy >= 0) {
      eckarg(kuy, 0, Env->ly, "KUY");
      Env->euy = (short)kuy; }
   if (egrid >= 0) {
      eckarg(egrid, 0, SHRT_MAX, "GRID INTERVAL");
      Env->egrid = (short)egrid; }
   if (ecolgrd)       memcpy(Env->ecolgrd, ecolgrd, ECOLMAXCH);
   if (ecolstm)       memcpy(Env->ecolstm, ecolstm, ECOLMAXCH);
   if (kpixel >= 0) {
      eckarg(kpixel, ENV_CIRCLE, ENV_SQUARE, "PIXEL SHAPE");
      Env->kpixel = (char)kpixel; }
   if (kfill >= 0)    Env->kfill = (char)kfill;
   if (kretrace >= 0) Env->kretrace = (char)kretrace;
   if (komit >= 0)    Env->komit = (char)komit;
   if (kobjlist >= 0) Env->kobjlist = (char)kobjlist;

   return ENV_NORMAL;

   } /* End envopt() */

/*---------------------------------------------------------------------*
*                               envser                                 *
*---------------------------------------------------------------------*/

void envser(struct ENVDEF *Env) {

   Env->newser = TRUE;

   } /* End envser() */
