/* (c) 1992 Neurosciences Research Foundation */
/* $Id: mfconv.h 1 2008-12-19 16:56:22Z  $ */
/**********************************************************************
 *                                                                    *
 *                              MFCONV.H                              *
 *                                                                    *
 * Version 1, 10/08/92, ROZ                                           *
 * ==>, 07/01/02, GNR - Last date before committing to svn repository *
 **********************************************************************/
#ifndef _MFCONV_H
#define _MFCONV_H

#define TITMAX 81          /* Length of title string */
#define DATMAX 81          /* Length of date string */
#define MAXPRN 32          /* Max number of printers */

#define MFC_ALL     0
#define MFC_SAVE    1
#define MFC_SCREEN  2
#define MFC_PRINT   3
#define MFC_EPS     0
#define MFC_XGL     1
#define NAMEMAX     81

typedef struct {
   char infn[NAMEMAX];     /* Input file name */
   char outfn[NAMEMAX];    /* Output file name */
   char devfn[NAMEMAX];    /* Printer device name */
   char tmpfn[NAMEMAX];    /* Temporary storage file name */
   FILE *infd;             /* Input output file descriptore */
   FILE *outfd;            /* Input output file descriptore */
   FILE *tmpfd;            /* Temporary file descriptore */
   int devfd;              /* Device's file descriptore */
   int currop;             /* Current operation flag (SAVE,LOAD) */
   int flgcol;             /* Current color flag (COLORE,MONOCH) */
   int flgbkg;             /* Current backgroud flag (BLACK,WHITE) */
   int flgbdr;             /* Current boarder flag (BDRON,BDROFF) */
   int device;             /* Device's type (MFC_PRINT,MFC_SAVE,MFC_SCREEN) */
   int type;               /* Implementaion (XGL,EPS) */
   int pgdes;              /* Page descripor */
   char title[TITMAX];     /* Meatafile title */
   char date[DATMAX];      /* Metafile-date created */
   char curdat[DATMAX];    /* Metafile-date printed */
   } MFCONV;

typedef struct {
   long maxfrms;           /* Total number of frames */
   long currframe;         /* Current frame number */
   long previous;          /* Previous frame number */
   long next;              /* Next frame nuber */
   long *index;            /* A pointer to an array that holds frame index */
   } MFFRAMES;

struct CDF {
   char *alias;            /* Alias as appears in printcap */
   float tmargin;          /* Top margin of printed area */
   float lmargin;          /* Left margin of printed area */
   float width;            /* Max width of printed area */
   float height;           /* Max height of printed area */
   char *visual;           /* Visual properties */
   int depth;              /* Depth of color */
   int lwidth;             /* Line width */
   } CPD;

#ifdef _MFCONV_C
struct CDF CPD  = {
   "phaser",               /* alias */
   1.09,                   /* tmargin */
   0.20,                   /* lmargin */
   8.10,                   /* width */
   8.825,                  /* Max height of printed area */
   "color",                /* height */
   24,                     /* visual */
   1                       /* lwidth */
   };

MFCONV MFC;
MFFRAMES MFF;
#else
extern struct CDF CPD;
extern MFCONV MFC;
extern MFFRAMES MFF;
#endif
#endif
