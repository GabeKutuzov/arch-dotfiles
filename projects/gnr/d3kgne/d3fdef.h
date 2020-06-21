/* (c) Copyright 1992-2001, Neurosciences Research Foundation, Inc. */
/* $Id: d3fdef.h 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                         D3FDEF Header File                           *
*                                                                      *
*  CNS file definitions                                                *
*                                                                      *
*  V4A, 11/18/88, Initial version                                      *
*  Rev, 09/24/91, GNR - Add digital input file entry for D1            *
*  Rev, 03/05/92, GNR - Use rf io routines                             *
*  V5E, 06/10/92, GNR - Add METAFILE definition                        *
*  Rev, 02/27/93, GNR - Remove snapshot support                        *
*  V8B, 10/07/01, GNR - Add UPROGLIB definition                        *
*  ==>, 06/28/02, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include "rfdef.h"

/* Because preprocessor cannot do arithmetic, variables of the
*  form nXXX are defined to take on value 1 if XXX is defined,
*  otherwise 0.  The nXXX's are then used to define FILETYPE_NUM
*  and some of the individual filetypes.  This results in more
*  compact macro expansions.  The macro text will get copied
*  into each appearance, so the defs are placed in parens.  */

#ifdef D1
#define nD1 1
#else
#define nD1 0
#endif

/* Number of unique file types */
#define FILETYPE_NUM (10+nD1)

#define SAVREP_IN  0          /* Saved repertoire input */
#define SAVREP_OUT 1          /* Saved repertoire output */
#define FDMATRIX   2          /* Feature detection matrices */
#define CONNLIST   3          /* Connection lists */
#define SIMDATA    4          /* Saved simulation data */
#define METAFILE   5          /* Graphics metafile */
#define UPROGLIB   6          /* User program library */
#define REPLAY     7          /* Replay input file */
#define PIXSTIM    8          /* Stimuli in pixel format */
#define SHAPELIB   9          /* Shape library (DSORG = DA) */
#define DIGINPUT (9+nD1)      /* Digital (Darwin 1) input */

/* Define an array of ptrs to linked lists of files grouped by type */
#ifndef MAIN
extern
#endif /* MAIN */
   struct RFdef *d3file[FILETYPE_NUM];

