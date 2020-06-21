/* (c) Copyright 1992-2017, The Rockefeller University *11113* */
/***********************************************************************
*                              snv8dump                                *
*                                                                      *
*  Program to dump header of a version 8 CNS SAVENET file.             *
*                                                                      *
*  Syntax:  snv8dump <savenet-file-name>                               *
*                                                                      *
*  (There is at present no option to dump the entire contents of the   *
*  file.  The workaround is to restore the file to a CNS run and look  *
*  at the data with a debugger.)                                       *
*                                                                      *
*  Abexit codes 400-409 are assigned to this program                   *
*                                                                      *
*  V1A, 04/01/92, GNR                                                  *
*  V8D, 06/28/08, GNR - Update to current CNS V8D SAVENET format       *
*  Rev, 01/05/12, GNR - Include d3global.h, remove local redefs        *
*  Rev, 12/12/16, GNR - Remove Darwin 1 support                        *
*  Rev, 02/19/17, GNR - Add new header items, types J vs L vs W        *
***********************************************************************/

#define  D1
#define  D3G_NOINCL

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "/home/cdr/src/d3mpi/d3global.h"
#include "/home/cdr/src/d3mpi/savblk.h"
#define  MAIN
#include "rocks.h"
#include "rkxtra.h"
#include "rfdef.h"

/*=====================================================================*
*                               prtvar                                 *
*                                                                      *
*  Print a variable, with or without the detailed information          *
*                                                                      *
*  Arguments:                                                          *
*     pk        Ptr to SFHKey information for this variable.           *
*     pd        Ptr to data (as big-endian chars) for the variable.    *
*     pn        Name of the variable.                                  *
*=====================================================================*/

#define BLKNUM_LEN   5
#define VARNM_LEN   16
#define qVARNM_LEN "16"
#define VDESC_LEN   12
#define VALUE_LEN   14

static void prtvar(SFHKey *pk, char *pd, char *pn) {

   char *pde = pd + (int)pk->len * (int)pk->dim;
   ui32 ic;
   char vnm[VARNM_LEN+VDESC_LEN+5];
   char val[VALUE_LEN+1];
   int  ktyp = pk->type & SFTPM;

   static char tlet[] = { "IUFDXA" };     /* Type codes */
   static char llet[] = { "?CH?J   W" };  /* Length codes */

   /* Set up the name for printing and insert
   *  the descriptor information after the name.  */
   memset(vnm, ' ', VARNM_LEN+VDESC_LEN+4);
   vnm[VARNM_LEN+VDESC_LEN+4] = '\0';
   if (pk->fsclidim)
      sconvrt(vnm, "(O4XA" qVARNM_LEN ",2H (J0UC3,1HBJ0UC2A1A1,1H))",
         pn, &pk->dim, &pk->fsclidim, tlet+ktyp, llet+pk->len, NULL);
   else if (ktyp <= UTYPE)
      sconvrt(vnm, "(O4XA" qVARNM_LEN ",2H (J0UC3,A1A1,1H))",
         pn, &pk->dim, tlet+ktyp, llet+pk->len, NULL);
   else if (ktyp == ATYPE)
      sconvrt(vnm, "(O4XA" qVARNM_LEN ",2H (J0UC3,1HA,J0UC3,1H))",
         pn, &pk->dim, &pk->len, NULL);
   else
      sconvrt(vnm, "(O4XA" qVARNM_LEN ",2H (J0UC3,A1,1H))",
         pn, &pk->dim, tlet+ktyp, NULL);
   cryout(RK_P1, vnm, RK_LN1+VARNM_LEN+VDESC_LEN, NULL);

   /* Convert the value(s) to ASCII for printing */
   ic  = RK_IORF|VALUE_LEN-1;
   switch (ktyp) {
   case FTYPE:
      if (pk->fsclidim) ic |=
         pk->fsclidim<<RK_BS|6<<RK_DS|RK_AUTO;
      /* Speed isn't a big issue here, so the switch
      *  is inside the loop... */
      for ( ; pd<pde; pd+=pk->len) {
         si64 iwv;
         si32 ijv;
         si16 isv;
         switch (pk->len) {
         case FMWSIZE:
            iwv = bemtoi8(pd);
            wbcdwt(&iwv, val, ic|RK_NI64);
            break;
         case FMJSIZE:
            ijv = bemtoi4(pd);
            wbcdwt(&ijv, val, ic|RK_NI32);
            break;
         case FMSSIZE:
            isv = bemtoi2(pd);
            wbcdwt(&isv, val, ic|RK_NHALF);
            break;
         case 1:
            wbcdwt(pd, val, ic|RK_NBYTE);
            break;
            } /* End len switch */
         cryout(RK_P1, val, RK_CCL+VALUE_LEN, NULL);
         } /* End dim loop */
      break;

   case UTYPE:
      if (pk->fsclidim) ic |=
         pk->fsclidim<<RK_BS|6<<RK_DS|RK_AUTO;
      for ( ; pd<pde; pd+=pk->len) {
         ui64 uwv;
         ui32 ujv;
         ui16 usv;
         switch (pk->len) {
         case FMWSIZE:
            uwv = bemtou8(pd);
            wbcdwt(&uwv, val, ic|RK_NUNS|RK_NI64);
            break;
         case FMJSIZE:
            ujv = bemtoi4(pd);
            wbcdwt(&ujv, val, ic|RK_NUNS|RK_NI32);
            break;
         case FMSSIZE:
            usv = bemtoi2(pd);
            wbcdwt(&usv, val, ic|RK_NUNS|RK_NHALF);
            break;
         case 1:
            wbcdwt(pd, val, ic|RK_NUNS|RK_NBYTE);
            break;
            } /* End len switch */
         cryout(RK_P1, val, RK_CCL+VALUE_LEN, NULL);
         } /* End dim loop */
      break;

   case ETYPE:
      ic = 6<<RK_DS|RK_IORF|RK_AUTO|VALUE_LEN-1;
      for ( ; pd<pde; pd+=pk->len) {
         float fv;
         fv = bemtor4(pd);
         bcdout(ic, val, (double)fv);
         cryout(RK_P1, val, RK_CCL+VALUE_LEN, NULL);
         } /* End dim loop */
      break;

   case DTYPE:
      ic = 6<<RK_DS|RK_IORF|RK_AUTO|VALUE_LEN-1;
      for ( ; pd<pde; pd+=pk->len) {
         double fv;
         fv = bemtor8(pd);
         bcdout(ic, val, fv);
         cryout(RK_P1, val, RK_CCL+VALUE_LEN, NULL);
         } /* End dim loop */
      break;

   case XTYPE:
      ic = RK_HEXF|RK_NZ0X|RK_NUNS|VALUE_LEN-1;
      for ( ; pd<pde; pd+=pk->len) {
         ui64 xwv;
         ui32 xjv;
         ui16 xsv;
         switch (pk->len) {
         case FMWSIZE:
            xwv = bemtou8(pd);
            wbcdwt(&xwv, val, ic|RK_NI64);
            break;
         case FMJSIZE:
            xjv = bemtoi4(pd);
            wbcdwt(&xjv, val, ic|RK_NI32);
            break;
         case FMSSIZE:
            xsv = bemtoi2(pd);
            wbcdwt(&xsv, val, ic|RK_NHALF);
            break;
         case 1:
            wbcdwt(pd, val, ic|RK_NBYTE); break;
            } /* End len switch */
         cryout(RK_P1, val, RK_CCL+VALUE_LEN, NULL);
         } /* End dim loop */
      break;

   case ATYPE:
      for ( ; pd<pde; pd+=pk->len) {
         cryout(RK_P1, "  ", RK_CCL+2, pd, RK_CCL+pk->len, NULL);
         } /* End dim loop */
      break;

      } /* End type switch */

   } /* End prtvar() */


/*=====================================================================*
*                        snv8dump main program                         *
*=====================================================================*/

int main(int argc, char *argv[]) {

   struct RFdef      *fp;
   union  SFANYHDR   h;
   int    GotOldSi;           /* TRUE if pre-V8C file */
   int    i,j,k;
   int    nd1b,nrep,nlyr;
   int    nconn,ngeom,nmodu;
   char   tblknum[BLKNUM_LEN+1];
   char   rnm[LSNAME],lnm[LSNAME];

/* Start up ROCKS etc. */

   RK.iexit = 0;
   settit("TITLE Dump CNS V8D SAVENET file");
   setpid("snv8dump");

/* Make sure there is a command-line argument */

   if (argc < 2)
      abexitm(400, "No SAVENET file specified");
   if (argc > 2)
      abexitm(400, "Too many arguments: snv8dump <file>");

/* Allocate rfdef block and try to open file */

   fp = rfallo(argv[1], READ, BINARY, SEQUENTIAL,
      TOP, LOOKAHEAD, REWIND, RELEASE_BUFF, 8192, 0, 0, ABORT);
   rfopen(fp, NULL, SAME, SAME, SAME, SAME, SAME, SAME, SAME,
      SAME, SAME, SAME, ABORT);

/* Read and print header info */

   rfread(fp, &h.sfhdr, SFVERS_LEN, ABORT);
   if (memcmp(h.sfhdr.filetype, SF_IDENT, SFVERS_LEN-4) ||
      ibcdin(RK_IORF+SFVERS_LEN-1, h.sfhdr.filetype) < SF_VERS)
      abexitm(401, "This is not a current CNS SAVENET file");
   /* Read the rest of the header.  Take care of change
   *  in series title length that occurred at V8C.  */
   if (h.sfhdr.filetype[10] < 'C') {
      GotOldSi = TRUE;
      memset(h.sfhdr.stitle, ' ', LSTITL);
      rfread(fp, h.sfhdr.title, SFTITLE_LEN+SFDATE_LEN+44, ABORT);
      }
   else {
      GotOldSi = FALSE;
      rfread(fp, h.sfhdr.title, SFTITLE_LEN+SFDATE_LEN+LSTITL, ABORT);
      }

   cryout(RK_P1, "0Overall header for SAVENET file ", RK_LN5,
      fp->fname, RK_CCL, NULL);
   cryout(RK_P1, "    Title: ", RK_LN0,
      h.sfhdr.title, SFTITLE_LEN, NULL);
   cryout(RK_P1, "    Date/time: ", RK_LN0,
      h.sfhdr.timestamp, SFDATE_LEN, NULL);
   cryout(RK_P1, "    Written after: ", RK_LN0,
      h.sfhdr.stitle+1, LSTITL-1, NULL);

   rfread(fp, &h.sfhdr2, sizeof(struct SFHDR2), ABORT);
   if (memcmp(h.sfhdr2.sfhdblk.blkname, "HDR2", SFKEY_LEN))
      abexitm(402, "Expected SFHDR2 not found");
   nd1b = bemtoi2(h.sfhdr2.sfhnd1b.d);
   nrep = bemtoi2(h.sfhdr2.sfhnrep.d);
   {  ui32 snsd = bemtoi4(h.sfhdr2.sfhsnseed.d);
      convrt("(P1,4X,'Number D1 blocks: ',j0ih4,"
         "', Number regions: ',j0ih4,', Stim noise seed: ',j0il12)",
         &nd1b, &nrep, &snsd, NULL);
      } /* End local scope */

/* Loop over repertoire blocks and print them */

   for (i=1; i<=nrep; i++) {
      rfread(fp, &h.rephdr, sizeof(struct SFREPHDR), ABORT);
      if (memcmp(h.rephdr.sfrepblk.blkname, "RPHD", SFKEY_LEN))
         abexitm(402, "Expected SFREPHDR not found");
      ibcdwt(RK_IORF+RK_LFTJ+BLKNUM_LEN-1, tblknum, i);
      cryout(RK_P1, "0Region/repertoire header ", RK_LN2|RK_SUBTTL|
         RK_NFSUBTTL, tblknum, RK_CCL+BLKNUM_LEN, NULL);
      prtvar(&h.rephdr.repnam.k,  h.rephdr.repnam.d,  "sname");
      prtvar(&h.rephdr.sfrngx.k,  h.rephdr.sfrngx.d,  "ngx");
      prtvar(&h.rephdr.sfrngy.k,  h.rephdr.sfrngy.d,  "ngy");
      prtvar(&h.rephdr.sfgpoff.k, (char *)h.rephdr.sfgpoff.d, "var offsets");
      prtvar(&h.rephdr.sfnlyr.k,  h.rephdr.sfnlyr.d,  "num layers");
      nlyr = (int)bemtoi2(h.rephdr.sfnlyr.d);
      memcpy(rnm, h.rephdr.repnam.d, LSNAME);

/* Loop over celltype blocks and print them */

      for (j=1; j<=nlyr; j++) {
         rfread(fp, &h.cthdr, sizeof(struct SFCTHDR), ABORT);
         if (memcmp(h.cthdr.sfctblk.blkname, "CTHD", SFKEY_LEN))
            abexitm(402, "Expected SFCTHDR not found");
         ibcdwt(RK_IORF+RK_LFTJ+BLKNUM_LEN-1, tblknum, j);
         cryout(RK_P1, "0Region ", RK_LN2|RK_SUBTTL|RK_NFSUBTTL,
            rnm, RK_CCL+LSNAME, ", Celltype header ", RK_CCL,
            tblknum, RK_CCL+BLKNUM_LEN, NULL);
         prtvar(&h.cthdr.celnam.k,    h.cthdr.celnam.d,    "lname");
         prtvar(&h.cthdr.sfctnel.k,   h.cthdr.sfctnel.d,   "cells/group");
         prtvar(&h.cthdr.sfctlel.k,   h.cthdr.sfctlel.d,   "tot bytes/cell");
         prtvar(&h.cthdr.sfctls.k,    h.cthdr.sfctls.d,    "len(cell data)");
         prtvar(&h.cthdr.sfctphshft.k,h.cthdr.sfctphshft.d,"phase shift");
         prtvar(&h.cthdr.sfctsdly1.k, h.cthdr.sfctsdly1.d, "max delay");
         prtvar(&h.cthdr.sfctnizv.k,  h.cthdr.sfctnizv.d,  "num izhi vars");
         prtvar(&h.cthdr.sfctnct.k,   h.cthdr.sfctnct.d,   "num scts");
         prtvar(&h.cthdr.sfctngct.k,  h.cthdr.sfctngct.d,  "num gcts");
         prtvar(&h.cthdr.sfctnmct.k,  h.cthdr.sfctnmct.d,  "num mcts");
         prtvar(&h.cthdr.sfctoff.k, (char *)h.cthdr.sfctoff.d, "var offsets");
         prtvar(&h.cthdr.sfctasmul.k, (char *)h.cthdr.sfctasmul.d, "autoscales");
         prtvar(&h.cthdr.sfctnsd.k,   h.cthdr.sfctnsd.d,   "noise seed");
         prtvar(&h.cthdr.sfctrsd.k,   h.cthdr.sfctrsd.d,   "round seed");
         prtvar(&h.cthdr.sfctpsd.k,   h.cthdr.sfctpsd.d,   "phi seed");
         prtvar(&h.cthdr.sfctzsd.k,   h.cthdr.sfctzsd.d,   "izhi seed");
         prtvar(&h.cthdr.offsi.k,     h.cthdr.offsi.d,     "si offset");
         prtvar(&h.cthdr.offprd.k,    h.cthdr.offprd.d,    "rd offset");
         nconn = (int)bemtoi2(h.cthdr.sfctnct.d);
         ngeom = (int)bemtoi2(h.cthdr.sfctngct.d);
         nmodu = (int)bemtoi2(h.cthdr.sfctnmct.d);
         memcpy(lnm, h.cthdr.celnam.d, LSNAME);

/*---------------------------------------------------------------------*
*                 Two levels of indenting suppressed                   *
*---------------------------------------------------------------------*/

/* Loop over specific conntype blocks and print them */

   for (k=1; k<=nconn; k++) {
      rfread(fp, &h.cnhdr, sizeof(struct SFCNHDR), ABORT);
      if (memcmp(h.cnhdr.sfcnblk.blkname, "CNHD", SFKEY_LEN))
         abexitm(402, "Expected SFCNHDR not found");
      ibcdwt(RK_IORF+RK_LFTJ+BLKNUM_LEN-1, tblknum, k);
      cryout(RK_P1, "0Region ", RK_LN2|RK_SUBTTL|RK_NFSUBTTL,
         rnm, RK_CCL+LSNAME, ", Celltype ", RK_CCL,
         lnm, RK_CCL+LSNAME, ", Conntype header ", RK_CCL,
         tblknum, RK_CCL+BLKNUM_LEN, NULL);
      prtvar(&h.cnhdr.srcsnam.k,    h.cnhdr.srcsnam.d,    "src rname");
      prtvar(&h.cnhdr.srclnam.k,    h.cnhdr.srclnam.d,    "src lname");
      prtvar(&h.cnhdr.sfcnnc.k,     h.cnhdr.sfcnnc.d,     "num conns");
      prtvar(&h.cnhdr.sfcnlc.k,     h.cnhdr.sfcnlc.d,     "len(conn data)");
      prtvar(&h.cnhdr.sfcnlxn.k,    h.cnhdr.sfcnlxn.d,    "len(nuk data)");
      prtvar(&h.cnhdr.sfcnnbc.k,    h.cnhdr.sfcnnbc.d,    "bits/conn");
      h.cnhdr.sfcnkgen.k.type = XTYPE;
      prtvar(&h.cnhdr.sfcnkgen.k,   h.cnhdr.sfcnkgen.d,   "kgen codes");
      prtvar(&h.cnhdr.sfcnoff.k, (char *)h.cnhdr.sfcnoff.d, "var offsets");
      prtvar(&h.cnhdr.sfxnoff.k, (char *)h.cnhdr.sfxnoff.d, "nuk offsets");
      prtvar(&h.cnhdr.sfcnsrcngx.k, h.cnhdr.sfcnsrcngx.d, "source ngx");
      prtvar(&h.cnhdr.sfcnsrcngy.k, h.cnhdr.sfcnsrcngy.d, "source ngy");
      prtvar(&h.cnhdr.sfcnsrcnel.k, h.cnhdr.sfcnsrcnel.d, "source nel");
      prtvar(&h.cnhdr.sfcnsrcnelt.k,h.cnhdr.sfcnsrcnelt.d,"source cells");
      prtvar(&h.cnhdr.sfcnloff.k,   h.cnhdr.sfcnloff.d,   "lij offset");
      prtvar(&h.cnhdr.sfcnlsg.k,    h.cnhdr.sfcnlsg.d,    "lij sigma");
      prtvar(&h.cnhdr.sfcnnrx.k,    h.cnhdr.sfcnnrx.d,    "nrx");
      prtvar(&h.cnhdr.sfcnnry.k,    h.cnhdr.sfcnnry.d,    "nry");
      prtvar(&h.cnhdr.sfcnnhx.k,    h.cnhdr.sfcnnhx.d,    "nhx");
      prtvar(&h.cnhdr.sfcnnhy.k,    h.cnhdr.sfcnnhy.d,    "nhy");
      prtvar(&h.cnhdr.sfcnnvx.k,    h.cnhdr.sfcnnvx.d,    "nvx");
      prtvar(&h.cnhdr.sfcnnvy.k,    h.cnhdr.sfcnnvy.d,    "nvy");
      prtvar(&h.cnhdr.sfcnnsa.k,    h.cnhdr.sfcnnsa.d,    "n subarbor");
      prtvar(&h.cnhdr.sfcnnsax.k,   h.cnhdr.sfcnnsax.d,   "n subarb on x");
      prtvar(&h.cnhdr.sfcnnux.k,    h.cnhdr.sfcnnux.d,    "nux");
      prtvar(&h.cnhdr.sfcnnuy.k,    h.cnhdr.sfcnnuy.d,    "nuy");
      prtvar(&h.cnhdr.sfcnradius.k, h.cnhdr.sfcnradius.d, "lij radius");
      prtvar(&h.cnhdr.sfcnstride.k, h.cnhdr.sfcnstride.d, "lij stride");
      prtvar(&h.cnhdr.sfcnxoff.k,   h.cnhdr.sfcnxoff.d,   "lij x offset");
      prtvar(&h.cnhdr.sfcnyoff.k,   h.cnhdr.sfcnyoff.d,   "lij y offset");
      prtvar(&h.cnhdr.sfcncseed.k,  h.cnhdr.sfcncseed.d,  "cij seed");
      prtvar(&h.cnhdr.sfcnlseed.k,  h.cnhdr.sfcnlseed.d,  "lij seed");
      prtvar(&h.cnhdr.sfcnpseed.k,  h.cnhdr.sfcnpseed.d,  "phase seed");
      prtvar(&h.cnhdr.sfcnrseed.k,  h.cnhdr.sfcnrseed.d,  "round seed");
      prtvar(&h.cnhdr.offprd.k,     h.cnhdr.offprd.d,     "data offset");
      prtvar(&h.cnhdr.sfcncbc.k,    h.cnhdr.sfcncbc.d,    "boundary cdx");
      prtvar(&h.cnhdr.sfcnkcmin.k,  h.cnhdr.sfcnkcmin.d,  "kernel cmin");
      } /* End conntype loop */

/* Loop over geometric connection types and print */

   for (k=1; k<=ngeom; k++) {
      rfread(fp, &h.cghdr, sizeof(struct SFCGHDR), ABORT);
      if (memcmp(h.cghdr.sfcgblk.blkname, "CGHD", SFKEY_LEN))
         abexitm(402, "Expected SFCGHDR not found");
      ibcdwt(RK_IORF+RK_LFTJ+BLKNUM_LEN-1, tblknum, k);
      cryout(RK_P1, "0Region ", RK_LN2|RK_SUBTTL|RK_NFSUBTTL,
         rnm, RK_CCL+LSNAME, ", Celltype ", RK_CCL,
         lnm, RK_CCL+LSNAME, ", Geometric conntype header ", RK_CCL,
         tblknum, RK_CCL+BLKNUM_LEN, NULL);
      prtvar(&h.cghdr.gsrcrid.k, h.cghdr.gsrcrid.d, "src rname");
      prtvar(&h.cghdr.gsrclid.k, h.cghdr.gsrclid.d, "src lname");
      prtvar(&h.cghdr.sfcgngb.k, h.cghdr.sfcgngb.d, "groups/band");
      prtvar(&h.cghdr.sfcgnib.k, h.cghdr.sfcgnib.d, "num bands");
      } /* End gconn loop */

/* Loop over modulatory connection types and print */

   for (k=1; k<=nmodu; k++) {
      rfread(fp, &h.cmhdr, sizeof(struct SFCMHDR), ABORT);
      if (memcmp(h.cmhdr.sfcmblk.blkname, "CMHD", SFKEY_LEN))
         abexitm(402, "Expected SFCMHDR not found");
      ibcdwt(RK_IORF+RK_LFTJ+BLKNUM_LEN-1, tblknum, k);
      cryout(RK_P1, "0Region ", RK_LN2|RK_SUBTTL|RK_NFSUBTTL,
         rnm, RK_CCL+LSNAME, ", Celltype ", RK_CCL,
         lnm, RK_CCL+LSNAME, ", Modulatory conntype header ", RK_CCL,
         tblknum, RK_CCL+BLKNUM_LEN, NULL);
      prtvar(&h.cmhdr.msrcrid.k, h.cmhdr.msrcrid.d, "src rname");
      prtvar(&h.cmhdr.msrclid.k, h.cmhdr.msrclid.d, "src lname");
      } /* End gconn loop */

/*---------------------------------------------------------------------*
*                      Resume original indenting                       *
*---------------------------------------------------------------------*/

         } /* End celltype loop */
      } /* End repertoire/region loop */

/* Finished */

   rfclose(fp,SAME,SAME,ABORT);
   cryout(RK_P1,"   ",RK_LN1+RK_FLUSH,NULL);
   return 0;
   } /* End snv8dump */

