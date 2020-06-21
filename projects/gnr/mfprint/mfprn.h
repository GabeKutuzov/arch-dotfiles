/* (c) 1992-1994 Neurosciences Research Foundation */
/* $Id: mfprn.h 1 2008-12-19 16:56:22Z  $ */
/**********************************************************************
 *                                                                    *
 *                              MFPRN.H                               *
 *                                                                    *
 * Version 1, 06/23/92, ROZ                                           *
 *      Rev., 08/21/94,  LC  Added RTR for retrace.                   *
 * ==>, 07/01/02, GNR - Last date before committing to svn repository *
 **********************************************************************/

#define NUMPRN 32          /* Initial number of printers */

#define KWLEN  6           /* Numner of keywords stored in usrkw array */
#define PRN  0             /* Printer field */
#define BDR  1             /* Border field */
#define HDR  2             /* Header field */
#define BK   3             /* Background field */
#define VSL  4             /* Visual field */
#define RTR  5             /* Retrace graphics */

/* Array of keywords used in the user defaults file */
char *usrkw[KWLEN]={"printer:","border:","header:",
                   "bk:","visual:","retrace:"};

#define KWPRN 10           /* Numner of keywords stored in usrkw array */
#define PNAM  0            /* Name field */
#define PALI  1            /* Alias field */
#define PTMA  2            /* Top margin field */
#define PLMA  3            /* Left margin field */
#define PWID  4            /* Painting area width field */
#define PHEI  5            /* Painting area height field */
#define PCLR  6            /* Visual properties field */
#define PGRY  7            /* Visual properties field */
#define PBW   8            /* Visual properties field */
#define PLWI  9            /* Line width field */

/* Array of keywords used in the printer defaults file */
char *prnkw[KWPRN]={"name:","alias:","tmargin:",
                   "lmargin:","width:","height:",
                   "color:","gray:","bw:",
                   "lwidth:"};

typedef struct {
   long maxfrms;           /* Total number of frames */
   long currframe;         /* Current frame number */
   long previous;          /* Previous frame number */
   long next;              /* Next frame number */
   char infn[NAMEMAX];     /* Input file name */
   char outfn[NAMEMAX];    /* Output file name */
   char devfn[NAMEMAX];    /* Printer device name */
   char tmpfn[NAMEMAX];    /* Temporary storage file name */
   char usrfn[NAMEMAX];    /* User defaults file name */
   char prnfn[NAMEMAX];    /* Printer data file name */
   FILE *infd;             /* Input file descriptore */
   FILE *outfd;            /* Output file descriptore */
   FILE *tmpfd;            /* Temporary file descriptore */
   FILE *usrfd;            /* User default file descriptore */
   FILE *prnfd;            /* Printer data file descriptore */
   int devfd;              /* Device's file descriptore */
   int flgcol;             /* Current color flag (COLORE,MONOCH) */
   int flgbkg;             /* Current backgroud flag (BLACK,WHITE) */
   int flgbdr;             /* Current boarder flag (BDRON,BDROFF) */
   int pgdes;              /* Page descriptor */
   int retrace;            /* Retrace graphics */
   char dfprn[NAMEMAX];    /* User printer name */
   } MFPRINT;

struct PDF {
   char *alias;            /* Alias as appears in printcap */
   float tmargin;          /* Top margin of printed area */
   float lmargin;          /* Left margin of printed area */
   float width;            /* Max width of printed area */
   float height;           /* Max height of printed area */
   char *visual;           /* Visual properties */
   int depth;              /* Depth of color */
   int lwidth;             /* Line width */
   } *MPD;

struct SPR {
   int num_prn;            /* Number of printers installed */
   char **name;            /* Name of printer */
   int curr_prn;           /* Current printer */
   int sel_prn;            /* Selected printer */
   int index;              /* Index into MPD database */
   } SLP;

#ifdef MAIN
MFPRINT MFP;
#else
extern MFPRINT MFP;
#endif

