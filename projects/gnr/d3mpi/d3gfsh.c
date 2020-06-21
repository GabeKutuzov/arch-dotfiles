/* (c) Copyright 1997-2018, The Rockefeller University *11116* */
/* $Id: d3gfsh.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3gfsh                                 *
*              CREATE SEGMENT HEADER FOR A SIMDATA FILE                *
*                                                                      *
*  Uses the graph data file format designed by Pearson & Shu (V.3).    *
*  This routine must be called on host node after last time svitms     *
*  can change and after d3gfrl sets up suitms for each cell type       *
*  before final tree broadcast to computational nodes.                 *
*                                                                      *
*  Arguments:                                                          *
*     pgd   Ptr to SDSELECT block defining items to be saved.          *
*     gdm   Mask specifying svitems bits that are to be included.      *
*                                                                      *
*  Design note:  In R72, calculation of simdata file record length was *
*  removed to a new routine d3gfrl().  This is so the record lengths   *
*  for segments after segment 0 can be calculated and stored on the    *
*  PAR0 node of a parallel computer before the membcst during trial    *
*  series initialization and d3nset can allocate memory for mpgather.  *
*  This is necessary because the d3gfsh() call for later segments must *
*  come after segment 0 is written and therefore after the d3nset call.*
************************************************************************
*  V8A, 08/26/97, GNR - Broken out from old d3gfhd routine.  Handle    *
*                       stride in cell lists, delete ENV/REP at LEV1,  *
*                       add phidist and sbar at LEV3, add new LEV4 to  *
*                       break out individual connection types with AK  *
*                       (afference) and NUK (number-used) variables,   *
*                       add LEV5 MIJ, support recording inner cycles,  *
*                       calc length of node data and save in extras,   *
*                       turn off svitms bits for nonexistent items,    *
*                       add input array and changes array              *
*  Rev, 05/18/98, GNR - Set GFSVS bit to control segment writing       *
*  Rev, 05/30/99, GNR - Use new swapping scheme                        *
*  Rev, 08/19/00, GNR - Add PPFIJ                                      *
*  V8B, 03/10/01, GNR - Use ROCKS iteration lists, GFGLBL cell lists   *
*  V8D, 07/29/06, GNR - Now possible to record Lij even if not stored  *
*  Rev, 12/27/07, GNR - Add QBAR                                       *
*  Rev, 01/31/08, GNR - Add SAVE SIMDATA SENSE                         *
*  ==>, 02/01/08, GNR - Last mod before committing to svn repository   *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
*  V8E, 02/05/09, GNR - Add Izhikevich u variable and a,b,c,d params   *
*  V8F, 06/05/10, GNR - Add CAMID and CAMINFO variables                *
*  V8G, 08/28/10, GNR - Add ASM variable                               *
*  V8H, 11/08/10, GNR - Add SNSID variable                             *
*  Rev, 05/06/11, GNR - Use 'R' for arm, 'J' for Sj                    *
*  R67, 11/23/16, GNR - Correct lcld,ldata calc for GFSI-->Node 0      *
*  R72, 02/22/17, GNR - Remove calc of lsd1c[0] to new d3gfrl routine  *
*  R76, 12/03/17, GNR - Add VMP variable                               *
*  R77, 02/08/18, GNR - Add GFRAW for RAW variable, GFD1J for joints   *
***********************************************************************/

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "jntdef.h"
#include "armdef.h"
#include "wdwdef.h"
#include "d3fdef.h"
#include "celldata.h"
#include "tvdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"
#include "swap.h"
#include "clblk.h"
#include "simdata.h"

/*** #define DBGRFFL ***/

#define LSELECTOR 16       /* Max length of an ASCII-coded selector
                           *  and following delimiter */
#define qLSELECT "15"      /* Quoted max length of selector */
#define MXDATA    80       /* Max length of data card */

/* Global variables used to communicate between following routines */
extern struct CELLDATA CDAT;
static struct RFdef *pgdf; /* Ptr to SIMDATA file descriptor */
static char *vlist;        /* Ptr to control card image */
static int nchar;          /* Number of characters in vlist */

/*=====================================================================*
*                              writesel                                *
*  Local function to write selector card from vlist into output file   *
*=====================================================================*/

static void writesel(void) {

   size_t llist = strlen(vlist);

   rfwi4(pgdf, (si32)(-llist));
   rfwrite(pgdf, vlist, llist, ABORT);
#ifdef DBGRFFL
rfflush(pgdf, ABORT);
#endif
   cryout(RK_P2, " ", RK_LN1+1, vlist, RK_CCL, NULL);
   } /* End writesel */

/*=====================================================================*
*                              addfield                                *
*  Local function to add a field to a LEVEL card, checking whether it  *
*  will fit, and generating a continuation if not.  Note that writesel *
*  determines length by strlen, so must keep strings properly ended.   *
*  Arguments:                                                          *
*     pitem    Ptr to item                                             *
*     litem    Length of item (not counting any final '\0')            *
*=====================================================================*/

static void addfield(char *pitem, int litem) {

   /* The '>=' allows for a continuation character to be added */
   if (nchar + litem >= MXDATA) {
      strcpy(vlist+nchar,"\\");  /* Add continuation char and '\0' */
      writesel();             /* Write full line */
      strcpy(vlist,"    ");      /* Start continuation line */
      nchar = 4;
      }
   memcpy(vlist+nchar,pitem,litem); /* Move item to line */
   vlist[nchar += litem] = '\0';    /* Terminate string */
   } /* End addfield */

/*=====================================================================*
*                              addselect                               *
*  Local function to add a numeric selector to a LEVEL card.  A given  *
*  delimiter is added after the value.  Continuations are generated    *
*  when necessary.                                                     *
*  Arguments:                                                          *
*     iselect  Integer selector value                                  *
*     delim    Delimiter character                                     *
*=====================================================================*/

static void addselect(int iselect, char delim) {

   char selector[LSELECTOR];     /* ASCII selector value */
   wbcdwt(&iselect, selector, RK_LFTJ+RK_IORF+RK_NINT+LSELECTOR-2);
   selector[RK.length+1] = delim;
   addfield(selector, RK.length+2);
   } /* End addselect */

/*=====================================================================*
*                              mapilist                                *
*  Local function to map iteration list into VLIST format and count    *
*  number of entries in list.  Now handles stride != 1 and EVERY       *
*  blocks.  Colon delimiters used in earlier versions have been        *
*  replaced with +/-.                                                  *
*                                                                      *
*  This version expands EVERY blocks into full lists.  This approach   *
*  avoids having to modify getgd3, but may result in long lists in     *
*  some cases.  The alternative version (activate by defining CLEB)    *
*  adds EVERY and BASE keywords to the output to pass the necessary    *
*  information to getgd3.                                              *
*                                                                      *
*  Arguments:                                                          *
*     pil      Ptr to iteration list                                   *
*     mxi      Maximum allowed item number                             *
*  Return value:                                                       *
*     count    Number of items included in this iteration list         *
*=====================================================================*/

static long mapilist(ilst *pil, long mxi) {

   ilstitem *pcl,*pcle;       /* Ptrs to cell list and its end */
   ilstitem tag;              /* Tag bits from list item */
   long count = 0;            /* Cell counters */
#ifdef CLEB
   long remct = 0;            /* Remainder in last EVERY block */
#else
   long ievb;                 /* Increment for EVERY block */
#endif
   long ibase;                /* Base for EVERY looping */
   long ievry;                /* EVERY block size */
   long lbmx;                 /* Largest cell number in last block */
   long rs,re,ri,rv;          /* Range start, end, increment, value */
   char delim = ' ';          /* Delimiter for previous field */
   char celltxt[LSELECTOR];   /* Space for ASCII cell number */

   nchar = strlen(vlist);
   ibase = pil->evry >> IL_BITS;
   ievry = pil->evry & IL_MASK;
   pcle  = pil->pidl + pil->nusm1;
   ri = 0;                    /* Flags pickup of start/single value */
#ifdef CLEB
   lbmx  = ievry ? (mxi % ievry) + ibase - 1 : -1;
   for (pcl=pil->pidl; pcl<=pcle; pcl++) {
      if (tag = *pcl & IL_REIN) {   /* Assignment intended */
         if (tag == IL_INCR)        /* Got range increment */
            rv = ri = *pcl & IL_MASK, delim = '+';
         else                       /* Got end of range    */
            rv = re = *pcl & IL_MASK, delim = '-';
         }
      else {                  /* Got single cell or start of range */
         if (ri > 0) {
            if (rs <= lbmx) remct += (min(re,lbmx) - rs)/ri + 1;
            count += (re - rs)/ri + 1, delim = ','; }
         rs = re = rv = *pcl, ri = 1;
         }
      addfield(&delim, 1);    /* Move delim to output text */
      wbcdwt(&rv, celltxt, RK_LFTJ+RK_IORF+RK_NLONG+LSELECTOR-1);
      addfield(celltxt, RK.length+1);
      } /* End loop over cell list */
   if (ri > 0) {
      if (rs <= lbmx) remct += (min(re,lbmx) - rs)/ri + 1;
      count += (re - rs)/ri + 1; }

   /* Deal with EVERY option */
   if (ievry) {
      addfield(",EVERY ", 7);
      wbcdwt(&ievry, celltxt, RK_LFTJ+RK_IORF+RK_NLONG+LSELECTOR-1);
      addfield(celltxt, RK.length+1);
      addfield(",BASE ", 6);
      celltxt[0] = '0' + ibase;
      addfield(celltxt, 1);
      count = count * (mxi/ievry) + remct;
      }
#else
   lbmx = mxi + ibase - 1;
   if (ievry == 0) ievry = mxi;
   for (ievb=0; ievb<mxi; ievb+=ievry) {
      for (pcl=pil->pidl; pcl<=pcle; pcl++) {
         if (tag = *pcl & IL_REIN) {   /* Assignment intended */
            if (tag == IL_REND) {      /* Got end of range */
               re = (*pcl & IL_MASK) + ievb, delim = '-';
               if (re > lbmx) re = lbmx;
               rv = re;
               }
            else                       /* Got range increment */
               rv = ri = *pcl & IL_MASK, delim = '+';
            }
         else {               /* Got single cell or start of range */
            if (ri > 0) count += (re - rs)/ri + 1, delim = ',';
            rv = re = rs = *pcl + ievb, ri = 1;
            if (rs > lbmx) break;
            }
         addfield(&delim, 1); /* Move delim to output text */
         wbcdwt(&rv, celltxt, RK_LFTJ+RK_IORF+RK_NLONG+LSELECTOR-1);
         addfield(celltxt, RK.length+1);
         } /* End loop over cell list */
      } /* End ievb loop */
   if (ri > 0) count += (re - rs)/ri + 1;
#endif

   addfield(" ", 1);
   return count;
   } /* End mapilist */

/*=====================================================================*
*                               d3gfsh                                 *
*=====================================================================*/

void d3gfsh (struct SDSELECT *pgd, ui32 gdm)  {

   struct CELLTYPE *il;          /* Ptr to current cell type */
   struct CONNTYPE *ix;          /* Ptr to current connection type */
   ilst *pil;                    /* Ptr to cell list being used */
   char *prevrep;                /* Name of previous repertoire */
   ilst glilst;                  /* Global cell list */
   ulng ldata = 0;               /* Length of data per time step */
   ui32 lsui,lsvgl;              /* Global svitms codes */

/* It is necessary to determine before starting whether any selectors
*  will be written for this segment, because, if not, it will be best
*  to skip writing the SEGMENT and LENGTH records, too.  */

   RP->CP.runflags &= ~RP_KSDSG; /* Initially, nothing to write */

   /* Get masked environmental selectors */
   lsvgl = pgd->svitms;
   if (RP->CP.runflags & RP_NOINPT) lsvgl &= ~GFIA;
   if (!RP0->ljnwd)                 lsvgl &= ~GFEFF;
   if (!(pgd->pgdcam && pgd->pgdcam->utv.tvsflgs & TV_ON))
                                    lsvgl &= ~GFUTV;
   if (!pgd->pilobj)                lsvgl &= ~(GFOXY|GFOID);
   if (!pgd->pilval)                lsvgl &= ~GFVAL;
   if (!pgd->pgdsns[PGD_SENSE])     lsvgl &= ~GFSNS;
   if (!pgd->pgdsns[PGD_SNSID])     lsvgl &= ~GFSID;
   if (RP->narms <= 0)              lsvgl &= ~GFD1J;
   if (RP->nwdws <= 0)              lsvgl &= ~GFD1W;
   lsui = lsvgl & (GFD1J|GFD1W|GFIA|GFEFF|GFVAL|GFOXY|GFOID|
      GFSNS|GFSID|GFUTV);
   /* Bring in variables for celltypes stored by d3gfrl */
   for (il=RP->pfct; il; il=il->pnct) lsui |= il->suitms;
   pgd->suitms = lsui;
   if (!(lsui &= gdm)) return;   /* Exit if nothing in this segment */
   RP->CP.runflags |= RP_KSDSG;  /* Indicate need to write segment */

/* There are items to be written, so now prepare segment header */

   /* Pick up file descriptor.  File already opened by d3gfhd.  */
   pgdf = d3file[pgd->gdfile];   /* Get file descriptor */
   if (!(pgdf->iamopen & IAM_ANYOPEN)) {
      cryout(RK_E1,"0***SIMDATA FILE NOT OPEN.",RK_LN2,NULL);
      return; }

   /* Use shared global work area for variable lists */
   vlist = (char *)CDAT.psws;

/*---------------------------------------------------------------------*
*  Generate the SEGMENT record and corresponding print subtitle        *
*---------------------------------------------------------------------*/

   convrt("(NP2,'0SIMDATA selector information for segment ',"
      "J0UH8,':')", &pgd->gdseg, NULL);
   sconvrt(vlist, "('SEGMENT',UH5,UJ12)",
      &pgd->gdseg, &pgd->gdsoff, NULL);
   pgd->gdsoff = rftell(pgdf);
   writesel();

/*---------------------------------------------------------------------*
*  1) Selector for INPUT ARRAY                                         *
*                                                                      *
*  This is treated as a single array of bytes of size RP->nst          *
*---------------------------------------------------------------------*/

   if (lsui & GFIA) {
      strcpy(vlist, "LEVEL 1 ENVIRONMENT 1 IA");
      writesel();
      ldata += RP->nst;
      } /* End processing IA selector */

/*---------------------------------------------------------------------*
*  2) Selectors for WINDOWs                                            *
*                                                                      *
*  Output only those windows that move independently--                 *
*  the mask passes negative kchng and kchng=1,2,3,5,6,7.               *
*---------------------------------------------------------------------*/

   if (lsui & GFD1W) {
      struct WDWDEF  *pw;     /* Ptr to current window */
      int iwdw = 0;           /* Number of current window */
      int nwdwu = 0;          /* Number of windows used */
      /* First count them */
      for (pw=RP0->pwdw1; pw; pw=pw->pnxwdw)
         if (pw->kchng & 0x8003) ++nwdwu;
      if (nwdwu > 0) {
         strcpy(vlist, "LEVEL 1 WINDOW");
         /* Now we can be cute with singular vs plural */
         strcat(vlist, (nwdwu > 1) ? "S " : " ");
         nchar = strlen(vlist);
         for (pw=RP0->pwdw1; pw; pw=pw->pnxwdw) {
            ++iwdw;
            if (pw->kchng & 0x8003) addselect(iwdw, ',');
            }
         strcpy(vlist+nchar-1, " WINXY");
         writesel();
         ldata += (2*FMSSIZE)*nwdwu;
         }
      else
         cryout(RK_E1,"0***NO MOVING WINDOWS FOR SVITMS=W.",
            RK_LN2,NULL);
      } /* End processing WINDOW selectors */

/*---------------------------------------------------------------------*
*  3) Selectors for OBJECTs (coordinates and identifiers)              *
*                                                                      *
*  N.B.  Max object ref no. is now obtained from active objects list.  *
*---------------------------------------------------------------------*/

   if (lsui & (GFOXY|GFOID)) {
      /* Check ilst again--may have changed at Group III time */
      long lmxobj = envgmxr(RP0->Eptr);
      ilstchk(pgd->pilobj, lmxobj, "SIMDATA object list");
      strcpy(vlist, "LEVEL 1 OBJECT");
      if (pgd->pilobj->nusm1 > 0) strcat(vlist, "S");
      pgd->ngdo = (short)mapilist(pgd->pilobj, lmxobj);
      /* Note order is OBJXY first, matching d3gfsv order */
      if (lsui & GFOXY) {
         addfield("OBJXY,", 6);
         ldata += (2*FMSSIZE)*pgd->ngdo; }
      if (lsui & GFOID) {
         addfield("OBJID,", 6);
         ldata += (2*FMSSIZE)*pgd->ngdo; }
      /* Get rid of extra comma at the end */
      vlist[nchar-1] = '\0';
      writesel();
      } /* End processing OBJECT selectors */

/*---------------------------------------------------------------------*
*  4) Selectors for VALUEs                                             *
*---------------------------------------------------------------------*/

   if (lsui & GFVAL) {
      /* Check ilst again--may have changed at Group III time */
      ilstchk(pgd->pilval, (long)RP->nvblk, "SIMDATA value list");
      strcpy(vlist, "LEVEL 1 VALUE");
      if (pgd->pilval->nusm1 > 0) strcat(vlist, "S");
      pgd->ngdv = (short)mapilist(pgd->pilval, (long)RP->nvblk);
      addfield("VALUE", 5);
      ldata += FMSSIZE*pgd->ngdv;
      writesel();
      } /* End processing VALUE selectors */

/*---------------------------------------------------------------------*
*  5) Selector for EFFerence (changes array)                           *
*                                                                      *
*  N.B. GFEFF bit will have been turned off earlier if ljnwd is zero.  *
*---------------------------------------------------------------------*/

   if (lsui & GFEFF) {
      strcpy(vlist, "LEVEL 1 EFFERENCE 1 EFF");
      writesel();
      ldata += FMESIZE*RP0->ljnwd;
      } /* End processing EFFerence selector */

/*---------------------------------------------------------------------*
*  6) Selectors for SNSIDs                                             *
*---------------------------------------------------------------------*/

   if (lsui & GFSID) {
      struct GDSNS *pgsns;
      struct VCELL *pvc;
      for (pgsns=pgd->pgdsns[PGD_SNSID]; pgsns; pgsns=pgsns->pngsns) {
         pvc = pgsns->pvsns;
         if (pgsns->vcname[0] == 0)    /* External (BBD) name */
            sconvrt(vlist, "('LEVEL 1 SENSE ',J1A" qLSELECT
               ",5HSNSID)", getrktxt(pgsns->hgsnm), NULL);
         else
            sconvrt(vlist, "('LEVEL 1 SENSE ',J0A" qLSN ",1H#,J1IH4",
               ",5HSNSID)", pgsns->vcname, &pgsns->hgsnm, NULL);
         ldata += 2*FMSSIZE;
         writesel();
         }
      } /* End processing SENSE selectors */

/*---------------------------------------------------------------------*
*  7) Selectors for JOINT coordinates                                  *
*                                                                      *
*  Since each arm may have a different number of joints,               *
*  each must have its own LEVEL 1 ARM selector card.                   *
*---------------------------------------------------------------------*/

   if (lsui & GFD1J) {
      struct ARMDEF *pa;      /* Ptr to current arm */
      int iarm = 0;           /* Number of current arm */
      for (pa=RP0->parm1; pa; pa=pa->pnxarm) {
         ++iarm;
         if (pa->njnts > 1) {
            sconvrt(vlist, "('LEVEL 1 ARM ',J0I8)", &iarm, NULL);
            writesel();
            sconvrt(vlist, "('LEVEL 2 JOINT 1-',J1IC8,'JOINTXY')",
               &pa->njnts, NULL);
            ldata += (2*FMSSIZE)*pa->njnts; }
         writesel();
         }
      } /* End processing JOINT selectors */

/*---------------------------------------------------------------------*
*  8) Selectors for SENSEs                                             *
*                                                                      *
*  Since each sense may have a different number of variables, each     *
*  must have its own LEVEL 1 SENSE selector card and then a LEVEL 2    *
*  card to describe the dimensionality.  But all are converted to E    *
*  format so there only needs to be one variable name.  Internal       *
*  senses may have a name and a number.  This is handled by concat-    *
*  enating the name and number with a '#' between them, e.g. 'VT#1'.   *
*---------------------------------------------------------------------*/

   if (lsui & GFSNS) {
      struct GDSNS *pgsns;
      struct VCELL *pvc;
      for (pgsns=pgd->pgdsns[PGD_SENSE]; pgsns; pgsns=pgsns->pngsns) {
         pvc = pgsns->pvsns;
         if (pgsns->vcname[0] == 0)    /* External (BBD) name */
            sconvrt(vlist, "('LEVEL 1 SENSE ',J0A" qLSELECT ")",
               getrktxt(pgsns->hgsnm), NULL);
         else
            sconvrt(vlist, "('LEVEL 1 SENSE ',J0A" qLSN ",1H#,J0IH4)",
               pgsns->vcname, &pgsns->hgsnm, NULL);
         writesel();
         /* Level 2 card with dimensionality info */
         sconvrt(vlist, "('LEVEL 2 SENSOR 1-',J1IL8,'SENSOR')",
            &pvc->nvcells, NULL);
         ldata += FMESIZE*pvc->nvcells;
         writesel();
         }
      } /* End processing SENSE selectors */

/*---------------------------------------------------------------------*
*  9) Selectors for CAMID and CAMINFO                                  *
*                                                                      *
*  At present these cannot be selected separately.  This could change. *
*---------------------------------------------------------------------*/

   if (lsui & GFUTV) {
      strcpy(vlist, "LEVEL 1 CAMERA 1 CAMID,CAMINFO");
      writesel();
      ldata += (2*FMSSIZE) + NUTVINFO;
      } /* End processing CAMID,CAMINFO selectors */

/*---------------------------------------------------------------------*
*                         Loop over celltypes                          *
*---------------------------------------------------------------------*/

   /* Set up an ilst that can be used to map all cells in cell type */
   memset((char *)&glilst, 0, sizeof(glilst));
   glilst.pidl  = (ilstitem *)&glilst.frst;
   prevrep = "";              /* No previous repertoire */
   for (il=RP->pfct; il; il=il->pnct) {

      struct REPBLOCK *ir;    /* Ptr to current region */
      struct CLBLK *pclb;
      ulng ncells = 0;        /* Total output cells, all nodes */
      ui32 ilsu;              /* Flags for cell & connection items */
      ui32 svand;             /* Temp for GFAFF,GFRAW,GFNUK tests */

      if ((ilsu = il->suitms & gdm) == 0)
         goto NO_CONNTYPE_VARIABLES;
      ir = il->pback;
      pclb = il->psdclb;

      /* The LEVEL 1 card can be omitted if the repertoire name
      *  is the same as that on the previous cell list.  */
      if (strncmp(prevrep, ir->sname, LSNAME)) {
         prevrep = ir->sname;
         sconvrt(vlist, "('LEVEL 1 REGION ',J0A" qLSN ")",
            prevrep, NULL);
         writesel();
         }

      sconvrt(vlist, "('LEVEL 2 CELLTYPE ',J0A" qLSN ")",
         il->lname, NULL);

/*---------------------------------------------------------------------*
*  10) Selectors for ASM                                               *
*---------------------------------------------------------------------*/

      if (ilsu & GFASM) {
         nchar = strlen(vlist);
         addfield(" ASM", 4);
         ldata += JSIZE; }
      writesel();

/*---------------------------------------------------------------------*
*  11) Selectors for STATE, PHASE, IFUW, VM, SBAR, QBAR, PHIDIST, and  *
*        Izhikevich a,b,c,d (only if variable) in that order.          *
*                                                                      *
*     It is assumed here that all the cells requested do in fact       *
*     exist.  This is assured by d3clck().                             *
*---------------------------------------------------------------------*/

      /* Map cell list to LEVEL 3 CELL control card.  If GFGLBL bit
      *  is set, use all cells, even if there is an explicit list.  */
      strcpy(vlist, "LEVEL 3 CELL");
      if (ilsu & GFGLBL) {
         pil = &glilst;
         glilst.nusm1 = (long)(il->nelt > 1);
         glilst.last = IL_REND | (long)(il->nelt - 1);
         }
      else {
         pil = pclb->pclil;
         }

      if (pil->nusm1 > 0) strcat(vlist, "S"); /* Make that "CELLS" */
      ncells = mapilist(pil, (long)il->nelt);

      /* State, Izhu, phase, Vm, sbar, qbar, phase distribution will be
      *  written only if requested and they exist.  No error if they
      *  don't exist.  Order of terms chosen to help make neater
      *  dumps.  */
      if (ilsu & GFSI)     addfield("STATE,", 6);
      if (ilsu & GFPHI)    addfield("PHASE,", 6);
      if (ilsu & GFIFUW)   addfield("IFUW,", 5);
      if (ilsu & GFVMP)    addfield("VM,"  , 3);
      if (ilsu & GFSBAR)   addfield("SBAR,", 5);
      if (ilsu & GFQBAR)   addfield("QBAR,", 5);
      if (ilsu & GFDIS)    addfield("PHIDIST,", 8);
      if (ilsu & GFABCD) {
         /* il->prfi must exist or GFABCD bit will be off */
         struct IZHICOM *piz = (struct IZHICOM *)il->prfi;
         if (piz->izra) addfield("IZHA,", 5);
         if (piz->izrb) addfield("IZHB,", 5);
         if (piz->izrc) addfield("IZHC,", 5);
         if (piz->izrd) addfield("IZHD,", 5);
         }
      /* Get rid of extra blank or comma at the end */
      vlist[nchar-1] = '\0';
      writesel();

/*---------------------------------------------------------------------*
*  12) Selectors for AK, RAW (afference) and NUK (number conns used)   *
*                                                                      *
*     All cell data are now consolidated under a single LEVEL 3 card.  *
*     In earlier versions of CNS, STATE and PHASE had their own LEV3.  *
*---------------------------------------------------------------------*/

      /* Quietly omit this whole section if nothing was requested.
      *  Logic above leaves ilsu clear if there are no conntypes.  */
      ilsu &=
         (GFRAW|GFAFF|GFNUK|GFLIJ|GFCIJ|GFCIJ0|GFSJ|GFDIJ|GFMIJ|GFPPF);
      if (!ilsu) goto NO_CONNTYPE_VARIABLES;

/* If the only variables to be written out for all connection types
*  are AK and/or RAW and/or NUK, and those written are written for all
*  conntypes, then a single LEVEL 4 CONNTYPE selector will do for all
*  of them.
*  Rev, 09/01/10, GNR - Make svand a local variable here, at cost of
*     an extra ix loop--collided with ctwk.phs1 items when it was
*     ctwk.gfsh.svand.  */

      svand = (GFRAW|GFAFF|GFNUK);
      for (ix=il->pct1; ix; ix=ix->pct) svand &= ix->svitms;
      if ((ilsu ^ svand) == 0) {
         strcpy(vlist, "LEVEL 4 CONNTYPE");
         if (il->nct > 1)
            sconvrt(vlist, "(T17,'S 1-',J1UH8)", &il->nct, NULL);
         else
            strcat(vlist, " 1 ");
         if (ilsu & GFAFF) strcat(vlist, "AK,");
         if (ilsu & GFRAW) strcat(vlist, "RAW,");
         if (ilsu & GFNUK) strcat(vlist, "NUK,");
         /* Get rid of extra comma at the end */
         vlist[strlen(vlist)-1] = '\0';
         writesel();
         }

/* Loop over connection types and prepare a LEVEL 4 CONNTYPE card for
*  each.  Add the AK, RAW and NUK variables at this level if needed. */

      else for (ix=il->pct1; ix; ix=ix->pct) {
         ui32 ixsu = ix->suitms & gdm;
         if (!ixsu) continue;
         sconvrt(vlist, "('LEVEL 4 CONNTYPE ',J1UH8)", &ix->ict, NULL);
         if (ixsu & GFAFF) strcat(vlist, "AK,");
         if (ixsu & GFRAW) strcat(vlist, "RAW,");
         if (ixsu & GFNUK) strcat(vlist, "NUK,");
         /* Get rid of extra comma at the end */
         vlist[strlen(vlist)-1] = '\0';
         writesel();

/*---------------------------------------------------------------------*
*  13) Selectors for LIJ, CIJ, CIJ0, SJ, MIJ, DIJ, PPFIJ               *
*                                                                      *
*     These share a common LEVEL 5 card because data are written       *
*     connection-by-connection.                                        *
*---------------------------------------------------------------------*/

         if ((ixsu & (GFLIJ|GFCIJ|GFCIJ0|GFSJ|GFMIJ|GFDIJ|GFPPF)) == 0)
            continue;
         /* Create LEVEL 5 CONNECTION card for nc connections */
         strcpy(vlist, "LEVEL 5 CONNECTION");
         {  si32 lnc1 = (si32)ix->nc - 1;
            if (lnc1 > 1)
               sconvrt(vlist, "(T19,'S 0-',J1IJ8)", &lnc1, NULL);
            else
               strcat(vlist, " 0 ");
            }
         /* Note order is Lij first, matching d3gfsv order */
         if (ixsu&GFLIJ)  strcat(vlist,"LIJ,");
         if (ixsu&GFCIJ)  strcat(vlist,"CIJ,");
         if (ixsu&GFCIJ0) strcat(vlist,"CIJ0,");
         if (ixsu&GFSJ)   strcat(vlist,"SJ,");
         if (ixsu&GFMIJ)  strcat(vlist,"MIJ,");
         if (ixsu&GFDIJ)  strcat(vlist,"DIJ,");
         if (ixsu&GFPPF)  strcat(vlist,"PPFIJ,");
         /* Get rid of extra comma at the end */
         vlist[strlen(vlist)-1] = '\0';
         writesel();
         } /* End loop over connection types */

NO_CONNTYPE_VARIABLES: ;
      /* Update node data and total data counts */
      ldata += ncells * (gdm & (GFLIJ|GFDIJ|GFABCD) ?
         il->lsd1c0 : il->lsd1c);
      } /* End cell type loop */

/*---------------------------------------------------------------------*
*           Finish up with record length description record            *
*  Note:  If this is segment 0, will pick up nits = default ntc2 = 1.  *
*  During regular simulations, a segment can have a mixture of series  *
*  with ntc = ntc1 and ntc = ntc2.  Reader programs must handle this.  *
*---------------------------------------------------------------------*/

   {  long nits = (pgd->svitms & GFNTC) ? max(RP->ntc1,RP->ntc2) : 1;
      sconvrt(vlist, "(6HLENGTH,IL12,5H NITS,IL9)",
         &ldata, &nits, NULL);
      writesel();
      }

   /* Clear print subtitle */
   cryout(RK_P2," ",RK_LN1+RK_NTSUBTTL+1,NULL);

   } /* End d3gfsh */
