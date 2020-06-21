/* (c) Copyright 1986-2018, The Rockefeller University *21116* */
/* $Id: d3gfhd.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3gfhd                                 *
*               CREATE GLOBAL HEADER FOR A SIMDATA FILE                *
*                                                                      *
*  Uses the graph data file format designed by Pearson & Shu (V.3)     *
*                                                                      *
*  Note:  Each type of sense is given its own name and format data so  *
*  that senses with different dimensions can be retrieved easily.      *
************************************************************************
*  V1A, 08/22/86, G.N. REEKE - Just set up for distance                *
*  V4A, 06/06/90,  MC - Convert from FORTRAN to C                      *
*  Rev, 05/01/91, GNR - Make RP->pgdcl a direct ptr to cell list       *
*  V5C, 02/24/92, KJF - Implement Lij/Cij & remove obsolete options    *
*  Rev, 03/07/92, GNR - Use rf io routines, add local functions, etc.  *
*  Rev, 08/08/92, GNR - Add support for Cij0                           *
*  V5E, 09/30/92, GNR - Make GRAFDATA little-endian on all systems     *
*  Rev, 01/19/93, GNR - Add support for OBJID, print selectors         *
*  Rev, 11/23/93, GNR - Correct bug in little-endian selector length   *
*  V6D, 02/09/94, GNR - Add Dij, check existence of Lij,Cij, etc.      *
*  V7A, 05/07/94, GNR - Number connections from 0 as they should be    *
*  V7B, 07/25/94, GNR - Use separate LEV1 selectors for env & brain,   *
*                       eliminate selectors with no variables          *
*  Rev, 10/03/94,  LC - Move to version 3 GRAFDATA format              *
*  V8A, 04/15/95, GNR - Merge kchng,flags into WDWDEF                  *
*  Rev, 05/01/95,  LC - Absolute indexing of val/obj, not renumbered   *
*  Rev, 05/12/97, GNR - Above LC changes merged into V8A code (remove  *
*                       cntList, which fails on continuation cards),   *
*                       change binary data to big-endian and header    *
*                       data to decimal per GRAFDATA V.3 spec, handle  *
*                       stride in cell lists, delete ENV/REP at LEV1,  *
*                       add phidist and afference items, add LEVEL 4   *
*                       to break out individual connection types, re-  *
*                       order so can get connection data at d3go time  *
*  Rev, 08/26/97, GNR - Break out segment headers to new d3gfsh.c,     *
*                       add input array, changes, Mij, NUK, Sbar       *
*  Rev, 05/30/99, GNR - Use new swapping scheme                        *
*  Rev, 08/19/00, GNR - Add SVITMS=P (PPF)                             *
*  V8C, 03/22/03, GNR - Cell responses in millivolts, add conductances *
*  V8D, 12/27/07, GNR - Add QBAR                                       *
*  Rev, 01/31/08, GNR - Add SAVE SIMDATA SENSE                         *
*  ==>, 02/01/08, GNR - Last mod before committing to svn repository   *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
*  V8E, 02/05/09, GNR - Add Izhikevich u variable and a,b,c,d params   *
*  V8F, 06/05/10, GNR - Add GFUTV user camera object info              *
*  V8G, 08/28/10, GNR - Add Autoscale multiplier (ASM) variable        *
*  V8H, 11/08/10, GNR - Add SNSID variable                             *
*  Rev, 05/06/11, GNR - Use 'R' for arm, 'J' for Sj                    *
*  Rev, 12/07/12, GNR - Convert ns[xy], ku[xy] to ui16, nst to ui32    *
*  R73, 04/16/17, GNR - Last writevar arg long --> int                 *
*  R77, 02/08/18, GNR - Use 'R' for raw afference, 'T' for all joints  *
*  R77, 02/27/18, GNR - Change AK,RAW from ETYPE to FTYPE, S(20/27)    *
***********************************************************************/

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "d3fdef.h"
#include "tvdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"
#include "clblk.h"
#include "simdata.h"

#define GDVERS "GRAFDATA V3A" /* Version number of SIMDATA spec. */
#define LVARNM     8       /* Length of variable name in file */
#define qLVN      "8"      /* Quoted LVARNM */
#define LVARDES   24       /* Length of variable descriptors in file */
#define MXDATA    80       /* Max length of data card */
#define NLVLS      5       /* Number of levels */
#define NVARS     33       /* Number of variables in data structure */

/* Global variables used to communicate between following routines */
static struct RFdef *pgdf; /* Ptr to SIMDATA file descriptor */

/*=====================================================================*
*                              writevar                                *
*  Local function to write variable name and description into header   *
*  Arguments:                                                          *
*     name     Name to be written                                      *
*     type     Code for type of name, defined in d3global.h            *
*     level    Level in hierarchy of this variable                     *
*     scale    Binary scale of variable (if general fixed-point)       *
*     len      Length of one item of this variable type                *
*     dim      Dimension if item is an array, 0 if it's a scalar       *
*=====================================================================*/

static void writevar(char *name, int type, int level, int scale,
                     int len, int dim) {

   char vardes[LVARNM+LVARDES];

   sconvrt(vardes, "(O,A" qLVN ",4*I4I8)",
      name, &type, &level, &scale, &len, &dim, NULL);
   rfwi4(pgdf, (si32)(-(LVARNM+LVARDES)));
   rfwrite(pgdf, vardes, sizeof(vardes), ABORT);
   } /* End writevar */

/*=====================================================================*
*                               d3gfhd                                 *
*=====================================================================*/

void d3gfhd (struct SDSELECT *pgd)  {

   int  i;                       /* Level counter */
   int  ibs,jbs;                 /* si, wksi binary scales */
   int  glist[2];                /* Header info: nlvls, nvars */
   char time[12];                /* Time stamp */
   char rec1[21];                /* Record 1 */

   /* Some names for the levels (max 12 chars/name) */
   static char *lvlnms[NLVLS] = {
      "LEV1,ENVIRONMENT,EFFERENCE,WINDOW,OBJECT,VALUE,ARM,REGION,"
         "SENSE,CAMERA",
      "LEV2,JOINT,CELLTYPE,SENSOR",
      "LEV3,CELL",
      "LEV4,CONNTYPE",
      "LEV5,CONNECTION" };

   /* Open file.
   *  Note:  If file cannot be opened, we avoid aborting so other
   *  input errors can be found in same run.  However, once it is
   *  opened, we do abort for simplicity if a write error occurs.  */
   pgdf = d3file[pgd->gdfile];      /* Get file descriptor */
   if (!(rfopen(pgdf,NULL,SAME,SAME,SAME,SAME,SAME,SAME,SAME,
         SAME,SAME,SAME,NO_ABORT))) {
      cryout(RK_E1,"0***CAN'T OPEN SIMDATA FILE.",RK_LN2,NULL);
      RP->ksv |= KSDNOK;
      return;
      }

/**********************************************************************/
/*                           Create header                            */
/**********************************************************************/

/*---------------------------------------------------------------------*
*  Record 1:  version, nlvls, nvars (A12,2I4)                          *
*     N.B.  "version" is the version number of the SIMDATA file,       *
*     not the version number of CNS.                                   *
*---------------------------------------------------------------------*/

   glist[0] = NLVLS;       /* Number of levels */
   glist[1] = NVARS;       /* Number of distinct variables */
   sconvrt(rec1, "(A12,2I4)", GDVERS, glist, NULL);
   rfwrite(pgdf, rec1, strlen(rec1), ABORT);

/*---------------------------------------------------------------------*
*  Record 2:  Title (A60)                                              *
*---------------------------------------------------------------------*/

   rfwrite(pgdf, gettit(), 60, ABORT);

/*---------------------------------------------------------------------*
*  Record 3:  Time stamp (6A2)                                         *
*---------------------------------------------------------------------*/

   tstamp(time);
   rfwrite(pgdf, time, 12, ABORT);

/*---------------------------------------------------------------------*
*  Level names                                                         *
*---------------------------------------------------------------------*/

   for (i=0; i<NLVLS; i++) {
      long llnm = strlen(lvlnms[i]);

      rfwi4(pgdf, (si32)(-llnm));
      rfwrite(pgdf, lvlnms[i], llnm, ABORT);
      } /* End levels loop */

/*---------------------------------------------------------------------*
*  Variable descriptors                                                *
*     For each variable, there are six fields:                         *
*     1) The name of the variable (padded to 8 chars)                  *
*     2) A type code (2 chars):                                        *
*        0 = signed integer or fixed point                             *
*        1 = real                                                      *
*        2 = unsigned integer or fixed point                           *
*     3) The level of the item in the data structure (2 chars)         *
*     4) The binary scale of the item, if fixed point (2 chars)        *
*     5) The length of one item of this type (2 chars)                 *
*     6) Dimension of the item if it's an array, 0 if it's a scalar    *
*---------------------------------------------------------------------*/

/* Variable 1: Input Array.  Note:  CNS does not currently handle
*  color with more than 8 bits, but the SIMDATA file can--just
*  increment the size variable when this feature is programmed.  */

   if (RP->kcolor)
      writevar("IA",  KTYPE, 1, 0, BSIZE, RP->nst);
   else
      writevar("IA",  UTYPE, 1, 8, BSIZE, RP->nst);

/* Variable 2: Window x,y coords */

   writevar("WINDOW", FTYPE, 1, 3, HSIZE, 2);

/* Variable 3: Object x,y coords */

   writevar("OBJXY",  FTYPE, 1, 3, HSIZE, 2);

/* Variable 4: Object identifiers--call it signed
*  so missing objects will register as '-1'.  */

   writevar("OBJID",  FTYPE, 1, 0, HSIZE, 2);

/* Variables 5-6: Object information from a user camera program.
*  (Needs two names because of different types, also, might want
*  to activate separately later.)  */

   writevar("CAMID",  FTYPE, 1, 0, HSIZE, 2);
   writevar("CAMINFO",UTYPE, 1, 0, BSIZE, NUTVINFO);

/* Variable 7: Value */

   writevar("VALUE",  FTYPE, 1, FBsv, HSIZE, 0);

/* Variable 8: Efference (changes array) */

   writevar("EFF",    ETYPE, 1, 0, ESIZE, RP0->ljnwd);

/* Variable 9: Sense ID (isn,ign) data */

   writevar("SNSID",  UTYPE, 1, 0, HSIZE, 2);

/* Variable 10: Joint x,y coords */

   writevar("JOINT",  FTYPE, 2, 3, HSIZE, 2);

/* Variable 11: Sensor data.  Sense data must be described as scalar
*  and floating point because each VCELL block may have different
*  type and dimensions.  Should've thought of this long ago.  */

   writevar("SENSOR", ETYPE, 2, 0, ESIZE, 0);

/* Variable 12: Autoscale multiplier (one per celltype) */

   writevar("ASM",    UTYPE, 2, FBsc, JSIZE, 0);

/* Variable 13: cell membrane potential */

   ibs = FBsi + RP->bsdc;
   jbs = FBwk + RP->bsdc;
   writevar("VM",     FTYPE, 3, jbs, JSIZE, 0); 

/* Variable 14: s(i) for cells */

   writevar("STATE",  FTYPE, 3, ibs, HSIZE, 0);

/* Variable 15: sbar (time-average s) for cells */

   writevar("SBAR",   FTYPE, 3, ibs, HSIZE, 0);

/* Variable 16: qbar (slower time-average s) for cells */

   writevar("QBAR",   FTYPE, 3, ibs, HSIZE, 0);

/* Variable 17: phase for cells */

   writevar("PHASE",  UTYPE, 3, 0, BSIZE, 0);

/* Variable 18: phase distribution for cells */

   writevar("PHIDIST",FTYPE, 3, jbs, JSIZE, PHASE_RES);

/* Variable 19: Integrate & Fire IZHI 'u', BREG 'w'  */

   writevar("IFUW",   FTYPE, 3, FBIu, JSIZE, 0);

/* Variables 20-23: Izhikevich 'a','b','c','d' parameters */

   writevar("IZHA",   FTYPE, 3, FBIab, JSIZE, 0);
   writevar("IZHB",   FTYPE, 3, FBIab, JSIZE, 0);
   writevar("IZHC",   FTYPE, 3, jbs, JSIZE, 0);
   writevar("IZHD",   FTYPE, 3, jbs, JSIZE, 0);

/* Variable 24: afference for a connection type */

   writevar("AK",     FTYPE, 4, jbs, JSIZE, 0);

/* Variable 25: raw afference for a connection type */

   writevar("RAW",    FTYPE, 4, jbs, JSIZE, 0);

/* Variable 26: number of connections used */

   writevar("NUK",    UTYPE, 4, 0, JSIZE, 0);

/* Variable 27:  Lij for connection--call it signed so skipped conns
*  will register as '-1'.  N.B.  Lij is currently kept in memory as
*  longs, but the max value is 2**31-1 in the design specs, so they
*  will be stored as si32.  */

   writevar("LIJ",    FTYPE, 5, 0, JSIZE, 0);

/* Variable 28: Cij for connection */

   writevar("CIJ",    FMZTYPE, 5, FBCij, HSIZE, 0);

/* Variable 29: Cij0 for connection */

   writevar("CIJ0",   FMZTYPE, 5, FBCij, HSIZE, 0);

/* Variable 30: Sj for connection.  Note:  Header as currently
*  designed can only handle global variable scales, so Sj scale
*  will be wrong (7 instead of 14) for non-REPSRC variables with
*  mV range.  Only fix would be to use a different name for these.
*  For now, decision is just to document it.  */

   writevar("SJ",     FMZTYPE, 5, ibs, HSIZE, 0);

/* Variable 31: Mij for connection */

   writevar("MIJ",    FTYPE, 5, FBsi+RP->bsdc, HSIZE, 0);

/* Variable 32: Dij for connection */

   writevar("DIJ",    UTYPE, 5, 0, BSIZE, 0);

/* Variable 33: PPFij for connection */

   writevar("PPFIJ",  FTYPE, 5, 12, HSIZE, 0);

/* Signal that this operation has been performed */

   RP->ksv |= KSDHW;

   } /* End d3gfhd */
