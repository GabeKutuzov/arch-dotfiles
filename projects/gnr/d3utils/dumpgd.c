/* (c) Copyright 1997-2017, The Rockefeller University *11113* */
/***********************************************************************
*                               DUMPGD                                 *
*                                                                      *
*  Program to dump the contents of an NSI "GRAFDATA" file, version 3.  *
*  This program also does a fair amount of error checking in the data. *
*                                                                      *
*  Synopsis:                                                           *
*     dumpgd [-c<n>] [-l<n>] [-p] [-t<m>-<n>] <file>                   *
*  The following options are recognized:                               *
*     -c<n> Output is in <n> columns.  <n> must be larger than 80.     *
*     -l<n> Omit printing data at hierarchy levels higher than <n>.    *
*     -p    Omit pagination.                                           *
*     -t<m>-<n>   Dump only time steps <m> through <n>                 *
*  <file>   The name of the file to be dumped.                         *
*                                                                      *
*  Abexit codes 570-575 are assigned to this program.                  *
*                                                                      *
*  V1A, 09/08/97, GNR - Newly written.                                 *
*  V2A, 11/13/97, GNR - Major revision to use new gditools library     *
*  Rev, 05/22/99, GNR - Use new swapping scheme                        *
*  V2B, 08/01/08, GNR - Add -t option                                  *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
*  Rev, 11/18/10, GNR - Fix bugs handling 64-bit longs, allow 64-bit   *
*                       fixed-point (although none in current files).  *
*  Rev, 12/03/16, GNR - Bug fix:  Check for and report early EOF       *
*  Rev, 03/01/17, GNR - Add new GDV type codes                         *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "graphd.h"        /* Brings in rfdef.h */
#include "swap.h"

/* Program configuration parameters */
#define ITEM_WIDTH      8  /* Printed width of one numerical item */
#define qIW            "8" /* Quoted ITEM_WIDTH */
#define ITEM_SPACE      2  /* Columns between items */
#define LEV_INDENT      2  /* Columns to indent per level */

/* Global data */
static char *fname = NULL; /* Name of file to be dumped */
static struct RFdef *pgdf; /* Ptr to RFdef for input file */
static char ***lnames;     /* Ptr to level names data */
static GDVar *pv0;         /* Ptr to variable information */
static char *pd;           /* Ptr to data buffer */
static char *spacer;       /* Just some blanks */
static si32 nt1,nt2;       /* Time steps to dump */
static int plev = 0;       /* Print level */
static char oval[ITEM_WIDTH];    /* Converted output value */


/*=====================================================================*
*                               dgdparm                                *
*                                                                      *
*  Analyze command-line parameters.  Note that error messages here are *
*  generated with fprintf, not cryout.  At this early stage, we don't  *
*  really want to create page titles and all that.                     *
*  Arguments:                                                          *
*     argc,argv   Values of argc,argv passed from system to main pgm.  *
*=====================================================================*/

static void dgdparm(int argc, char *argv[]) {

   int iarg;

   if (argc < 2 || argc > 6) {
      fprintf(stderr, "\nSynopsis: "
         "dumpgd [-c<n>] [-l<n>] [-p] [-t<m>-<n>] <file>\n");
      exit(11); }

   for (iarg=1; iarg<argc; iarg++) {
      char *ps = argv[iarg] + 2;
      int la = strlen(argv[iarg]);

      if (strncmp(argv[iarg], "-c", 2) == 0 && la > 2) {
         RK.pgcls = (short)ibcdin(RK_IORF+RK_CTST+RK_QPOS+la-3,ps) + 1;
         if (RK.pgcls < CDSIZE + 1 || RK.iexit) {
            fprintf(stderr, "\nInvalid page width\n");
            exit(12); }
         }
      else if (strncmp(argv[iarg], "-l", 2) == 0 && la > 2) {
         plev = (int)ibcdin(RK_IORF+RK_CTST+RK_QPOS+RK_ZTST+la-3,ps);
         if (RK.iexit) {
            fprintf(stderr, "\nInvalid level number\n");
            exit(13); }
         }
      else if (strcmp(argv[iarg], "-p") == 0)
         nopage(RK_PGNONE);
      else if (strncmp(argv[iarg], "-t", 2) == 0 && la > 2) {
         /* This parsing allows just m, just -n, or both */
         char *pm = strchr(ps,'-');
         if (pm != ps) {      /* Got a lower bound */
            si32 ll = (pm > ps) ? pm - ps - 1 : la - 3;
            nt1 = ibcdin(RK_IORF+RK_CTST+RK_QPOS+RK_ZTST+ll,ps);
            }
         if (pm) {            /* Got an upper bound */
            si32 ll = la - (pm - ps) - 4;
            nt2 = ibcdin(RK_IORF+RK_CTST+RK_QPOS+RK_ZTST+ll,pm+1);
            }
         if (nt2 < nt1) {     /* Swap just in case */
            nt1 = nt1 ^ nt2; nt2 = nt1 ^ nt2; nt1 = nt1 ^ nt2; }
         }
      else if (argv[iarg][0] == '-') {
         fprintf(stderr, "\nUnrecognized command-line switch\n");
         exit(14); }
      else if (fname) {
         fprintf(stderr,
            "\nApparently more than one file name entered\n");
         exit(15); }
      else {
         if (!(fname = malloc(la+1))) {   /* Assignment intended */
            fprintf(stderr,
               "\nUnable to allocate space for file name\n");
            exit(16); }
         strcpy(fname, argv[iarg]);
         }
      } /* End loop over arguments */

   if (!fname) {
      fprintf(stderr,
         "\nYou must enter the name of the file to be dumped\n");
      exit(17); }

   } /* End dgdparm() */


/*=====================================================================*
*                              gcblevnm                                *
*                                                                      *
*  Callback routine used by gdilvlnm to print level name info.         *
*  Arguments:                                                          *
*     ilvl        Number of the level being processed.                 *
*     klvl        GDL_FIRST (name is first) + GDL_LAST (name is last). *
*     name        Ptr to a level name.                                 *
*=====================================================================*/

static void gcblevnm(int ilvl, int klvl, char *name) {

   int NotLast = klvl & GDL_NLAST;
   int Width = strnlen(name, LLevName);

   if (klvl & GDL_FIRST)
      convrt("(P2I4,4X)", &ilvl, NULL);
   else if (Width + (NotLast ? 1 : 0) > RKC.PU.rmcls)
      convrt("(P2,8X)", NULL);
   cryout(RK_P2, name, Width, ",", (NotLast ? 1 : RK_SKIP), NULL);

   } /* End gcblevnm() */


/*=====================================================================*
*                               gcbvar                                 *
*                                                                      *
*  Callback routine used by gdivars to print variable descriptions     *
*  and to let us fill in pv->vic with output conversion codes.         *
*  Arguments:                                                          *
*     ivar        Number of the variable being processed.              *
*     pvd         Ptr to the data portion of the variable descriptor   *
*                 record for this variable.                            *
*     lvd         Length of pvd text string.                           *
*     pv          Ptr to the GDVar constructed for this variable.      *
*                 At gcbvar call, all fields but vic are filled in.    *
*=====================================================================*/

static void gcbvar(int ivar, char *pvd, int lvd, GDVar *pv) {

   convrt("(P2I4,4X)", &ivar, NULL);
   cryout(RK_P2, pvd, lvd, NULL);

   pv->vic = 0;
   switch (pv->vtype) {
   case GDV_FixMZero:
      pv->vic |= RK_MZERO;
      goto GDVFix;
   case GDV_UFix:
      pv->vic |= RK_NUNS;              /* No break here */
   case GDV_Fix:
GDVFix:
      pv->vic |= RK_IORF + ITEM_WIDTH - 2;
      switch (pv->vlen) {
      case 1:  pv->vic |= RK_NBYTE; break;
      case 2:  pv->vic |= RK_NHALF; break;
      case 4:  pv->vic |= RK_NI32;  break;
      case 8:  pv->vic |= RK_NI64;  break;
      default: abexitm(571, "Variable length is not 1,2,4, or 8");
         } /* End vlen switch */
      if (pv->vscl > 0)
         pv->vic |= (pv->vscl<<RK_BS) + RK_AUTO;
      break;
   case GDV_Real:
      pv->vic = RK_SNGL+RK_IORF+RK_AUTO+RK_UFLW + ITEM_WIDTH - 2;
      break;
   case GDV_Dble:
      pv->vic = RK_DBL+RK_IORF+RK_AUTO+RK_UFLW + ITEM_WIDTH - 2;
      break;
   default:
      break;
      } /* End vic setting */

   } /* End gcbvar() */


/*=====================================================================*
*                              gcbparse                                *
*                                                                      *
*  Callback routine used by gdiparse to print selector cards and       *
*  their continuations.                                                *
*  Arguments:                                                          *
*     psd         Ptr to the data portion of the selector record.      *
*     lsd         Length of psd text string.                           *
*=====================================================================*/

static void gcbparse(char *psd, int lsd) {

   cryout(RK_P2, " ", RK_LN1+1, psd, lsd, NULL);

   } /* End gcbparse() */


/*=====================================================================*
*                              nodeprint                               *
*                                                                      *
*  Recursively print the contents of a node and all its descendents.   *
*  Argument                                                            *
*     ptree       Ptr to head of tree to be printed.                   *
*=====================================================================*/

void nodeprint(GDNode *ptree) {

   GDNode *pn;                /* Ptr to current node */

/* Loop over this node and its linear (left) descendents.
*  If not printing at current level, skip over the data.  */

   for (pn=ptree ; pn; pn=pn->pns) {

      int lev = pn->level;
      if (lev > plev)
         rfseek(pgdf, pn->ldata*pn->nitms, SEEKREL, ABORT);
      else {

/* Loop over selectors and ranges on this node */

         GDSel *ps, *pse = pn->psel + pn->nsel;
         char *plnm = lnames[lev-1][pn->levid-1];
         long iter1,iter2,iiter;
         int indent = LEV_INDENT * lev;
         unsigned int ccis1 = (RK_LN1+ITEM_SPACE+1) + indent;
         unsigned int ccis2 = (RK_LN1+2*ITEM_SPACE+1) + indent;

         for (ps=pn->psel; ps<pse; ps++) {
            switch (ps->ks) {
            case GDS_NUM:
               iter1 = iter2 = ps->us.it;
               iiter = 1;
               break;
            case GDS_RANGE:
               iter1 = ps->us.r.is;
               iter2 = ps->us.r.ie;
               iiter = ps->us.r.ii;
               break;
            case GDS_NAME:
               iter1 = iter2 = 0;
               iiter = 1;
               } /* End setting iteration limits */

            for ( ; iter1 <= iter2; iter1 += iiter) {

               /* Hope these vars are not saved during recursion... */
               unsigned short *pvi, *pvie = pn->pvin + pn->nvar;

/* Print heading identifying level, category, and selector.
*  convrt() codes are chosen to give this info a fixed length.  */

               if (ps->ks == GDS_NAME)
                  convrt("(H0RX,'LEVEL ',JI4,A" qLLN "XA" qLSN ")",
                     &indent, &lev, plnm, ps->us.in, NULL);
               else
                  convrt("(H0RX,'LEVEL ',JI4,A" qLLN "XJIL" qLSN ")",
                     &indent, &lev, plnm, &iter1, NULL);

/* Print variables on this node */

               for (pvi=pn->pvin; pvi<pvie; pvi++) {

                  GDVar *pv = pv0 + *pvi;
                  char *px, *pxe;
                  int nx = pv->vdim;
                  int lx = pv->vlen;
                  size_t lxall = nx * lx;

                  /* If the data won't fit on the current line,
                  *  start a new one with an extra two indents.  */
                  if (ITEM_SPACE + LVarName + ITEM_WIDTH*nx > RK.rmcls)
                     cryout(RK_P2, spacer, ccis1, NULL);

                  /* Print the name of the variable.  (Use cryout
                  *  for the blanks to avoid starting a new line,
                  *  but use convrt for the name to pad width.)  */
                  cryout(RK_P2, spacer, ITEM_SPACE, NULL);
                  convrt("(A" qLVN ")", pv->vname, NULL);

                  /* Read the data from the input file */
                  if (!rfread(pgdf, pd, lxall, ABORT)) {
                     abexitm(572,"EOF on input file, dump terminated");
                     }

                  /* Loop over these items, unpack and swap them as
                  *  required, then convert and print.  To save a
                  *  little time, put the switch outside the loop.  */
                  px = pd; pxe = pd + lxall;
                  switch (pv->vtype) {

                  case GDV_Fix:        /* Signed fixed-point */
                  case GDV_UFix:       /* Unsigned fixed-point */
                  case GDV_FixMZero:   /* Fixed-point w/minus zero */
                     for ( ; px<pxe; px+=lx) {
#if BYTE_ORDRE > 0
                        wbcdwt(px, oval+1, pv->vic);
#else
                        char Fdata[WSIZE];
                        int i;
                        for (i=0; i<lx; ++i) Fdata[i] = px[lx-i-1];
                        wbcdwt(Fdata, oval+1, pv->vic);
#endif
                        cryout(RK_P2, spacer, (ITEM_WIDTH > RK.rmcls) ?
                           ccis2 : RK_SKIP, oval, ITEM_WIDTH, NULL);
                        } /* End loop over array of one variable */
                     break;

                  case GDV_Real:       /* Real */
                     for ( ; px<pxe; px+=lx) {
                        float Rdata = bemtor4(px);
                        bcdout(pv->vic, oval+1, (double)Rdata);
                        cryout(RK_P2, spacer, (ITEM_WIDTH > RK.rmcls) ?
                           ccis2 : RK_SKIP, oval, ITEM_WIDTH, NULL);
                        } /* End loop over array of one variable */
                     break;

                  case GDV_Dble:       /* Double-precision real */
                     for ( ; px<pxe; px+=lx) {
                        double Ddata = bemtor8(px);
                        bcdout(pv->vic, oval+1, Ddata);
                        cryout(RK_P2, spacer, (ITEM_WIDTH > RK.rmcls) ?
                           ccis2 : RK_SKIP, oval, ITEM_WIDTH, NULL);
                        } /* End loop over array of one variable */
                     break;

                  case GDV_Color:      /* Colored pixel data */
                     /* To allow for the possibility that pixels of
                     *  more than one width are present in the same
                     *  file, the prestored 'vic' code is not used.  */
                     memset(oval, ' ', ITEM_WIDTH);
                     for ( ; px<pxe; px+=lx) {
                        ui32 Kdata = bemtoi4(px);
                        switch (lx) {
                        case 1:
                           ubcdwt(RK_BYTE+RK_OCTF+RK_PAD0+2,
                              oval+2, Kdata>>24);
                           break;
                        case 2:     /* Repackage as 24-bit */
                           Kdata = ((Kdata&0x7c000000U)>> 7) +
                                   ((Kdata&0x03e00000U)>>10) +
                                   ((Kdata&0x001f0000U)>>13) ;
                           ubcdwt(RK_HEXF+RK_PAD0+5, oval+2, Kdata);
                           break;
                        case 3:
                           ubcdwt(RK_HEXF+RK_PAD0+5, oval+2, Kdata>>8);
                           } /* End length switch */
                        cryout(RK_P2, spacer, (ITEM_WIDTH > RK.rmcls) ?
                           ccis2 : RK_SKIP, oval, ITEM_WIDTH, NULL);
                        } /* End loop over array of one variable */

                     } /* End switch over variable type */

                  } /* End loop over variables on this node */

               /* If there is a lower-level (right) descendent,
               *  print it and the tree below it.  */
               if (pn->pls) nodeprint(pn->pls);
               } /* End range/item loop */

            } /* End loop over selectors */

         } /* End else level <= plev */

      } /* End while (ptree) */

   } /* End nodeprint() */


/*=====================================================================*
*                        dumpgd - main program                         *
*=====================================================================*/

int main(int argc, char *argv[]) {

   /* Title for printed output */
   static char ttl[] = "TITLE  GRAFDATA File Dump - Version 3 ";

   GDNode *ptree = NULL;         /* Ptr to hierarchy of selectors */

   size_t lpd = 0;               /* Current length of data buffer */
   size_t mxitsz;                /* Maximum item size in segment */
   si32 istep,icyc;              /* Number of current step, cycle */
   si32 lsr;                     /* Length or step number record */
   int iseg;                     /* Number of the current segment */
   int lspacer;                  /* Length of level indent spacer */
   int nits;                     /* Number of iterations/cycle */
   int nlvls;                    /* Number of levels */
   int wplev;                    /* Working print level */

/* Initialize ROCKS I/O system */

   RK.iexit = 0;
   settit(ttl);
   setpid("dumpgd");
   memset(oval,   ' ', ITEM_WIDTH);

/* Analyze command-line parameters */

   nt1 = 0, nt2 = SI32_MAX;
   dgdparm(argc, argv);

/* Attempt to open the specified input file */

   pgdf = gdiopen(fname);

/* Dump the global header information */

   cryout(RK_P1, "0HEADER INFORMATION FOR ", RK_LN2+RK_NFSUBTTL,
      fname, RK_CCL, NULL);
   cryout(RK_P2, "0IDENT: ", RK_LN2, gdickid(), LIdRec, NULL);
   cryout(RK_P2, "0TITLE: ", RK_LN2, gdititle(), LTitRec, NULL);
   cryout(RK_P2, "0TIME:  ", RK_LN2, gditime(), LTimeStamp, NULL);

/* Set default level dump level.  Print the names of the levels and
*  store them for checking against the names occurring in the data.  */

   nlvls = gdinlvls();
   if (plev <= 0) plev = nlvls;  /* Default: print all */
   wplev = plev;                 /* For resetting after skip */
   cryout(RK_P1, "0Names of the data levels for ", RK_LN2+RK_NFSUBTTL,
      fname, RK_CCL, NULL);
   lnames = gdilvlnm(gcblevnm);

/* Dump the definitions of the variables in this file and store
*  them for use in formatting the data listing.  Make a bcdout
*  code suitable for converting an item of each type.  */

   cryout(RK_P1, "0Definitions of data variables in ",
      RK_LN3+RK_NFSUBTTL, fname, RK_CCL,
      " Num    Name      Typ Lev Scl Len    Dim", RK_LN0, NULL);
   pv0 = gdivars(gcbvar);

/* Make some spaces for indenting data according to level.  */

   lspacer = 2 + 2*ITEM_SPACE + nlvls*LEV_INDENT;
   spacer = mallocv(lspacer, "Blank Space");
   memset(spacer, ' ', lspacer);

/*---------------------------------------------------------------------*
*                        End of Global Headers                         *
*                        Expect Segment Header                         *
*---------------------------------------------------------------------*/

ReadNewSegment:
   gdiclear(&ptree);

   iseg = gdiqseg();
   convrt("(NP1,'0DATA SEGMENT ',J1I8,'HEADER')", &iseg, NULL);

   mxitsz = gdiparse(&ptree, gcbparse);
   nits = gdinits();

   /* Make sure pd array is long enough to read any data */
   if (mxitsz > lpd) {
      pd = reallocv(pd, mxitsz, "Data buffer");
      lpd = mxitsz; }

/*---------------------------------------------------------------------*
*                     Read and dump data records                       *
*---------------------------------------------------------------------*/

   istep = icyc = -1;
   for (;;) {

      /* Read, check, and print time step and cycle numbers.
      *  Note that cycles/trial < nits must be allowed, because
      *  this happens in CNS with "frozen" trial series.  */
      lsr = rfri4(pgdf);
      if (pgdf->lbsr == ATEOF) {
         abexitm(572,"EOF on input file, dump terminated");
         }
      if (lsr < 0) break;

      if (lsr == istep)
         ++icyc;
      else {
         if (icyc > nits)
            abexitm(570, "More than NITS cycles found.");
         istep = lsr, icyc = 1;
         }

      /* Skip if outside nt1,nt2 limits, but do not terminate--
      *  user might want to see these same trials in next segment */
      if (istep < nt1 || istep > nt2)
         plev = 0;
      else {
         plev = wplev;
         convrt("(NP1,'0DATA FOR TIME STEP ',J1IJ12,'CYCLE ',J0IJ12)",
            &istep, &icyc, NULL);
         }

      /* Traverse the data selector tree recursively and print */
      nodeprint(ptree);

      } /* End reading data records */

   if (lsr != EndMark) {
      /* Back up over the data just read so gdiparse can reread it */
      rfseek(pgdf, -sizeof(lsr), SEEKREL, ABORT);
      goto ReadNewSegment;
      }

   gdiclose();
   cryout(RK_P1, "0DUMP OF ", RK_LN2, fname, RK_CCL,
      " COMPLETED.", RK_CCL+RK_FLUSH, NULL);

   return 0;
   } /* End dumpgd() */
