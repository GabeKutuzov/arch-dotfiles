/* (c) Copyright 1997-2017, The Rockefeller University *11114* */
/* $Id: gditools.c 1 2009-12-29 23:04:48Z  $ */
/***********************************************************************
*                              GDITOOLS                                *
*                                                                      *
*  This is a small collection of routines for parsing GRAFDATA files.  *
*  These routines use only the subset of ROCKS routines that are       *
*  compatible with an environment that does not contain the card/page  *
*  interface.  For use in a MATLAB mex file, the calling program must  *
*  supply appropriate substitute versions of abexit, abexitm, callocv, *
*  freev, mallocv, and reallocv.  See getgd3.c for examples.           *
*                                                                      *
*  Some of the routines take arguments that are pointers to "callback" *
*  routines that may be used to print or perform other operations on   *
*  the information being processed.  The arguments to each callback    *
*  routine are documented along with the routine that calls it.  Any   *
*  callback can be skipped by passing a NULL pointer to its caller.    *
*                                                                      *
*  Notes regarding 64-bit version:                                     *
*  (1) This will allow fixed-point arguments of length 8, but these    *
*      will not be supported in 32-bit version, therefore should       *
*      not be written into GRAFDATA files, at least until this         *
*      restriction is removed.                                         *
*                                                                      *
*  Abend codes in the range 240-259 are reserved for this package.     *
*----------------------------------------------------------------------*
*  V1A, 09/17/97, GNR - Newly written                                  *
*  V1B, 12/04/97, GNR - Make it safe for reuse while still in memory   *
*  Rev, 05/22/99, GNR - Use new swapping scheme                        *
*  V2A, 09/16/08, GNR - Mods to allow 32-bit or 64-bit compilation     *
*  ==>, 12/29/09, GNR - Last mod before committing to svn repository   *
*  Rev, 01/13/16, GNR - Inrease INIT_DESCLEN 132 --> 160 (new CDSIZE)  *
*  Rev, 03/01/17, GNR - Add recognition of GDV_FixMZero type           *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sysdef.h"
#include "rksubs.h"
#include "swap.h"
#include "graphd.h"           /* Brings in rfdef.h */

/* Program configuration parameters */
#define DFLT_BLKSIZE 4096     /* Default input buffer size */
#define INIT_DESCLEN  160     /* Initial maximal descriptor length */
#if INIT_DESCLEN < CDSIZE
#error "INIT_DESCLEN must be at least " CDSIZE
#endif
#define INIT_NSEL       8     /* Initial no. of selectors to alloc */
#define INIT_NVAR       4     /* Initial no. of variables to alloc */
#define MAX_FIELDLEN   16     /* Max field length */
#define IC  (RK_IORF+RK_CTST) /* Integer input code for sibcdin */
#define MF1 (MAX_FIELDLEN-1)  /* Max field length code for sibcdin */

/* Private global data */
static void (*pgcbparse)(char *pd, int ld);
static struct RFdef *pgdf;    /* Ptr to RFdef for input file */
static char  ***lnames;       /* Ptr to array of ptrs by level to
                              *  ptrs by index to level names */
static char  *plnm0;          /* Ptr to actual level names */
static char  *pd;             /* Ptr to data buffer */
static GDVar *pv0;            /* Ptr to variable information */
static int   *pvhash;         /* Ptr to variable hash table */
static size_t lpd;            /* Current length of pd buffer */
static size_t reclen;         /* Length of data record */
static enum GDTERM ctrig;     /* Continuation trigger */
static int jf;                /* Offset in field parse */
static int ld;                /* Length of a descriptor */
static int nits;              /* Number of iterations/cycle */
static int nlvls,nvars;       /* Number of levels, variables */
static int nvhash;            /* Size of variable hash table */


/***********************************************************************
*            PRIVATE ROUTINES USED ONLY INSIDE THIS MODULE             *
***********************************************************************/

/*=====================================================================*
*                                 rdd                                  *
*                                                                      *
*  Read a data descriptor and return its length.  If the current pd    *
*  buffer is too short, make it longer.  If a data or eof record is    *
*  found, return the negative time value but do not read any data.     *
*                                                                      *
*  Arguments:     None.                                                *
*=====================================================================*/

static int rdd(void) {

   si32 lrec = -rfri4(pgdf);
   if (lrec > 0 && lrec < -EndMark) {
      size_t szrec = (size_t)lrec;
      if (szrec > lpd)   {
         lpd += lpd; if (lpd < szrec) lpd = szrec;
         pd = (char *)reallocv(pd, lpd, "Descriptor buffer"); }
      rfread(pgdf, pd, szrec, ABORT);
      }
   return (int)lrec;

   } /* End rdd() */


/*=====================================================================*
*                                vhash                                 *
*                                                                      *
*  Get hash value corresponding to a variable name.  Hash function     *
*  modified from "Expert C Programming" by P. van der Linden, p. 54.   *
*                                                                      *
*  Argument                                                            *
*     vname       Name of variable to be hashed.                       *
*=====================================================================*/

static int vhash(char *vname) {

   ui32 overflow, ih = 0;
   while (*vname) {
      ih = (ih << 4) + *vname++;
      if ((overflow = ih & 0xf0000000U) != 0)
         ih ^= overflow | (overflow >> 24);
      }
   return (int)(ih % nvhash);

   } /* End vhash() */


/*=====================================================================*
*                    setfield, getfield, rdcntinu                      *
*                                                                      *
*  Call setfield to initialize parsing of selector cards.              *
*                                                                      *
*  Arguments:                                                          *
*     pgcbp       Ptr to callback routine to use for continuations,    *
*                 of form void (*pgcbp)(char *pd, int ld).             *
*  Returns:       Nothing.                                             *
*                                                                      *
*  Call getfield to parse off one field from a selector card.  The     *
*  value returned is a code from the enum GDTERM indicating the type   *
*  of delimiter that was found.  Fields longer than MAX_FIELDLEN will  *
*  be silently truncated.  ROCKS continuation rules will be followed,  *
*  except that quoted strings and comments are not supported.  As it   *
*  should be under these rules, a comma, +, or - followed by blanks    *
*  does not induce a continuation.                                     *
*                                                                      *
*  Argument:                                                           *
*     field       Address where field should be returned.              *
*  Returns:       Type of delimiter that was found.                    *
*                                                                      *
*  Call rdcntinu to read a continuation of a selector card.            *
*                                                                      *
*  Arguments:     None.                                                *
*  Returns:       Text of continuation card is in pd array.            *
*=====================================================================*/

static void rdcntinu(void) {

   ld = rdd();
   if (pgcbparse) pgcbparse(pd, ld);   /* Print it or whatever */
   if (pd[0] != ' ' && pd[0] != '\t')
      abexitm(259, "Expected selector continuation not found.");
   jf = 0;

   } /* End rdcntinu() */

static void setfield(void (*pgcbp)(char *pd, int ld)) {

   pgcbparse = pgcbp;
   ctrig = Blank;
   jf = 0;

   } /* End setfield() */

static enum GDTERM getfield(char *field) {

   char *pf = field, *pfe = field + MAX_FIELDLEN;

   /* If already at EndOfCard, do not keep scanning */
   if (ctrig == EndOfCard) goto GetFieldNoInput;

   /* Begin with plus or minus that ended previous field */
   if      (ctrig == Plus)    *pf++ = '+';
   else if (ctrig == Minus)   *pf++ = '-';

   /* If a continuation is expected, read it now */
   if (ctrig != Blank && jf >= ld) rdcntinu();

   /* Skip over any leading whitespace */
   while (jf < ld && (pd[jf] == ' ' || pd[jf] == '\t')) jf++;

   /* Move data to output until either a delimiter is reached,
   *  or the field gets full, or the input runs out.  */
   while (jf < ld) {
      if (pd[jf] == ' ' || pd[jf] == '\t') {
         *pf = '\0'; jf++; return (ctrig = Blank); }
      if (pd[jf] == ',') {
         *pf = '\0'; jf++; return (ctrig = Comma); }
      if (pd[jf] == '+' && pf > field) {
         *pf = '\0'; jf++; return (ctrig = Plus); }
      if (pd[jf] == '-' && pf > field) {
         *pf = '\0'; jf++; return (ctrig = Minus); }
      if (pd[jf] == '\\')
         rdcntinu();
      else if (pf < pfe)
         *pf++ = pd[jf++];
      } /* End while */

GetFieldNoInput:
   /* Ran out of input */
   *pf = '\0'; return (ctrig = EndOfCard);

   } /* End getfield() */


/***********************************************************************
*               PUBLIC ROUTINES EXPORTED BY THIS MODULE                *
***********************************************************************/

/*=====================================================================*
*                               gdiopen                                *
*                                                                      *
*  Given a file name, allocate and open an RFdef path to the file.     *
*  Allocate an initial data buffer for use by subsequent routines.     *
*  Errors are terminal and result in calls to abexit() or abexitm().   *
*  This routine must be called before using the rest of this module.   *
*                                                                      *
*  Argument:                                                           *
*     fname       Name of the GRAFDATA file to be opened.              *
*  Returns:       Ptr to open RFdef structure for accessing the file.  *
*                 A copy is retained within the package for use by     *
*                 other routines.                                      *
*=====================================================================*/

struct RFdef *gdiopen(char *fname) {

   pgdf = rfallo(fname, READ, BINARY, SEQUENTIAL, TOP, LOOKAHEAD,
      REWIND, RELEASE_BUFF, DFLT_BLKSIZE, IGNORE, IGNORE, ABORT);
   rfopen(pgdf, fname, READ, BINARY, SEQUENTIAL, TOP, LOOKAHEAD,
      REWIND, RELEASE_BUFF, DFLT_BLKSIZE, IGNORE, IGNORE, ABORT);
   pd = (char *)reallocv(pd, lpd = INIT_DESCLEN, "Descriptor buffer");
   return pgdf;

   } /* End gdiopen() */


/*=====================================================================*
*                               gdickid                                *
*                                                                      *
*  Reads the initial record of a GRAFDATA file and checks the id and   *
*  version.  Reads and saves the number of levels and variables.       *
*                                                                      *
*  Arguments:     None                                                 *
*  Returns:       Ptr to the header in case caller wants to analyze    *
*                 or print it.  Remains valid until next gdi call.     *
*=====================================================================*/

char *gdickid(void) {

   rfread(pgdf, pd, LIdRec, ABORT);
   if (memcmp(pd, "GRAFDATA V3A", 12))
      abexitm(240, "File does not appear to be V.3 GRAFDATA.");
   nlvls = sibcdin(IC+RK_QPOS+RK_ZTST+3, pd+12);
   nvars = sibcdin(IC+RK_QPOS+RK_ZTST+3, pd+16);
   return pd;

   } /* End gdickid() */


/*=====================================================================*
*                              gdinlvls                                *
*                                                                      *
*  Returns the number of levels in a GRAFDATA file as read by the      *
*  previous call to gdickid.                                           *
*                                                                      *
*  Arguments:     None                                                 *
*  Returns:       nlvls, the number of selection levels in the file.   *
*=====================================================================*/

int gdinlvls(void) {

   return nlvls;

   } /* End gdinlvls() */


/*=====================================================================*
*                              gdinvars                                *
*                                                                      *
*  Returns the number of variables in a GRAFDATA file as read by the   *
*  previous call to gdickid.                                           *
*                                                                      *
*  Arguments:     None                                                 *
*  Returns:       nvars, the number of variables possibly in the file. *
*=====================================================================*/

int gdinvars(void) {

   return nvars;

   } /* End gdinvars() */


/*=====================================================================*
*                              gdititle                                *
*                                                                      *
*  Reads the title record of a GRAFDATA file and returns a ptr to it.  *
*                                                                      *
*  Arguments:     None                                                 *
*  Returns:       Ptr to the title record.  Valid until next gdi call. *
*=====================================================================*/

char *gdititle(void) {

   rfread(pgdf, pd, LTitRec, ABORT);
   return pd;

   } /* End gdititle() */


/*=====================================================================*
*                               gditime                                *
*                                                                      *
*  Reads the time stamp of a GRAFDATA file and returns a ptr to it.    *
*                                                                      *
*  Arguments:     None                                                 *
*  Returns:       Ptr to timestamp record.  Valid until next gdi call. *
*=====================================================================*/

char *gditime(void) {

   rfread(pgdf, pd, LTimeStamp, ABORT);
   return pd;

   } /* End gditime() */


/*=====================================================================*
*                              gdilvlnm                                *
*                                                                      *
*  Reads and stores the level names from a GRAFDATA header.            *
*                                                                      *
*  Argument:                                                           *
*     gcblevnm    Ptr to a "callback" routine (described below) that   *
*                 is called once for each level name in the file.      *
*  Returns:       lnames, a ptr to an array of nlvls ptrs to NULL-     *
*                 terminated arrays of ptrs to strings containing      *
*                 the individual level names.                          *
*  Callback Synopsis:  void gcblevnm(int ilvl, int klvl, char *name)   *
*     ilvl        Number of the level being processed.                 *
*     klvl        GDL_FIRST (name first) + GDL_NLAST (name not last).  *
*     name        Ptr to a level name.                                 *
*=====================================================================*/

char ***gdilvlnm(void (*gcblevnm)(int ilvl, int klvl, char *name)) {

   char ***ppplnm,**pplnm,*plnm; /* Ptrs used to build data struct */
   size_t bookmark=rftell(pgdf); /* Current location in input file */
   size_t llnm = 0;              /* Lengths of names */
   int ilvl,klvl;                /* Level name index and kind */
   int lplnm = 0;                /* Lengths of pointers */
   char field[MAX_FIELDLEN+1];   /* One level name field */

/* First count total length of pointers and names */

   lplnm = nlvls /* Outer ptrs */ + nlvls /* Array terminators */ ;

   for (ilvl=0; ilvl<nlvls; ilvl++) {
      if ((ld = rdd()) <= 0)
         abexitm(241, "Found data while reading level names.");
      setfield(NULL);
      do {
         getfield(field);
         if (ctrig != Comma && ctrig != EndOfCard)
            abexitm(241, "Improper punctuation of level names.");
         ++lplnm;
         llnm += strnlen(field, LLevName) + 1;
         } while (ctrig != EndOfCard);
      }

/* Allocate space for pointers and names and back up the input file */

   if (lnames) freev((char *)lnames, "Level name directory");
   ppplnm = lnames = (char ***)mallocv(lplnm*sizeof(char *),
      "Level name directory");
   pplnm  = (char  **)(lnames + nlvls);
   if (plnm0) freev(plnm0, "Level names");
   plnm   = plnm0  = mallocv(llnm, "Level names");

   rfseek(pgdf, bookmark, SEEKABS, ABORT);

/* Pass through a second time and store all the names.
*  Invoke callback routine to print the names.
*  The error checking does not need to be repeated.  */

   for (ilvl=0; ilvl<nlvls; ilvl++) {
      ld = rdd();
      setfield(NULL);
      *ppplnm++ = pplnm;
      klvl = GDL_FIRST | GDL_NLAST;
      do {
         getfield(field);
         llnm = strnlen(field, LLevName) + 1;
         if (ctrig == EndOfCard) klvl &= ~GDL_NLAST;
         *pplnm++ = plnm;
         memcpy(plnm, field, llnm);
         if (gcblevnm) gcblevnm(ilvl+1, klvl, field);
         plnm += llnm;
         klvl = GDL_NLAST;
         } while (ctrig != EndOfCard);
      *pplnm++ = NULL;     /* Mark end of list for this level */
      } /* End level names loop */

   return lnames;

   } /* End gdilvlnm() */


/*=====================================================================*
*                               gdivars                                *
*                                                                      *
*  Reads and stores variable descriptor info from a GRAFDATA file.     *
*  Checks that type and length codes are acceptable to the package.    *
*  On the fly, makes a little hash table to speed up later lookups.    *
*  The table is made big enough so that an empty slot is always found. *
*                                                                      *
*  Argument:                                                           *
*     gcbvar      Ptr to a "callback" routine (described below) that   *
*                 is called once for each variable descriptor found    *
*                 in the file.  It can be used to print the data and/  *
*                 or to fill in the vic codes with application values. *
*  Returns:       Ptr to array of nvars GDVar structs describing       *
*                 the variables in the current file.                   *
*  Callback Synopsis:  void gcbvar(int ivar, char *pd, int ld,         *
*                 GDVar *pv)                                           *
*     ivar        Number of the variable being processed.              *
*     pd          Ptr to the data portion of the variable descriptor   *
*                 record for this variable.                            *
*     ld          Length of pd text string.                            *
*     pv          Ptr to the GDVar constructed for this variable.      *
*                 At gcbvar call, all fields but vic are filled in.    *
*=====================================================================*/

GDVar *gdivars(void (*gcbvar)(int ivar, char *pd, int ld, GDVar *pv)) {

   GDVar *pv;              /* Ptr to variable information */
   int ivar;

   if (pv0) {
      freev(pv0, "Variable info");
      freev(pvhash, "Variable table"); }
   pv0 = (GDVar *)mallocv(nvars*sizeof(GDVar), "Variable info");
   nvhash = (nvars + nvars + 23);
   pvhash = (int *)callocv(nvhash, sizeof(int), "Variable table");

   for (ivar=1,pv=pv0; ivar<=nvars; ivar++,pv++) {
      int ih, kbad;

      /* Read one data descriptor record */
      if ((ld = rdd()) <= 0)
         abexitm(242, "Found data while reading variable defs.");

      /* Input the data from the variable descriptor */
      memcpy(pv->vname, pd, LVarName);
      pv->vtype = (short)sibcdin(IC+RK_QPOS+3, pd + LVarName);
      pv->vlev  = (short)sibcdin(IC+RK_QPOS+RK_ZTST+3, NULL);
      pv->vscl  = (short)sibcdin(IC+RK_QPOS+3, NULL);
      pv->vlen  = (short)sibcdin(IC+RK_QPOS+RK_ZTST+3, NULL);
      pv->vdim  =        sibcdin(IC+RK_QPOS+7, NULL);

      /* Truncate the name to a standard C string */
      for (ih=1; ih<LVarName; ih++)
         if (pv->vname[ih] == ' ') { pv->vname[ih] = '\0'; break; }

      /* Check for types and lengths that the package can't handle */
      switch (pv->vtype) {
      case GDV_Fix:
      case GDV_UFix:
      case GDV_FixMZero:
         kbad = pv->vlen > sizeof(long);
         break;
      case GDV_Real:
         kbad = pv->vlen != sizeof(float);
         break;
      case GDV_Dble:
         kbad = pv->vlen != sizeof(double);
         break;
      case GDV_Color:
         kbad = pv->vlen > 3;
         break;
      default:
         kbad = TRUE;
         } /* End length checking */
      if (kbad)
         abexitm(243, ssprintf(NULL, "Type or length of variable %" qLVN
            "s is invalid.", pv->vname));
      if (pv->vdim == 0) pv->vdim = 1;
      ih = vhash(pv->vname);
      while (pvhash[ih])
         { ih += 1; if (ih >= nvhash) ih = 0; }
      pvhash[ih] = ivar;

      if (gcbvar) gcbvar(ivar, pd, ld, pv);
      } /* End variable definition loop */

   return pv0;

   } /* End gdivars() */


/*=====================================================================*
*                               gdiqlvl                                *
*                                                                      *
*  Matches a string against a set of possible level names using the    *
*  minimal abbreviation algorithm of the ROCKS smatch() function,      *
*  but ignoring a final 'S' in the name unless it is needed to break   *
*  an ambiguity.  Terminates if name not found.                        *
*                                                                      *
*  Arguments:                                                          *
*     item        Ptr to the data string to be matched.                *
*     levnms      Ptr to array of ptrs to the possible names, which    *
*                 should end with a NULL pointer.                      *
*  Returns:       Ordinal number of the level id in the given list,    *
*                 counting from 1, terminates if no match found.       *
*=====================================================================*/

int gdiqlvl(char *item, char **levnms) {

   char *plnm;                /* Ptr to level name being considered */
   int ik;                    /* Position of this match */
   int retval = 0;            /* Position of best match */
   int best_match = 0;        /* Longest match */
   int tie_match;             /* TRUE if tie match */

   for (ik=0; plnm=levnms[ik]; ik++) {    /* Assignment intended */
      register int mc;        /* Match count */
      for (mc=0; item[mc]; mc++) {
         char upitmc = toupper((int)item[mc]);
         if (!plnm[mc]) {     /* At end of level name */
            if (upitmc == 'S' && !item[mc+1]) break;
            goto NextName; }
         if (upitmc != toupper((int)plnm[mc])) goto NextName;
         } /* End of item */
      /* Return at once if there is an exact match */
      if (!plnm[mc]) return (ik+1);
      /* Otherwise, determine best match */
      if (mc > best_match) {
         best_match = mc;
         tie_match = FALSE;
         retval = ik; }
      else if (mc == best_match) tie_match = TRUE;
NextName: ;
      }
   if (best_match && !tie_match) return (retval+1);
   abexitm(254, ssprintf(NULL, "Level name %" qLLN
      "s not found in header.", item));
#ifndef GCC
   return 0;         /* Never gets here--avoids warning */
#endif
   } /* End gdiqlvl() */


/*=====================================================================*
*                               gdiqvar                                *
*                                                                      *
*  Obtains the variable index corresponding to a given variable name.  *
*  gdiqvar() must not be called until after gdivars has been called.   *
*                                                                      *
*  Arguments:                                                          *
*     varnm       Name of variable to be looked up.                    *
*  Returns:       Variable index for this variable.                    *
*=====================================================================*/

int gdiqvar(char *varnm) {

   int ih,iv;

   ih = vhash(varnm);
   while (iv = pvhash[ih]) {  /* Assignment intended */
      if (!strncmp(varnm, pv0[iv-1].vname, LVarName)) break;
      ih += 1; if (ih >= nvhash) ih = 0;
      }
   if (!iv)
      abexitm(257, ssprintf(NULL, "Undefined variable %" qLVN
         "s on data selector.", varnm));
   return iv;

   } /* End gdiqvar() */


/*=====================================================================*
*                               gdiqseg                                *
*                                                                      *
*  Reads the current header record and generates an error if it is not *
*  a SEGMENT record.  Returns the number of the segment just found.    *
*                                                                      *
*  Arguments:     None.                                                *
*  Returns:       Current segment number.                              *
*=====================================================================*/

int gdiqseg(void) {

   if (rdd() < 24 || memcmp(pd, "SEGMENT", 7) != 0)
      abexitm(244, "Expected segment header, not found.");
   return (int)sibcdin(IC+RK_QPOS+4, pd+7);

   } /* End gdiqseg() */


/*=====================================================================*
*                              gdigoseg                                *
*                                                                      *
*  Accesses a requested segment in a GRAFDATA file.  Generates an      *
*  error if the read location is not at a SEGMENT record.  On return,  *
*  read location is just after the requested SEGMENT record.           *
*                                                                      *
*  Arguments:                                                          *
*     iseg        Number of the segment to be located.                 *
*  Returns:       Nothing.                                             *
*=====================================================================*/

void gdigoseg(int iseg) {

   size_t lseg;               /* Length of data in current segment */
   si32 lrec;                 /* Length of next record */
   int jseg;                  /* Current segment number */

   while ((jseg = gdiqseg()) != iseg) {
      if (jseg > iseg) {      /* Move backwards */
         size_t oprvseg = (size_t)sibcdin(IC+RK_QPOS+11, pd+12);
         if (oprvseg == 0) abexitm(245, "GRAFDATA file "
            "does not contain requested segment.");
         rfseek(pgdf, oprvseg, SEEKABS, ABORT);
         }
      else {                  /* Move forwards */
         /* Scan forward to data length record */
         do ld = rdd();
            while (ld < 6 || memcmp(pd, "LENGTH", 6) != 0);
         lseg = (size_t)sibcdin(IC+RK_QPOS+RK_ZTST+11, pd+6);

         /* Skip forward until reach a new SEGMENT header */
         for (;;) {
            lrec = rfri4(pgdf);
            if (lrec == EndMark) abexitm(245, "GRAFDATA file "
               "does not contain requested segment.");
            if (lrec < 0) break;
            rfseek(pgdf, lseg, SEEKREL, ABORT);
            } /* End skipping to segment header */

         /* Back up over the length just read so rdd can reread it */
         rfseek(pgdf, -sizeof(lrec), SEEKREL, ABORT);
         }
      } /* End while */

   } /* End gdigoseg() */


/*=====================================================================*
*                              gdiparse                                *
*                                                                      *
*  Parses the data selectors in a segment header and builds a GDNode   *
*  tree. Reads, checks, and stores information from the LENGTH record. *
*  Arguments:                                                          *
*     pptree      Ptr to a ptr where address of new tree will be       *
*                 stored.                                              *
*     gcbparse    Ptr to a "callback" routine (described below) that   *
*                 is called once for each data selector (or continu-   *
*                 ation of one) found in the current segment.  This    *
*                 routine can be used to print the data and/or to do   *
*                 any other analysis required by the application.      *
*  Returns:       Maximum size of any one item.                        *
*  Callback Synopsis:  void gcbparse(char *pd, int ld)                 *
*     pd          Ptr to the data portion of the selector record.      *
*     ld          Length of pd text string.                            *
*=====================================================================*/

size_t gdiparse(GDNode **pptree, void (*gcbparse)(char *pd, int ld)) {

   GDNode *plsel,*pnsel;         /* Ptr to latest, new selector */
   GDVar *pv;                    /* Ptr to variable information */

   size_t lldata;                /* Length of data from one node */
   size_t mxitsz;                /* Max size of one data item */
   int ilvl;                     /* Level of current node */
   int nallo;                    /* Number variables allocated
                                 *  at current node */
   char field[MAX_FIELDLEN+1];   /* One selector field */

   gdiclear(pptree);             /* Remove any previous tree here */
   reclen = mxitsz = 0;
   for (;;) {                    /* Read data selectors */
      if ((ld = rdd()) <= 0)
         abexitm(246, "Found data before LENGTH record.");
      if (gcbparse) gcbparse(pd, ld);

/* If get LENGTH record, this is end of segment header.  Check
*  that length recorded in header matches length of data items.  */

      if (!memcmp(pd, "LENGTH", 6)) {
         size_t treclen;
         if (!*pptree || !reclen) abexitm(247, "Reached end of "
            "segment header, but no data items were defined.");
         treclen = (size_t)sibcdin(IC+RK_QPOS+RK_ZTST+11, pd+6);
         nits    = (int)sibcdin(IC+RK_QPOS+RK_ZTST+8, pd+23);
         if (treclen != reclen) abexitm(248, "Record length re"
            "corded in header does not match length of data items.");
         break;
         }

/* Otherwise, this is a node.  Parse it and build tree */

      setfield(gcbparse);
      if (getfield(field) != Blank)
         abexitm(249, "Expecting selector id followed by blank.");
      if (getfield(field) != Blank)
         abexitm(250, "Expecting level num followed by blank.");
      ilvl = (int)sibcdin(IC+RK_QPOS+RK_ZTST+MF1, field);
      pnsel = (GDNode *)callocv(1, sizeof(GDNode), "Data node");
      pnsel->level = ilvl;

/* Insert the new node at the appropriate point in the tree.
*  Abexit 252 should never happen, so not much detail given.  */

      if (!*pptree) {
         if (ilvl != 1)
            abexitm(251, "Expecting selector at level 1.");
         *pptree = plsel = pnsel; }
      else for (;;) {
         if (plsel->level == ilvl) {
            /* Found one at same level--become pns successor */
            if (plsel->pns) abexitm(252, "Invalid level structure--"
               "two successors at same level.");
            pnsel->odata = plsel->odata + plsel->ldata*plsel->nitms;
            plsel->pns = pnsel; }
         else if (plsel->level == ilvl - 1) {
            /* Found one at next lower level--become pls successor.
            *  N.B.  Child data are interleaved after each occurrence
            *  of parent data--do not multiply ldata by nitms here.  */
            if (plsel->pls) abexitm(252, "Invalid level structure--"
               "two successors to one parent.");
            pnsel->odata = plsel->odata + plsel->ldata;
            plsel->pls = pnsel; }
         else {
            /* Look back until find a worthy place to attach */
            if (!plsel->par) abexitm(252, "Invalid level structure--"
                  "no legal attachment for this node.");
            plsel = plsel->par;
            continue; }
         pnsel->par = plsel;
         plsel = pnsel;
         break;
         } /* End back search for */

/* Parse level name and see if it matches one for this level.
*  Save it for use later in printing.  */

      if (getfield(field) != Blank)
         abexitm(253, "Expecting level name followed by blank.");
      pnsel->levid = gdiqlvl(field, lnames[ilvl-1]);

/* Parse selectors */

      do {
         GDSel *ps;           /* Ptr to selector information */
         getfield(field);
         /* Allocate space as needed */
         if (pnsel->nsel >= pnsel->nsallo) {
            if (pnsel->nsallo == 0) {
               pnsel->nsallo = INIT_NSEL;
               pnsel->psel = (GDSel *)mallocv(
                  INIT_NSEL*sizeof(GDSel), "Data selectors");
               }
            else {
               pnsel->nsallo += pnsel->nsallo;
               pnsel->psel = (GDSel *)reallocv(pnsel->psel,
                  pnsel->nsallo*sizeof(GDSel), "Data selectors");
               }
            }
         ps = pnsel->psel + pnsel->nsel;
         /* Handle ranges--putting this case first avoids
         *  need for punctuation tests on names--instead,
         *  error shows up as bad chars in number field... */
         if (ctrig == Plus || ctrig == Minus) {
            long inrange;     /* Size of this range */
            ps->ks = GDS_RANGE;
            ps->us.r.is = sibcdin(IC+RK_QPOS+MF1, field);
            ps->us.r.ii = 1;
            do {
               if (ctrig == Plus) {
                  getfield(field);
                  ps->us.r.ii = sibcdin(IC+RK_QPOS+RK_ZTST+MF1, field);
                  }
               else {
                  getfield(field);
                  ps->us.r.ie = -sibcdin(IC+RK_ZTST+MF1, field);
                  }
               } while (ctrig == Plus || ctrig == Minus);
            if ((inrange = ps->us.r.ie - ps->us.r.is) < 0)
               abexitm(255, ssprintf(NULL, "End of range %10ld is "
                  "below start %10ld.", ps->us.r.ie, ps->us.r.is));
            pnsel->nitms += (1 + inrange/ps->us.r.ii);
            }
         /* Handle names */
         else if (cntrl(field)) {
            ps->ks = GDS_NAME;
            strncpy(ps->us.in, field, LSelName);
            pnsel->nitms += 1L;
            }
         /* Handle individual numeric selectors */
         else {
            ps->ks = GDS_NUM;
            ps->us.it = sibcdin(IC+RK_QPOS+MF1, field);
            pnsel->nitms += 1L;
            }
         pnsel->nsel += 1;
         } while (ctrig != Blank && ctrig != EndOfCard);

/* Parse variables and identify them using hash table */

      lldata = 0;
      nallo = 0;
      while (ctrig != EndOfCard) {
         size_t isz;
         int  iv;
         getfield(field);
         if (ctrig != Comma && ctrig != EndOfCard)
            abexitm(256, "Bad punctuation in variable list.");
         iv = gdiqvar(field);
         /* Allocate space as needed */
         if (pnsel->nvar >= nallo) {
            if (nallo == 0) {
               nallo = INIT_NVAR;
               pnsel->pvin = (unsigned short *)mallocv(
                  INIT_NVAR*sizeof(short), "Variable lists");
               }
            else {
               nallo += nallo;
               pnsel->pvin = (unsigned short *)reallocv(pnsel->pvin,
                  nallo*sizeof(short), "Variable lists");
               }
            }
         pnsel->pvin[pnsel->nvar++] = iv - 1;

         /* Increment record length accordingly */
         pv = pv0 + iv - 1;
         lldata += (isz = (size_t)pv->vlen * (size_t)pv->vdim);
         if (mxitsz < isz) mxitsz = isz;
         } /* End parsing variable names */

/* Propagate the length contribution to parental nodes */

      if (lldata) {
         for (;;) {
            int mlev = pnsel->level;   /* Marking level */
            pnsel->ldata += lldata;
            lldata *= pnsel->nitms;
            if (mlev <= 1) break;
            do {
               pnsel = pnsel->par;
               } while (pnsel->level >= mlev);
            } /* End backtracking to level 1 parent */
         reclen += lldata;
         } /* End if lldata */
      } /* End reading selector cards */

   return mxitsz;

   } /* End gdiparse() */


/*=====================================================================*
*                               gdinits                                *
*                                                                      *
*  Returns the number of inner iterations in a trial cycle of the      *
*  model as read by the previous call to gdiparse.                     *
*                                                                      *
*  Arguments:     None                                                 *
*  Returns:       nits, the number of inner iterations per cycle.      *
*=====================================================================*/

int gdinits(void) {

   return nits;

   } /* End gdinits() */


/*=====================================================================*
*                               gdirecl                                *
*                                                                      *
*  Returns the length of a data record in a GRAFDATA file as read by   *
*  the previous call to gdiparse.                                      *
*                                                                      *
*  Arguments:     None                                                 *
*  Returns:       reclen, the length of a data record in this segment. *
*=====================================================================*/

size_t gdirecl(void) {

   return reclen;

   } /* End gdirecl() */


/*=====================================================================*
*                              gdiclear                                *
*                                                                      *
*  If there is an existing selector tree, removes it.                  *
*  Argument:                                                           *
*     pptree      Ptr to a ptr to the tree that is to be cleared.      *
*  Returns:       Nothing.                                             *
*=====================================================================*/

void gdiclear(GDNode **pptree) {

   GDNode *ptree;

   while (ptree = *pptree) {     /* Assignment intended */

      /* Clear lower-level stuff recursively */
      if (ptree->pls) gdiclear(&ptree->pls);

      /* Now the present node can be deleted */
      *pptree = ptree->pns;
      if (ptree->psel) freev(ptree->psel, "Data selectors");
      if (ptree->pvin) freev(ptree->pvin, "Variable lists");
      freev(ptree, "Data node");
      }

   } /* End gdiclear() */


/*=====================================================================*
*                              gdiclose                                *
*                                                                      *
*  Closes an open GRAFDATA file and releases all storage allocated     *
*  in connection with processing that file.                            *
*                                                                      *
*  Arguments:     None                                                 *
*  Returns:       Nothing                                              *
*=====================================================================*/

void gdiclose(void) {

   if (pgdf) {                /* Close file, release buffer */
      rfclose(pgdf, REWIND, RELEASE_BUFF, ABORT);
      if (pgdf->fname) freev(pgdf->fname, "File name");
      freev(pgdf, "File definition");
      pgdf = NULL; }
   if (pd) {                  /* Release header buffer */
      freev(pd, "Data buffer");
      pd = NULL, lpd = 0; }
   if (lnames) {
      freev((char *)lnames, "Level name directory");
      lnames = NULL; }
   if (plnm0) {
      freev(plnm0, "Level names");
      plnm0 = NULL; }
   if (pv0) {
      freev(pv0, "Variable info");      pv0 = NULL;
      freev(pvhash, "Variable table");  pvhash = NULL;
      }

   } /* End gdiclose() */

