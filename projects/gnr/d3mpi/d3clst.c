/* (c) Copyright 1990-2017, The Rockefeller University *21116* */
/* $Id: d3clst.c 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*     findcl, getclid, d3clrm, d3clrx, d3clst, d3clck, and d3clsm      *
*    INTERPRET, CHECK, FIND, AND MANIPULATE MULTI-TYPE CELL LISTS      *
*                                                                      *
************************************************************************
*  V3L, 04/07/90, GNR - Translated from the FORTRAN version            *
*  Rev, 03/06/92, KJF,GNR - Add KCHIGH, clear names                    *
*  Rev, 01/15/93, GNR - Correct bug due to LSNAME < DFLT_MAXLEN        *
*  V8B, 02/18/01, GNR - Complete rewrite, use ROCKS iteration lists    *
*  Rev, 07/14/01, GNR - Add findcl(), d3clrm(), d3clsm(), and CLHDRs   *
*  Rev, 08/03/01, GNR - Embed ilst in clblk to cut overhead            *
*  V8C, 08/31/03, GNR - Use ptr to ilst, not ilst itself, in CLBLK to  *
*                       to avoid bug if ilstread deletes then creates  *
*  ==>, 01/13/07, GNR - Last mod before committing to svn repository   *
*  V8D, 06/18/08, GNR - Make CLKM_SINGLE only scan one list            *
*  V8H, 04/23/11, GNR - Postpone locating CELLTYPE in CLKM_INDIV mode  *
*  R67, 11/11/16, GNR - Add provision for anonymous cell lists,        *
*                       duplication of cell lists now via COPY option. *
*  R72, 02/23/17, GNR - Add CLCK_SIMDAT mode, error return to d3clck   *
*  R72, 03/30/17, GNR - Use clloc for all clid, clndx, add clckndx     *
***********************************************************************/

#ifdef PARn
#error This routine should not be compiled on comp nodes
#else

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "simdata.h"
#include "rocks.h"
#include "rkxtra.h"
#include "clblk.h"
#include "ijpldef.h"

/* Length of cell list scan field is max of LSNAME or SCANLEN */
#define LCLSF ((LSNAME > SCANLEN) ? LSNAME : SCANLEN)


/***********************************************************************
*                                                                      *
*                               FINDCL                                 *
*                                                                      *
*  To locate a cell list, given its number:                            *
*                                                                      *
*     struct CLHDR *findcl(clloc *pclloc, int ierr)                    *
*                                                                      *
*  Where:                                                              *
*     pclloc   is a ptr to a clloc describing the desired cell list    *
*     ierr     is TRUE to generate an error if the cell list is        *
*              missing, otherwise FALSE.                               *
*                                                                      *
*  If an index is given, findcl() checks that it exists, but still     *
*  returns the pointer to the entire list (which may have >1 index).   *
*  If the specified cell list does not exist, a NULL pointer is        *
*  returned.  Additionally, if ierr > 0, an error message is issued    *
*  and RK.iexit is set nonzero.                                        *
*                                                                      *
*  Implementation note:                                                *
*     A normal run is expected to have no more than half a dozen or    *
*     so cell lists, therefore, a simple sequential search is done.    *
*                                                                      *
***********************************************************************/

struct CLHDR *findcl(clloc *pclloc, int ierr) {

   struct CLHDR *pclh;

/* Search for cell list with requested id number and index */

   for (pclh=RP0->pclh1; pclh; pclh=pclh->pnclh) {
      if ((int)pclh->clnum == pclloc->clnum) {
         if (pclh->nclb >= pclloc->clndx) return pclh;
         else break;
         }
      }

/* The specified list was not found.  If ierr > 0,
*  generate an error.  Return a NULL pointer.  */

   if (ierr > 0) {
      convrt("(P1,'0***CELL LIST ',J1I4,'DOES NOT EXIST.')",
         &pclloc->clnum, NULL);
      RK.iexit |= 1;
      }

   return NULL;

   } /* End findcl() */


/***********************************************************************
*                                                                      *
*                               GETCLID                                *
*                                                                      *
*  To process the construct "CLIST={clnum|(clnum,clndx)|(clnum,OFF)}"  *
*     (where the keyword "CLIST" has already been scanned):            *
*                                                                      *
*     int getclid(clloc *pclloc)                                       *
*                                                                      *
*  Where:                                                              *
*     pclloc   is a pointer to location where clloc is to be stored.   *
*                                                                      *
*  Value returned:                                                     *
*     TRUE     if OFF keyword was found, otherwise FALSE.              *
*                                                                      *
*  R72, 03/30/17, GNR - Code no longer can issue an error for an       *
*  unwanted index -- caller must do this.                              *
***********************************************************************/

int getclid(clloc *pclloc) {

   char lea[SCANLEN1];

   eqscan(&pclloc->clnum, "V>IH", RK_EQCHK);
   pclloc->clndx = 0;

   if ((RK.scancode & (RK_INPARENS|RK_RTPAREN)) == RK_INPARENS) {
      scan(lea, RK_NEWKEY|RK_REQFLD);
      if (RK.scancode != (RK_INPARENS|RK_RTPAREN))
         ermark(RK_PUNCERR);
      if (ssmatch(lea, "OFF", 3))
         return TRUE;
      wbcdin(lea, &pclloc->clndx, (SCANLEN-1) |
         RK_ZTST|RK_CTST|RK_QPOS|RK_IORF|RK_NHALF);
      }

   return FALSE;

   } /* End getclid() */


/***********************************************************************
*                                                                      *
*                               D3CLRM                                 *
*                                                                      *
*  This routine removes a cell list, given its clnum.  If the speci-   *
*  fied list does not exist, the request is quietly ignored.           *
*                                                                      *
*  Syntax:                                                             *
*     void d3clrm(int clnum)                                           *
*                                                                      *
*  Argument:                                                           *
*     clnum     Cell list id of list to be deleted                     *
*                                                                      *
***********************************************************************/

void d3clrm(int clnum) {

   struct CLHDR **ppclh,*pclh;
   struct CLBLK *pclb,*pnxt;

   ilstsalf(d3alfpcv, d3alfprv, d3alfree);
   /* Assignment intended in test clause of following for loop */
   for (ppclh=&RP0->pclh1; pclh=*ppclh; ppclh=&pclh->pnclh) {
      if ((int)pclh->clnum == clnum) {
         for (pclb=pclh->pfclb; pclb; ) {
            pnxt = pclb->pnclb;
            freeilst(pclb->pclil);
            freep(pclb);
            pclb = pnxt;
            }
         *ppclh = pclh->pnclh;
         free(pclh);
         break;
         }
      }

   } /* End d3clrm() */


/***********************************************************************
*                                                                      *
*                               D3CLRX                                 *
*                                                                      *
*  This routine removes all references to cell lists of a given class  *
*  from all CELLTYPE blocks in the network.  If the reference is to    *
*  a negative clnum, the cell list itself is also removed.  But the    *
*  chore of removing any pointers to the cell list is left to d3news.  *
*                                                                      *
*  Syntax:                                                             *
*     void d3clrx(int kctcl)                                           *
*                                                                      *
*  Argument:                                                           *
*     kctcl     Kind of entry in ctclloc being removed                 *
*                                                                      *
*  Note:  il->ctclloc was set up assuming there might be need to clear *
*  out all references of a given CTCL class at once.  Currently, only  *
*  CTCL_PRINT is handled this way, but we might need this again later. *
***********************************************************************/

void d3clrx(int kctcl) {

   struct CELLTYPE *il;

   for (il=RP->pfct; il; il=il->pnct) {
      if (il->ctclloc[kctcl].clnum < 0)
         d3clrm(il->ctclloc[kctcl].clnum);
      il->ctclloc[kctcl].clnum = 0;
      }

   } /* End d3clrx() */


/***********************************************************************
*                                                                      *
*                               D3CLST                                 *
*                                                                      *
*  This routine is intended for interpreting any CNS control card      *
*  that has lists of cells from possibly more than one cell type.      *
*  It constructs (or removes) a linked list of CLBLK structures        *
*  based on the input received.                                        *
*                                                                      *
*  The control card syntax accepted by d3clst() is:                    *
*     <MXCELLS=mxcell> <OFF|ADD|DEL|NEW> snln(c1,c2,c3-c4,...),        *
*  where snln may be a new-style layer identifier, '(sn,ln)', an old-  *
*  style layer identifier, 'snln', '(*)' or '(*,*)' to indicate that   *
*  the list is anonymous (does not specify one particular cell type),  *
*  or there may be no snln at all (CLKM_SINGLE mode only).             *
*  An anonymous list is coded as zeros in rlnm.rnm,rlnm.lnm.           *
*  The mode keywords (OFF, etc.) set the default mode for all fol-     *
*  lowing cell lists.  MXCELLS (unless inside a parenthesized cell     *
*  list) is accepted for compatibility but the value is ignored.       *
*                                                                      *
*  Typically, d3clst() is called when a card interpreter, having       *
*  read, printed, and starting scanning a card, finds a field that     *
*  it does not recognize and calls scanagn().  The syntax is then:     *
*                                                                      *
*     struct CLHDR *d3clst(int clnum, int kmult)                       *
*                                                                      *
*  Where:                                                              *
*     clnum     is an integer label to be assigned to this cell list.  *
*              If a list already exists with this label, it is edited. *
*     kmult    is one of the following values:                         *
*              CLKM_MULTI   (0)     Allow multiple cell types.  Allow  *
*                                   anonymous lists but then more than *
*                                   one on a card is a d3clck error.   *
*                                   (Used with CLIST cards.)           *
*              CLKM_SINGLE  (1)     Restrict to single list.  This is  *
*                                   for parsing 'CELLS=' options where *
*                                   no 'snln' spec is expected.        *
*                                   (Used for CELLS=(list) on PROBE or *
*                                   xxPLOT cards.)                     *
*              CLKM_INDIV   (2)     Multiple lists are processed with  *
*                                   a return after each component.     *
*                                   Entry must have an 'snln' spec.    *
*                                   (Used with SAVE, DETAIL cards.     *
*                                   Anonymous lists are allowed with   *
*                                   SAVE but not with DETAIL, so must  *
*                                   check in d3clck.)                  *
*                                                                      *
*  Value returned:                                                     *
*     A pointer to the CLHDR block for a cell list that was created    *
*     or edited, or a NULL pointer if the specified list does not      *
*     exist or was removed.  RK.highrc is set to larger of its old     *
*     value or 4 if an error occurred or 1 if a list was deleted.      *
*     If no list is created, it is up to the caller to know whether    *
*     an ilstreq() or an RK_IDERR or no error is needed--d3clst()      *
*     does not generate any error message in these cases, except       *
*     when a mode keyword was found and then no list.                  *
*                                                                      *
*  If the list is new, a CLHDR block is added to the linked list       *
*  starting at RP0->pclh1 and a linked list of CLBLK blocks is built   *
*  starting at this header, one for each cell layer that is referenced *
*  in the list.  Lists that are deleted are removed from the chain.    *
*  When a list is linked to a function that requires it to be present  *
*  in shared memory on comp nodes, code at d3clsm changes the pool of  *
*  each CLBLK from Host to Dynamic.  The CLHDRs are not shared--each   *
*  use of a cell list has its own pointer to the chain of CLBLKs, and  *
*  that pointer allows it to be found in shared memory. d3clst() does  *
*  not check that layer names are valid or attempt to cross-link the   *
*  CLBLK blocks with the network tree.  Thus, it may be called from    *
*  d3grp1().  RK.iexit is set non-zero when an error is detected.      *
*                                                                      *
*  An entire layer block may be deleted by entering 'DEL snln()' or    *
*  'snln (OFF)' or 'OFF snln' with no list.  The entire chain may be   *
*  deleted by entering just 'OFF' as the last option on the card.      *
*  In this case, the CLHDR is deleted and a NULL pointer is returned.  *
*                                                                      *
*  In the FORTRAN version, any field that was followed, even after     *
*  intervening blanks, by a left parenthesis was considered to be a    *
*  potential layer identifier, even if it matched a mode keyword.      *
*  However, the C version considers a field to be a layer identifier   *
*  only if it immediately follows a mode keyword or if it is itself    *
*  enclosed in parentheses.  This feature permits a mode keyword to    *
*  be followed, after an intervening comma or blank, by either an      *
*  old-style or a new-style layer identifier.  An unrecognized option  *
*  will result in a scanagn() followed by a return to the caller.      *
*  Thus, cell lists may be embedded within other control card options. *
*                                                                      *
***********************************************************************/

struct CLHDR *d3clst(int clnum, int kmult) {

   /* Local declarations: */

   struct CLHDR **ppclh,*pclh;   /* Ptrs for scanning CLHDR list */
   struct CLBLK **pppcl,*pclb;   /* Ptrs for scanning CLBLK list */
   ilst         *pil;            /* Ptr to an iteration list */
   int   iopd;                   /* Default list mode */
   int   rc;                     /* Return code from match() */
   short oldhighrc;              /* Old value of RK.highrc */
   rlnm  tname;                  /* Temporary rep, layer names */
   char  lea[SCANLEN1];          /* Scan field */

   /* Option keys--order of first four must match order of
   *  corresponding codes in rkilst.h  */
#define NUMOPS 5              /* Number of options */
   static char *opkeys[NUMOPS] =
      { "NEW", "ADD", "DEL", "OFF", "MXCELLS" };

/* Indicate list header is not yet available.  Save existing
*  value of RK.highrc and set to zero so value set by ilstread()
*  can be used to detect intentional list deletion.  Defer search-
*  ing for matching list for deletion until rlnm is available.  */

   pclh = NULL;
   oldhighrc = RK.highrc;
   RK.highrc = 0;

/* Lists must be stored using memory pool management */

   ilstsalf(d3alfpcv, d3alfprv, d3alfree);

/* Scan a field and attempt to interpret */

   iopd = IL_NEW|IL_ELOK|IL_ESOK;   /* Default list mode */
   while ((rc = match(RK_NMERR, RK_MPMDL, RK_ENDCARD,
         0, opkeys, NUMOPS)) != RK_MENDC) {

      /* If in parens, this has to be an rlnm or cell list.  Reset rc
      *  so that if a region name happens to be the same as one of the
      *  recognized keywords, we will treat it as a cell list and not
      *  as one of those keywords.  */
      if (RK.scancode & RK_INPARENS) rc = 0;

      switch (rc) {

      case 0:
         scanagn();        /* break to see what it is */
         break;

      case IL_NEW:         /* NEW   Set default mode to the */
      case IL_ADD:         /* ADD   one given and break out */
      case IL_DEL:         /* DEL   to look for an rlnm.    */
         iopd = rc;
         /* Peek ahead.  To avoid ambiguity, any following field
         *  has to be taken as an rlnm or list, not a main option.
         *  If there is no such field, give an error.  */
         if (scan(lea, 0) & RK_ENDCARD) {
            ilstreq();
            goto Finito;
            }
         scanagn();
         break;

      case IL_OFF:         /* OFF                           */
         iopd = rc;
         /* Peek ahead.  If there are no more fields,
         *  delete the entire chain of lists.
         *  R67, 11/11/16, GNR - Remove test that skipped
         *                 d3clrm in CLKM_INDIV case.  */
         if (scan(lea, 0) & RK_ENDCARD) {
            d3clrm(clnum);
            return NULL;
            }
         scanagn();
         break;

      case 5:              /* MXCELLS--ignore numeric value */
         eqscan(&rc, "VI", RK_EQCHK);
         continue;         /* Get another field */

         } /* End rc switch */

/* Got a possible layer name */

      setrlnm(&tname, NULL, NULL);
      if (kmult != CLKM_SINGLE) {
         d3rlnm(&tname);
         /* If what should have been a layer name is followed by bad
         *  punctuation, d3rlnm() will have swallowed it but may or
         *  may not have issued an error--equals signs are OK there.
         *  So we must issue the error and it is safest to abandon
         *  this card.  If a name was too long, d3rlnm() marks it
         *  and it is safe to keep on scanning.  */
         if (RK.scancode & ~(RK_BLANK|RK_COMMA|RK_INPARENS|RK_RTPAREN|
               RK_ENDCARD|RK_LFTPAREN) || ((RK.scancode & RK_INPARENS)
               && !(RK.scancode & RK_RTPAREN))) {
            ermark(RK_PUNCERR);
            skip2end();
            goto Finito;
            }
         }

/* Find or create a header for this list */

      if (pclh) goto GotHeader;
      /* Assignment intended in test clause of following for loop */
      for (ppclh=&RP0->pclh1; pclh=*ppclh; ppclh=&pclh->pnclh) {
         if ((int)pclh->clnum == clnum) goto GotHeader;
         }
      /* Specified list does not exist, create a header for it */
      pclh = *ppclh = (struct CLHDR *)callocv(1, sizeof(struct CLHDR),
         "Cell List Header");
      pclh->clnum = clnum;
GotHeader:

/* Search for an existing list entry with the same layer name.
*  Maintain a pointer to previous entry, which will be needed
*  if the whole block has to be deleted or a new one added.
*  N.B.  strncmp (see ~/test/pgms/strncmpt.c) yields nonzero when
*  a name is compared to an empty string and zero when two empty
*  strings are compared.  This is OK behavior for anonymous lists.
*  In SINGLE mode, we don't have a layer name, so accept any
*  CLBLK that is there.  */

      pppcl = &pclh->pfclb;
      if (kmult == CLKM_SINGLE)
         pclb = *pppcl;
      else while (pclb = *pppcl) {    /* Assignment intended */
         if (!strncmp(tname.rnm, pclb->clnm.rnm, LSNAME) &&
            !strncmp(tname.lnm, pclb->clnm.lnm, LSNAME))
            break;
         pppcl = &pclb->pnclb;
         }

/* Now ready to build the cell list.  If a matching CLBLK does
*  not exist, pass a NULL ptr to ilstread(), which will generate
*  an error if operation is DEL.  ADD will be treated like NEW.  */

      pil = pclb ? pclb->pclil : NULL;
      RP0->ilseed += 109;  /* Any old number will do */
      pil = ilstread(pil, iopd, 0, RP0->ilseed);

/* Now reconcile cell list and CLBLK combinations */

      if (pil) {           /* There is a list */
         if (!pclb) {         /* Link new CLBLK for this list */
            pclb = *pppcl = (struct CLBLK *)allocpcv(
               Host, 1, IXstr_CLBLK, "Cell list block");
            pppcl = &pclb->pnclb;
            pclb->pclil = pil;
            pclb->clnm = tname;
            ++pclh->nclb;
            }
         }
      else if (pclb) {     /* No list, delete containing block */
         *pppcl = pclb->pnclb;
         freepv(pclb, "Cell List Block");
         pclb = NULL;
         --pclh->nclb;
         }

      /* No additional lists if SINGLE or INDIV calls */
      if (kmult != CLKM_MULTI)
         goto Finito;
      } /* End field scan */

/* If list exists, check for violation of CLKM_SINGLE condition,
*  otherwise, list is empty, so remove its header.  */

Finito:
   if (pclh) {
      if (pclh->pfclb) {
         if (kmult == CLKM_SINGLE && pclh->nclb > 1)
            cryout(RK_E1, "0***CELL LIST HERE MUST NOT REFER "
               "TO MORE THAN ONE CELL TYPE.", RK_LN2, NULL);
         }
      else {
         *ppclh = pclh->pnclh;
         free(pclh);
         pclh = NULL;
         }
      }

   if (oldhighrc > RK.highrc) RK.highrc = oldhighrc;
   return pclh;

   } /* End d3clst() */


/***********************************************************************
*                               CLCKNDX                                *
*                                                                      *
*  This private routine called from d3clck handles clndx checking that *
*  had to be done with separate code before the clndx was incorporated *
*  into the clloc structure.                                           *
***********************************************************************/

/* Non-error values returned by this routine.  WARNING:  Must not equal
*  CLCK_BADLYR or other error to be returned by d3clck() */
#define CKNDX_CONT   0     /* Continue, not a match */
#define CKNDX_SAVE   1     /* Found match, list is OK, save ref */

static int clckndx(struct CELLTYPE *jl, clloc *pclloc,
      struct CLBLK *pclb, int iclb) {

   if (pclloc->clndx) {
      /* User specified index of sublist--per docs, the
      *  celltype name is not required to match.  */
      if (pclloc->clndx != iclb + 1) return CKNDX_CONT;
      else goto CheckThisMatch;
      }
   else if (pclb->clseq == CLBLK_ANON) goto CheckThisMatch;
   else if (jl != pclb->pcll)
      /* We return an error here because caller can print a more
      *  informative message indicating list usage.  */
      return CLCK_BADLYR;
   return CKNDX_SAVE;
CheckThisMatch:
   ilstchk(pclb->pclil, (long)jl->nelt, fmturlnm(jl));
   return CKNDX_SAVE;
   } /* End clckndx */


/***********************************************************************
*                                                                      *
*                               D3CLCK                                 *
*                                                                      *
*  To complete and check a cell list, call d3clck.  The syntax is:     *
*                                                                      *
*     int d3clck(struct CLHDR *pclh, void *pdblk, int kclck)           *
*                                                                      *
*  Where:                                                              *
*     pclh     is a ptr to the head of the cell list.                  *
*     pdblk    is a ptr to a struct containing info to be merged       *
*              with the cell list data.  The type depends on kclck.    *
*     kclck    is the desired action code from this list:              *
*              CLCK_DPRINT    Causes information from a DPDATA block   *
*                             to be merged into all affected CELLTYPEs.*
*                             An anonymous list is illegal here.       *
*                             pdblk is a ptr to the DPDATA.            *
*              CLCK_PROBE     Just do ilstchk on selected clblk.       *
*                             pdblk is a ptr to the PRBDEF blk.        *
*              CLCK_IJPL      Stores a pointer to the selected CLBLK   *
*                             in the IJPLDEF block.                    *
*                             pdblk is a ptr to the IJPLDEF block.     *
*              CLCK_SIMDAT    Store a pointer to the selected CLBLK    *
*                             (may be anon) in the CELLTYPE block.     *
*                             pdblk is a ptr to the CELLTYPE block.    *
*              CLCK_NANAT     Store a pointer to the selected CLBLK    *
*                             (may be anon) in the CELLTYPE block.     *
*                             pdblk is a ptr to the CELLTYPE block.    *
*              CLCK_LPLT      Store a pointer to the selected CLBLK    *
*                             (may be anon) in the CELLTYPE block.     *
*                             pdblk is a ptr to the CELLTYPE block.    *
*                                                                      *
*  Return values:                                                      *
*     0                       All is well                              *
*     CLCK_BADNDX (=1)        User index exceeds list size             *
*     CLCK_BADLYR (=2)        Layer name for CLCK_IJPL does not match  *
*     CLCK_BADPRB (=3)        Cell list number mismatch for probe      *
*                                                                      *
*  d3clck scans the CLBLK list, finds the CELLTYPE block for each      *
*  given 'rnln' name, checks that the largest cell number in the list  *
*  does not exceed the size of the layer, and completes processing of  *
*  LAST and RANDOM list items.                                         *
*                                                                      *
*  Rev, 04/23/11, GNR - Repeat checks every time called                *
***********************************************************************/

int d3clck(struct CLHDR *pclh, void *pdblk, int kclck) {

/* Local declarations: */

   struct CELLTYPE *il,*jl = NULL;
   struct CLBLK  *pclb;
   struct IJPLDEF *pc = NULL;
   int    iclb;                  /* Cell list index */
   int    GotIJPLMatch = FALSE;  /* TRUE when IJPL matched */
   int    GotSIMDMatch = FALSE;  /* TRUE when SIMDAT matched */
   int    GotANATMatch = FALSE;  /* TRUE when NANAT matched */
   int    GotLPLTMatch = FALSE;  /* TRUE when LPLT matched */
   int    rc = 0;                /* Return code */

/* Nothing to do if the cell list is empty */

   if (!pclh) return 0;

/* If this is a list for a probe, as of R67, some CLBLKs may be shared
*  among multiple probes, therefore instead of using the pcll CELLTYPE
*  ptr from the CLBLK, use pprb->psct to locate the CELLTYPE for
*  ilstchk() checking.  Also set the CLFL_SHARED bit so blocks get
*  moved to Dynamic storage pool.
*
*  First do a sanity check to be sure we have the correct cell list
*  and then step out to the specified index number on the chain.  */

   if (kclck & CLCK_PROBE) {
      struct PRBDEF *pprb = (struct PRBDEF *)pdblk;
      int jndx = (int)pprb->prbclloc.clndx;
      if (pclh->clnum != pprb->prbclloc.clnum) {
         char tnum[6];
         wbcdwt(&pprb->ictp, tnum, RK_LFTJ|RK_IORF|RK_NHALF|4);
         cryout(RK_E1,"0***", RK_LN3+4, "INTERNAL ERROR: A WRONG "
            "CELL LIST IS ASSOCIATED WITH PROBE ", RK_CCL,
            tnum, RK.length+1, " FOR ", RK_CCL,
            fmturlnm(pprb->psct), LXRLNM, NULL);
         return CLCK_BADPRB;
         }
      pclb = pclh->pfclb;
      while (--jndx > 0) pclb = pclb->pnclb;
      il = pprb->psct;
      ilstchk(pclb->pclil, (long)il->nelt, fmturlnm(il));
      pclh->clflgs |= (CLFL_USED|CLFL_SHARED);
      return 0;
      }

/* If caller specifies a list index that is larger than the number
*  of lists under this header, this is an error.  */

   if (kclck & CLCK_IJPL) {
      pc = (struct IJPLDEF *)pdblk;
      if (pc->cmclloc.clndx > pclh->nclb) return CLCK_BADNDX;
      }
   jl = (struct CELLTYPE *)pdblk;
   if (kclck & CLCK_SIMDAT) {
      if (jl->ctclloc[CTCL_SIMDAT].clndx > pclh->nclb)
         return CLCK_BADNDX;
      }
   else if (kclck & CLCK_NANAT) {
      if (jl->ctclloc[CTCL_NANAT].clndx > pclh->nclb)
         return CLCK_BADNDX;
      }
   else if (kclck & CLCK_LPLT) {
      if (jl->ctclloc[CTCL_LPLT].clndx > pclh->nclb)
         return CLCK_BADNDX;
      }

/* Checking for DPRINT, IJPL, etc. -- not for a probe.
*  Scan list of CLBLKs associated with this CLIST */

   for (pclb=pclh->pfclb,iclb=0; pclb; pclb=pclb->pnclb,++iclb) {

/* Match layer name to repertoire tree.  Pick up sequence number
*  (not used).  This checking is done even if only only one CLBLK
*  in the list will actually be used (CLIST=(id,ndx) construct).  */

      if (pclb->pcll)         /* Name already matched earlier */
         il = pclb->pcll;
      else if (pclb->clnm.rnm[0] == '\0')   /* Anonymous list */
         il = NULL, pclb->clseq = CLBLK_ANON;
      else {
         il = findln(&pclb->clnm);
         if (il) {   /* Name is matched, store ptr to cell type */
            pclb->pcll = il, pclb->clseq = il->seqvmo;
            }
         else {      /* Name not found, generate error message */
            char tnum[6];
            wbcdwt(&pclh->clnum, tnum, RK_LFTJ|RK_IORF|RK_NHALF|4);
            cryout(RK_E1,"0***", RK_LN2+4, fmtrlnm(&pclb->clnm),
               LXRLNM, " NOT FOUND FOR CELL LIST ", RK_CCL,
               tnum, RK.length+1, NULL);
            rc = CLCK_BADLYR;
            continue;
            } /* End layer name not matched */
         } /* End checking layer name */

/* Check that highest numbered cell does not exceed layer size.
*  This code assumes the list exists and is non-empty, because
*  otherwise d3clst() would have unlinked and freed the entry.
*  N.B.  This check is done even if the list is not used.  */

      if (il) ilstchk(pclb->pclil, (long)il->nelt, fmturlnm(il));

/* Merge DETAIL PRINT information into CELLTYPE block (unless
*  trumped by an explicit list already in place).  Force Lij
*  to be printed in first cycle of next series, if requested,
*  in case cell list was modified since last time Lij printed.  */

      if (kclck & CLCK_DPRINT) {
         struct DPDATA *pdpd = (struct DPDATA *)pdblk;
         if (pdpd->dpclloc.clnum < 0 ||
               il->ctclloc[CTCL_DPRINT].clnum >= 0) {
            il->dpclb = pclb;          /* Iteration list */
            il->ctclloc[CTCL_DPRINT] = pdpd->dpclloc;
            il->dpitem &= ~DPDIDL;
            if (pdpd->jdpt > 0)        /* Timer number */
               il->jdptim = pdpd->jdpt;
            if (pdpd->mckpm >= 0) {    /* Item codes */
               RK.mcbits = pdpd->dpitem;
               RK.mckpm  = pdpd->mckpm;
               mcodoph(&il->dpitem);
               }
            if (pdpd->dpdis >= 0) {    /* Disable/enable */
               if (pdpd->dpdis) il->ctf |= CTDPDIS;
               else             il->ctf &= ~CTDPDIS;
               }
            pclh->clflgs |= (CLFL_USED|CLFL_SHARED);
            }
         } /* End CLCK_DPRINT */

/* If this is a list for a connection data plot, either (1) the
*  CLHDR chain must have just a single cell list (checked in
*  d3ijpi because we are in a CLBLK loop here), or else (2) a
*  clndx must have been entered to select one of them.  In
*  this case, the highest cell number must be within range of
*  both the cell type specified in the cell list (checked above)
*  and the one specified in the IJPLDEF (checked here).
*  R72, 02/23/17, GNR - Added check of celltype when no clndx,
*                       allow anonymous lists for IJPL calls.  */

      if (kclck & CLCK_IJPL && !GotIJPLMatch) {
         int trc = clckndx(pc->pcmtl, &pc->cmclloc, pclb, iclb);
         if (trc == CKNDX_CONT) continue;
         if (trc != CKNDX_SAVE) { rc = trc; continue; }
         pc->pcmcl = pclb;
         pclh->clflgs |= (CLFL_USED|CLFL_SHARED);
         GotIJPLMatch = TRUE;
         }

/* The situation is similar for a SIMDATA cell list for a cell type */

      if (kclck & CLCK_SIMDAT && !GotSIMDMatch) {
         int trc = clckndx(jl, jl->ctclloc + CTCL_SIMDAT, pclb, iclb);
         if (trc == CKNDX_CONT) continue;
         if (trc != CKNDX_SAVE) { rc = trc; continue; }
         jl->psdclb = pclb;
         jl->svitms |= GFSI;
         pclh->clflgs |= (CLFL_USED|CLFL_SHARED);
         GotSIMDMatch = TRUE;
         }

/* And again for a NANAT cell list for a cell type */

      if (kclck & CLCK_NANAT && !GotANATMatch) {
         int trc = clckndx(jl, jl->ctclloc + CTCL_NANAT, pclb, iclb);
         if (trc == CKNDX_CONT) continue;
         if (trc != CKNDX_SAVE) { rc = trc; continue; }
         jl->ptclb = pclb;
         pclh->clflgs |= (CLFL_USED|CLFL_SHARED);
         GotANATMatch = TRUE;
         }

/* And again for a LPLT (Layer plot) cell list for a cell type */

      if (kclck & CLCK_LPLT && !GotLPLTMatch) {
         int trc = clckndx(jl, jl->ctclloc + CTCL_LPLT, pclb, iclb);
         if (trc == CKNDX_CONT) continue;
         if (trc != CKNDX_SAVE) { rc = trc; continue; }
         jl->plpclb = pclb;
         pclh->clflgs |= (CLFL_USED|CLFL_SHARED);
         GotLPLTMatch = TRUE;
         }

      } /* End loop over CLBLKs */

/* Mark the list as having been processed */

   return 0;

   } /* End d3clck() */


#ifdef PAR0
/***********************************************************************
*                                                                      *
*                               D3CLSM                                 *
*                                                                      *
*  Assign cell lists to Host or Dynamic memory as appropriate.         *
*  (This routine is only needed on PAR0 nodes.)                        *
*                                                                      *
***********************************************************************/

void d3clsm(void) {

   struct CLHDR *pclh;
   struct CLBLK *pclb;

   for (pclh=RP0->pclh1; pclh; pclh=pclh->pnclh) {
      memtype kmem = (pclh->clflgs & CLFL_SHARED) ? Dynamic : Host;
      for (pclb=pclh->pfclb; pclb; pclb=pclb->pnclb) {
         chngpool(pclb, kmem);
         chngpool(pclb->pclil, kmem);
         chngpool(pclb->pclil->pidl, kmem);
         } /* End loop over CLBLKs */
      } /* End loop over CLHDRs */

   } /* End d3clsm() */
#endif

#endif   /* Not PARn */

