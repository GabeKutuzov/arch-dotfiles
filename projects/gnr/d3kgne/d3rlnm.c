/* (c) Copyright 1991-2010 Neurosciences Research Foundation, Inc. */
/* $Id: d3rlnm.c 29 2010-06-15 22:02:00Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3rlnm.c                                *
*                                                                      *
*       d3rlnm, readrlnm, fmtrlnm, setrlnm                             *
*       Host routines to read and format repertoire and layer names    *
*                                                                      *
************************************************************************
*  V8A, 10/06/96, GNR - Include readrlnm() function from d3tree        *
*  Rev, 02/10/97, GNR - Add fmtlrlnm and fmturlnm functions            *
*  Rev, 09/22/98, GNR - Change args to rlnm type, add fmtrlnm, setrlnm *
*  V8D, 09/04/06, GNR - Move fmtlrlnm and fmturlnm to d3exit for nodes *
*  V8D, 02/16/07, GNR - Add hbcnm field to rlnm for camera & BBD names *
*  ==>, 04/30/07, GNR - Last mod before committing to svn repository   *
*  V8F, 04/20/10, GNR - Add PREPROC input type                         *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#ifdef BBDS
#include "bbd.h"
#endif
#include "rocks.h"
#include "rkxtra.h"

/*---------------------------------------------------------------------*
*                               d3rlnm                                 *
*                                                                      *
*     Routine to read repertoire name and layer name using either      *
*        the old-style concatenated name or the new style setting      *
*        in which two strings of length <= LSNAME are separated by     *
*        a comma (or blank) and enclosed in parentheses.               *
*     This routine is only used where an actual layer name (not a      *
*        camera or BBD device name) is expected, so it fills in        *
*        the hbcnm with -1 as a signal.                                *
*                                                                      *
*     Synopsis:                                                        *
*        void d3rlnm(rlnm *rlname)                                     *
*                                                                      *
*     Argument:                                                        *
*        rlname      Ptr to rlnm struct where repertoire and layer     *
*                    names are to be stored.  Names should be          *
*                    initialized with suitable defaults before call    *
*                    is made.                                          *
*                                                                      *
*     INPUT SYNTAX         RESULT                                      *
*        i) *              Use default names. rlname is not touched.   *
*                                                                      *
*       ii) <2 chars>      The characters are taken to be the layer    *
*                             name, and are loaded into rlname->lnm.   *
*                             rlname->rnm is not touched.              *
*                                                                      *
*      iii) <4 chars>      The first 2 chars are taken to be the       *
*                             repertoire name and are loaded into      *
*                             rlname->rnm. The second 2 chars are      *
*                             taken to be the layer name and are       *
*                             loaded into rlname->lnm.                 *
*                                                                      *
*       iv) (rname,lname)  The first field is loaded into rlname->rnm  *
*                             and the second into rlname->lnm.  The    *
*                             fields may be separated by a comma or    *
*                             blanks.  Either field may be an asterisk *
*                             ('*'), in which case the corresponding   *
*                             argument is left untouched.              *
*                                                                      *
*     Any of the first three formats above may or may not be enclosed  *
*        in parentheses.  If not in parentheses, old-style is always   *
*        implied.  If in parentheses, old-style is implied if there is *
*        but a single field (first field encountered is delimited by   *
*        right paren), new-style is implied if the first field is not  *
*        delimited by a right paren.  (This proviso is made to handle  *
*        the parameter qualifiers on the CHANGE card.)                 *
*                                                                      *
*---------------------------------------------------------------------*/

void d3rlnm(rlnm *rlname) {

   int old_maxlen;            /* Holding spot for maximum field length
                              *  at the time d3rlnm was called. */
   char lea[LSNAME];          /* Temp for names as they are scanned */

   old_maxlen = scanlen(LSNAME);    /* Set new maximum field
                                    *  length and save old one. */
   rlname->hbcnm = -1;        /* Flags it as a non-numeric value */
   scan(lea, RK_REQFLD+RK_FENCE);
   if ((RK.scancode & (RK_INPARENS|RK_RTPAREN)) == RK_INPARENS) {
      /* If we're in parentheses, and we don't have a right paren,
      *  then we have new style assignment */
      if (RK.scancode & ~(RK_COMMA|RK_BLANK|RK_INPARENS))
         ermark(RK_PUNCERR);
      if (strcmp(lea, "*")) strncpy(rlname->rnm, lea, LSNAME);

      /* Now pick up second field (there had better be one) and
      *  check punctuation. */
      scan(lea, RK_REQFLD+RK_FENCE);
      if ((RK.scancode & (RK_INPARENS|RK_RTPAREN)) ==
            RK_INPARENS|RK_RTPAREN) {
         if (strcmp(lea, "*")) strncpy(rlname->lnm, lea, LSNAME);
         }
      else
         ermark(RK_PUNCERR);
      } /* End new style assignment */

/* Old style assignment */

   else {
      /* If we are in parens, we MUST have right paren.  Any
      *  punctuation after the right paren is OK--should be
      *  checked by caller.  */
      if ((RK.scancode & (RK_INPARENS|RK_RTPAREN)) == RK_INPARENS)
         ermark(RK_PUNCERR);
      else if (strcmp(lea, "*")) {
         /* Legal punctuation and it's not a "*":  Check length
         *  using old value (4), which may not always equal LSNAME */
         if (RK.length >= 4) ermark(RK_LENGERR);
         else { /* Name is not too long for old style */
            if (RK.length >= 2) {
               /* Both names are supplied */
               strncpy(rlname->rnm, lea, 2);
               rlname->rnm[2] = '\0';
               strncpy(rlname->lnm, lea+2, 2);
               }
            else {
               /* Here if only layer name is supplied */
               strncpy(rlname->lnm, lea, 2);
               }
            rlname->lnm[2] = '\0';
            } /* End name is not too long */
         } /* End not an asterisk */
      } /* End old style */

   scanlen(old_maxlen);   /* Restore max field length */

   }  /* End d3rlnm() */

/*---------------------------------------------------------------------*
*                              readrlnm                                *
*  This little function initiates scanning and reads the source region *
*  and layer names for the MODULATE, NOISE MODULATE, CONNTYPE, and now *
*  also GCONN cards using the conventions defined for those cards. The *
*  getsnsid() type is returned as the function value.  An asterisk in  *
*  either field indicates self-reference.  It is replaced by the name  *
*  of the current region or layer and a REPSRC type is assumed.  BBD   *
*  devices and cameras may have names longer than LSNAME.  In those    *
*  cases, the first LSNAME chars go in the lnm field (could still be   *
*  used for error messages from comp nodes) and a getrktxt handle to   *
*  the full name goes in the hbcnm field.  Otherwise, either a '-' or  *
*  a positive integer is required.  The value of the integer is stored *
*  in hbcnm, or '-1' if just a '-' is coded.  This latter is a signal  *
*  for the caller to insert the appropriate arm, window, or D1 index   *
*  (this cannot be done here because the case IA with kgen=S can not   *
*  be detected until later).  Anything else is an error.  inform()     *
*  checks for missing fields, bad lengths, bad punctuation.            *
*  Rev, 04/29/07, GNR - Names in parens are forced to be layer names   *
*---------------------------------------------------------------------*/

int readrlnm(rlnm *srcnm, struct CELLTYPE *il) {

   int ityp;                     /* Input type */
   int plev1;                    /* Paren level of first field */

   /* Read the first field (region/repertoire/protocol name) */
   cdscan(RK.last, 1, SCANLEN, RK_WDSKIP);
   scanlen(LSNAME);
   scanck(srcnm->rnm, RK_REQFLD|RK_FENCE,
      ~(RK_BLANK|RK_COMMA|RK_INPARENS));
   plev1 = RK.plevel;

   /* Determine the input type */
   ityp = (RK.scancode & RK_INPARENS) ? REPSRC : getsnsid(srcnm->rnm);

   /* Type-dependent interpretations */
   switch (ityp) {
   case REPSRC:
      scanck(srcnm->lnm, RK_REQFLD|RK_FENCE,
         ~(RK_BLANK|RK_COMMA|RK_INPARENS|RK_RTPAREN));
      if (RK.plevel != plev1 ||
            RK.scancode & (RK_INPARENS|RK_RTPAREN) == RK_INPARENS)
         ermark(RK_PUNCERR);
      srcnm->hbcnm = 0;
      /* Handle self-reference indicated by asterisk */
      if (!strcmp(srcnm->rnm, "*"))
         strncpy(srcnm->rnm, il->pback->sname, LSNAME);
      if (!strcmp(srcnm->lnm, "*"))
         strncpy(srcnm->lnm, il->lname, LSNAME);
      break;
   case TV_SRC:
   case PP_SRC:
   case USRSRC:
      scanlen(SCANLEN);
      inform("(S,TH" qLTF ")", &srcnm->hbcnm, NULL);
      strncpy(srcnm->lnm, getrktxt(srcnm->hbcnm), LSNAME);
      break;
   default:
      scanck(srcnm->lnm, RK_REQFLD|RK_FENCE, ~(RK_BLANK|RK_COMMA));
      if (!strcmp(srcnm->lnm, "-"))
         srcnm->hbcnm = -1;
      else
         srcnm->hbcnm = (si16)ibcdin(RK_SNGL|VSBIC, srcnm->lnm);
      } /* End type switch */

   scanlen(SCANLEN);
   return ityp;
   } /* End readrlnm() */

/*---------------------------------------------------------------------*
*                               setrlnm                                *
*  This little function is used to set initial values into an rlnm.    *
*  The idea is to hide the details in case of some future changes...   *
*  The arguments are: (1) ptr to the rlnm, (2) ptr to rname part,      *
*  (3) ptr to lname part.  Nothing is returned.                        *
*---------------------------------------------------------------------*/

void setrlnm(rlnm *rlname, char *rname, char *lname) {

   rlname->hbcnm = 0;
   if (rname)  strncpy(rlname->rnm, rname, LSNAME);
   else        memset (rlname->rnm, 0,     LSNAME);
   if (lname)  strncpy(rlname->lnm, lname, LSNAME);
   else        memset (rlname->lnm, 0,     LSNAME);

   } /* End setrlnm() */

/*---------------------------------------------------------------------*
*                               fmtrlnm                                *
*  This little function formats repertoire and layer names for inclu-  *
*  sion in a warning message or an error message.  This version is for *
*  use with input data before CELLTYPE and REPBLOCK have been set up.  *
*  Routines fmtlrlnm and fmturlnm that use those data structs and are  *
*  available on all node types are now located in d3exit.c.            *
*  The action is very simple, but is always done in this one place so  *
*  all messages will be formatted consistently.                        *
*  fmtrlnm returns, for an rlnm rlname:                                *
*     Ptr to uppercase formatted string "CELLTYPE (rnm,lnm)".          *
*  Result string is in static storage, and remains valid until next    *
*     call to any of these routines.                                   *
*---------------------------------------------------------------------*/

extern char fmtrlmsg[14+2*LSNAME];

char *fmtrlnm(rlnm *rlname) {

   sconvrt(fmtrlmsg,"(10HCELLTYPE (J0A" qLSN ",H,J0A" qLSN ",H))",
      rlname->rnm, rlname->lnm, NULL);
   return fmtrlmsg;
   } /* End fmtlrlnm() */
