/* (c) Copyright 1993-2010, Neurosciences Research Foundation, Inc. */
/* $Id: findvc.c 29 2010-06-15 22:02:00Z  $ */
/***********************************************************************
*                             CNS Program                              *
*        dupmsg, getsnsid, getcsel, getcsel3, getcshm, findkn,         *
*              findpp, findtv, findvc, fmtvcn, fmtcsel,                *
*                                                                      *
*  This file contains functions used to identify camera and sense      *
*  names, to identify a COLOR option used with image or IA input, to   *
*  find the KERNEL, PREPROC, TVDEF, or VCELL block associated with a   *
*  given name, or to format an identification name for a sense error.  *
*                                                                      *
*  Handling of camera and BBD senses:  The vcname will be VVTV, BBD,   *
*  or UPGM to identify the protocol.  (Others can be added later.)     *
*  The hvname will be the user-assigned name.  In these cases, findvc, *
*  and always findtv, will search the lname.  CONNTYPE, etc. cards     *
*  will give the protocol name followed by the user name in the %srcid *
*  and %type fields, bzw., and multiple uses of the same name will not *
*  be allowed.                                                         *
************************************************************************
*  V6A, 07/09/93, GNR - Newly written                                  *
*  V7A, 04/29/94, GNR - First arg is name so can find user-defined sns *
*  V8A, 04/15/95, GNR - Delete 'vsrclid' arg, document d3grp1 use,     *
*                       move getsnsid function here from d3tree        *
*  Rev, 08/21/96, GNR - Modify for use with Group III SENSE card       *
*  V8B, 12/09/00, GNR - Move to new memory management routines         *
*  V8D, 02/12/07, GNR - Add findtv(), BBDS senses, getcsel, getcshm    *
*  Rev, 05/01/07. GNR - Add fmtvcn()                                   *
*  ==>, 02/05/08, GNR - Last mod before committing to svn repository   *
*  Rev, 05/30/08, GNR - Add color opponency color selectors            *
*  V8F, 02/13/10, GNR - Modify for UPGM cameras, PREPROC inputs        *
***********************************************************************/

#define TVTYPE struct TVDEF
#define KNTYPE struct KERNEL
#define PPTYPE struct PREPROC

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"
#include "tvdef.h"
#ifdef BBDS
#include "bbd.h"
#endif
#include "rocks.h"
#include "rkxtra.h"

#ifdef PARn
#error This routine is not used on comp nodes
#else

/*---------------------------------------------------------------------*
*                               dupmsg                                 *
*                                                                      *
*  This routine prints a message in a standard format when a name      *
*  entered by the user is found to duplicate a name already entered.   *
*  An error flag is set in RK.iexit.                                   *
*                                                                      *
*  Call by: dupmsg(char *typnm, char *badnm, txtid hnm)                *
*                                                                      *
*  Arguments:                                                          *
*     'typnm' is the name of the type of item that is duplicated.      *
*     'badnm' is a pointer to the offending name or NULL if the        *
*        following 'hnm' should be used instead.                       *
*     'hnm' is a getrktxt handle to the offending name if 'badnm'      *
*        is NULL, otherwise it is the maximum length of the name       *
*        (which may be an unterminated LNAME string).                  *
*                                                                      *
*---------------------------------------------------------------------*/

void dupmsg(char *typnm, char *badnm, txtid hnm) {

#define MAX_TYPNAM 40         /* An arbitrary jic cutoff */
   int mxlnm;
   if (badnm)
      mxlnm = (int)hnm;
   else
      badnm = getrktxt(hnm), mxlnm = strlen(badnm);
   cryout(RK_E1, "0***", RK_LN3, typnm, RK_CCL+MAX_TYPNAM,
      " NAME \"", RK_CCL+7, badnm, RK_CCL+mxlnm, "\"", 1,
      " DUPLICATES A NAME ALREAD ENTERED", RK_LN0, NULL);

   } /* End dupmsg() */


/*---------------------------------------------------------------------*
*                     getcsel, getcsel3, fmtcsel                       *
*                                                                      *
*  getcsel() matches an input color designator from a COLOR= option    *
*  on a CONNTYPE, IMGTOUCH, MODULATE, or NMOD card and stores a code   *
*  (ColorSens enum) in the argument EXTRCOLR struct that can be used   *
*  by getcshm() to set up the shift and mask needed to extract the     *
*  color information from the IA or image data array.  If the name     *
*  is not matched, an error is given.  The argument is a pointer to    *
*  the result, making getcsel suitable for use as a kwsreg routine.    *
*                                                                      *
*  getcsel3() is the same but also accepts the "(color, 3)" construct  *
*  indicating that the input is to cycle among the three colors        *
*  starting at the designated color or color opponent name.            *
*                                                                      *
*  (These routines are distinct so they can be distinguished in        *
*  kwscan() %N calls rather than using a switch in the call.)          *
*                                                                      *
*  fmtcsel() performs the opposite function, returning a pointer to    *
*  the color selector name given the code.                             *
*---------------------------------------------------------------------*/

/* Names of color identifiers for visual input conntypes.
*  Order must be same as ColorSens enum in d3global.h.  */
static char *colnms[NColorSels] = { "BW", "BLUE", "GREEN", "RED",
   "R-G", "B-R", "G-B", "G-R", "R-B", "B-G" };
static char *col3c[1] = { "3" };

void getcsel(struct EXTRCOLR *pxtc) {

   int rc = match(RK_EQCHK, RK_MREQF, ~RK_COMMA, 0,
      colnms, NColorSels);
   if (rc > 0) rc -= 1;
   pxtc->tvcsel = (byte)rc;

   } /* End getcsel() */

void getcsel3(struct EXTRCOLR *pxtc) {

   int rc = match(RK_EQCHK, RK_MREQF, ~(RK_COMMA|RK_INPARENS), 0,
      colnms, NColorSels);
   if (rc > 0) rc -= 1;
   if (RK.scancode & RK_INPARENS && match(0, RK_MREQF,
         ~RK_COMMA, (RK_INPARENS|RK_RTPAREN), col3c, 1)) rc |= EXTR3C;
   if (rc == (EXTR3C|BandW)) {
      ermark(RK_MARKFLD);
      cryout(RK_E1, "0***3 OPTION MEANINGLESS WITH NO COLOR",
         RK_LN2, NULL);
      }
   pxtc->tvcsel = (byte)rc;

   } /* End getcsel3() */

char *fmtcsel(int csel) {

   csel &= ~EXTR3C;
   if (csel < 0 || csel >= NColorSels)
      d3exit(COLOR_ERR, NULL, csel);
   return colnms[csel];

   } /* End fmtcsel() */


/*---------------------------------------------------------------------*
*                               getcshm                                *
*                                                                      *
*  This routine analyzes the color requested by a user and the nature  *
*  of the data source and returns a shift and mask as described below. *
*                                                                      *
*  Argument:                                                           *
*     pxtc     Ptr to an EXTRCOLR structure that has the user color    *
*              selector and source color mode and which receives the   *
*              output shift and mask variables.                        *
*     emsg     Ptr to an indentifying string that can be inserted in   *
*              any error message.                                      *
*                                                                      *
*  Return values (in EXTRCOLR struct pointed to by pxtc):              *
*  Col_GS mode:   All zeros.  An error is generated if a color value   *
*              is requested.                                           *
*  Any color opponency ColorSens:  tvshft is excit color, tvmask is    *
*              inhib color.  Otherwise, for single colors:             *
*  Col_C24 mode:  tvshft is the index needed to pick up the tvcsel     *
*              color from the image data.  tvmask is zero.             *
*  Col_C16 mode:  tvshft is the shift needed to shift data of the      *
*              tvcsel color to S15 position.  tvmask is zero.          *
*  Col_C8 mode:   tvshft is the shift needed to shift data of the      *
*              selected color to S14 position and tvmask is the mask   *
*              needed to isolate the selected color before shifting.   *
*  Col_R4 mode:  tvshft and tvmask not used, both set to 0.            *
*  If the EXTR3C bit is set, the above values are stored for each of   *
*  the three possible colors in the single or opponency set, otherwise *
*  the same selections are stored in all three tvshft,tvmask locations.*
*---------------------------------------------------------------------*/

void getcshm(struct EXTRCOLR *pxtc, char *emsg) {

   int ktvc = pxtc->tvcsel;
   int mode = pxtc->tvmode;
   enum ColorSens ektvc = (enum ColorSens)ktvc;
   enum ColorMode emode = (enum ColorMode)mode;

   /* Values for pxtc->tvshft arrayed by mode and selector.  */
   static const byte xtshft[4][4] = {
      { 0,  0, 0,  0 },                /* Mode =  8-bit Gray Scale  */
      { 0,  6, 8, 11 },                /* Mode =  8-bit (2-3-3 BGR) */
      { 0, 10, 5,  0 },                /* Mode = 16-bit (5-5-5 RGB) */
      { 0,  2, 1,  0 } };              /* Mode = 24-bit (8-8-8 RGB) */
   /* Values for color mask (applied before shift) for Col_GS data */
   static const byte xtmask[4] = { 0, 0xc0, 0x38, 0x07 };
   /* Values for excit color for opponency ColorSens choices */
   static const byte xtcexc[6] = { 0, 2, 1, 1, 0, 2 };
   /* Values for inhib color for opponency ColorSens choices */
   static const byte xtcinh[6] = { 1, 0, 2, 0, 2, 1 };

   if ((emode == Col_GS || emode == Col_R4) && ektvc > BandW)
      cryout(RK_E1, "0***CANNOT GET COLOR FROM A B&W SOURCE FOR ",
         RK_LN2, emsg, RK_CCL, NULL);
   else if (emode != Col_R4) {
      int i,iinc,limc;
      /* Set color increment to cycle or not cycle */
      if (ktvc & EXTR3C) iinc = 1, ktvc &= ~EXTR3C;
      else               iinc = 0;
      limc = NColorDims*((ktvc - 1)/NColorDims);

      for (i=0; i<NColorDims; ++i) {
         if ((enum ColorSens)ktvc >= RmG) {
            pxtc->tvshft[i] = xtcexc[ktvc - RmG];
            pxtc->tvmask[i] = xtcinh[ktvc - RmG];
            }
         else {
            pxtc->tvshft[i] = xtshft[mode][ktvc];
            if (emode == Col_C8)
               pxtc->tvmask[i] = xtmask[ktvc];
            }
         if ((ktvc -= iinc) <= limc) ktvc += NColorDims;
         } /* End loop over 1 or 3 colors */
      } /* End else */

   } /* End getcshm() */


/*---------------------------------------------------------------------*
*                              getsnsid                                *
*                                                                      *
*  This little function matches a source identifier for a CONNTYPE,    *
*  GCONN, MODULATE, or NMOD input against all possible built-in and    *
*  user-defined senses, and returns a "source flag" code or REPSRC     *
*  (=0) if the name is not recognized.  It is also used by REPERTOIRE  *
*  card processing to detect illegal use of a predefined name and by   *
*  vcdflt to identify the type of VCELL block to be initialized.       *
*  Note that built-in senses are matched whether or not a corres-      *
*  ponding SENSE card was entered, because that is how we know we      *
*  must create the VCELLs.                                             *
*---------------------------------------------------------------------*/

int getsnsid(char *srcrid) {

/* Reserved connection source names.
*  Note: must be consistent with XX_SRC constants (see "d3global.h").
*  '==' is used for slots that don't exist in current compilation--
*     this cannot match anything scan can return.  Also note that
*  '*' indicating self-reference will not match and will return 0,
*  indicating REPSRC.  */
   static char *bltsnm[NRST-1] = { "IA", "V", "VJ", "VW", "VT",
      "TV", "PP",
#ifdef D1
      "D1",
#else
      "==",
#endif
      "VH",
      "VITP",
      };

   int ivcid;              /* bltsnm index */

   /* First check against built-in names */
   ivcid = smatch(RK_NMERR,srcrid,bltsnm,NRST-1);
   if (ivcid > 0) return ivcid;

#ifdef BBDS
   /* If we are a BBD server, check that name */
   if (ssmatch(srcrid, "BBD", 3)) return USRSRC;
#endif
   /* Add other protocols here as needed */

   return 0;               /* Signals absence of match */
   } /* End getsnsid */


/*---------------------------------------------------------------------*
*                               findkn                                 *
*                                                                      *
*  Finds the KERNEL struct corresponding to a given kernel name.       *
*                                                                      *
*  Call by: findkn(txtid hker, char *erstring)                         *
*                                                                      *
*  Arguments:                                                          *
*     'hker' is a getrktxt handle to the name of the desired kernel.   *
*     'erstring' is a string to be appended to an error message when   *
*        the kernel is not found.  If NULL, not-found is not an error. *
*                                                                      *
*  Returns:                                                            *
*     A pointer to the KERNEL block of the desired kernel, or NULL     *
*        if the kernel is not found.                                   *
*---------------------------------------------------------------------*/

struct KERNEL *findkn(txtid hker, char *erstring) {

   struct KERNEL *pker;

   for (pker=RP0->pfkn; pker; pker=pker->pnkn) {
      if (hker == pker->hknm) return pker;
      }
   if (erstring) cryout(RK_E1, "0***REQUESTED KERNEL \"", RK_LN2,
      getrktxt(hker), RK_CCL, "\" NOT FOUND", RK_CCL,
      erstring, RK_CCL, NULL);
   return NULL;
   } /* End findkn() */


/*---------------------------------------------------------------------*
*                               findpp                                 *
*                                                                      *
*  Finds PREPROC struct corresponding to a given preprocessor name.    *
*                                                                      *
*  Call by: findpp(txtid hip, char *erstring)                          *
*                                                                      *
*  Arguments:                                                          *
*     'hip' is a getrktxt handle to the name of the desired            *
*        image preprocessor.                                           *
*     'erstring' is a string to be appended to an error message when   *
*        the preprocessor is not found.  If NULL, not-found is not an  *
*        error.                                                        *
*                                                                      *
*  Returns:                                                            *
*     A pointer to the PREPROC block of the desired preprocessor,      *
*        or NULL if the kernel is not found.                           *
*---------------------------------------------------------------------*/

struct PREPROC *findpp(txtid hip, char *erstring) {

   struct PREPROC *pip;

   for (pip=RP->pfip; pip; pip=pip->pnip) {
      if (hip == pip->hipnm) return pip;
      }
   if (erstring) cryout(RK_E1, "0***REQUESTED PREPROCESSOR\"", RK_LN2,
      getrktxt(hip), RK_CCL, "\" NOT FOUND", RK_CCL,
      erstring, RK_CCL, NULL);
   return NULL;

   } /* End findpp() */


/*---------------------------------------------------------------------*
*                               findtv                                 *
*                                                                      *
*  Unlike findvc(), findtv() only searches for an existing camera,     *
*  it cannot create one without a user control card.                   *
*                                                                      *
*  Call by: findtv(txtid hcam, char *erstring)                         *
*                                                                      *
*  Arguments:                                                          *
*     'hcam' is a getrktxt handle to the name of the desired camera.   *
*     'erstring' is a string to be appended to an error message when   *
*        the camera is not found.  If NULL, not-found is not an error. *
*                                                                      *
*  Returns:                                                            *
*     A pointer to the TVDEF block of the desired camera, or NULL      *
*        if the camera is not found.                                   *
*---------------------------------------------------------------------*/

struct TVDEF *findtv(txtid hcam, char *erstring) {

   struct TVDEF *cam;

   for (cam=RP->pftv; cam; cam=cam->pnxtv) {
      if (hcam == cam->hcamnm) return cam;
      }
   if (erstring) cryout(RK_E1, "0***REQUESTED CAMERA \"", RK_LN2,
      getrktxt(hcam), RK_CCL, "\" NOT FOUND", RK_CCL,
      erstring, RK_CCL, NULL);
   return NULL;

   } /* End findtv() */


/*---------------------------------------------------------------------*
*                               findvc                                 *
*                                                                      *
*  Find a virtual cell block in VCELL list, create one, or give error  *
*  message if not there.  Called from d3grp1 when reading SENSE cards, *
*  from g1arm or g1wdw to create speculative VCELL blocks for touch    *
*  and kinesthesia, from d3tchk while matching sources for normal,     *
*  modulatory, or noise modulation connection types, and from d3grp3   *
*  to find VCELL blocks for SENSE card updates.  'findvc' is different *
*  from the 'getsnsid' routine above, which checks against built-in    *
*  sense *NAMES*, not instances of such names.                         *
*                                                                      *
*                                                                      *
*  Call by: findvc(char *vsrcrid, int ivcid, int ivcndx,               *
*        char *erstring)                                               *
*                                                                      *
*  Arguments:                                                          *
*  'vsrcrid' is the short name for the virtual source.  It can be one  *
*     of the values defined in d3global.h.  If the sense is a user-    *
*     defined sense, it must be the name of the protocol used to get   *
*     values from that sense.  Allowed values will depend on compile-  *
*     time definitions.  The only current possibility is "BBD".        *
*  'ivcid' is the srctype code corresponding to 'vsrcrid'.  This is    *
*     already known from a getsnsid call, so passing it saves us from  *
*     having to search again.  It is used to distinguish built-in      *
*     from user-defined source names.  If its value is 0, non-match    *
*     is an error (d3grp3 call).                                       *
*  'ivcndx' for built-in senses is the numeric value extracted from    *
*     or implied by the literal 'type' field from the CONNTYPE, MOD,   *
*     or NMOD card.  It counts sense blocks of the specified type      *
*     from 1.  By convention, this number is incremented by g2conn     *
*     each time a '-' code is entered for VJ,VT,VW.  For user-         *
*     defined senses, this is the value returned by a findtxt() call   *
*     against the sense name.  It will be matched against the hvname   *
*     field in the VCELL block, allowing the specific sense name to    *
*     be matched.  User senses are created in d3grp1(), not here.      *
*  'erstring' is text for body of error message identifying target     *
*     for which match was sought.                                      *
*                                                                      *
*  Return:                                                             *
*     The value returned is a pointer to the specified vcell block.    *
*     If it does not exist, it is created--but only if ivcid indi-     *
*     cates a predefined type and ivcndx is just one greater than      *
*     the last existing block.  Note that if the requested name is     *
*     not the name of a sense, then ivcid will indicate REPSRC and     *
*     findvc will generate an error.                                   *
*---------------------------------------------------------------------*/

struct VCELL *findvc(char *vsrcrid, int ivcid, int ivcndx,
      char *erstring) {

   struct VCELL *jvc;         /* Scan pointer */
   int jvctyp = ivcndx - 1;   /* Virtual cell type number, from 0 */

#ifdef BBDS
/* Search for user-defined senses */
   if (ivcid == USRSRC) {
      for (jvc=RP0->pbbds1; jvc; jvc=jvc->pnvc) {
         if (ivcndx == jvc->hvname) return jvc;
         }
      convrt("(P1,4H0***J1A" qLSN ",7HSENSE \",J0A" qLTF
         ",11H\" NOT FOUND/4XJ0A80)", vsrcrid, getrktxt(ivcndx),
         erstring, NULL);
      RK.iexit |= 1;
      return NULL;
      }
#endif

/* Search for virtual cells of required id and type.  By the time this
*  function can be called at d3tchk time, the built-in and user-defined
*  sense block linked lists will have been concatenated.  When called
*  from g1arm or g1wdw, we only need to look at the built-in list.  */

   for (jvc=RP0->pvc1; jvc; jvc=jvc->pnvc) {
      if (strncmp(vsrcrid,jvc->vcname,LSNAME) == 0) {
         if (jvctyp == 0) break;
         --jvctyp; }
      }

/* If requested item not found and type condition is met, create
*  it at end of list of built-in senses, otherwise give error.  */

   if (!jvc) {
      if (ivcid == REPSRC || ivcid >= USRSRC) {
         convrt("(P1,'0***SENSORY CELLS \"',J0A" qLSN ",H(J0I4,')\" "
            "NOT FOUND'/4XJ0A80)", vsrcrid, &ivcndx, erstring, NULL);
         RK.iexit |= 1; }
      else if (jvctyp) {
         convrt("(P1,'0***SENSORY CELLS \"',J0A" qLSN ",H(J0I4,"
            "')\" COULD NOT BE CREATED'/4XJ0A80/"
            "4X,'TYPE NUMBER WAS TOO LARGE.')",
            vsrcrid, &ivcndx, erstring, NULL);
         RK.iexit |= 1; }
      else {
         jvc = (struct VCELL *)allocpcv(
            Host, 1, sizeof(struct VCELL), "virtual cell block");
         vcdflt(jvc,vsrcrid);       /* Initialize VCELL block */
         jvc->hvname = ivcndx;
         *RP0->plvc = jvc;          /* Update linked list end */
         RP0->plvc = &jvc->pnvc;
         }
      } /* End if block not found */

   return jvc;
   } /* End findvc */
#endif

/*---------------------------------------------------------------------*
*                           fmtvcn, fmtvcns                            *
*                                                                      *
*  Format name information from a VCELL block in a uniform fashion     *
*  for inclusion in error or warning messages.                         *
*                                                                      *
*  Call by:  fmtvcn(struct VCELL *pvc)       (Long form output)        *
*            fmtvcns(struct VCELL *pvc)      (Short form output)       *
*                                                                      *
*  Argument:                                                           *
*     'pvc' is a pointer to the VCELL block in question.               *
*                                                                      *
*  Return:                                                             *
*     Pointer to a string that can be included in an error message.    *
*  This is a static string which remains valid only until the next     *
*  call to fmtvcn.                                                     *
*---------------------------------------------------------------------*/

#ifdef BBDS
char fmtvcnmsg[LSNAME+MaxDevNameLen+8];
#else
char fmtvcnmsg[LSNAME+12+8];
#endif

char *fmtvcn(struct VCELL *pvc) {

   strncpy(fmtvcnmsg, pvc->vcname, LSNAME);
   fmtvcnmsg[LSNAME] = '\0';  /* JIC */
   strcat(fmtvcnmsg, " SENSE ");
#ifdef BBDS
   if (pvc->lvrn > 0)
      strncat(fmtvcnmsg, getrktxt(pvc->hvname), MaxDevNameLen);
   else
#endif
   if (pvc->hvname > 0)
      strncat(fmtvcnmsg, ssprintf(NULL, "%4d", (int)pvc->hvname), 4);
   else
      strcat(fmtvcnmsg, "(unnumbered)");
   return fmtvcnmsg;

   } /* End fmtvcn() */

char *fmtvcns(struct VCELL *pvc) {

   char *pvcn;
   static char imgt[] = "ImgTouch";

   if (pvc->vtype == ITPSRC)
      pvcn = imgt;
   else if (pvc->vtype == USRSRC)
      pvcn = getrktxt(pvc->hvname);
   else pvcn = ssprintf(NULL, "%4s#%4d",
      pvc->vcname, (int)pvc->hvname);

   return pvcn;

   } /* End fmtvcns() */
