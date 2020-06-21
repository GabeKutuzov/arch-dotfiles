/* (c) Copyright 1993-2017, The Rockefeller University *21113* */
/***********************************************************************
*                               getgd3                                 *
*                                                                      *
* SYNOPSIS (MATLAB command):                                           *
*     x = getgd3(file, segment, item, s1, s2, ..., time)               *
*                                                                      *
* DESCRIPTION:                                                         *
*     getgd3 is a user-written MATLAB(TM) function that permits a      *
*  MATLAB user to read one variable per invocation as a function       *
*  of time from an NSI GRAFDATA file into a MATLAB array or matrix,    *
*  making the data available for any desired MATLAB operations.        *
*     This program is compatible with GRAFDATA file version 3 and      *
*  with MATLAB version 5 or higher.                                    *
*                                                                      *
* ARGUMENTS:                                                           *
*  file  is a character string giving the file name (or full path      *
*     name) of the GRAFDATA file to be processed.                      *
*  segment  is a positive integer giving the number of the segment     *
*     within the GRAFDATA file to be accessed.                         *
*  item  is a character string giving the name of the data item to     *
*     be retrieved.  This argument must match one of the item          *
*     names in the GRAFDATA file header.                               *
*  s1, s2, ... are "selectors" that define the particular data to be   *
*     retrieved.  There must be one selector for each level out to the *
*     requested data item and each selector must specify data that are *
*     present at its level in the data tree in the requested segment   *
*     (precise matching rules are given below).  Each selector may be  *
*     specified in full form or in short form.  A full-form selector   *
*     is a 1 x 2 MATLAB cell array in which the first element is a     *
*     string giving the applicable level name (an added 'S' will be    *
*     ignored) and the second element is a short-form selector (see    *
*     next paragraph).  Level names may be abbreviated to any unambi-  *
*     guous initial substring.  Full-and short-form selectors may be   *
*     mixed in the same call.  Full-form selectors may give slightly   *
*     slightly better performance.                                     *
*        Each short-form selector specifies one or more selections.    *
*     The data will be returned in the order the selectors are given.  *
*     Selectors need not be in increasing numerical order, but data    *
*     retrieval may be more efficient if they are.  Selectors can be   *
*     names, numbers, ranges, lists of names, or lists of numbers,     *
*     encoded as follows:                                              *
*     (1) Names are given by MATLAB strings.                           *
*     (2) Numbers are given by positive integers.                      *
*     (3) Ranges are given by matrices of two or three numbers in      *
*         which the first number is a positive integer giving the      *
*         beginning of the range, the second is a negative integer     *
*         giving minus the end of the range, and the third is an       *
*         optional negative integer giving minus the increment         *
*         (stride) of the range.  If the third number is not present,  *
*         the stride is assumed to be 1.                               *
*     (4) A list of names is a cell array containing one or more       *
*         names.  (A single name used in a full-form selector must     *
*         be coded as a list.                                          *
*     (5) A list of numeric selectors is a vector containing one or    *
*         more concatenated numbers or ranges.  The first number in    *
*         a list cannot be negative.  Names and numbers cannot be      *
*         mixed in the same list.                                      *
*  time  is a row vector of integers defining the time points (CNS     *
*     trials) for which data are to be retrieved.  Time values are     *
*     encoded using the same rules as for variable selectors:          *
*     positive values indicate single time points, while positive-     *
*     negative pairs indicate ranges of time points.  The program      *
*     will look for each time point where it ought to be if every      *
*     time point is in the file with nits cycles.  If this fails,      *
*     it will search from the start of the segment for requested       *
*     time points.                                                     *
*                                                                      *
* MATCHING RULES:                                                      *
*  A request selector (rSel) matches a file data selector (fSel) if:   *
*     (1) rSel is a name and fSel is a name and the names match.       *
*     (2) rSel is a number and fSel is a number and the numbers match. *
*     (3) rSel is a number and fSel is a name and fSel is the rSel'th  *
*         name in a list of names.                                     *
*     (4) rSel is a number and fSel is a range and the number is       *
*         contained within the range.                                  *
*     (5) rSel is a range and fSel is a range and the values in the    *
*         rSel range are a subset of the values in the fSel range.     *
*     (6) rSel is a range and fSel contains discrete elements and      *
*         ranges that together include all of rSel.  This case is      *
*         handled by expanding the rSel range into discrete elements   *
*         which are then matched by rules (2) and (3).                 *
*  The selectors at all levels must form a product set, that is, for   *
*     each selector specified at any one level, there must be matches  *
*     for all specified selectors at all other levels.  The number of  *
*     items selected is the product of the numbers of items selected   *
*     at each level.  Equivalently, the selectors must form a tree,    *
*     i.e.  no branches in the selector array may recoalesce at a      *
*     lower (higher numbered) selection level.                         *
*                                                                      *
* RETURN VALUE:                                                        *
*  x is a multidimensional MATLAB 5 array containing the requested     *
*     data.  The dimensionality of this array is equal to the number   *
*     of indices needed to assign, in left-to-right order, one index   *
*     corresponding to the array index of the item if it is stored as  *
*     an array in the GRAFDATA file, one index for each selector that  *
*     has more than one value in order from low-numbered levels to     *
*     high-numbered level, one index to select an iteration cycle if   *
*     NITS in the segment header is greater than 1, and one index to   *
*     select a time point.  All of these dimensions are condensed so   *
*     that each index runs from 1 to the number of items contained in  *
*     that dimension.  All data are scaled as specified in the         *
*     GRAFDATA file header.                                            *
*                                                                      *
* NOTES:                                                               *
*  This routine takes advantage of the ability of the gditools and     *
*     certain ROCKS routines to work in an environment where standard  *
*     I/O interface and memory allocation tools are not present.  To   *
*     make this work, abexit[m][e] and [m|c|re]allocv routines that do *
*     not use the standard libraries must be and are supplied here.    *
*  Additionally, to insure reusability after an error, all error exits *
*     must pass via abexit[m][e], not just mexErrMsgTxt, so that       *
*     global memory can be cleaned up.                                 *
*                                                                      *
* ERROR CODES:                                                         *
*  Error codes in the range 300-329 are assigned to this program.      *
*                                                                      *
* REVISION HISTORY                                                     *
*  Version 1, 01/28/93, Rony Zarom.                                    *
*  Version 3, 11/03/97, G. N. Reeke.  This is a complete rewrite to    *
*                       enable support for Version 3 GRAFDATA files.   *
*  V3B, 12/04/97, GNR - Make it safe for reuse while still in memory.  *
*  Rev, 09/26/08, GNR - Minor type changes for 64-bit compilation      *
*  Rev, 03/01/17, GNR - Handle long ints and GDV_FixMZero types        *
***********************************************************************/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mex.h"              /* Put before sysdef.h */
#include "sysdef.h"
#include "rkarith.h"
#include "graphd.h"
#include "rksubs.h"
#include "swap.h"

/* Configuration parameters */
#define FileArg      0        /* prhs index of file arg */
#define SegmArg      1        /* prhs index of segment arg */
#define ItemArg      2        /* prhs index of item arg */
#define FSelArg      3        /* prhs index of first selector arg */
#define InitSplits   6        /* Initial allowance for GDSel splits */
#define Round      0.5        /* Roundoff constant */

/* Private data structures */
struct LevSel {               /* Permanent level selection list */
   GDSel *psel;               /* Ptr to array of selectors */
   int    nsel;               /* Number of selectors in list */
   int    nitms;              /* Number of items in list */
   int    levid;              /* Level id, 0 if not known */
   };

/* Private global data */
int abexloop;                 /* Flag for detecting abexit loop */
static struct RFdef *pgdf;    /* Ptr to file descriptor */
static char *pD;              /* Ptr to file data buffer */
static double *pR;            /* Ptr to result matrix data */
static struct LevSel *pUrls0; /* Ptr to permanent selector lists */
static GDVar *pV0;            /* Ptr to variable information */
static size_t Litem;          /* Length of one data item */
static int DataLevel;         /* Level of requested data item */
static unsigned short VinReq; /* Variable index requested */

/***********************************************************************
*          Replacement ROCKS Routines for MATLAB Environment           *
***********************************************************************/

/*=====================================================================*
*                    abexit, abexitm, and abexitme                     *
*  This code replaces the ROCKS abexit() and abexitm() functions for   *
*  getgd3, implementing the cryout() calls instead with mexPrintf and  *
*  mexErrMsgTxt.  This assures that no ROCKS interface routines will   *
*  be linked incorrectly into the MATLAB environment.  Additionally,   *
*  ggdclean() is called so that getgd3() can be restarted from MATLAB. *
*  abexitme() is made equivalent to abexitm(), as errno is irrelevant. *
*=====================================================================*/

static void ggdclean(void);
void abexit(int code) {

   if (!abexloop) {
      abexloop = TRUE;
      ggdclean();
      }
   mexErrMsgTxt(ssprintf(NULL, "getgd3 terminated with abexit "
      "code %4d", code));
   exit(102);     /* Avoid compile-time no-return warning */

   } /* End abexit() */

void abexitm(int code, char *msg) {

   mexPrintf("\n*** %.128s\n", msg);
   abexit(code);

   } /* End abexitm() */

/* Not used here, just in case called from some library routine */
void abexitme(int code, char *msg) {

   mexPrintf("\n*** %.128s\n", msg);
   abexit(code);

   } /* End abexitme() */


/*=====================================================================*
*                mallocv, callocv, reallocv, and freev                 *
*  This code replaces the ROCKS routines of the same names, implement- *
*  ing the memory management calls with the corresponding MATLAB calls.*
*=====================================================================*/


void *mallocv(size_t length, char *msg) {

   void *newstorage;
   if ((newstorage=(void *)mxMalloc(length)) == NULL && msg) {
      static char efmt[] =
         "Memory alloc failed for %48s (%10ld bytes requested).";
      abexitm(32, ssprintf(NULL, efmt, msg, length));
      }
   return newstorage;

   } /* End mallocv */


void *callocv(size_t n, size_t size, char *msg) {

   void *newstorage;
   if ((newstorage=(void *)mxCalloc(n, size)) == NULL && msg) {
      static char efmt[] =
         "Memory alloc failed for %48s (%10ld bytes requested).";
      abexitm(32, ssprintf(NULL, efmt, msg, n*size));
      }
   return newstorage;

   } /* End callocv */


void *reallocv(void *ptr, size_t length, char *msg) {

   void *newstorage;
   /* Deal with the fact that standard UNIX realloc is not
   *  guaranteed to work if the pointer passed to it is NULL.  */
   if ((newstorage = ptr ? (void *)mxRealloc(ptr, length) :
         (void *)mxMalloc(length)) == NULL && msg) {
      static char efmt[] =
         "Memory realloc failed for %48s (%10ld bytes requested).";
      abexitm(32, ssprintf(NULL, efmt, msg, length) );
      }
   return newstorage;

   } /* End reallocv */

/* N.B.  If any tests that result in an abexit[m] are ever added to
*  this routine, it is essential to put in suitable tests to prevent
*  recursion when freev is called during ggdclean() processing.  */

void freev(void *freeme, char *msg) {

   mxFree(freeme);

   } /* End freev */


/***********************************************************************
*                          Other Subroutines                           *
***********************************************************************/

/*=====================================================================*
*                            StoreSelName                              *
*                                                                      *
*  This routine checks that a MATLAB array contains a string and       *
*  that the string is not too long for a name selector, and then       *
*  copies the string into a GDSel structure for later use.             *
*                                                                      *
*  Arguments:                                                          *
*     prSel       Ptr to the GDSel where the name is to be stored.     *
*     pmxa        Ptr to the MATLAB array containing the string.       *
*  Returns:       Nothing                                              *
*=====================================================================*/

static void StoreSelName(GDSel *prSel, const mxArray *pmxa) {

   char tname[LSelName+1];       /* Temp for name + end mark */
   int N = mxGetN(pmxa);         /* Size of string */

   if (!mxIsChar(pmxa) || mxGetNumberOfDimensions(pmxa) != 2
         || mxGetM(pmxa) != 1 || N <= 0) abexitm(301,
      "An apparent selector name in a cell array was not a string.");
   if (N > LSelName) abexitm(302,
      "A name-type selector is too long (max " qLSN " chars).");
   prSel->ks = GDS_NAME;         /* Build GSSel block */
   mxGetString(pmxa, tname, N+1);
   memcpy(prSel->us.in, tname, LSelName);

   } /* End StoreSelName() */


/*=====================================================================*
*                             MakeReqNode                              *
*                                                                      *
*  This routine makes a request node as a pls child of a given         *
*  node and copies a set of selectors for its level to it.             *
*  N.B.  To prevent gdiclear from trying to free() space that          *
*  was not malloc'd, pvin,nvar fields are left empty.                  *
*                                                                      *
*  Argument:                                                           *
*     prNode0     Ptr to parent node at next lower-numbered level.     *
*  Returns:       Ptr to the new node.                                 *
*=====================================================================*/

static GDNode *MakeReqNode(GDNode *prNode0) {

   GDNode *prNode;
   /* This will be one level below prNode0 because subscripts
   *  start at 0 but we do not subtract 1 from prNode0->level. */
   struct LevSel *purls = pUrls0 + prNode0->level;
   int nsel1 = purls->nsel;
   int nsall = purls->nsel;

   prNode = prNode0->pls = (GDNode *)callocv(1, sizeof(GDNode),
      "Level selection node");
   prNode->par = prNode0;
   prNode->level = prNode0->level + 1;
   if (nsall > 1 || purls->psel->ks == GDS_RANGE) nsall += InitSplits;
   prNode->psel = (GDSel *)mallocv(
      nsall * sizeof(GDSel), "Selectors");
   memcpy((char *)prNode->psel, (char *)purls->psel,
      nsel1 * sizeof(GDSel));
   prNode->levid  = purls->levid;
   prNode->nsallo = nsall;
   prNode->nsel   = nsel1;
   return prNode;

   } /* End MakeReqNode() */


/*=====================================================================*
*                            SplitReqNode                              *
*                                                                      *
*  This routine splits a request node at a given selector, leaving     *
*  all selectors already stored in the parent and making room for      *
*  as-yet unstored selectors in the child.  The new node is a pns      *
*  child of the parent, i.e. it is at the same level.  SplitReqNode    *
*  assumes that it will not be called unless there is at least one     *
*  selector on each side of the split.                                 *
*                                                                      *
*  Since we know exactly how many selectors will stay in the parent,   *
*  but not how many may later be created in the child, we make a new   *
*  selector array of exactly the right size for the parent and attach  *
*  the existing one, which should have some spare space, to the child. *
*                                                                      *
*  Arguments:                                                          *
*     prNode0     Ptr to parent node.                                  *
*     prSel       Ptr to the first selector to go in the new node.     *
*  Returns:       Ptr to the new node.                                 *
*=====================================================================*/

static GDNode *SplitReqNode(GDNode *prNode0, GDSel *prSel) {

   GDNode *prNode;                     /* Ptr to newly made node */
   GDSel  *ps,*psE;                    /* Ptr to moved GDSels */
   int nsl = prSel - prNode0->psel;    /* No. selectors left behind */
   size_t nmove = nsl * sizeof(GDSel); /* Size of nsl selectors */

   prNode = prNode0->pns = (GDNode *)callocv(1, sizeof(GDNode),
      "Level selection node");
   prNode->par  = prNode0;
   prNode->psel = ps = prNode0->psel;
   prNode->level  = prNode0->level;
   prNode->levid  = prNode0->levid;
   prNode->nsallo = prNode0->nsallo;
   prNode->nsel   = prNode0->nsel - nsl;

   /* Make space on the old node for the selectors that
   *  are to be left behind and copy them there.  */
   prNode0->psel = (GDSel *)mallocv(nmove, "Selectors");
   prNode0->nsallo = prNode0->nsel = nsl;
   memcpy((char *)prNode0->psel, (char *)ps, nmove);

   /* Move the remaining selectors on the new node down over
   *  the ones that were just moved back to the old node.  */
   psE = ps + prNode->nsel;
   while (ps < psE) *ps++ = *prSel++;

   return prNode;

   } /* End SplitReqNode() */


/*=====================================================================*
*                             SplitRange                               *
*                                                                      *
*  This little routine splits a requested range whenever a partial     *
*  match (to a file range or single item) is found.  The unmatched     *
*  fragment is encoded as a range or single item depending on what     *
*  is left, and it will be the next thing examined in the main loop.   *
*                                                                      *
*  Arguments:                                                          *
*     prNode      Ptr to the request node to which the range belongs.  *
*     prSel       Ptr to the selector that is to be split.             *
*     last        Last item in the part of the range that is accepted. *
*  Returns:       Possibly altered value of prSel argument.  Since     *
*                 it is not convenient to return two results, the      *
*                 caller should be sure to update prSelE if needed.    *
*=====================================================================*/

static GDSel *SplitRange(GDNode *prNode, GDSel *prSel, long last) {

   GDSel *ps;
   long newstart = last + prSel->us.r.ii;

/* A little safety check:  If the new start would be beyond the
*  end of the existing range, nothing needs to be done at all.  */

   if (newstart <= prSel->us.r.ie) {

/* If there is not enough room for one more selector, add enough
*  for another InitSplits or 12.5% of what is already there.
*  This requires adjusting prSel to point into the new array.  */

      if (prNode->nsel >= prNode->nsallo) {
         int jsel  = prSel - prNode->psel;   /* Save prSel offset */
         int newsz = (prNode->nsallo >> 3);
         if (newsz < InitSplits) newsz = InitSplits;
         newsz = (prNode->nsallo += newsz);
         prNode->psel = reallocv(prNode->psel, newsz*sizeof(GDSel),
            "Split request range");
         prSel = prNode->psel + jsel;        /* Adjust prSel */
         }

/* Move everybody past prSel up one, copying prSel while at it.
*  (Cannot use memcpy because fields overlap.)  */

      for (ps=prNode->psel+prNode->nsel++; ps>prSel; ps--)
         *ps = *(ps-1);

/* Modify the left fragment to include everything up to the last item.
*  If the new range has only one member, convert it back to a GDS_NUM.
*  (Irrelevant to ggdmatch, but makes this routine logically complete.)
*/
      if (last == ps->us.r.is)
         ps->ks = GDS_NUM, ps->us.it = last;
      else
         ps->us.r.ie = last;

/* Modify the right fragment to start at the next member after the
*  last member before the split.  If this is now the last member
*  in the range, convert it back to a GDS_NUM.  */

      ++ps;
      if (newstart + ps->us.r.ii > ps->us.r.ie)
         ps->ks = GDS_NUM, ps->us.it = newstart;
      else
         ps->us.r.is = newstart;

      } /* End case newstart <= end of range */

   return prSel;

   } /* End SplitRange */


/*=====================================================================*
*                             ggdmatch()                               *
*                                                                      *
*  This routine recursively matches the requested data selections      *
*  depth first against the data present in the file.  At each level,   *
*  if the data are found, it calculates and stores offsets to these    *
*  data.  Selectors and nodes can be split if the data are found in    *
*  multiple file nodes.  Success or failure is returned to the         *
*  calling level.                                                      *
*     This program depends on the documented restriction that numeric  *
*  data selectors within a single file data selector node must be in   *
*  increasing order.  The request selectors do not have to be in any   *
*  particular order, but the program may operate more quickly if the   *
*  requests are in the same order as the file data, (1) because the    *
*  search for each item commences where the previous item was found,   *
*  and (2) because unnecessary file seeks are avoided.                 *
*                                                                      *
*  Arguments:                                                          *
*     prNode0     Ptr to parent node one up from current level.        *
*     pfNode      Ptr to initial file selector node at current level.  *
*  Returns:       TRUE if all items matched, otherwise FALSE           *
*=====================================================================*/

static int ggdmatch(GDNode *prNode0, GDNode *pfNode) {

   GDNode *pnode,*pnode0;     /* Ptrs to cur,first node being checked */
   GDNode *prNode;            /* Ptr to selection node being built */
   GDSel *prSel,*prSelE;      /* Ptrs to cur,last rqst selectors */
   GDSel *pfSel,*pfSelE;      /* Ptrs to cur,last file selectors */
   long  lastmatch;           /* Used to trigger node rescan */
   long  oitem;               /* Offset to matched item at this node */
   enum {                     /* Action to occur at next match */
      StoreMatch, TestNextLevel, MSplitReqNode, MMakeReqNode
      } ktrig;

/* Return success if beyond DataLevel.  This is how the recursion gets
*  terminated, avoiding a test at each point where ggdmatch is called.
*/

   if (prNode0->level < DataLevel) {

/* Match all requested selections at this level against file data.
*  Begin by matching the first selector in the permanent list.  Once
*  a match has been found and a request node created, switch over to
*  using the copy in the new request node.  This too can change during
*  execution of the prSel loop as ranges and even nodes may be split.
*/
      pnode = pfNode;
      lastmatch = LONG_MAX;   /* Trigger new node scan */

      ktrig = MMakeReqNode;
      prSelE = (prSel = pUrls0[prNode0->level].psel) + 1;
      for ( ; prSel < prSelE; prSel++) {

         for (pnode0=pnode; ; ) {

/* If entering a new node, or if matching names, or if current
*  item comes before the previous match on the same node, then
*  reinitialize the node scan and offset to the start of the node.
*  This allows the program to deal with out-of-order requests.
*
*  The file node does not need to be inspected if its levid does not
*  match the levid requested, or if the request level is non-terminal
*  and the file node has no pls children, or if the request level is
*  terminal and the file node does not contain the requested variable.
*  Otherwise, oitem is set to the offset of the requested item in the
*  file or 0 if the node is not terminal.  */

         if (prSel->ks == GDS_NAME ||
             prSel->ks == GDS_NUM && prSel->us.it <= lastmatch ||
             prSel->ks == GDS_RANGE && prSel->us.r.is <= lastmatch) {

            struct LevSel *purls = pUrls0 + pnode->level - 1;
            if (purls->levid > 0 && purls->levid != pnode->levid)
               goto NoMatchOnThisFileNode;
            if (pnode->level == DataLevel) {    /* Terminal node */
               unsigned short *pfvin  = pnode->pvin;
               unsigned short *pfvinE = pnode->pvin + pnode->nvar;
               oitem = pnode->odata;
               while (pfvin<pfvinE) {
                  register unsigned short vin = *pfvin++;
                  if (vin == VinReq) goto FoundVinReq;
                  oitem += pV0[vin].vlen * pV0[vin].vdim;
                  }
               goto NoMatchOnThisFileNode;
               }
            else {                              /* Not terminal */
               oitem = 0;
               /* Skip node if later search for children must fail */
               if (!pnode->pls) goto NoMatchOnThisFileNode;
               }
FoundVinReq:
            pfSelE = (pfSel  = pnode->psel) + pnode->nsel;
            } /* End initialization of node scan */

/*---------------------------------------------------------------------*
*          Match a request selector against a file selector            *
*                                                                      *
*  As soon as a tentative match is found, and before it is recorded,   *
*  the match is tested at lower (higher-numbered) selection levels.    *
*  The first time data are found at a new file node, a new request     *
*  node is constructed to contain the offset and width information     *
*  and to provide a root for the lower-level ggdmatch to build on.     *
*  All selectors from the one just matched to the end are copied to    *
*  the new node, allowing the node to be split later if necessary.     *
*  If a tentative match fails due to mismatch at a lower level, the    *
*  scan continues at the next file node.  If the data cannot all be    *
*  matched, the tentative request nodes are destroyed and failure is   *
*  returned to the calling level.                                      *
*                                                                      *
*  When a match is found, the selector is changed to a GDS_OFFR type   *
*  and the offset and increment needed to collect the data are stored  *
*  in it.  All cases are treated as ranges to avoid type switch later. *
*                                                                      *
*  Four levels of indenting suppressed down to end of this block.      *
*---------------------------------------------------------------------*/

/* The following macro does the node construction testing described
*  above each time a tentative match is found.  It is not necessary
*  to check the existence of pfNode->pls, since we go directly to
*  NoMatchOnThisFileNode if we are not terminal and it is not there.
*  There is no obvious non-ugly way (without using co-routines) to
*  avoid this huge macro.  */

#define PerformTriggerAction(ktrig) switch (ktrig) {              \
   case MMakeReqNode:                                             \
   case MSplitReqNode:                                            \
      prNode = (ktrig == MMakeReqNode) ? MakeReqNode(prNode0) :   \
         SplitReqNode(prNode, prSel);                             \
      ktrig = TestNextLevel;     /* Drop into next case */        \
   case TestNextLevel:                                            \
      if (!ggdmatch(prNode, pnode->pls))                          \
         goto NoMatchOnThisFileNode;                              \
      prNode->ldata = pnode->ldata;                               \
      prSelE = (prSel = prNode->psel) + prNode->nsel;             \
      ktrig = StoreMatch;        /* Drop into next case */        \
   case StoreMatch:                                               \
      ;                                                           \
   } /* End switch and macro */

/* Inspect all the selectors on the current file node */

for ( ; pfSel<pfSelE; pfSel++) {

   switch (prSel->ks) {

   case GDS_NAME:             /* Requested item is a name */
      /* Rule (1):  Match a name to a name */
      if (pfSel->ks == GDS_NAME && strncmp(prSel->us.in,
            pfSel->us.in, LSelName) == 0) {
         PerformTriggerAction(ktrig);
         lastmatch = LONG_MAX;
         prSel->us.r.is = prSel->us.r.ie = oitem;
         goto SelectorConsumedSingle;
         }
      break;

   case GDS_NUM:              /* Requested item is a number */
      /* Rule (2):  Match a number to a number */
      if (pfSel->ks == GDS_NUM && prSel->us.it == pfSel->us.it) {
         PerformTriggerAction(ktrig);
         lastmatch = prSel->us.it;
         prSel->us.r.is = prSel->us.r.ie = oitem;
         goto SelectorConsumedSingle;
         }
      /* Rule (3):  Match a number to a list of names */
      if (pfSel->ks == GDS_NAME && prSel->us.it <= pnode->nsel) {
         PerformTriggerAction(ktrig);
         lastmatch = LONG_MAX;
         /* On arriving here, any previous match must also have
         *  been to a name, therefore oitem will remain set at
         *  the offset of the first named item in the list... */
         prSel->us.r.is = prSel->us.r.ie = oitem + pnode->ldata *
            (prSel->us.it - 1);
         goto SelectorMatchedSingle;
         }
      /* Rule (4):  Match a number to a range */
      if (pfSel->ks == GDS_RANGE &&
            prSel->us.it >= pfSel->us.r.is &&
            prSel->us.it <= pfSel->us.r.ie &&
            (prSel->us.it - pfSel->us.r.is) % pfSel->us.r.ii == 0) {
         PerformTriggerAction(ktrig);
         lastmatch = prSel->us.it;
         prSel->us.r.is = prSel->us.r.ie = oitem + pnode->ldata *
            (prSel->us.it - pfSel->us.r.is) / pfSel->us.r.ii;
         goto SelectorMatchedSingle;
         }
      break;

   case GDS_RANGE:            /* Requested item is a range */
      /* Rule (5):  Match a range to a number.
      *  N.B.  Since there can be no data duplication in the file,
      *  if we find a match this way, we have to use it, i.e. there
      *  is no use looking for a match against the whole range.  */
      if (pfSel->ks == GDS_NUM && prSel->us.r.is == pfSel->us.it) {
         PerformTriggerAction(ktrig);
         lastmatch = prSel->us.r.is;
         prSel = SplitRange(prNode, prSel, pfSel->us.it);
         prSelE = prNode->psel + prNode->nsel;
         prSel->us.r.is = prSel->us.r.ie = oitem;
         goto SelectorConsumedSingle;
         }
      /* Rule (6):  Match a range to another range.
      *  N.B.  The start must lie within the pfSel range, but the end
      *  can extend beyond it (the request range will be split).  */
      if (pfSel->ks == GDS_RANGE &&
            prSel->us.r.is >= pfSel->us.r.is &&
            prSel->us.r.is <= pfSel->us.r.ie &&
            (prSel->us.r.is - pfSel->us.r.is) % pfSel->us.r.ii == 0) {
         PerformTriggerAction(ktrig);
         {  long rii = prSel->us.r.ii, fii = pfSel->us.r.ii;
            lastmatch = (rii % fii) ? prSel->us.r.is : prSel->us.r.is +
               ((min(prSel->us.r.ie, pfSel->us.r.ie) -
                  prSel->us.r.is) / rii) * rii;
            prSel = SplitRange(prNode, prSel, lastmatch);
            prSelE = prNode->psel + prNode->nsel;
            if (prSel->ks == GDS_NUM) {   /* Now it's a number */
               prSel->us.r.is = prSel->us.r.ie = oitem + pnode->ldata *
                  (prSel->us.it - pfSel->us.r.is) / fii;
               /* If the increments and bounds are such that the next
               *  member of the prSel range is inside the pfSel range
               *  but not a member of it, there is no use looking any
               *  further on this file node and we go on to another
               *  after doing the bookkeeping that would normally be
               *  done at SelectorMatchedSingle.  The program should
               *  give identical results if this optimization were
               *  to be removed--perhaps useful during testing.  */
               if (lastmatch + rii <= pfSel->us.r.ie) {
                  prSel->us.r.ii = 1;
                  prSel->ks = GDS_OFFR;
                  prSel++;
                  goto NoMatchOnThisFileNode;
                  }
               goto SelectorMatchedSingle;
               }
            else {                        /* It's a subrange */
               prSel->us.r.is = oitem + pnode->ldata *
                  (prSel->us.r.is - pfSel->us.r.is) / fii;
               prSel->us.r.ie = oitem + pnode->ldata *
                  (prSel->us.r.ie - pfSel->us.r.is) / fii;
               prSel->us.r.ii = pnode->ldata * (rii / fii);
               goto SelectorMatchedRange;
               }
            } /* End scope of rii, fii */
         }
      break;
      } /* End switch on type of request selector */

   /* Skip over the data on the file node that will not be used */
   oitem += pnode->ldata;
   if (pfSel->ks == GDS_RANGE) oitem += pnode->ldata *
      ((pfSel->us.r.ie - pfSel->us.r.is)/pfSel->us.r.ii);

   } /* End scanning a file node */

/*---------------------------------------------------------------------*
*  Match not found on present file node.  Try next in chain, advancing *
*  in a circle until get back to starting node.  Trigger a full scan   *
*  of the new node.  Set trigger to split the request node if missing  *
*  part of match is found in the new file node.  This scheme allows    *
*  requests to be unordered.                                           *
*                                                                      *
*  Resume normal indenting.                                            *
*---------------------------------------------------------------------*/

NoMatchOnThisFileNode:
            pnode = pnode->pns ? pnode->pns : pfNode;
            if (pnode == pnode0) break;
            lastmatch = LONG_MAX;
            if (ktrig == StoreMatch) ktrig = MSplitReqNode;
            } /* End node loop */

/* Item was not found in any node at this level.
*  Delete any nodes that were created and
*  return a failure report to invoking level.  */

         gdiclear(&prNode0->pls);
         return FALSE;

/* Selector was matched--Store info and go on to next one */

SelectorConsumedSingle:          /* Consume the item matched */
         /* This case is an optimization to eliminate retesting the
         *  same file item for the next request item--code should
         *  give identical results if these two lines removed.  */
         oitem += pnode->ldata;
         pfSel++;
SelectorMatchedSingle:           /* Make a degenerate range */
         prSel->us.r.ii = 1;
SelectorMatchedRange:
         prSel->ks = GDS_OFFR;   /* Indicate changed use of range */
         } /* End request scan at current level */

      } /* End processing done if prNode exists */

/* Found matches for all items at this level:
*  Report success to invoking level.  */

   return TRUE;

   } /* End ggdmatch() */


/*=====================================================================*
*                             ggdfetch()                               *
*                                                                      *
*  This routine recursively processes one data record.  It locates,    *
*  unpacks, and stores all selected data in the pR result array.       *
*                                                                      *
*  Argument:                                                           *
*     prNode      Ptr to request node at current selection level.      *
*     orec        Offset to data selected at parent level.             *
*                                                                      *
*  Returns:       Data, converted to type double, in pR array.         *
*=====================================================================*/

static void ggdfetch(GDNode *prNode, long orec) {

   GDNode *pn;                /* Ptr to current node */
   GDSel *ps,*pse;            /* Ptrs to selectors */
   long iter1,iter2,iiter;    /* Iteration controls */

/* Loop over this node and its same-level descendents  */

   for (pn=prNode; pn; pn=pn->pns) {
      pse = (ps = pn->psel) + pn->nsel;

/* If not at terminal level, loop over selectors on this level and
*  call self recursively to handle deeper levels.  For efficiency,
*  the selection loop is repeated for both cases of the level test.  */

      if (pn->level < DataLevel) {
         /* Error trap--remove after debugging */
         if (pn->pls == NULL)
            abexitm(300,"Nonterminal node has no pls child.");
         for ( ; ps<pse; ps++) {
            /* Error trap--remove after debugging */
            if (ps->ks != GDS_OFFR)
               abexitm(300,"Selector not in offset form.");
            iter1 = ps->us.r.is;
            iter2 = ps->us.r.ie;
            iiter = ps->us.r.ii;
            for ( ; iter1<=iter2; iter1+=iiter)
               ggdfetch(pn->pls, orec+iter1);
            }} /* End if-for over selectors--not terminal */

/* If at terminal level, loop over selectors on this level.
*  Unpack and store the requested data.  */

      else {
         GDVar *pv = pV0 + VinReq;
         char *px, *pxe;
         double scl = 1.0/(double)(1L << pv->vscl);
         int nx = pv->vdim;
         int lx = pv->vlen;
         for ( ; ps<pse; ps++) {
            /* Error trap--remove after debugging */
            if (ps->ks != GDS_OFFR)
               abexitm(300,"Selector not in offset form.");
            iter1 = ps->us.r.is;
            iter2 = ps->us.r.ie;
            iiter = ps->us.r.ii;
            for ( ; iter1<=iter2; iter1+=iiter) {
               /* Position file at next data item */
               rfseek(pgdf, orec+iter1, SEEKABS, ABORT);
               /* Read the data from the input file */
               rfread(pgdf, pD, Litem, ABORT);
               /* Loop over data items, unpack and swap them as
               *  required, then convert and store.  To save a
               *  little time, put the switch outside the loop.
               *  For starters, treat color same as fixed-point.
               *  Rev, 03/01/17, GNR - Fix-point code revised to
               *  handle lengths up to long and GDV_FixMZero types.
               *  GDV_FixMZero must avoid sign-extension of -0.  */
               px = pD; pxe = pD + Litem;
               switch (pv->vtype) {

               case GDV_Fix:        /* Signed fixed-point */
               case GDV_Color:      /* Colored pixel data */
                  switch (lx) {
                  case 1:
                     for ( ; px<pxe; px+=lx)
                        *pR++ = scl*(double)(signed char)*px;
                     break;
                  case 2:
                     for ( ; px<pxe; px+=lx) {
                        si16 Hdata = (si16)bemtoi2(px);
                        *pR++ = scl*(double)Hdata;
                        }
                  case 4:
                     for ( ; px<pxe; px+=lx) {
                        si32 Jdata = (si32)bemtoi4(px);
                        *pR++ = scl*(double)Jdata;
                        }
                     break;
                  case 8:
                     for ( ; px<pxe; px+=lx) {
                        si64 Wdata = bemtoi8(px);
                        *pR++ = scl*swdbl(Wdata);
                        }
                     break;
                     } /* End lx switch */
                  break;

               case GDV_UFix:       /* Unsigned fixed-point */
                  switch (lx) {
                  case 1:
                     for ( ; px<pxe; px+=lx)
                        *pR++ = scl*(double)(unsigned char)*px;
                     break;
                  case 2:
                     for ( ; px<pxe; px+=lx) {
                        ui16 Hdata = (ui16)bemtoi2(px);
                        *pR++ = scl*(double)Hdata;
                        }
                  case 4:
                     for ( ; px<pxe; px+=lx) {
                        ui32 Jdata = (ui32)bemtoi4(px);
                        *pR++ = scl*(double)Jdata;
                        }
                     break;
                  case 8:
                     for ( ; px<pxe; px+=lx) {
                        ui64 Wdata = bemtou8(px);
                        *pR++ = scl*uwdbl(Wdata);
                        }
                     break;
                     } /* End lx switch */
                  break;

               case GDV_Real:       /* Real */
                  for ( ; px<pxe; px+=lx) {
                     float Rdata = bemtor4(px);
                     *pR++ = (double)Rdata;
                     } /* End loop over array of GDV_Real */
                  break;

               case GDV_Dble:       /* Double-precision real */
                  for ( ; px<pxe; px+=lx) {
                     double Ddata = bemtor8(px);
                     *pR++ = Ddata;
                     } /* End loop over array of GDV_Dble */
                  break;

               case GDV_FixMZero: { /* Fix-point, -0 allowed */
                  /* A test indicated that gcc converts "double(-0)"
                  *  to 0x0, so we try this method.  */
                  double dmz = (double)0x8000000000000000;
                  switch (lx) {
                  case 1:
                     for ( ; px<pxe; px+=lx)
                        *pR++ = (*px == 0x80) ? dmz :
                           scl*(double)(signed char)*px;
                     break;
                  case 2:
                     for ( ; px<pxe; px+=lx) {
                        si16 Hdata = (si16)bemtoi2(px);
                        *pR++ = (Hdata == 0x8000) ? dmz :
                           scl*(double)Hdata;
                        }
                  case 4:
                     for ( ; px<pxe; px+=lx) {
                        si32 Jdata = (si32)bemtoi4(px);
                        *pR++ = (Jdata == SI32_SGN) ? dmz :
                           scl*(double)Jdata;
                        }
                     break;
                  case 8:
                     for ( ; px<pxe; px+=lx) {
                        si64 Wdata = bemtoi8(px);
                        *pR++ = quw(jrsw(Wdata, jesl(SI32_SGN))) ?
                           dmz : scl*swdbl(Wdata);
                        }
                     break;
                     } /* End lx switch */
                  break; } /* End GDV_FixMZero local scope */

                  } /* End switch over variable type */

               } /* End loop over iterators */

            } /* End loop over selectors */

         } /* End else level is terminal */

      } /* End loop over nodes */

   } /* End ggdfetch() */


/*=====================================================================*
*                             ggdclean()                               *
*                                                                      *
*  This routine closes the input file and cleans up global memory      *
*  on normal termination or after an error.  It is necessary to        *
*  prevent errors when getgd3() is called again after an error.        *
*                                                                      *
*  Arguments:     None                                                 *
*  Returns:       Nothing                                              *
*=====================================================================*/

static void ggdclean(void) {

   gdiclose();
   if (pD) {
      freev(pD, "Data buffer");
      pD = NULL; }
   if (pUrls0) {
      freev(pUrls0, "Level selectors");
      pUrls0 = NULL; }

   } /* End ggdclean() */


/***********************************************************************
*                              getgd3()                                *
***********************************************************************/

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]) {

   struct LevSel *purls;      /* Ptr to selections at one level */
   GDNode *pfNode1 = NULL;    /* Root the segment selector tree */
   GDNode *prNode0 = NULL;    /* Root the request selector tree */
   GDSel  *prSel,*prSelE;     /* Ptrs to requested GDSels */
   GDVar  *pv;                /* Ptr to info on requested variable */
   int  *dims;                /* Ptr to dimensions of output array */
   char *fname;               /* Name of input file */
   char *item;                /* Name of the data item sought */
   char ***lname;             /* Ptr to array of ptrs to level names */

   long filetime;             /* Time index read from file */
   long lasttime;             /* Previous time point */
   long lmatx;                /* Length of matrix data--1 time */
   long lrecl;                /* Length of data record--1 time */
   long orec1;                /* File position of first record */
   long oreci;                /* File position of i'th  record */
   int iarg;                  /* Argument index */
   int iits;                  /* Inner time cycle index */
   int ilvl;                  /* Level index */
   int ndim;                  /* Number of dims of output array */
   int nits;                  /* Number of iterations/cycle */
   int nSelArgs = nrhs - 3;   /* Number of selector+time args */
   int nlvls;                 /* Number of levels */
   int segment;               /* Input segment to read */

/*---------------------------------------------------------------------*
*                               STAGE I                                *
*  Check the type and number of the MATLAB array arguments.            *
*  Interpret 'file', 'segment', and 'item' arguments.                  *
*---------------------------------------------------------------------*/

   if (nlhs != 1)
      abexitm(303,"Exactly one result array is required.");
   if ((DataLevel = nSelArgs - 1) <= 0)
      abexitm(304,"Not enough arguments--"
         "At least one selector and one time must be given.");

/* Process the 'file' argument */

   if (!mxIsChar(prhs[FileArg]) ||
         mxGetNumberOfDimensions(prhs[FileArg]) != 2)
      abexitm(305,"First argument must be file name.");
   {  size_t lfnm = mxGetM(prhs[FileArg]) * mxGetN(prhs[FileArg]) + 1;
      fname = mallocv(lfnm, "File name");
      if (mxGetString(prhs[FileArg], fname, lfnm))
         abexitm(305,"Unable to interpret file name.");
      } /* End lfnm scope */

/* Process the 'segment' argument */

   /* This test, rather than one based on mxIsDouble(), is
   *  designed to be compatible with some future MATLAB where
   *  integer data types actually get used... */
   if (!mxIsNumeric(prhs[SegmArg]) || mxIsChar(prhs[SegmArg]) ||
         mxGetNumberOfDimensions(prhs[SegmArg]) != 2 ||
         mxGetNumberOfElements(prhs[SegmArg]) != 1)
      abexitm(306,"Second argument must be segment number.");
   segment = (int)mxGetScalar(prhs[SegmArg]);

/* Process the 'item' argument */

   if (!mxIsChar(prhs[ItemArg]) ||
         mxGetNumberOfDimensions(prhs[ItemArg]) != 2)
      abexitm(307,"Third argument must be a valid variable name.");
   /* litm must be long, not size_t, for while test to work */
   {  long litm = mxGetM(prhs[ItemArg]) * mxGetN(prhs[ItemArg]);
      if (litm > LVarName) abexitm(307,
         "Variable name is too long (max " qLVN " chars).");
      item = mallocv(++litm, "Item name");
      if (mxGetString(prhs[ItemArg], item, litm))
         abexitm(307,"Unable to interpret variable name.");
      while (--litm >= 0) item[litm] = toupper(item[litm]);
      } /* End litm scope */

/*---------------------------------------------------------------------*
*                              STAGE II                                *
*  Open the specified GRAFDATA file.  Be sure the requested variable   *
*  exists. Locate the requested segment and parse the segment headers. *
*---------------------------------------------------------------------*/

/* Open the file */

   pgdf = gdiopen(fname);

/* Read and check id record */

   gdickid();

/* Print title and timestamp records */

   mexPrintf("File Title: %.*s\n", LTitRec, gdititle());
   mexPrintf("Time stamp: %.*s\n", LTimeStamp, gditime());

/* Read and store level names */

   nlvls = gdinlvls();
   lname = gdilvlnm(NULL);

/* Read and store variable descriptors.  Before going on to find
*  the requested segment, two simple sanity checks can be done:
*  (1) The requested item name must exist in the file.
*  (2) The number of selectors provided must match the level
*        of the item in the file.
*/

   pV0   = gdivars(NULL);
   VinReq = (unsigned short)gdiqvar(item) - 1;
   pv = pV0 + VinReq;
   if (DataLevel != (int)pv->vlev)
      abexitm(308,"The number of selectors does not match "
         "the level of the data item in the GRAFDATA file.");

/* Locate the requested segment */

   gdigoseg(segment);

/* Parse the selectors in the segment header */

   gdiparse(&pfNode1, NULL);
   nits  = gdinits();
   lrecl = gdirecl() + FMJSIZE;
   orec1 = rftell(pgdf);

/*---------------------------------------------------------------------*
*                              STAGE III                               *
*  Reduce the 'selector' and 'time' arguments to selector arrays.      *
*  Match the request against the available data.                       *
*  Calculate offsets and strides needed to access the data.            *
*---------------------------------------------------------------------*/

/* Process the 'selector' and 'time' arguments similarly.
*  A LevSel is constructed for each selector array argument,
*     pointing to an array of GDSels for the selections as entered.
*  These are kept separate from the GDNodes built by ggdmatch() so
*     that when one of these has to be abandoned due to a lower-level
*     mismatch, the program can revert to the original selectors.  */

   /* Make an array to hold info about the ur-selections */
   if (pUrls0) freev(pUrls0, "Level selectors");
   purls = pUrls0 = (struct LevSel *)callocv(
      nSelArgs, sizeof(struct LevSel), "Level selectors");

   /* Create lists of selectors for each level */
   for (iarg=FSelArg,ilvl=0; iarg<nrhs; ilvl++,purls++) {
      const mxArray *pff1,*pff2,*prhi = prhs[iarg++];
      int N;

      /* If the argument is a cell array, it could be a full-form
      *  selector or just a plain name list.  Decide which.  If it's
      *  a full-form selector, determine and store the level id.  */
      if (mxIsCell(prhi) && mxGetNumberOfElements(prhi) == 2 &&
             mxIsChar(pff1 = mxGetCell(prhi, 0)) &&
            !mxIsChar(pff2 = mxGetCell(prhi, 1)))  {
         char tname[LLevName+1];    /* Temp for name + end mark */
         N = mxGetN(pff1);          /* Size of string */
         if (iarg == nrhs)
            abexitm(309,"Time selectors must be numeric.");
         if (mxGetNumberOfDimensions(pff1) != 2
               || mxGetM(pff1) != 1 || N <= 0)
            abexitm(310,"Level name in a full-form selector "
               "is not a simple string.");
         if (N > LLevName)
            abexitm(311,"Level name in a full-form selector "
               "is too long (max " qLLN " chars).");
         mxGetString(pff1, tname, N+1);
         purls->levid = gdiqlvl(tname, lname[ilvl]);
         prhi = pff2;
         } /* End reading level name from full-form selector */

      /* If list is a cell array, all elements must be strings */
      if (mxIsCell(prhi)) {         /* Selector is cell array type */
         int i;
         if (iarg == nrhs)
            abexitm(309,"Time selectors must be numeric.");
         purls->nitms = N = mxGetNumberOfElements(prhi);
         prSel = purls->psel = (GDSel *)mallocv(
            N * sizeof(GDSel), "Name selector list");
         for (i=0; i<N; i++)
            StoreSelName(prSel++, mxGetCell(prhi, i));
         } /* End storing array of name selectors */

      /* If argument is an ordinary string, just store it.
      *  (Contrary to the MATLAB documentation, a string is NOT
      *  considered to be numeric by mxIsNumeric()...)  */
      else if (mxIsChar(prhi) && mxGetNumberOfDimensions(prhi) == 2
            && mxGetM(prhi) == 1 && mxGetN(prhi) > 0) {
         if (iarg == nrhs)
            abexitm(309,"Time selectors must be numeric.");
         purls->nitms = 1;
         prSel = purls->psel = (GDSel *)mallocv(
            sizeof(GDSel), "Name selector");
         StoreSelName(prSel++, prhi);
         } /* End storing single name selector */

      /* If argument is a numeric matrix, the number of selectors is not
      *  known until any ranges are parsed, but the size of the matrix is
      *  a fair upper limit to use for mallocv().  */
      else if (mxIsNumeric(prhi) && mxGetNumberOfDimensions(prhi) == 2
            && mxGetM(prhi) == 1 && mxGetN(prhi) > 0) {
         double *pdd,*pdde;      /* Ptrs to access list items */
         long iis,iie,iii;       /* Range definition */
         pdd = mxGetPr(prhi);    /* N > 0 already checked above */
         pdde = pdd + (N = mxGetN(prhi));
         prSel = purls->psel = (GDSel *)mallocv(
            N * sizeof(GDSel), "Numeric Selectors");
         while (pdd < pdde) {
            if (*pdd < 0.0) abexitm(312,
               "Range must begin with positive selector.");
            iis = (long)(*pdd++ + Round);
            if (pdd >= pdde || *pdd >= 0.0) {   /* Got a number */
               prSel->ks = GDS_NUM;
               prSel->us.it = iis;
               ++purls->nitms;
               } /* End storing single number */
            else {                              /* Got a range */
               iie = (long)(Round - *pdd++);
               iii = (pdd >= pdde || *pdd >= 0.0) ? 1 :
                     (long)(Round - *pdd++);
               if (iis > iie)
                  abexitm(313,"Range must be in ascending order.");
               if (iii <= 0)
                  abexitm(314,"Range increment must be nonzero.");
               prSel->ks = GDS_RANGE;
               prSel->us.r.is = iis;
               prSel->us.r.ie = iie;
               prSel->us.r.ii = iii;
               purls->nitms += 1 + (iie - iis)/iii;
               } /* End storing range */
            ++prSel;
            } /* End selector scan */
         } /* End storing numerical selectors */
      else
         abexitm(315,"A selector argument has bad type or dimension.");
      purls->nsel = prSel - purls->psel;
      } /* End iarg loop */

/* Construct a selector tree to control data selection at each level.
*  This tree is anchored on a dummy root node, permitting ggdmatch() to
*  treat level 1 the same as any other level when it has to backtrack.
*/

   prNode0 = (GDNode *)callocv(1, sizeof(GDNode),
      "Root selector node");
   if (!ggdmatch(prNode0, pfNode1))    /* Recursive call */
      abexitm(316,"Requested data items not found in GRAFDATA file.");

/*---------------------------------------------------------------------*
*                              STAGE IV                                *
*  Read the data, extract the requested variable items, and return.    *
*---------------------------------------------------------------------*/

/* Delete the file selector nodes, which are no longer needed,
*  and allocate a buffer to read the variable of interest.  */

   gdiclear(&pfNode1);
   Litem = pv->vlen * pv->vdim;
   pD = mallocv(Litem, "Data buffer");

/* Locate the time selectors */

   purls = pUrls0 + DataLevel;

/* Construct a matrix to hold the output */

   /* Allocate a dimension array big enough to hold the worst case
   *  max number of dimensions.  Determine the number of dims and
   *  store the size of each in the dims array.  Because MATLAB
   *  uses the FORTRAN convention (left indices faster moving),
   *  the data levels are examined in reverse order here.  */
   ndim = 0;
   dims = (int *)mallocv((DataLevel+3)*sizeof(int), "Dimension array");
   if ((lmatx = pv->vdim) > 1)
      dims[ndim++] = lmatx;
   for (ilvl=DataLevel-1; ilvl>=0; ilvl--) if (pUrls0[ilvl].nitms > 1)
      lmatx *= (dims[ndim++] = pUrls0[ilvl].nitms);
   if (nits > 1)
      dims[ndim++] = nits;
   if (purls->nitms > 1)
      dims[ndim++] = purls->nitms;
   /* MATLAB demands at least two dimensions */
   while (ndim < 2) dims[ndim++] = 1;
   /* Size of data to zero for missing times */
   lmatx *= sizeof(double);

   plhs[0] = mxCreateNumericArray(ndim, dims, mxDOUBLE_CLASS, mxREAL);
   pR = mxGetPr(plhs[0]);

/* Loop over requested time points.
*  (It is not necessary to check that all time selectors
*  are numeric--this has already been done above.)  */

   prSelE = (prSel = purls->psel) + purls->nsel;
   oreci = orec1;
   for (lasttime=(-1); prSel<prSelE; prSel++) {
      long it,ite,iti;           /* Time controls */
      if (prSel->ks == GDS_NUM)     /* Single time point */
         it = ite = prSel->us.it, iti = 1;
      else {                        /* Range of time values */
         it  = prSel->us.r.is;
         ite = prSel->us.r.ie;
         iti = prSel->us.r.ii;
         }
      for ( ; it<=ite; it+=iti) {

/* Search for the current time point in the file.
*
*  N.B.  It is not correct to calculate where the time point should be
*  and seek to that position in the file--might end up in the wrong
*  segment.  Instead, search forward from the present position if the
*  new time point is later than the previous one, otherwise search
*  from the start of the segment.  This will work even if the appli-
*  cation writes only selected time points and <= nits cycles.  */

         if (it <= lasttime) oreci = orec1;
         do {
            rfseek(pgdf, oreci, SEEKABS, ABORT);
            oreci += lrecl;
            filetime = (long)rfri4(pgdf);
            if (filetime < 0)
               abexitm(317,"Requested time point not found.");
            } while (filetime != it);
         lasttime = it;

/* Retrieve data from nits cycles at this time point.
*  If they are not all recorded, zero the missing ones.  */

         for (iits=nits; ; ) {
            ggdfetch(prNode0->pls, rftell(pgdf));
            if (--iits <= 0) break;
            /* Check time value of next record */
            rfseek(pgdf, oreci, SEEKABS, ABORT);
            filetime = (long)rfri4(pgdf);
            if (filetime != it) {
               /* Missing nits--zero output */
               long lmissing = iits * lmatx;
               memset((char *)pR, 0, lmissing);
               pR = (double *)((char *)pR + lmissing);
               break;   /* Without updating oreci */
               }
            oreci += lrecl;
            } /* End loop over nits cycles */

         } /* End loop over time range */

      } /* End loop over time selectors */

/* Close the input file and clean up memory */

   ggdclean();

   } /* End getgd3() */

