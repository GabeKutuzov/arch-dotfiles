/* (c) Copyright 1991-2017, The Rockefeller University *11115* */
/* $Id: d3fchk.c 75 2017-10-13 19:38:52Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               d3fchk                                 *
*                                                                      *
*     This routine performs all checking relating to various kinds of  *
*  input files used by CNS.  In particular, it:                        *
*     (1) Opens any SAVENET files to be read for restoring network     *
*         and matches headers with current input, setting variables    *
*         in D1BLK, CELLTYPE, CONNTYPE structures to control d3rstr.   *
*         See d3save.c for discussion of little- vs big-endian issues. *
*     (2) If reading Darwin 1 data, opens files and checks headers.    *
*                                                                      *
************************************************************************
*  V5C, 12/10/91, GNR - Newly written, with replay piece from main     *
*  Rev, 02/14/92, GNR - Add support for Vers 2 D1 input with grpno's   *
*  Rev, 03/12/92, GNR - Merge in former d3savchk routine               *
*  Rev, 09/28/92, GNR - Make SAVENET file little-endian on all systems *
*  V6D, 02/05/94, GNR - Add support for axonal delay, V4 SAVENET file  *
*  V7B, 07/07/94, GNR - Eliminate klcd & warnings when override set,   *
*                       handle machine-dependent padding in ls data,   *
*                       keep unused files to d3rstr so fno preserved   *
*  V8A, 11/27/96, GNR - Remove support for non-hybrid version          *
*  Rev, 09/28/97, GNR - Version 8 SAVENET file with self-descriptive   *
*                       header data, independent allocation for each   *
*                       conntype, add SFREPHDR, GC & MOD phase seeds.  *
*  Rev, 05/23/99, GNR - Use new swapping scheme                        *
*  Rev, 07/23/00, GNR - Check xoff, yoff, loff with rules J,X          *
*  Rev, 11/24/00, GNR - Add RADIUS to conntype block                   *
*  V8D, 06/23/05, GNR - Add conductances and ions, remove replays,     *
*                       correct header length conversion code          *
*  ==>, 01/06/08, GNR - Last mod before committing to svn repository   *
*  V8E, 02/06/09, GNR - Add Izhikevich neurons                         *
*  V8G, 09/04/10, GNR - Add autoscale                                  *
*  V8H, 10/20/10, GNR - Add xnoff,lxn, allow earlier w/nuklen == lxn   *
*  R65, 01/02/16, GNR - n[gq][xyg] to ui16, ngrp to ui32, ncpg to ubig *
*  R67, 04/27/16, GNR - Remove Darwin 1 support                        *
*  R72, 03/18/17, GNR - Remove KGNDG support                           *
*  R74, 08/23/17, GNR - Reorder SFCNHDR items, add sfcncmin(not SFMCK) *
*  R75, 09/22/17, GNR - Add tcwid                                      *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "armdef.h"
#include "allo_tbl.h"
#include "d3fdef.h"
#include "savblk.h"
#include "rocks.h"
#include "rkxtra.h"
#include "bapkg.h"
#include "rkarith.h"

/* Maximum length of one line in error messages */
#define MaxHMErrLine    80

/* cryout argument to generate FILE_ERR */
#define CROFE  (RK_P1 + (FILE_ERR << 12))

/* Macro to check whether a header item was received */
#define SFHGot(hdr, itm) (h->hdr.itm.k.type & SFGOT)

/* Struct used to allocate space for all header info */
struct SFALLHDRS {
   struct SFHDR    sfhdr;
   struct SFHDR2   sfhdr2;
   struct SFREPHDR rephdr;
   struct {    /* Temps for checking retrieved rep info */
      ui16  trngx,trngy;
      short tgpoff[GPNDV];
      } trp;
   struct SFCTHDR  cthdr;
   struct {    /* Temps for checking retrieved ct info */
      int   tlnel;
      short tctoff[CTNDV];
      ui16  tlls,tnizv;
      } tct;
   struct SFCNHDR  cnhdr;
   struct {    /* Temps for checking retrieved cn info */
      ui32  tnnc;
      ui16  tnlc,tnlxn;
      short tnnbc,tncbc;
      ui32  tnkgen;
      short tcnoff[CNNDV],txnoff[XNNDV];
      ui16  tnsngx,tnsngy;
      ui32  tnsnel,tnsnelt;
      si32  tnloff,tnlsg;
      ui16  tnnrx,tnnry;
      si16  tnnhx,tnnhy;
      ui32  tnnsa,tnnsax;
      si32  tnnux,tnnuy;
      ui32  tnradius;
      float tntcwid;
      si32  tnstride;
      si32  tnxoff,tnyoff;
      si32  tncmin;
      } tcn;
   struct SFCGHDR  cghdr;
   struct {    /* Temps for checking retrieved cg info */
      ui16 tgngb,tgnib;
      } tcg;
   struct SFCMHDR  cmhdr;
   };

/* Ptr to file def for error msgs, NULL once it is printed first time */
static struct RFdef *psvft;

/*---------------------------------------------------------------------*
*                              readsfhd                                *
*                                                                      *
*  This routine reads a header from an input RESTORE file, matches its *
*  fields against a prototype header from the current version of CNS,  *
*  and fills in the information as much as can be from the available   *
*  data in the file.  Matching is done by linear search starting just  *
*  after the item most recently matched, as the order in the file and  *
*  in the prototype will almost always be the same.                    *
*                                                                      *
*  Arguments:                                                          *
*     psvf     Ptr to RFdef defining the file to be read.              *
*     hdr      Location where header is stored after checking.         *
*     urhd     Location of prototype header to be matched.             *
*  Error actions:                                                      *
*     Aborts on file read errors or if the SFHBlk in the file has a    *
*     different name than the SFHBlk in the prototype (since things    *
*     are probably hopelessly misaligned).  Gives a development error  *
*     if the type of a data item in the file is incompatible with the  *
*     type of the data item in the prototype or if a required item is  *
*     not present in the header.  Data items whose key names do not    *
*     match are quietly discarded.                                     *
*  Return value:                                                       *
*     Upon return, all header values have the expected type, length,   *
*     and scale, but they have not yet been converted from message to  *
*     memory format.  Use bemtoxx() routines to access them.           *
*  Restrictions:                                                       *
*     In reading RESTORE files written by earlier versions of CNS,     *
*     this code can now handle increases and decreases in array di-    *
*     mension and changes in length between short and long in either   *
*     direction.  The necessary information is tabulated to handle     *
*     fixed point variables of length other than 2 or 4 and scale      *
*     changes, but for the present, the code has been omitted since    *
*     these kinds of changes are unlikely to ever be needed.  This is  *
*     left as an exercise for anyone who may add such items in future. *
*---------------------------------------------------------------------*/

static void readsfhd(struct RFdef *psvf, void *hdr, void *urhd) {

   SFHBlk rdhblk;          /* SFHBlk read from file */
   SFHKey rdhkey;          /* SFHKey read from file */
   SFHBlk *phblk = (SFHBlk *)urhd;
   SFHKey *pkb,*pnkb,*pkbf,*pkbl,*pkb0;
   long lfdr;              /* Length of file data remaining */
   long lfitem;            /* Length of item in file */
   int  litem;             /* Length of header field */
   char cdat[FMDSIZE];     /* Long enough for longest string */

/* Read the SFHBlk and check the name against the prototype.
*  Of course, the length is now allowed to be different.  */

   rfread(psvf, &rdhblk, sizeof(SFHBlk), ABORT);
   if (memcmp(rdhblk.blkname, phblk->blkname, SFKEY_LEN)) {
      cryout(RK_P1, "0***RESTORE FILE HEADERS CORRUPTED--"
         "EXPECTED BLOCK NAME ", RK_LN2, phblk->blkname,
         SFKEY_LEN, " NOT FOUND.", RK_CCL, NULL);
      d3exit(NULL, FILECK_ERR, 0);
      }
   lfdr = bemtoi4(rdhblk.blklen) - sizeof(SFHBlk);

/* Pick up the length of the prototype and copy the prototype to the
*  work area.  Set locations to begin and end key searches.  */

   lfitem = bemtoi4(phblk->blklen);
   memcpy((char *)hdr, (char *)urhd, lfitem);
   pkb0 = pkbf = (SFHKey *)((char *)hdr + sizeof(SFHBlk));
   pkbl =        (SFHKey *)((char *)hdr + lfitem);

/* Read keys and data from the file until given length is exhausted.
*  Note:  SFHKey items are designed not to require alignment or
*  swapping.  If changed, this code must be updated accordingly.  */

   while (lfdr > 0) {
      int GotMatch = FALSE;
      rfread(psvf, &rdhkey, sizeof(SFHKey), ABORT);
      lfitem = (int)rdhkey.len * (int)rdhkey.dim;
      lfdr -= (sizeof(SFHKey) + lfitem);

/* Do circular linear search for the item key in the prototype */

      pkb = pkb0;
      do {
         /* Calculate location of next key block */
         litem = sizeof(SFHKey) + (int)pkb->len*(int)pkb->dim;
         pnkb = (SFHKey *)((char *)pkb + litem);
         if (pnkb >= pkbl) pnkb = pkbf;
         if (memcmp(rdhkey.key, pkb->key, SFKEY_LEN)) {
            /* No match, advance to next prototype key */
            pkb = pnkb;
            }
         else {
            /* Got a match.  Set to start next match after
            *  this item, then exit search loop to store data.  */
            pkb0 = pnkb;
            GotMatch = TRUE;
            break;
            }
         } while (pkb != pkb0);

      if (GotMatch) {         /* Key name was matched */
         char *pdata;         /* Ptr to data destination */
         int ktyp = pkb->type & SFTPM;
         int idim, idime;
         /* Generate an error for any differences in attributes
         *  that we are not willing to process.  If code is added
         *  to handle some case that arises in a future version of
         *  CNS, the test here can be revised to allow that case.  */
         if (((rdhkey.type & SFTPM) != ktyp) ||
               (rdhkey.fsclidim != pkb->fsclidim))
            d3exit(pkb->key, RESTYP_ERR, pkb->type);

/* Copy data from file to prototype, performing any necessary length
*  changes, but leaving data in file-message format (mostly so it is
*  not necessary to define memory-format structs for all the different
*  header types).  Since idime is almost always 1, it's not too
*  inefficient to put the switch inside the loop this way.  And
*  reading each item individually saves allocating a temp array.  */

         idime = min(rdhkey.dim, pkb->dim);
         pdata = (char *)pkb + sizeof(SFHKey);
         for (idim=0; idim<idime; idim++) {
            rfread(psvf, cdat, (long)rdhkey.len, ABORT);
            switch (ktyp) {
            case FTYPE: {
               long tl;
               if (rdhkey.len == FMSSIZE)
                  tl = (long)bemtoi2(cdat);
               else if (rdhkey.len == FMJSIZE)
                  tl = (long)bemtoi4(cdat);
               else if (rdhkey.len == FMLSIZE)
                  tl = (long)bemtoild(cdat);
               else
                  d3exit(pkb->key, RESTYP_ERR, pkb->type);
               /* Need braces: bemfmi[24] can be macros */
               if (pkb->len == FMSSIZE) {
                  bemfmi2(pdata, (short)tl); }
               else if (pkb->len == FMJSIZE) {
                  bemfmi4(pdata, (si32)tl); }
               else if (pkb->len == FMLSIZE) {
                  bemfmil(pdata, tl); }
               else
                  d3exit(pkb->key, RESTYP_ERR, pkb->type);
               break;
               }
            case UTYPE: {
               ulng tul;
               if (rdhkey.len == FMSSIZE)
                  tul = (ulng)bemtoi2(cdat);
               else if (rdhkey.len == FMJSIZE)
                  tul = (ulng)bemtoi4(cdat);
               else if (rdhkey.len == FMLSIZE)
                  tul = (ulng)bemtould(cdat);
               else
                  d3exit(pkb->key, RESTYP_ERR, pkb->type);
               /* Need braces: bemfmu[24] can be macros */
               if (pkb->len == FMSSIZE) {
                  bemfmi2(pdata, (ui16)tul); }
               else if (pkb->len == FMJSIZE) {
                  bemfmi4(pdata, (ui32)tul); }
               else if (pkb->len == FMLSIZE) {
                  bemfmul(pdata, tul); }
               else
                  d3exit(pkb->key, RESTYP_ERR, pkb->type);
               break;
               }
            default:       /* ETYPE, DTYPE, or ATYPE */
               if (rdhkey.len == pkb->len)
                  memcpy(pdata, cdat, rdhkey.len);
               else
                  d3exit(pkb->key, RESTYP_ERR, pkb->type);
               break;
               } /* End type switch */
            pdata += pkb->len;
            } /* End dim loop */
         /* Indicate that this item was found */
         pkb->type |= SFGOT;
         /* Store dimension of data actually read from file--may
         *  differ from current dim in earlier versions of CNS.  */
         pkb->fsclidim = idime;
         /* Set to skip over data not read due to smaller dim */
         lfitem = (int)rdhkey.len * (idime - (int)rdhkey.dim);
         } /* End GotMatch */
      if (lfitem) rfseek(psvf, lfitem, SEEKREL, ABORT);
      } /* End reading file data */

/* Check that all required items have been found in the data */

   for (pkb=pkbf; pkb<pkbl; pkb=pnkb) {
      if ((pkb->type & (SFGOT|SFREQ)) == SFREQ)
         d3exit(pkb->key, RESITM_ERR, pkb->type);
      litem = sizeof(SFHKey) + (int)pkb->len*(int)pkb->dim;
      pnkb = (SFHKey *)((char *)pkb + litem);
      } /* End check for missing data */

   } /* End readsfhd() */

/*---------------------------------------------------------------------*
*                             listhmerrs                               *
*                                                                      *
*  This routine prints error information for an item that cannot be    *
*  restored from a corresponding file item due to mismatching header   *
*  data and sets error flag in RK.iexit.  It prints the following:     *
*     (1) The name of the input file (once only).                      *
*     (2) The rlname and connection type (if relevant) of the item.    *
*     (3) The rlname and connection type of the matching file item     *
*           if different from the name in the current run.             *
*     (4) A list of the header fields that were not the same in the    *
*  restore file and the cell or connection type that it should have    *
*  matched.  This output can exceeds one line.  Each entire line is    *
*  built up before cryout() is called, because it cannot be used to    *
*  concatenate line fragments to an error message, which is flushed.   *
*                                                                      *
*  Arguments:                                                          *
*     phm      Ptr to FHMATCH block for which error is being reported. *
*     eclass   Text identifying class of item that failed to match.    *
*     estr     Array of pointers to text strings identifying fields.   *
*     kbad     A word with bits set corresponding to the items that    *
*              failed to match.  The meanings of these bits must       *
*              correspond to the error strings in estr.                *
*---------------------------------------------------------------------*/

static void listhmerrs(struct SFHMATCH *phm, char *eclass,
      char *estr[], unsigned long kbad) {

   struct CELLTYPE *il = phm->ppl;
   struct REPBLOCK *ir = il->pback;
   static char HMErrLeader[] =
      " ***FIELDS:";                /* Start of each line */

   int i=0;                         /* String selector */
   int ictm = phm->rct & SHRT_MAX;  /* Number of conntype matched to */
   int kctm = phm->ict != 0;        /* Connection type match switch */
   int lrem;                        /* Length of etxt remaining */
   char etxt[MaxHMErrLine+1];       /* Work space */

/* (1) Identify bad file, once only */

   if (psvft) {
      cryout(RK_P1, "0***SAVENET FILE ", RK_LN2,
         psvft->fname, RK_CCL, NULL);
      psvft = NULL; }

/* (2) Name and connection type of the item that has a mismatch */

   convrt("(P1,4H ***J0A20,' HDR NE GROUP II INPUT FOR ',J0A" qLXN
      ",RA11,RJ0IH6)", eclass, fmturlnm(il),
      &kctm, ", CONNTYPE ", &kctm, &phm->ict, NULL);

/* (3) Name of the item in the file, if different */

   if (strncmp(ir->sname, phm->rrennm.rnm, LSNAME) ||
       strncmp(il->lname, phm->rrennm.lnm, LSNAME) ||
       phm->ict != ictm) {
      convrt("(P1,' ***MATCHED TO: ',J0A" qLXN ",RA11,RJ0I6)",
         fmtrlnm(&phm->rrennm), &kctm, ", CONNTYPE ",
         &kctm, &ictm, NULL);
      }

/* (4) Names of the fields that fail to match */

   strcpy(etxt, HMErrLeader);
   lrem = MaxHMErrLine - sizeof(HMErrLeader);
   while (kbad) {
      if (kbad & 1) {
         int li = strlen(estr[i]);
         if (li > lrem) {
            cryout(RK_P1, etxt, RK_LN1, NULL);
            etxt[sizeof(HMErrLeader)] = '\0';
            lrem = MaxHMErrLine - sizeof(HMErrLeader);
            }
         strcat(etxt, estr[i]);
         lrem -= li;
         }
      i++, kbad >>= 1;
      } /* End bit testing */
   cryout(CROFE, etxt, RK_LN1, NULL);

   } /* End listhmerrs() */

/*---------------------------------------------------------------------*
*                               d3fchk                                 *
*---------------------------------------------------------------------*/

void d3fchk(void) {

   struct RFdef     *psvf;    /* Ptr to RFdef for current file */
   struct REPBLOCK    *ir;    /* Ptr to current repertoire */
   struct CELLTYPE    *il;    /* Ptr to current layer */
   struct CONNTYPE    *ix;    /* Ptr to current specif. conntype */
   struct INHIBBLK    *ib;    /* Ptr to current geomet. conttype */
   struct MODBY       *im;    /* Ptr to current modul.  conntype */
   struct SFALLHDRS    *h;    /* Work space for headers */
   struct SFHMATCH   *phm;    /* Ptr to header match of interest */
   struct SFHMATCH  *phma;    /* Ptr to header match array */
   short            *pnhm;    /* Ptr to index of next header match */
   char             *pord;    /* Ptr to an order table */

   unsigned long kbaditem;    /* Error flag for bad fields */
   int ifile;                 /* Number of file being checked */
   int icl;                   /* SFHMATCH class for error checks */
   int irh,irhe;              /* Repertoire header counter, limit */
   int ilh,ilhe;              /* Layer header counter, limit */
   int ixh,ixhe;              /* Conntype header counter, limit */
   int iul,iux;               /* Index of unmatched layer,conntype */
   short rctmask;             /* Mask for checking conntype number */
   byte DidRCTMsg = FALSE;    /* TRUE when RCT explanatory msg out */
   byte GotSNSeed = FALSE;    /* TRUE when RP->snseed has been read */
   byte GotOldSi;             /* = RSTOLDSI if reading pre-V8C file */
   byte Restoring;            /* Indicates RESTORE in progress */
   char Emcc;                 /* Error message carriage control */

   static char *ctetxt[] =    /* CELLTYPE errors */
      { " NGX", " NGY", " NEL", " VARS", " MEM-MAP", "nABCD" };
   static char *cnetxt[] =    /* CONNTYPE errors */
      { " KGEN", " NC", " NGX(Src)", " NGY(Src)", " NEL(Src)",
        " CELLS(Src)", " NSA", " NSAX", " DATA-LEN", " NBC", " BC",
        " MEM-MAP", " OFFSET", " WIDTH", " NRX", " NRY", " NUX",
        " NUY", " TCWIDTH", " STRIDE", " XOFF", " YOFF", " NHX",
        " NHY", " RADIUS", " CMIN" };
   static char *cgetxt[] =    /* GCONNTYPE errors */
      { " NGB", " NIB" };
   static char *herrtxt[] =   /* Errors by header class */
      { "CELL DATA", "CONNECTION TYPE", "GCONN TYPE",
        "MODULATION TYPE" };
   static char *hwrntxt[] =   /* Warnings by header class */
      { "cell data", "connection type", "gconn type",
        "modulation type" };

/*---------------------------------------------------------------------*
*     (1) Open and check SAVENET files for network restoration         *
*---------------------------------------------------------------------*/

   /* Setup to handle overflows (shortening 8-byte longs to 4 bytes) */
   e64dec(EAfl(OVF_FCHK));

/* Allocate work space for headers and possible error messages */

   phma = RP0->psfhm;
   h = (struct SFALLHDRS *)mallocv(sizeof(struct SFALLHDRS),
      "RESTORE file headers");

/* It is necessary to initialize the blklen fields of the prototype
*  SAVENET headers by executable code, inasmuch as the C language
*  gives us no way to initialize unaligned/byte swapped data.  The
*  values set here remain in place for use by d3save.  */

   bemfmi4(ursfhdr2.sfhdblk.blklen, (si32)sizeof(struct SFHDR2));
   bemfmi4(ursfrephdr.sfrepblk.blklen, (si32)sizeof(struct SFREPHDR));
   bemfmi4(ursfcthdr.sfctblk.blklen, (si32)sizeof(struct SFCTHDR));
   bemfmi4(ursfcnhdr.sfcnblk.blklen, (si32)sizeof(struct SFCNHDR));
   bemfmi4(ursfcghdr.sfcgblk.blklen, (si32)sizeof(struct SFCGHDR));
   bemfmi4(ursfcmhdr.sfcmblk.blklen, (si32)sizeof(struct SFCMHDR));

#if 0       /****DEBUG****/

/* List isfhm1 lists as they stand on entry to d3fchk */

   for (icl=SFHMCELL; icl<SFHMNTYP; icl++) {
      cryout(RK_P1, "0Listing of ", RK_LN2, hwrntxt[icl], RK_CCL,
         " SFHMATCH blocks at start of d3fchk:", RK_CCL, NULL);
      for (iul=RP0->isfhm1[icl]; iul>=0; iul=phm->nxthm) {
         phm = &phma[iul];
         il = phm->ppl;
         cryout(RK_P1, "    RRname = ", RK_LN1, phm->rrennm.rnm, 4,
            ", RLname = ", RK_CCL, phm->rrennm.lnm, 4, ", Parent ",
            RK_CCL, fmtlrlnm(il), LXRLNM, NULL);
         convrt("(#4,'    nxthm =',IH6,', fno =',IH6,', rct =',IH8,"
            "' ict =',IH8)", &phm->nxthm, NULL);
         } /* End layer not matched */
      } /* End loop over header types */

#endif      /****ENDDEBUG****/

/* Clear warning flags for all celltypes before starting */

   for (il=RP->pfct; il; il=il->pnct)
      memset(il->ctwk.phs1.hmwarn, 0, SFHMNTYP);

/* Loop over SAVENET files to be restored */

   for (psvf=d3file[SAVREP_IN],ifile=1; psvf; psvf=psvf->nf,ifile++) {
      int tvers;
      /* Open current file */
      rfopen(psvf, NULL, SAME, SAME, SAME, SAME, SAME, SAME,
         SAME, SAME, SAME, SAME, ABORT);

      /* Read just the file version number */
      rfread(psvf, &h->sfhdr, SFVERS_LEN, ABORT);
      /* Error if old-style savenet or other file found */
      wbcdin(h->sfhdr.filetype, &tvers, RK_IORF|RK_NI32|SFVERS_LEN-1);
      if (memcmp(h->sfhdr.filetype, SF_IDENT, SFVERS_LEN-4) ||
            tvers < SF_VERS) {
         cryout(CROFE,"0***FILE ", RK_LN2, psvf->fname, RK_CCL,
            " DOES NOT APPEAR TO BE A CURRENT CNS SAVENET FILE.",
            RK_CCL, NULL);
         goto NextFile;
         }
      /* Read the rest of the header.  Take care of change
      *  in series title length that occurred at V8C.  */
      if (h->sfhdr.filetype[10] < 'C') {
         GotOldSi = RSTOLDSI;
         memset(h->sfhdr.stitle, ' ', LSTITL);
         rfread(psvf, h->sfhdr.title, SFTITLE_LEN+SFDATE_LEN+44,
            ABORT); }
      else {
         GotOldSi = 0;
         rfread(psvf, h->sfhdr.title, SFTITLE_LEN+SFDATE_LEN+
            LSTITL, ABORT); }

      psvft = psvf;     /* Trigger file name print on later errors */

      /* Print identifying header information */
      cryout(RK_P2, "0SAVENET file ", RK_LN4, psvf->fname, RK_CCL,
         " created ", RK_CCL, h->sfhdr.timestamp, SFDATE_LEN,
         "    Title: ", RK_LN0, h->sfhdr.title, SFTITLE_LEN,

         "    Written after: ", RK_LN0, h->sfhdr.stitle+1, LSTITL-1,
         NULL);

/* Read and pick up information from SFHDR2.  Use snseed
*  only from first file in which a value is found.  */

      readsfhd(psvf, &h->sfhdr2, &ursfhdr2);
      if (!GotSNSeed && SFHGot(sfhdr2, sfhsnseed)) {
         RP->snseed = bemtoi4(h->sfhdr2.sfhsnseed.d);
         GotSNSeed = TRUE; }

/*---------------------------------------------------------------------*
*     (2) Read repertoire and layer headers and try to match them      *
*        (Two levels of indenting suppressed down to NextFile)         *
*  For each header type, items that will be accessed multiple times in *
*  searches are pulled out into swapped temps for quicker comparisons. *
*  These items are assumed to have SFREQ bit set, so their existence   *
*  has already been tested in readsfhd() before bemtoxx is executed.   *
*---------------------------------------------------------------------*/

irhe = (int)bemtoi2(h->sfhdr2.sfhnrep.d);
for (irh=1; irh<=irhe; irh++) {

   readsfhd(psvf, &h->rephdr, &ursfrephdr);
   h->trp.trngx = bemtoi2(h->rephdr.sfrngx.d);
   h->trp.trngy = bemtoi2(h->rephdr.sfrngy.d);

   ilhe = (int)bemtoi2(h->rephdr.sfnlyr.d);
   for (ilh=1; ilh<=ilhe; ilh++) {

      readsfhd(psvf, &h->cthdr, &ursfcthdr);
      h->tct.tlnel = bemtoi4(h->cthdr.sfctnel.d);
      h->tct.tlls =  bemtoi2(h->cthdr.sfctls.d);
      h->tct.tnizv = bemtoi2(h->cthdr.sfctnizv.d);

/* In pulling out the dynamic allocation offsets, we flag a file
*  variable as not allocated if it did not exist at the time the
*  file was written.  This greatly simplifies the test inside the
*  loop over celltypes below.  */

      {  register int icto;
         int mxcto = (int)h->cthdr.sfctoff.k.fsclidim;
         if (mxcto > CTNDV) mxcto = CTNDV;
         else for (icto=mxcto; icto<CTNDV; icto++)
            h->tct.tctoff[icto] = (DVREQ - 1);
         for (icto=0; icto<mxcto; icto++)
            h->tct.tctoff[icto] = bemtoi2(h->cthdr.sfctoff.d[icto]);
         } /* End scope of icto, mxcto */

/* Check celltype header against unmatched repertoires and layers.
*  Ignore if current file is not the one requested by the user.
*  Multiple matches are allowed.  Note that all celltypes in each
*  repertoire must be checked even if the repertoire name disagrees,
*  because of RNAME.  */

      pnhm = &RP0->isfhm1[SFHMCELL];
      for (iul=*pnhm; iul>=0; iul=*pnhm) {
         phm = &phma[iul];
         if (strncmp(h->rephdr.repnam.d, phm->rrennm.rnm, LSNAME))
            goto NoCellTypeMatch;
         if (strncmp(h->cthdr.celnam.d,  phm->rrennm.lnm, LSNAME))
            goto NoCellTypeMatch;
         if (phm->fno != 0 && phm->fno != ifile)
            goto NoCellTypeMatch;

/* Here when repertoire name, layer name, and file number match.
*  Match data fields that must agree.  Memory offsets (ctoff) only
*  must agree for fields that are being restored--see note above.  */

         il = (struct CELLTYPE *)phm->path;
         ir = il->pback;      /* Locate parent repertoire block */
         kbaditem = 0;
         if (h->trp.trngx != ir->ngx)                 kbaditem |= 0x01;
         if (h->trp.trngy != ir->ngy)                 kbaditem |= 0x02;
         if (h->tct.tlnel != il->nel)                 kbaditem |= 0x04;
         if (h->tct.tlls < il->lsrstr)                kbaditem |= 0x08;
         for (pord=ordct; *pord<SAVE_LS_COUNT; pord++)
            if (h->tct.tctoff[*pord] != il->ctoff[*pord] &&
               h->tct.tctoff[*pord] < il->lsrstr)     kbaditem |= 0x10;
         /* Check that items with variable length have same length
         *  (add ion checks here when implemented).  */
         if (ctexists(il,IZRA)) {
            ui16 tnizvar = (ui16)il->nizvar;
            if (il->ctf & CTFI3IVR) tnizvar |= I3IVRBIT;
            if (!SFHGot(cthdr,sfctnizv) ||
               h->tct.tnizv != tnizvar)               kbaditem |= 0x20;
            }
         if (kbaditem) {
            /* Not a match.  If the user requested that we restore from
            *  this file, then we have an error.  Otherwise, just go on
            *  to the next layer and keep looking.  */
            if (phm->fno > 0)
               listhmerrs(phm, herrtxt[SFHMCELL], ctetxt, kbaditem);
            goto NoCellTypeMatch;
            } /* End baditem msg */

/* We got a match--save needed fields in celltype */

         il->ctwk.phs1.fdes = psvf;
         il->ctf |= (ui32)GotOldSi;
         il->ctwk.phs1.offsi =    bemtou8(h->cthdr.offsi.d);
         il->ctwk.phs1.offprd =   bemtou8(h->cthdr.offprd.d);
         il->ctwk.phs1.rflel =    bemtou8(h->cthdr.sfctlel.d);
         il->ctwk.phs1.rfphshft = bemtoi2(h->cthdr.sfctphshft.d);
         il->ctwk.phs1.rfdelay =  bemtoi2(h->cthdr.sfctsdly1.d);
         /* Copy updated autoscale multiplier(s) if found and
         *  exists and "override restore" was not specified.  */
         if (SFHGot(cthdr, sfctasmul) && !(il->Ct.ctopt & OPTOR) &&
               il->pauts) {
            il->pauts->asmul[0] = bemtoi4(h->cthdr.sfctasmul.d[0]);
            il->pauts->asmul[1] = bemtoi4(h->cthdr.sfctasmul.d[1]);
            }
         /* Use updated seeds from file header */
         if (SFHGot(cthdr, sfctnsd))
            il->No.nsd = bemtoi4(h->cthdr.sfctnsd.d);
         if (SFHGot(cthdr, sfctrsd))
            il->rsd = bemtoi4(h->cthdr.sfctrsd.d);
         if (SFHGot(cthdr, sfctpsd) && il->pctpd)
            il->pctpd->phiseed = bemtoi4(h->cthdr.sfctpsd.d);
         if (SFHGot(cthdr, sfctzsd) && il->prfi)
            ((struct IZHICOM *)il->prfi)->izseed =
               bemtoi4(h->cthdr.sfctzsd.d);

         /* Remove this match from the scan list */
         *pnhm = phm->nxthm;
         continue;
NoCellTypeMatch:
         pnhm = &phm->nxthm;
         } /* End of loop over still-unmatched celltypes */

/*---------------------------------------------------------------------*
*        Read specific conntype headers and try to match them          *
*---------------------------------------------------------------------*/

      ixhe = (int)bemtoi2(h->cthdr.sfctnct.d);
      for (ixh=1; ixh<=ixhe; ixh++) {

         readsfhd(psvf, &h->cnhdr, &ursfcnhdr);
         h->tcn.tnnc =     bemtoi4(h->cnhdr.sfcnnc.d);
         h->tcn.tnlc =     bemtoi2(h->cnhdr.sfcnlc.d);
         /* This lxn is incorrect for old files, corrected later */
         h->tcn.tnlxn =    bemtoi2(h->cnhdr.sfcnlxn.d);
         h->tcn.tnnbc =    bemtoi2(h->cnhdr.sfcnnbc.d);
         h->tcn.tncbc =    bemtoi2(h->cnhdr.sfcncbc.d);
         h->tcn.tnkgen =   bemtoi4(h->cnhdr.sfcnkgen.d);
         h->tcn.tnsngx =   bemtoi2(h->cnhdr.sfcnsrcngx.d);
         h->tcn.tnsngy =   bemtoi2(h->cnhdr.sfcnsrcngy.d);
         h->tcn.tnsnel =   bemtoi4(h->cnhdr.sfcnsrcnel.d);
         h->tcn.tnsnelt =  bemtoi4(h->cnhdr.sfcnsrcnelt.d);
         h->tcn.tnloff =   bemtoi4(h->cnhdr.sfcnloff.d);
         h->tcn.tnlsg =    bemtoi4(h->cnhdr.sfcnlsg.d);
         h->tcn.tnnrx =    bemtoi2(h->cnhdr.sfcnnrx.d);
         h->tcn.tnnry =    bemtoi2(h->cnhdr.sfcnnry.d);
         h->tcn.tnnhx =    bemtoi2(h->cnhdr.sfcnnhx.d);
         h->tcn.tnnhy =    bemtoi2(h->cnhdr.sfcnnhy.d);
         h->tcn.tnnsa =    bemtoi4(h->cnhdr.sfcnnsa.d);
         h->tcn.tnnsax =   bemtoi4(h->cnhdr.sfcnnsax.d);
         h->tcn.tnnux =    bemtoi4(h->cnhdr.sfcnnux.d);
         h->tcn.tnnuy =    bemtoi4(h->cnhdr.sfcnnuy.d);
         h->tcn.tnradius = bemtoi4(h->cnhdr.sfcnradius.d);
         h->tcn.tntcwid =  bemtor4(h->cnhdr.sfcntcwid.d);
         h->tcn.tnstride = bemtoi4(h->cnhdr.sfcnstride.d);
         h->tcn.tnxoff =   bemtoi4(h->cnhdr.sfcnxoff.d);
         h->tcn.tnyoff =   bemtoi4(h->cnhdr.sfcnyoff.d);
         h->tcn.tncmin =   bemtoi4(h->cnhdr.sfcncmin.d);

/* Pull out dynamic allocation offsets.  Treat items that did not
*  exist at the time the file was made as if they existed but were
*  not allocated.  If the SF_IDENT is prior to V8H, xnoff will not
*  exist, but we can make a fake one with NUK allocated at 0 and
*  all other variables not allocated.  */

         {  register int icno,mxcno;
            mxcno = (int)h->cnhdr.sfcnoff.k.fsclidim;
            if (mxcno > CNNDV) mxcno = CNNDV;
            else for (icno=mxcno; icno<CNNDV; icno++)
               h->tcn.tcnoff[icno] = (DVREQ - 1);
            for (icno=0; icno<mxcno; icno++)
               h->tcn.tcnoff[icno] = bemtoi2(h->cnhdr.sfcnoff.d[icno]);

            if (SFHGot(cnhdr,sfxnoff)) {
               /* V8H or later has xnoff field */
               mxcno = (int)h->cnhdr.sfxnoff.k.fsclidim;
               if (mxcno > XNNDV) mxcno = XNNDV;
               else for (icno=mxcno; icno<XNNDV; icno++)
                  h->tcn.txnoff[icno] = (DVREQ - 1);
               for (icno=0; icno<mxcno; icno++)
                  h->tcn.txnoff[icno] =
                     bemtoi2(h->cnhdr.sfxnoff.d[icno]);
               }
            else {
               /* Earlier version file, make artificial xnoff */
               for (icno=0; icno<XNNDV; icno++)
                  h->tcn.txnoff[icno] = (DVREQ - 1);
               h->tcn.txnoff[NUK] = 0;
               }
            } /* End scope of icno, mxcno */

/* Check conntype header against unmatched conntypes, totally inde-
*  pendent of whether celltype was a match, but ignoring current file
*  if it is not the one specifically requested.  Only the target rlnm,
*  not the source, is compared--this policy may change as experience
*  dictates, and the info is there in the headers if needed.  Multiple
*  matches to same file data (if rct > 0) and out-of-order matches are
*  now permitted.  */

         rctmask = SHRT_MAX;  /* Match explicit or implicit rct */
         pnhm = &RP0->isfhm1[SFHMCONN];
         for (iul=*pnhm; iul>=0; iul=*pnhm) {
            phm = &phma[iul];
            if (strncmp(h->rephdr.repnam.d, phm->rrennm.rnm, LSNAME))
               goto NoConnTypeMatch;
            if (strncmp(h->cthdr.celnam.d,  phm->rrennm.lnm, LSNAME))
               goto NoConnTypeMatch;
            if (phm->fno != 0 && phm->fno != ifile)
               goto NoConnTypeMatch;
            if ((phm->rct & rctmask) != ixh)
               goto NoConnTypeMatch;

/* Here when target cell type, file number, and ict/rct match.
*  Match data fields that must agree.  */

            ix = (struct CONNTYPE *)phm->path;
            il = phm->ppl;
            kbaditem = 0;
            if (h->tcn.tnkgen != ix->kgen)      kbaditem |= 0x00000001;
            if (h->tcn.tnnc   != ix->nc)        kbaditem |= 0x00000002;
            if (h->tcn.tnsngx != ix->srcngx)    kbaditem |= 0x00000004;
            if (h->tcn.tnsngy != ix->srcngy)    kbaditem |= 0x00000008;
            if (h->tcn.tnsnel != ix->srcnel)    kbaditem |= 0x00000010;
            if (h->tcn.tnsnelt!= ix->srcnelt)   kbaditem |= 0x00000020;
            if (h->tcn.tnnsa  != ix->nsa)       kbaditem |= 0x00000040;
            if ((ix->Cn.saopt & (DOSARB|SAOPTL|SAOPTI)) == DOSARB &&
                h->tcn.tnnsax != ix->nsax)      kbaditem |= 0x00000080;
            if (h->tcn.tnlc   != ix->lc)        kbaditem |= 0x00000100;
            if (h->tcn.tnnbc  != ix->Cn.nbc)    kbaditem |= 0x00000200;
            if (h->tcn.tncbc  != ix->cbc)       kbaditem |= 0x00000400;
            /* In old files, lxn does not exist, but is effectively
            *  equal to nuklen, which is also not saved, but must be
            *  same as current nuklen inasmuch as nc must be same.
            *  xnoff also does not exist, but fake is built above.  */
            if (SFHGot(cnhdr,sfcnlxn) &&
               h->tcn.tnlxn != ix->lxn)         kbaditem |= 0x00000100;
            for (pord=ordcn; *pord<END_OF_ALLOC_TABLE; pord++)
               if (h->tcn.tcnoff[*pord] != ix->cnoff[*pord])
                                                kbaditem |= 0x00000800;
            for (pord=ordxn; *pord<END_OF_ALLOC_TABLE; pord++)
               if (h->tcn.txnoff[*pord] != ix->xnoff[*pord])
                                                kbaditem |= 0x00000800;
            /* The remaining tests are sensitive to the particular
            *  generating code--only test variables that are in use */
            if (ix->kgen & (KGNJN|KGNXO|KGNYO|KGNHV) &&
                h->tcn.tnloff != ix->loff)      kbaditem |= 0x00001000;
            if (ix->kgen & (KGNJN|KGNKN|KGNND|KGNOG|KGNYO) &&
                h->tcn.tnlsg  != ix->lsg)       kbaditem |= 0x00002000;
            if (ix->kgen & (KGNBL|KGNCF|KGNAN|KGNMC|KGNKN)) {
               if (h->tcn.tnnrx  != ix->nrx)    kbaditem |= 0x00004000;
               if (h->tcn.tnnry  != ix->nry)    kbaditem |= 0x00008000;
               }
            /* Only test nux,nuy with KGNJN,KGNXO if > 1 cell because
            *  default changed in V8D Rev 8, can still use old files
            *  this way.  */
            if (ix->kgen & (KGNFU|KGNTP|KGNBV) || (il->nelt > 1 &&
                  ix->kgen & (KGNJN|KGNXO|KGNYO))) {
               if (h->tcn.tnnux  != ix->nux)    kbaditem |= 0x00010000;
               if (h->tcn.tnnuy  != ix->nuy)    kbaditem |= 0x00020000;
               }
            if ((ix->kgen & (KGNKN|KGNTW) || ix->Cn.cnopt & NOPVP) &&
               h->tcn.tntcwid != ix->Cn.tcwid)  kbaditem |= 0x00040000;
            if (ix->kgen & (KGNAJ|KGNBL) &&
               h->tcn.tnstride != ix->stride)   kbaditem |= 0x00080000;
            if (ix->kgen & (KGNFU|KGNTP|KGNJN|KGNXO|KGNYO|KGNBV)) {
               if (h->tcn.tnxoff != ix->xoff)   kbaditem |= 0x00100000;
               if (h->tcn.tnyoff != ix->yoff)   kbaditem |= 0x00200000;
               }
            if (ix->kgen & KGNAN) {
               if (h->tcn.tnnhx  != ix->nhx)    kbaditem |= 0x00400000;
               if (h->tcn.tnnhy  != ix->nhy)    kbaditem |= 0x00800000;
               }
            if (ix->kgen & (KGNE2|KGNHV) &&
               h->tcn.tnradius != ix->hradius)  kbaditem |= 0x01000000;
            if (ix->kgen & KGNE2 &&
               h->tcn.tncmin != ix->cijmin)     kbaditem |= 0x02000000;

            if (kbaditem) {
               /* Not a match.  If the user requested that we restore
               *  from this file, then we have an error.  Otherwise,
               *  just go on to the next conntype and keep looking.  */
               if (phm->fno > 0)
                  listhmerrs(phm, herrtxt[SFHMCONN], cnetxt, kbaditem);
               goto NoConnTypeMatch;
               } /* End baditem msg */

/* We got a match--save needed fields in conntype block */

            /* If we matched an implicit rct (sign bit set),
            *  prevent future matches of other implicit rct's.  */
            rctmask |= phm->rct;
            ix->cnwk.phs1.fdes = psvf;
            il->ctwk.phs1.hmwarn[SFHMCONN] = HMWM1;   /* Kill warning */
            ix->cnwk.phs1.offprd = bemtou8(h->cnhdr.offprd.d);
            ix->cnwk.phs1.rflel =  bemtou8(h->cthdr.sfctlel.d);

            /* Use updated seeds from file header */
            if (SFHGot(cnhdr, sfcncseed))
               ix->cseed = bemtoi4(h->cnhdr.sfcncseed.d);
            if (SFHGot(cnhdr, sfcnlseed))
               ix->lseed = bemtoi4(h->cnhdr.sfcnlseed.d);
            if (SFHGot(cnhdr, sfcnpseed))
               ix->phseed = bemtoi4(h->cnhdr.sfcnpseed.d);
            if (SFHGot(cnhdr, sfcnrseed))
               ix->rseed = bemtoi4(h->cnhdr.sfcnrseed.d);

            /* Remove this match from the scan list */
            *pnhm = phm->nxthm;
            continue;
NoConnTypeMatch:
            pnhm = &phm->nxthm;
            } /* End of loop over still-unmatched conntypes */
         } /* End loop over conntype headers in file */

/*---------------------------------------------------------------------*
*        Read geometric conntype headers and try to match them         *
*---------------------------------------------------------------------*/

      ixhe = (int)bemtoi2(h->cthdr.sfctngct.d);
      for (ixh=1; ixh<=ixhe; ixh++) {

         readsfhd(psvf, &h->cghdr, &ursfcghdr);
         h->tcg.tgngb = bemtoi2(h->cghdr.sfcgngb.d);
         h->tcg.tgnib = bemtoi2(h->cghdr.sfcgnib.d);

/* Check gconn header against unmatched gconntypes.  Matching policies
*  should be kept consistent with those applicable to specific connec-
*  tion types, as noted above.  */

         rctmask = SHRT_MAX;  /* Match explicit or implicit rct */
         pnhm = &RP0->isfhm1[SFHMGCON];
         for (iul=*pnhm; iul>=0; iul=*pnhm) {
            phm = &phma[iul];
            if (strncmp(h->rephdr.repnam.d, phm->rrennm.rnm, LSNAME))
               goto NoGConnTypeMatch;
            if (strncmp(h->cthdr.celnam.d,  phm->rrennm.lnm, LSNAME))
               goto NoGConnTypeMatch;
            if (phm->fno != 0 && phm->fno != ifile)
               goto NoGConnTypeMatch;
            if ((phm->rct & rctmask) != ixh)
               goto NoGConnTypeMatch;

/* Here when target cell type, file number, and ict/rct match.
*  Match data fields that must agree.  */

            ib = (struct INHIBBLK *)phm->path;
            il = phm->ppl;
            kbaditem = 0;
            if (h->tcg.tgngb != ib->ngb)        kbaditem |= 0x00000001;
            if (h->tcg.tgnib != ib->nib)        kbaditem |= 0x00000002;

            if (kbaditem) {
               /* Not a match.  If the user requested that we restore
               *  from this file, then we have an error.  Otherwise,
               *  just go on to the next gconntype and keep looking.  */
               if (phm->fno > 0)
                  listhmerrs(phm, herrtxt[SFHMGCON], cgetxt, kbaditem);
               goto NoGConnTypeMatch;
               } /* End baditem msg */

/* We got a match--save needed fields in INHIBBLK--(currently none) */

            /* If we matched an implicit rct (sign bit set),
            *  prevent future matches of other implicit rct's.  */
            rctmask |= phm->rct;
            il->ctwk.phs1.hmwarn[SFHMGCON] = HMWM1;   /* Kill warning */

            /* Remove this match from the scan list */
            *pnhm = phm->nxthm;
            continue;
NoGConnTypeMatch:
            pnhm = &phm->nxthm;
            } /* End of loop over still-unmatched gconns */
         } /* End of loop over gconn headers in file */

/*---------------------------------------------------------------------*
*       Read modulatory conntype headers and try to match them         *
*---------------------------------------------------------------------*/

      ixhe = (int)bemtoi2(h->cthdr.sfctnmct.d);
      for (ixh=1; ixh<=ixhe; ixh++) {

         readsfhd(psvf, &h->cmhdr, &ursfcmhdr);

/* Check modulatory connection header against unmatched mconntypes.
*  Matching policies should be kept consistent with those applicable
*  to specific connection types, as noted above.  */

         rctmask = SHRT_MAX;  /* Match explicit or implicit rct */
         pnhm = &RP0->isfhm1[SFHMMODU];
         for (iul=*pnhm; iul>=0; iul=*pnhm) {
            phm = &phma[iul];
            if (strncmp(h->rephdr.repnam.d, phm->rrennm.rnm, LSNAME))
               goto NoMConnTypeMatch;
            if (strncmp(h->cthdr.celnam.d,  phm->rrennm.lnm, LSNAME))
               goto NoMConnTypeMatch;
            if (phm->fno != 0 && phm->fno != ifile)
               goto NoMConnTypeMatch;
            if ((phm->rct & rctmask) != ixh)
               goto NoMConnTypeMatch;

/* Here when target cell type, file number, and ict/rct match.
*  Match data fields that must agree--currently there are none.  */

            im = (struct MODBY *)phm->path;
            il = phm->ppl;

/* We got a match--save needed fields in MODBY block--
*  currently there are none.  */

            /* If we matched an implicit rct (sign bit set),
            *  prevent future matches of other implicit rct's.  */
            rctmask |= phm->rct;
            il->ctwk.phs1.hmwarn[SFHMMODU] = HMWM1;   /* Kill warning */

            /* Remove this match from the scan list */
            *pnhm = phm->nxthm;
            continue;
NoMConnTypeMatch:
            pnhm = &phm->nxthm;
            } /* End of loop over still-unmatched modulatory conntypes */
         } /* End of loop over modulatory headers in file */

      }} /* End loops over celltype and repertoire headers in file */

/*---------------------------------------------------------------------*
*  Leave all files open, whether used or not.                          *
*  They will get closed and storage freed at end of d3rstr.            *
*---------------------------------------------------------------------*/

NextFile:
      ;
      } /* End of loop over files */

/*---------------------------------------------------------------------*
*        Check for unmatched repertoires and connection types          *
*  The following code must run even if there are no restore files. It  *
*  sets bypass-restore bits for all repertoires that will not be read  *
*  from files, thereby activating d3genr to generate them.             *
*  A warning is given in each case only if there are restore files.    *
*  A fatal error is given if a file number was specified by user.      *
*---------------------------------------------------------------------*/

   Restoring = (RP->ksv & KSVRR) != 0;

/* Check isfhm1 lists for unmatched layers and conntypes */

   for (icl=SFHMCELL; icl<SFHMNTYP; icl++) {
      if ((iul = RP0->isfhm1[icl]) >= 0) {
         iux = icl > 0;    /* Used here as convrt format switch */
         Emcc = '0';
         for ( ; iul>=0; iul=phm->nxthm) {
            phm = &phma[iul];
            il = phm->ppl;
            if (Restoring) {
               /* If celltype or conntype and got at least one match,
               *  issue specific message.  */
               if (!iux || il->ctwk.phs1.hmwarn[icl] & HMWM1) {
                  if (phm->fno) {   /* Specific file requested */
                     convrt("(P1A1,3H***J1A" qLXN ",'NOT MATCHED W/RES"
                        "TORE FILE ',J1IH4,4HFOR J0A40,RX,RJ0IH4,H.)",
                        &Emcc, fmturlnm(il), &phm->fno, herrtxt[icl],
                        &iux, &iux, &phm->ict, NULL);
                     RK.iexit |= FILE_ERR;
                     if (iux && (phm->rct < 0) && !DidRCTMsg) {
                        cryout(RK_P2,"    NOTE: Use explicit RCT para"
                           "meters if you want to restore more than ",
                           RK_LN2, "    one connection type from the "
                           "same restore file data.", RK_LN0, NULL);
                        DidRCTMsg = TRUE;
                        }
                     }
                  else
                     convrt("(P1A1,'-->WARNING: No match w/restore"
                        " file(s) for ',J1A" qLXN ",J0A40,RX,RJ0IH4,"
                        "', will generate.')", &Emcc, fmtlrlnm(il),
                        hwrntxt[icl], &iux, &iux, &phm->ict, NULL);
                  } /* End specific item error or warning message */
               else if (!(il->ctwk.phs1.hmwarn[icl] & HMWWI)) {
                  convrt("(P1A1,'-->WARNING: No match w/restore"
                     "file(s) for ',J1A" qLXN ",'for any ',J0A40,"
                     "'s, will generate.')", &Emcc, fmtlrlnm(il),
                     hwrntxt[icl], NULL);
                  il->ctwk.phs1.hmwarn[icl] |= HMWWI;
                  } /* End generic connection type warning messages */
               Emcc = ' ';
               } /* End if Restoring */
            switch (icl) {    /* Set to generate */
            case SFHMCELL:
               ((struct CELLTYPE *)phm->path)->Ct.ctopt |= OPTOR;
               break;
            case SFHMCONN:
               ((struct CONNTYPE *)phm->path)->Cn.cnopt |= NOPOR;
               break;
            case SFHMGCON:
               ((struct INHIBBLK *)phm->path)->ibopt |= IBOPOR;
               break;
            case SFHMMODU:
               ((struct MODBY *)phm->path)->Mc.mopt |= IBOPOR;
               break;
               } /* End generate-request switch */
            } /* End layer not matched */
         } /* End if isfhm1[icl] */
      } /* End loop over header types */

/* Free header and message allocations */

   if (RP0->psfhm) free(RP0->psfhm);
   free(h);

/*---------------------------------------------------------------------*
*     (3) Calculate record length for replay file                      *
*---------------------------------------------------------------------*/

/* Record size calculated to hold (in this order):
*     trial (8 bytes) + ser (4 bytes) +
*     2 + 32*mxobj bytes for stimulus objects (if esvfia is false)
*        or 2**(kx+ky) bytes for input array (if esvfia is true) +
*     8 bytes per joint for x,y coords
*     16 bytes per window for x, y, ht, width +
*     1 byte per s(i) + space for OPT=R layer data (in d3allo) +
*     4*ljnwd + 2*nvblk bytes for the changes and values data
*
*  N.B.! The version of this information contained in subroutine
*     envsav is considered definitive with regard to environment
*     information in the replay file.  */

   if (RP->CP.runflags & RP_REPLAY) {
      /* Don't save a replay if reading one in! */
      RP->ksv |= KSDNOK;

#if 0    /* This code is obsolete */
      /* Determine total number of joints in all arms */
      struct ARMDEF *pa;
      int tjnts = 0;
      for (pa=RP0->parm1; pa; pa=pa->pnxarm) tjnts += pa->njnts;

      RP0->rprecl += sizeof(long) + sizeof(ui32) +
         (RP0->ljnwd + 2*tjnts + 4*RP->nwdws)*sizeof(float) +
         (RP->nvblk<<1);
      if (RP0->pgds && RP0->pgds->svitms & GFIA)
         RP0->rprecl += RP->nst*sizeof(s_type);
      else if (!(RP->CP.runflags & RP_NOINPT))
         RP0->rprecl += 2 + (RP0->mxobj<<5);

      RP0->rprecp = (RP0->rprecl + 32759L)/32760L;
      RP0->rprecl = (RP0->rprecl + RP0->rprecp - 1)/RP0->rprecp;
#endif
      }  /* End replay setup */

   } /* End d3fchk() */
