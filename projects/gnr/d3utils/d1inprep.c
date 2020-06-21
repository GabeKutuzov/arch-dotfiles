/* (c) Copyright 1992-2010, The Rockefeller University *11113* */
/***********************************************************************
*                              D1INPREP                                *
*                                                                      *
*  Program to prepare randomized input data for testing CNS with D1.   *
*  Currently only makes non-indexed DIGINPUT data.  Accepts control    *
*  cards from stdin as follows:                                        *
*                                                                      *
*  TITLE (or other cryin-recognized cards)                             *
*                                                                      *
*  FILE name [VERSION=<1|2>]                                           *
*                                                                      *
*     name = full pathname of file to be created                       *
*     VERSION=1 omits the category (pattern number) which is written   *
*              in each record in version 2.  Version 2 is the default. *
*                                                                      *
*  TRIALS nt                                                           *
*                                                                      *
*     nt   = number of trials for which to generate data               *
*                                                                      *
*  BLOCK  nd1r nbpe [seed]                                             *
*                                                                      *
*     nd1r = number of D1 repertoires in this block                    *
*     nbpe = number of bits per recogonizing element                   *
*     seed = random number generating seed for pattern selection       *
*              this block                                              *
*                                                                      *
*  PATTERN jrep [frac] [fmod] [STRING=bitstring],[MASK=bitmask],       *
*      [SEED=seed],[GROUP=ign]                                         *
*                                                                      *
*     jrep = repertoire (1..nd1r) to which pattern applies             *
*     frac = fraction of data to be generated using this pattern       *
*              (error if not 0 and CORRELATE card follows)             *
*     fmod = fraction of bits with mask=1 to be modified in each       *
*              trial (default 0.20)                                    *
*     bitstring = prototype string for this pattern (nbpe bits)        *
*     bitmask   = mask for bit modification (0 = freeze, 1 = modify)   *
*     seed = random number generating seed for bit modification for    *
*              this pattern                                            *
*     ign  = group (category) number assigned to this pattern.  This   *
*              number is incremented by one for each PATTERN card      *
*              that is encountered without a group specified.          *
*                                                                      *
*  CORRELATE iblk irep c1k,c2k,c3k,...                                 *
*                                                                      *
*     iblk = number of earlier BLOCK card to which pattern selection   *
*              is linked                                               *
*     irep = number of repertoire in that block to which linked        *
*     c1k,c2k,... = correlation coefficients relating probability of   *
*              selection of this pattern (k) to the selection of the   *
*              i'th (i=1,2,...) pattern for BLOCK iblk and REP irep    *
*                                                                      *
*  AFTER nt iblk [seed]                                                *
*                                                                      *
*     Action:  The AFTER card causes the patterns entered for a        *
*              particular block to be erased after trial 'nt'.         *
*              This card may be followed by as many PATTERN and        *
*              CORRELATE cards as desired, just as if starting over.   *
*              Default GROUP assignment begins with the group number   *
*              previously assigned to the first pattern in the block.  *
*              AFTER cards must be entered after all initial setup     *
*              cards, and must be in order of increasing 'nt'.         *
*     nt   = trial number after which pattern change takes place       *
*     iblk = number of block to which new patterns apply               *
*     seed = new pattern selection seed for this block                 *
*                                                                      *
*  There may be as many BLOCK cards as desired, each followed by as    *
*  many PATTERN cards as desired.  All of the PATTERN cards for a      *
*  given block must either use the 'frac' parameter or have CORRELATE  *
*  cards following (no mixing allowed).  If the sum over PATTERN cards *
*  of the 'frac' parameters or the cik coefficients for any value of i *
*  and for a given block and repertoire exceeds 1.0, an error occurs.  *
*  If the sum is less than 1.0, random patterns will be used with the  *
*  residual probability.                                               *
*                                                                      *
*  The output record for each stimulus in a version 2 file includes a  *
*  two-byte (big-endian) integer which gives the serial position in    *
*  the control file of the PATTERN card that generated that stimulus.  *
*  If the stimulus was generated without using a pattern, the number   *
*  assigned is one larger than the number of the last PATTERN card.    *
*  These numbers can be used in CNS to assign environmental value to   *
*  stimuli according to externally given categories.  They can also    *
*  be used to generate cross-response statistics.                      *
*                                                                      *
*  CAUTION:  The current definition of the input file permits the      *
*  various patterns for some repertoire to be correlated to DIFFERENT  *
*  prior repertoires.  It is not clear if this is meaningful.  In      *
*  particular, the sum(prob) = 1.0 rule above cannot even be checked,  *
*  because there is no guarantee that the patterns to which correla-   *
*  tion occurs themselves have total probability unity.                *
*                                                                      *
*  This program is assigned error codes in the range 390-399           *
*                                                                      *
*  V1A, 01/13/92, GNR                                                  *
*  V1B, 01/29/92, GNR - Add correlated patterns                        *
*  V1C, 02/07/92, GNR - Add separate seed for bit modification         *
*  V2A, 02/12/92, GNR - Add category byte to output format, AFTER card *
*  Rev, 03/06/92, GNR - Use rf io routines                             *
*  Rev, 09/05/99, GNR - Add proper swapping to make big-endian file    *
*  Rev, 06/19/10, GNR - Change type of ic, add ia as int counter       *
***********************************************************************/

typedef unsigned char s_type;

#define MAIN
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "../d3/d1def.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"
#include "rfdef.h"
#include "swap.h"
#include "bapkg.h"

#define FileVersion  2        /* Latest version number allowed */
#define TermErr      2        /* Rk.iexit code for main pgm error */
#define BitsPerByte  8        /* Number of bits in a byte */

/* Definition of structures to hold control information */

struct BLKDEF {               /* Structure created from BLOCK card */
   struct BLKDEF *pnbd;          /* Ptr to next block def */
   struct PATDEF *pfpd;          /* Ptr to first pattern def */
   char *pwk;                    /* Ptr to pattern work area */
   long *ipat;                   /* Array holding pattern choices--
                                 *  is long to hold prob check sums
                                 *  in setup (not yet implemented). */
   short nd1r;                   /* Number of repertoires */
   short nbpe;                   /* Number of bits per repertoire */
   long seed;                    /* Random number generating seed */
   short ignb;                   /* Initial group number in block */
   };

struct PATDEF {               /* Structure created from PATTERN card */
   struct PATDEF *pnpd;          /* Ptr to next pattern definition */
   struct BLKDEF *pcblk;         /* Ptr to block to which correlated */
   byte *ppatt;                  /* Ptr to initial pattern */
   byte *pmask;                  /* Ptr to modification mask */
   long *pcorr;                  /* Ptr to correlation probs (S28) */
   long jrep;                    /* Repertoire number */
   long frac;                    /* Fraction of patterns (S28) */
   long fmod;                    /* Fraction of bits modified (S28) */
   long seed;                    /* Random number generating seed */
   short jcrep;                  /* Repertoire correlated with */
   short ignp;                   /* Group number of this pattern */
   };

int main(void) {

   struct D1FHDR fhdr;        /* Output file general header */
   struct D1FBHD fbhd;        /* Output file block header */
   struct BLKDEF *pfbd;       /* Ptr to first block def */
   struct BLKDEF *pbd;        /* Ptr to current block def */
   struct BLKDEF *pcbd;       /* Ptr to correlation block def */
   struct BLKDEF **ppbd;      /* Ptr to ptr to current block def */
   struct PATDEF *ppd;        /* Ptr to current pattern def */
   struct PATDEF *pcpd;       /* Ptr to control card pattern def */
   struct PATDEF **pppd;      /* Ptr to ptr to current pattern def */
   struct RFdef  *pdf;        /* Ptr to data file */
   byte *pstr;                /* Ptr to string being scanned */
   char *pm;                  /* Ptr to file buffer data */
   long it;                   /* Trial counter */
   long mseed;                /* Master seed for defaults */
   long mt;                   /* Last trial to generate now */
   long nd1b;                 /* Number of BLOCK cards */
   long nt;                   /* Last trial to generate in full run */
   ui32 ic;                   /* Return code from kwscan */
   int ia,ib;                 /* Pattern byte or bit loop index */
   int icb,icr;               /* Control card block, repertoire */
   int kwret;                 /* Return value from kwscan */
   int nbyt;                  /* Num bytes needed for nbpe bits */
   int nrpat;                 /* Number of patterns for some rep */
   int oldlen;                /* Old scan length */
   int smret;                 /* Match value of control card */
   short ir;                  /* Repertoire index */
   short ign;                 /* Number of pattern selected */
   short igno;                /* Highest group number overall */
   short Vers = 2;            /* Version number of output file */
   char fnam[LFILNM+1];       /* File name */

/* Define control card keys and positioning constraints */
   static char *d1pcc[] =
      { "FILE", "TRIALS", "BLOCK", "PATTERN", "CORRELATE", "AFTER" };
   static int NCC = sizeof(d1pcc)/sizeof(char *);
   /* d1lcc = 1 if card legal when it > 0, otherwise 0 */
   static int d1lcc[] = {0, 0, 0, 1, 1, 1};
/* Define match code of each control card type */
#define FILE_CARD    1
#define TRIALS_CARD  2
#define BLOCK_CARD   3
#define PATTERN_CARD 4
#define CORREL_CARD  5
#define AFTER_CARD   6

/* Initialize for no blocks, no errors, no trials */

   fnam[0] = '\0';
   ppbd = &pfbd;
   mseed = 1009;
   nd1b = 0;
   igno = 1;
   it = mt = nt = 0;
   RK.iexit = 0;
   settit("TITLE DARWIN I STIMULUS PREPARATION PROGRAM");

/*---------------------------------------------------------------------*
*           Stage I:  Read control cards until end of stdin            *
*---------------------------------------------------------------------*/

RESTART:                      /* Here when AFTER count satisfied */
   while (cryin()) {

      /* Match the card.  Exit if error (RK.iexit will be set) */
      if ((smret = smatch(0,RK.last,d1pcc,NCC)) <= 0) break;

      /* If the card is an AFTER card whose time hasn't come yet,
      *  push it back without printing and go run.  */
      if (smret == AFTER_CARD) {
         inform("(SW1,VIL)",&mt,NULL);
         if (mt > nt) {
            cdprnt(RK.last);
            cryout(RK_P1,"0***AFTER nt exceeds total trials.",
               RK_LN2,NULL);
            RK.iexit |= TermErr;
            } /* End if mt > nt */
         if (it < mt) {
            rdagn();          /* Push back the card */
            break;            /* Generate up to trial mt */
            } /* End if it < mt */
         mt = nt;             /* Go to end if no more AFTER cards */
         } /* End preliminary processing of AFTER card */

      cdprnt(RK.last);        /* Print the card */

      /* Generate error if card is illegally placed */
      if (it && !d1lcc[smret-1]) {
         cryout(RK_P1,"0***This card must precede any cycling.",
            RK_LN2,NULL);
         abexit(390); }

      /* Switch according to type of control card found */
      switch (smret) {

      case FILE_CARD:
         cdscan(RK.last, 1, LFILNM, RK_WDSKIP);
         scanck(fnam, RK_REQFLD, ~(RK_BLANK|RK_COMMA));
         scanlen(DFLT_MAXLEN);
         kwscan(&ic, "VERSION%VIH", &Vers, NULL);
         if (Vers <= 0 || Vers > FileVersion) ermark(RK_NUMBERR);
         break;

      case TRIALS_CARD:
         inform("(SW1,VIL)",&nt,NULL);
         mt = nt;
         break;

      case BLOCK_CARD:
         /* Create linked list of BLKDEF structures */
         pbd = (struct BLKDEF *)
            mallocv(sizeof(struct BLKDEF), "block definition");
         *ppbd = pbd;
         ppbd = &pbd->pnbd;
         pbd->pnbd = NULL;
         /* Make an empty pattern list */
         pbd->pfpd = NULL;
         pppd = &pbd->pfpd;
         /* Set defaults */
         pbd->nd1r = pbd->nbpe = 0; /* Safety */
         pbd->seed = mseed;
         mseed += 1001;
         /* Save first grpno for possible pattern reload */
         pbd->ignb = igno;
         /* Read parameters */
         inform("(SW1,V>IH,V>IH,NVIL)",
            &pbd->nd1r, &pbd->nbpe, &pbd->seed, NULL);
         if (pbd->nd1r <= 0 || pbd->nbpe <= 0) break;
         /* Set up for pattern cards */
         nbyt = (pbd->nbpe + BitsPerByte - 1)/BitsPerByte;
         pbd->pwk = mallocv(pbd->nbpe+1, "pattern work area");
         pbd->ipat = mallocv(pbd->nd1r*sizeof(long),
            "pattern choice array");
         nd1b++;
         break;

      case PATTERN_CARD:
         /* Create linked list of PATDEF structures */
         ppd = (struct PATDEF *)
            mallocv(sizeof(struct PATDEF), "pattern definition");
         *pppd = ppd;
         pppd = &ppd->pnpd;
         ppd->pnpd = NULL;
         ppd->pcorr = NULL;
         /* Allocate strings, fill in defaults */
         ppd->ppatt = mallocv(nbyt, "pattern string");
         ppd->pmask = mallocv(nbyt, "pattern mask");
         for (ib=0; ib<nbyt; ib++) {
            ppd->ppatt[ib] = (byte)udev(&pbd->seed);
            ppd->pmask[ib] = 0xff;
            } /* End byte loop */
         ppd->frac = 0;
         ppd->fmod = (long)(0.20 * (float)(1L<<28) + 0.5);
         ppd->seed = mseed;
         mseed += 1001;
         /* Read parameters */
         inform("(SW1,VIL=VB28IL,VB28IL)",
            &ppd->jrep, &ppd->frac, &ppd->fmod, NULL);
         while (kwret = kwscan(&ic, "STRING%X", "MASK%X",
               "SEED%VIL",&ppd->seed, "GROUP%VIH",&igno, NULL)) {
            if (RK.scancode != RK_EQUALS) ermark(RK_PUNCERR);
            else {            /* Process string */
               pstr = (kwret == 1) ? ppd->ppatt : ppd->pmask;
               oldlen = scanlen((int)pbd->nbpe);
                    scanck(pbd->pwk, RK_REQFLD, ~RK_BLANK);
               if (RK.length + 1 !=  pbd->nbpe) ermark(RK_NUMBERR);
               memset((char *)pstr, 0, nbyt);
               for (ib=1; ib<=pbd->nbpe; ib++) {
                  if (pbd->pwk[ib-1] == '1') bitset(pstr,ib);
                  else if (pbd->pwk[ib-1] != '0') ermark(RK_NUMBERR);
                  } /* End bit loop */
               scanlen(oldlen);
               } /* End processing string */
            } /* End kwret while */

         ppd->ignp = igno++;  /* Store pattern number */
         if (ppd->jrep > pbd->nd1r) {
            cryout(RK_P1,"0***jrep exceeds nrep.",RK_LN2,NULL);
            RK.iexit |= TermErr; }
         break;

      case CORREL_CARD:
         if (ppd->pcorr) {
            cryout(RK_P1,"0***Multiple CORREL cards illegal.",
               RK_LN2,NULL);
            RK.iexit |= TermErr; }
         if (ppd->frac) {
            cryout(RK_P1,"0***CORREL and frac incompatible.",
               RK_LN2,NULL);
            RK.iexit |= TermErr; }
         /* Get block and rep numbers */
         inform("(SW1,VI,VI)",&icb,&icr,NULL);
         /* Find the requested block */
         if (icb > nd1b) {
            cryout(RK_P1,"0***Block number > nd1b.",RK_LN2,NULL);
            RK.iexit |= TermErr;
            break; }       /* Must abandon this CORREL card */
         for (ib=1,pcbd=pfbd; ib<icb; ib++)
            pcbd = pcbd->pnbd;
         ppd->pcblk = pcbd;
         /* Check that selected repertoire exists and store */
         if (icr > pcbd->nd1r) {
            cryout(RK_P1,"0***Rep number > nd1r.",RK_LN2,NULL);
            RK.iexit |= TermErr; }
         ppd->jcrep = icr;
         /* Count patterns for selected repertoire */
         nrpat = 0;
         for (pcpd=pcbd->pfpd; pcpd; pcpd=pcpd->pnpd)
            if (pcpd->jrep == icr) ++nrpat;
         /* Allocate space for correlation probabilities */
         if (nrpat <= 0) {
            cryout(RK_P1,"0***No patterns in selected rep.",
               RK_LN2,NULL);
            RK.iexit |= TermErr; }
         else {
            ppd->pcorr = mallocv(nrpat*sizeof(long), "pattern mask");
            /* Read correlation probabilities */
            inform("(S,+RB28IL)",&nrpat,ppd->pcorr,NULL);
            if (scan(NULL,0) != RK_ENDCARD) {
               cryout(RK_P1,"0***Too many correlations entered.",
                  RK_LN2,NULL);
               RK.iexit |= TermErr; }
            } /* End found matching patterns */
         break;

      case AFTER_CARD:
         /* N.B. mt has already been read and found >= it */
         if (it > mt) {
            cryout(RK_P1,"0***AFTER cards out of order.",
               RK_LN2,NULL);
            RK.iexit |= TermErr; }
         inform("(S,VI)",&icb,NULL);
         /* Locate the block selected for repatterning */
         if (icb > nd1b)      /* Must die now */
            abexitm(391, "Block number > nd1b");
         for (ib=1,pbd=pfbd; ib<icb; ib++) pbd = pbd->pnbd;
         /* Replace the seed if specified */
         inform("(SNVIL)",&pbd->seed,NULL);
         /* Remove all patterns associated with this BLKDEF */
         for (ppd=pbd->pfpd; ppd; ppd=pcpd) {
            pcpd = ppd->pnpd;
            free(ppd->ppatt);
            free(ppd->pmask);
            if (ppd->pcorr) free(ppd->pcorr);
            free(ppd);
            } /* End removing patterns */
         /* Reset BLKDEF for building new pattern list */
         pbd->pfpd = NULL;
         pppd = &pbd->pfpd;
         igno = pbd->ignb;    /* Restart pattern counter */
         } /* End control card switch */

      } /* End of control cards */

/* Terminate if there were control card errors */

   if (nt <= 0) {
      cryout(RK_P1,"0***nt not > 0.",RK_LN2,NULL);
      RK.iexit |= TermErr; }

   if (nd1b <= 0) {
      cryout(RK_P1,"0***No BLOCKs found.",RK_LN2,NULL);
      RK.iexit |= TermErr; }

   if (RK.iexit) {
      cryout(RK_P1,"0***Terminated due to input errors.",RK_LN2,NULL);
      abexit(392); }

/*---------------------------------------------------------------------*
*                      Stage II:  Write the file                       *
*---------------------------------------------------------------------*/

/* Once-only initialization */

   if (it <= 0) {

/* Attempt to open file.  Terminate if failure */

      pdf = rfallo(fnam,WRITE,BINARY,SEQUENTIAL,TOP,NO_LOOKAHEAD,
         REWIND,RELEASE_BUFF,2048,IGNORE,IGNORE,ABORT);
      rfopen(pdf,NULL,SAME,SAME,SAME,SAME,SAME,SAME,SAME,
         SAME,SAME,SAME,ABORT);

/* Prepare and write the general header.
*  N.B.  Accesses to file header info via fhdr struct is
*  technically incorrect--should define a file version of
*  this struct with each element defined as a string with
*  a length from the set FMxSIZE in swap.h and rewrite the
*  bemfmxx calls accordings.  This change deferred to such
*  time as this program is every actually used again...  */

      memcpy(fhdr.dhtype,"D1IN",sizeof(fhdr.dhtype));
      pm = (char *)&fhdr.dhvers;
      bemfmi2(pm,Vers);
      pm = (char *)&fhdr.dhmode;
      bemfmi2(pm,DHM_NORM);
      pm = (char *)&fhdr.dhnd1b;
      bemfmi4(pm,nd1b);
      pm = (char *)fhdr.dhnt;
      bemfmi4(pm,nt);
      rfwrite(pdf, &fhdr, sizeof(struct D1FHDR), ABORT);

/* Prepare and write the block headers */

      for (pbd=pfbd; pbd; pbd=pbd->pnbd) {
         pm = (char *)&fbhd.dhnd1r;
         bemfmi2(pm,pbd->nd1r);
         pm = (char *)&fbhd.dhnbpe;
         bemfmi2(pm,pbd->nbpe);
         rfwrite(pdf, &fbhd, sizeof(struct D1FBHD), ABORT);
         } /* End writing block headers */
      } /* End once-only initialization */

/* Sort patterns by repertoire and check total frac for each */

   for (pbd=pfbd,ib=1; pbd; pbd=pbd->pnbd,ib++) {
      sort(pbd->pfpd,
         (char *)&((struct PATDEF *)0)->jrep - (char *)0,
         2*sizeof(long), 0);
      ppd=pbd->pfpd;
      for (ir=1; ir<=pbd->nd1r; ir++) {
         unsigned long totfrac = 0; /* Total pattern fraction */
         for ( ; ppd && ppd->jrep <= ir; ppd=ppd->pnpd) {
            if (ppd->jrep < ir) continue;
            if (!ppd->frac && !ppd->pcorr) {
               convrt("('0***Neither frac nor CORREL found for block ',"
                  "j1i4,'repertoire ',j0ih4)",&ib,&ir,NULL);
               RK.iexit |= TermErr; }
            totfrac += (unsigned long)ppd->frac;
            } /* End pattern definition scan */
         if (totfrac > 1L<<28) {
            convrt("('0***Total frac exceeds 1.0 for block ',j1i4,"
               "'repertoire ',j0ih4)",&ib,&ir,NULL);
            RK.iexit |= TermErr; }
         } /* End repertoire loop */
      } /* End block loop */

   if (RK.iexit) abexit(393); /* Stop if new errors found */

/* Create data for nt trials */

   mt = min(mt,nt);           /* Don't exceed original trial count */
   for ( ; it<mt; it++) {
      /* Loop over blocks requested */
      for (pbd=pfbd; pbd; pbd=pbd->pnbd) {
         nbyt = (pbd->nbpe + BitsPerByte - 1)/BitsPerByte;
         ppd = pbd->pfpd;     /* Locate first pattern */
         /* Loop over repertoires in this block */
         for (ir=1; ir<=pbd->nd1r; ir++) {
            long tfrac = 0;   /* Indicate udev call needed */
            long ufrac;       /* Probability actually used */
            int ipt;          /* Pattern used in correlated rep */

            /* Select which pattern to use.  This algorithm walks
            *  through the pattern chain in step with the current
            *  repertoire number (ir).  If it runs off the end of
            *  the chain, or if the generated fraction exceeds the
            *  sum of the pattern fractions, the loop falls through
            *  and a random pattern is generated.  If correlation
            *  in effect, probability is chosen according to pat-
            *  tern selection for prior block.  */
            for (ia=0; ppd && ppd->jrep <= ir; ppd=ppd->pnpd) {
               if (ppd->jrep < ir) continue;
               if (!tfrac) tfrac = udev(&pbd->seed) >> 3; /* (S28) */
               if (ppd->pcorr) {
                  ipt = ppd->pcblk->ipat[ppd->jcrep-1];
                  ufrac = ppd->pcorr[ipt]; }
               else
                  ufrac = ppd->frac;
               if (tfrac <= ufrac) goto UsePrototype;
               tfrac -= ufrac;   /* (tfrac cannot become zero) */
               ++ia;
               } /* End pattern definition scan */

            /* Generate a random pattern */
            for (ib=0; ib<nbyt; ib++)
               pbd->pwk[ib] = (byte)udev(&pbd->seed);
            ign = igno;
            goto WritePattern;

            /* Generate by modifying a prototype pattern */
UsePrototype:
            memcpy(pbd->pwk,(char *)ppd->ppatt,nbyt);
            for (ib=1; ib<=pbd->nbpe; ib++)
               if (bittst(ppd->pmask,ib) &&
                  (udev(&ppd->seed) >> 3 <= ppd->fmod))
                     bitcmp((unsigned char *)pbd->pwk,ib);
            ign = ppd->ignp;

            /* Write the pattern just generated */
WritePattern:
            pbd->ipat[ir-1] = ia;
            if (Vers >= 2) rfwi2(pdf, ign);
            rfwrite(pdf, pbd->pwk, nbyt, ABORT);

            } /* End repertoire loop */
         } /* End block loop */
      } /* End trial loop */
   if (mt < nt) goto RESTART;

/* Finish up */

   rfclose(pdf, SAME, SAME, ABORT);

   cryout(RK_P1, "0D1 input file preparation complete.",
      RK_LN2+RK_FLUSH, NULL);

   return 0;
   } /* End d1inprep */

