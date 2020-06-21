/* (c) Copyright 1996-2016, The Rockefeller University *21113* */
/***********************************************************************
*                           NXDR, Version 2                            *
*                                                                      *
*  The purpose of this program is to prepare conversion tables that    *
*  can be compiled into an application, allowing the application to    *
*  manipulate shared memory arrays and to use nncom to convert data    *
*  automatically between in-core format and a standard binary data     *
*  format for portable file and interprocessor message construction.   *
*                                                                      *
*  The program scans the declarations in a user's C program and        *
*  uses this information to prepare two output files:  A .c file       *
*  containing conversion tables for a list of specified object         *
*  types and a .h file that user routines can use to access these      *
*  tables.  The format of the conversion tables is described in        *
*  detail in the nxdr2.doc documentation file.                         *
*                                                                      *
*  In short, the .c output file contains (1) a copy of the -f input    *
*  file that specifies inclusion of the application's header files,    *
*  so that the tables can include 'sizeof' information based on the    *
*  declarations in the headers, (2) the actual conversion table,       *
*  and (3) a table giving the addresses of user-written conversion     *
*  routines for unions, so they can be accessed in the first table     *
*  based on indexes rather than pointers, making the lengths of the    *
*  entries in the first table independent of pointer length.  The      *
*  names of the two tables are controlled by the -t command-line       *
*  option, but by default are NXDRTT and NXDRUT, respectively.         *
*                                                                      *
*  The .h output file contains extern statements pointing to the       *
*  two tables in the .c file and "#define" statements giving the       *
*  offsets | sign bit of the information for each object type in       *
*  the NXDRTT table.  The name of the preprocessor variable defined    *
*  for each object is the name of the object concatenated onto the     *
*  constant string "IX".                                               *
*                                                                      *
*  The program does not attempt to consolidate conversion tables for   *
*  objects that have the same data layout.  The user can minimize      *
*  conversion table redundancy by using structure tags or typedefs     *
*  rather than anonymous structs to incorporate objects that have      *
*  identical definitions.                                              *
*                                                                      *
*  Wish list:                                                          *
*    (1) Consolidate conversion tables for objects that have the       *
*        same data layout, e.g. structs and union elements.            *
*                                                                      *
*    Error codes in the range 330-359 are assigned to this program     *
*                                                                      *
*----------------------------------------------------------------------*
*  Based on nxdr, originally written by Ariel Ben Porath               *
*  Rev, 10/03/96, GNR - Corrected errors parsing hexadecimal and       *
*                       floating-point constants, removed dependency   *
*                       output file and some debug codes.              *
*  V2A, 09/12/99, GNR - Complete rewrite.  Output files now specify    *
*                       tables via offsets rather than pointers. Keep  *
*                       track of full length of each type and type of  *
*                       largest element in a complex type rather than  *
*                       length of largest element, input -f and -b     *
*                       options via files rather than command-line     *
*                       lists, full support for long longs, recursive  *
*                       parsing to permit embedded struct/union in a   *
*                       typedef, better error reporting, use of ROCKS  *
*                       formatted I/O and error reporting.             *
*  V2B, 05/29/00, GNR - Change defs in .h table to offset+0x80000000L  *
*  Rev, 04/22/09, GNR - Fix bug writing "When compiled with -DPAR"     *
*                       message to pho file after it was closed.       *
*  Rev, 01/06/11, GNR - Fix segfault on typedef an incomplete struct   *
*  V2C, 10/03/15, GNR - Add -t command-line option                     *
*  Rev, 12/07/15, GNR - Workaround: treat __builtin_va_list as void *  *
*  V2D, 02/05/16, GNR - Add '-m32' and '-m64' options.                 *
*  V2E, 08/14/16, GNR - Provide NULL NXDRUT for serial programs        *
*  Rev, 09/25/16, GNR - Provide alignment info for unions              *
***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#define MAIN
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "nxdr2.h"
#ifdef UNIX
#include <sys/wait.h>
#endif

/*---------------------------------------------------------------------*
*              Global information for the whole program                *
*                (All implicitly initialized to NULL.)                 *
*---------------------------------------------------------------------*/

FILE   *pif;            /* Output .i file from C preprocessor */
item   *pavit;          /* Ptr to first available item record */
type   *pavtp;          /* Ptr to first available type record */
type   *headtyprec;     /* Head of linked type list */
type   **tailtyprec;    /* Ptr to place to add next type */
keyrec *headkeyrec;     /* Head of linked list of key types */
char   *pitlens;        /* Ptr to type lengths table to be used */
struct htbl *htynr;     /* Head of hash table for tynrecs */
struct htbl *hkeyr;     /* Head of hash table for keyrecs */
char   *ccmd;           /* C command - used to compile */
long   siln[NCONTEXT];  /* Seek positions of last 3 input lines */
idfr   cname;           /* Conversion name */
idfr   dname;           /* Declarator name */
idfr   token;           /* Token currently being processed */
idfr   dbgtoken;        /* Wait for debugger at this token */
size_t lcname;          /* High-water length of cname */
size_t ldname;          /* High-water length of dname */
size_t ltoken;          /* Length of current token */
size_t mxlname;         /* Maximum length of an identifier name */
size_t mxltoken;        /* Maximum length of any token */
int    oldstdout;       /* Holds stdout handle during forks */
int    ttype;           /* Type of this token */

/* Following items used only in this file--not extern'd in nxdr2.h */
FILE   *pco, *pho;      /* Output .c and .h files */
type   *headptut;       /* Head of linked union type list */
type   **tailptut;      /* Ptr to place to add next union type */
char   *obuf;           /* Buffer for generating .h output */
char   *ofmt;           /* Format for #define in output .h file */
long   ioff;            /* Offset to be assigned to next table */
long   joff;            /* Offset to be assigned to next unicvtf */
int    kverb;           /* Flag for verbose output */

/*---------------------------------------------------------------------*
*                intrtypes, intrtypecodes, itemlengths                 *
*                                                                      *
* intrtypes:                                                           *
*  An array of type name strings that is used by Typinit() to create   *
*  the initial data structure for the built-in C scalar types.  The    *
*  array is terminated with a NULL pointer.  Sizes for determination   *
*  of lengths of largest item in and total size of a compound type     *
*  depend on the system where the using application is run and are     *
*  selected according to the '-m' options or default to sizes on       *
*  on system where nxdr2 is compiled.  A pointer to the selected       *
*  table is stored in global pitlens.  The type "void" is given the    *
*  special code '#' which produces an error if it is instantiated by   *
*  anything other than a pointer.  The derived types 'si16', 'ui16',   *
*  'si32', 'ui32', 'si64', 'ui64', and 'size_t' are included so con-   *
*  version tables of defined length can be generated for them.         *
*     Entries are also included for pointers, function, unions,        *
*  structures and and functions for easy access to their codes and     *
*  lengths when needed.  These have names that would be illegal as     *
*  names of 'C' objects.                                               *
*                                                                      *
* intrtypecodes:                                                       *
*  An array of single character type codes for the types given in the  *
*  intrtypes array, arranged in the same order.  The lower-case code   *
*  in the last entry of each output table is changed to upper case.    *
*                                                                      *
* itlendflt, itlen32, itlen64:                                         *
*  Arrays of single characters containing bzw the lengths of the base  *
*  types for default, m32, and m64 source code analyses.  These are    *
*  the lengths for alignment determination in the machine where a      *
*  message is received and may not be the same as the message length   *
*  or the length in the originating machine.  Additional tables may be *
*  added, with corresponding command line '-m' options to select them, *
*  to implement this package on systems with different memory models.  *
*---------------------------------------------------------------------*/

/* N.B.  If order of items is changed, must also change corresponding
*  defined index values in nxdr2.h  */
static char *intrtypes[] = { "=ptr=", "=func=", "=jump=", "=kjump=",
   "=exec=", "void", "__builtin_va_list", "enum", "byte",
   "unsigned char", "char", "unsigned short", "ui16", "short",
   "si16", "unsigned int", "int", "ui32", "si32", "unsigned long",
   "long", "unsigned long long", "ui64", "long long", "si64", "float",
   "double", "size_t", NULL };

/* Codes 'p' for pointer, '%' for function, are generated by a
*  mechanism that does not depend on the type names in intrtypes.  */
static char intrtypecodes[] = "p%jkx##ebbcmmhhuiuinlyywwfdz";

static char itlendflt[] = { sizeof(void *), 0, 0, 0, 0, sizeof(void *),
   sizeof(void *), sizeof(int), sizeof(byte), sizeof(byte),
   sizeof(char), sizeof(short), sizeof(ui16), sizeof(short),
   sizeof(si16), sizeof(int), sizeof(int), sizeof(ui32), sizeof(si32),
   sizeof(long), sizeof(long), sizeof(long long), sizeof(ui64),
   sizeof(long long), sizeof(si64), sizeof(float), sizeof(double),
   sizeof(size_t) };

static char itlen32[] = { 4, 0, 0, 0, 0, 4, 4, 4, 1, 1, 1, 2, 2, 2,
   2, 4, 4, 4, 4, 4, 4, 8, 8, 8, 8, 4, 8, 4 };

static char itlen64[] = { 8, 0, 0, 0, 0, 8, 8, 4, 1, 1, 1, 2, 2, 2,
   2, 4, 4, 4, 4, 8, 8, 8, 8, 8, 8, 4, 8, 4 };

/*---------------------------------------------------------------------*
*              Typhash() -- Hash function for type names               *
*---------------------------------------------------------------------*/

unsigned long Typhash(void *tynm) {
   byte *b = (byte *)tynm;
   unsigned long c,h = 0;
   int s = 0;
   while (c = (unsigned long)(*b++))   /* Assignment intended */
      h ^= c << s, s = (s+3)&15;
   return h;
   } /* End Typhash() */

/*---------------------------------------------------------------------*
*                               Typinit                                *
*                                                                      *
*  This function creates the pre-defined part of the global type data  *
*  structure, i.e. the entries for the C intrinsic data types.  For    *
*  each such type, a tynrec (type name record) is created, added to    *
*  the linked list, and hashed.  A type record with one entry is       *
*  also created, with the appropriate code from the intrtypes table.   *
*                                                                      *
*---------------------------------------------------------------------*/

static void Typinit(void) {

   int    iseq = IT_VOID;     /* Sequence number of current type */
   type   *ptp;               /* Ptr to current type record */
   tynrec *ptyn;              /* Ptr to current type name record */

   tailtyprec = &headtyprec;  /* Where to store next type */

   while (intrtypes[iseq]) {

/* Create a type record for the new type, link it
*  into the headtyprec list, and initialize it.  */

      ptp = allotype(Tk_BASE);
      *tailtyprec = ptp;
      tailtyprec = &ptp->pntp;
      ptp->mxtlen = pitlens[iseq];
      additem(ptp, NULL, 1, iseq);

/* Create a type name record for the new type, initialize
*  it, link it to the type record, and hash it.  */

      ptyn = (tynrec *)callocv(1, sizeof(tynrec) +
         strlen(intrtypes[iseq]), "Type Name Record");
      ptyn->ptype = ptp;      /* Point to new table */
      ptp->ptnm = ptyn;
      strcpy(ptyn->typname, intrtypes[iseq]);
      hashadd(htynr, ptyn);
      iseq += 1;
      }

   } /* End typinit() */

/*---------------------------------------------------------------------*
*                           ReadObjectList                             *
*                                                                      *
*  Read the object list file supplied by the user and use it to        *
*  generate a linked list of keyrec structures, one for each object,   *
*  starting at headkeyrec, and a hash table of object names starting   *
*  at hkeyr.  Returns the number of objects in the list.               *
*                                                                      *
*  Parses the file using the same nexttkn() routine used to parse the  *
*  C code--only the open file descriptor in pif is different.          *
*---------------------------------------------------------------------*/

static int ReadObjectList(void) {

   keyrec *pnew,**ppnew = &headkeyrec;
   int lnm;                      /* Length of an object name */
   int nobj = 0;                 /* Number of objects */

/* Parse the input line into words */

   while (TRUE) {
      nexttkn();                 /* Get next input token */

      /* Test for end of file */
      if (ttype == TKN_EOF) break;

      /* Test for valid syntax */
      if (token_is(",")) continue;
      if (ttype != TKN_IDF) abexitm(341, ssprintf(NULL,
         "Invalid data in object list file at %s", token));

      /* Make a keyrec, move the name into it, and doctor it */
      if (token_is("struct")) {
         nexttkn();
         pnew = (keyrec *)callocv(1, sizeof(keyrec)+ltoken+5,
            "struct name");
         strcpy(pnew->keyname, "str_");
         strcat(pnew->keyname, token);
         }
      else if (token_is("union")) {
         nexttkn();
         pnew = (keyrec *)callocv(1, sizeof(keyrec)+ltoken+5,
            "union name");
         strcpy(pnew->keyname, "uni_");
         strcat(pnew->keyname, token);
         }
      else if (token_is("enum")) {
         nexttkn();
         pnew = (keyrec *)callocv(1, sizeof(keyrec)+ltoken+5,
            "enum name");
         strcpy(pnew->keyname, "enu_");
         strcat(pnew->keyname, token);
         }
      else {
         pnew = (keyrec *)callocv(1, sizeof(keyrec)+ltoken+1,
            "object name");
         strcpy(pnew->keyname, token);
         }
      /* Keep track of longest name */
      lnm = strlen(pnew->keyname);
      if (lnm > mxlname) mxlname = lnm;
      /* Insert at end of list */
      *ppnew = pnew;
      ppnew = &pnew->nxtkey;
      ++nobj;
      } /* End parsing input file */

/* Make the hash table */

   if (nobj <= 0) abexitm(333,
      "No valid object names found in file specified by -b option");

   hkeyr = hashinit(Typhash, nobj, 0,
      ((keyrec *)0)->keyname - (char *)0,
      (char *)&((keyrec *)0)->hlink - (char *)0);
   for (pnew=headkeyrec; pnew; pnew=pnew->nxtkey) {
      hashadd(hkeyr, pnew);
      }

   return nobj;
   } /* End ReadObjectList() */

/*---------------------------------------------------------------------*
*                              Scanhfile                               *
*                                                                      *
*  This routine parses the output file produced by the C preprocessor  *
*  from the user's -f input file.  On return, the global type records  *
*  have been expanded to include the types defined in the application. *
*---------------------------------------------------------------------*/

static void Scanhfile(void) {

   int esc = FALSE;        /* Flag set after reading an escape char
                           *  in a string ==> skip next character. */
   int instr = FALSE;      /* Flag set when reading tokens enclosed
                           *  within double quotes */

/* Read tokens from input .i file and search for any of "typedef",
*  "struct", "union", or "enum".  When found, call the appropriate
*  Scan routine.  Since these tokens are reserved words, they always
*  introduce a declaration unless inside a quoted string.  So quoted
*  strings are detected and their contents skipped over.  */

GetAnotherToken:
   nexttkn();
   while (TRUE) {

      /* Test for end of file */
      if (ttype == TKN_EOF && !instr)
         break;

      /* Handle quoted strings.  Inside strings, ignore any
      *  token which is neither double quotes nor backslash.  */
      if (token_is("\"")) {      /* If token is double quotes, */
         if (!instr || (instr && !esc))
            instr = !instr;      /* Toggle in-string flag */
         esc = FALSE;
         goto GetAnotherToken;
         }
      if (token_is("\\")) {      /* If token is backslash, */
         esc = !esc;             /* Toggle escape flag */
         goto GetAnotherToken;
         }
      esc = FALSE;
      if (instr)
         goto GetAnotherToken;

      /* Ignore any material that is in parentheses */
      if (token_is("("))
         findmatch(FM_ROUND);

      /* Deal with typedefs, enums, structures, and unions */
      else if (token_is("typedef"))
         Scan_typedef();
      else if (token_is("enum"))
         Scan_enum(TOPLEVEL, NULL);
      else if (token_is("struct"))
         Scan_struct(TOPLEVEL, NULL);
      else if (token_is("union"))
         Scan_union(TOPLEVEL, NULL);

      /* Ignore all other tokens or insert new code here */
      else
         goto GetAnotherToken;
      } /* End while */
   } /* End Scanhfile() */

/*---------------------------------------------------------------------*
*                          SingleRefOptimize                           *
*                                                                      *
*  This routine eliminates type records that are only referenced once, *
*  collapsing their contents into the type record for the container.   *
*  (The test for a type 'X' linker is redundant, but is included for   *
*  safety.)  Some care is needed in updating pfit,plit in the parent:  *
*  if the child is a union, it has no items, so pfit,plit must end up  *
*  NULL.  If the child is not a union, it must be an embedded struct,  *
*  in which case, it has already been encountered and optimized, so    *
*  it is OK to skip over its contents while optimizing the parent.     *
*                                                                      *
*  Success of this procedure depends on the fact that an inner struct  *
*  is not aligned to the border that might be used for outer struct,   *
*  nor is the next item after an embedded struct aligned to anything   *
*  larger than its intrinsic alignment.  This was checked on Linux     *
*  gcc, SunOS acc, and Solaris cc, but might not be true on all        *
*  systems.  If this is ever a problem, the quick and dirty workaround *
*  is to use -z to turn off optimization.                              *
*---------------------------------------------------------------------*/

static void SingleRefOptimize(type *ptp) {
   type *pin;
   item *pit,**ppit = &ptp->pfit;

   while (pit = *ppit) {         /* Assignment intended */
      if (pit->itype == IT_STRUC && (pin = pit->pinctbl) != NULL &&
            pin != ptp && pin->nref <= 1 && pin->plit &&
            !(pin->key & (Tk_BASE|Tk_KEY|Tk_IBD|Tk_INC)) &&
            (!(pin->key & Tk_UNI) || is_simple(ptp))) {

         /* Propagate the characteristics of the vanquished type.
         *  N.B.  Do not propagate the Tk_UNI or Tk_STR bits--
         *  these determine how the type name is written into
         *  the sizeof operator in the output .c file.  */
         *ppit = pin->pfit;
         if (ptp->plit == pit) ptp->plit = pin->plit;
         if (pin->key & Tk_UNI) {
            ptp->pnut = pin->pnut;
            ptp->ninc = pin->ninc;
            }
         else {
            pin->plit->pnit = pit->pnit;
            ppit = &pin->plit->pnit;
            }

         /* Report promotions if requested */
         if (kverb) {
            cryout(RK_P3, "0Promoting ", RK_LN2,
               (pin && pin->ptnm && pin->ptnm->typname) ?
               pin->ptnm->typname : "unnamed object", RK_CCL,
               " into parent ", RK_CCL,
               (ptp && ptp->ptnm && ptp->ptnm->typname) ?
               ptp->ptnm->typname : "anonymous type", RK_CCL, NULL);
            convrt("(P3,' Object nel = ',J1M6,', parent nel = ',J1M6)",
               &pin->nel, &ptp->nel, NULL);
            } /* End -v output */

         ptp->nel += pin->nel - 3;
         if (pin->mxtlen > ptp->mxtlen) ptp->mxtlen = pin->mxtlen;

         /* Free up the unused type and item records */
         freetype(pin);
         freeitem(pit);
         }
      else
         ppit = &pit->pnit;
      }
   } /* End SingleRefOptimize() */

/*---------------------------------------------------------------------*
*                            AssignOffsets                             *
*                                                                      *
*  This routine scans one type definition chain (stored by Scanhfile)  *
*  and calculates the offset where this information will be stored in  *
*  the output NXDRTT table.  It also constructs names for anonymous    *
*  types and union members.  It calls itself recursively when it runs  *
*  into an embedded lower-level object.                                *
*---------------------------------------------------------------------*/

static void AssignOffsets(type *parent, type *ptp) {

   tynrec *ptyn;
   type   *pin;
   item   *pit;

   /* If offset for this object already assigned, just return */
   if (ptp->offset >= 0) return;

   /* If the object has no name, not even a #declarator name,
   *  there is an error in the program logic.  */
   if (!ptp || !ptp->ptnm || !ptp->ptnm->typname)
      abexit(359);
   ptyn = ptp->ptnm;

   /* If the definition of the object was never completed,
   *  there is an error in the source program.  */
   if (ptp->key & (Tk_IBD|Tk_INC)) abexitm(353, ssprintf(NULL,
      "Definition of %s was never completed", ptyn->typname));

   /* If object has a temporary name, expand it to the final
   *  form by prepending the name of the parent.  Flag this
   *  situation with the Tk_ANON bit so we can catch later if
   *  an attempt is made to find the size of this type.  */
   if (ptyn->typname[0] == '#') {
      tynrec *potn = parent->ptnm;
      size_t lnm;
      if (!potn) abexitm(359, ssprintf(NULL,
         "No parent name found for %s", ptyn->typname));
      ptyn->typname[0] = '_';
      lnm = strlen(potn->typname) + strlen(ptyn->typname);
      hwmidfr(cname, lcname, lnm, "Name of anonymous object");
      strcpy(cname, potn->typname);
      strcat(cname, ptyn->typname);
      ptyn = ptp->ptnm = reallocv(ptyn, sizeof(tynrec) + lnm,
         "Name of anonymous object");
      strcpy(ptyn->typname, cname);
      ptp->key |= Tk_ANON;
      }

   if (kverb)
      cryout(RK_P3, "0AssignOffsets called for ", RK_LN2,
         ptyn->typname, RK_CCL, NULL);

   /* If object is a union, assign names and offsets to all its
   *  elements.  Skip this for union elements.  */
   if (ptp->pnut && !(ptp->key & Tk_STR)) {
      tynrec *potr;
      size_t lnme,lin,lnm = strlen(ptyn->typname) + 1;
      for (pin=ptp->pnut; pin && pin->offset < 0; pin=pin->pnut) {
         lnme = lnm;
         lin = pin->ninc; if (lin == 0) lin = 1;
         while (lin > 0) { lnme += 1; lin /= 10; }
         potr = (tynrec *)callocv(1, sizeof(tynrec) + lnme,
            "Union Element Type Name Record");
         potr->ptype = pin;
         pin->ptnm  = potr;
         strcpy(potr->typname, ssprintf(NULL, "%s_%d",
            ptyn->typname, (int)pin->ninc));
         AssignOffsets(ptp, pin);
         }
      if (lnme > mxlname) mxlname = lnme;
      ptp->nel += 2;                /* Allow for 'J' codes */
      } /* End processing union */

   else {      /* Not a union */
      /* Scan for embedded structures */
      for (pit=ptp->pfit; pit; pit=pit->pnit)
         if (pin = pit->pinctbl)    /* Assignment intended */
            AssignOffsets(ptp, pin);
      if (!is_simple(ptp)) ptp->nel += 1;    /* Allow for 'S' code */
      }

   /* Assign next slot to current object and update ioff */
   if (kverb)
      convrt("(P3,'   Current ioff is ',J0IL6,', increment will "
         "be ',J0M6)", &ioff, &ptp->nel, NULL);

   ptp->offset = ioff;
   ioff += ptp->nel;

   } /* End AssignOffsets() */

/*---------------------------------------------------------------------*
*                            MakeUnionDefs                             *
*                                                                      *
*  This routine generates '#define' statements in the .h output file   *
*  for members of unions.  It handles two cases:  (1) the calling      *
*  argument points to a union, or (2) one or more anonymous unions     *
*  are embedded inside whatever object is being processed.             *
*---------------------------------------------------------------------*/

static void MakeUnionDefs(type *ptp) {

   tynrec *ptyn;           /* Ptr to type name record for this object */
   type   *pin;            /* Ptr to inner object */
   item   *pit;            /* Ptr to element of object */

   /* If this union has already been processed, just return */
   if (ptp->ninc < 0) return;

   /* If object is a union, make defs for all its elements.
   *  N.B.  Do not test the Tk_UNI bit here--it will be OFF for
   *  a union that was optimized up into a containing typedef.  */
   if (ptp->pnut && !(ptp->key & Tk_STR)) {
      for (pin=ptp->pnut; pin; pin=pin->pnut) {
         unsigned long negoff;
         if ((ptyn = pin->ptnm) == NULL) abexit(357);
         negoff = UI32_SGN | pin->offset;
         sconvrt(obuf, ofmt, ptyn->typname, &negoff, NULL);
         fputs(obuf, pho);
         MakeUnionDefs(pin);
         }
      } /* End processing union */

   /* Scan for embedded unions  */
   for (pit=ptp->pfit; pit; pit=pit->pnit)
      if (pin = pit->pinctbl)    /* Assignment intended */
         MakeUnionDefs(pin);

   ptp->ninc = -1;

   } /* End MakeUnionDefs() */

/*---------------------------------------------------------------------*
*                             WriteLength                              *
*                                                                      *
*  If object is anonymous, use computed length in output table,        *
*  otherwise write a 'sizeof' code to be sure allocs are correct.      *
*---------------------------------------------------------------------*/

static void WriteLength(type *ptp) {

#define COMCOL 37             /* Column where comment starts */

   char *pls;
   int  ils;
   if (!keylkup(ptp->ptnm->typname))
      pls = ssprintf(NULL, "   %ld,", ptp->length);
   else if (ptp->key & Tk_STR)
      pls = ssprintf(NULL, "   sizeof(struct %s),",
         ptp->ptnm->typname+4);
   else if (ptp->key & Tk_UNI)
      pls = ssprintf(NULL, "   sizeof(union %s),",
         ptp->ptnm->typname+4);
   else
      pls = ssprintf(NULL, "   sizeof(%s),", ptp->ptnm->typname);
   ils = strlen(pls);
   if (ils > COMCOL) abexitm(339,ptp->ptnm->typname);
   memset(pls+ils, ' ', COMCOL-ils);
   fputs(pls, pco);
   fputs("/* ", pco);
   fputs(ptp->ptnm->typname, pco);
   fputs(" */\n", pco);

   } /* End WriteLength() */

/*---------------------------------------------------------------------*
*                             WriteTTable                              *
*                                                                      *
*  This routine processes one key type, writing the conversion table   *
*  to the pco output file.  Recursion to handle embedded objects is    *
*  depth-first, matching the algorithm used to precalculate offsets    *
*  in AssignOffsets() above.  Three scans are needed, since we cannot  *
*  insert one table in the middle of another.  We could use ssprintf() *
*  here to save a lot of little calls to fputs(), but this way there   *
*  is no limit on the length of object names.                          *
*                                                                      *
*  Note, 02/20/16, GNR - Calculated lengths are sometimes short, so I  *
*  have gone back to outputtig 'sizeof' to be sure they are accurate,  *
*  but using calculated lengths for embedded types, which will never   *
*  (we hope) be used as allocation sizes.                              *
*---------------------------------------------------------------------*/

static void WriteTTable(type *parent, type *ptp) {

   type   *pin;
   item   *pit;
   char   *pname;
   int    isunion;
   byte   ekey;         /* Effective key */

   /* If this object has been already written, just return */
   if (ptp->nref < 0) return;

   /* If object is a union, write tables for all its elements.
   *  N.B.  Do not test the Tk_UNI bit here--it will be OFF for
   *  a union that was optimized up into a containing typedef.  */
   isunion = ptp->pnut && !(ptp->key & Tk_STR);
   if (isunion) {
      for (pin=ptp->pnut; pin; pin=pin->pnut) {
         WriteTTable(ptp, pin);
         if (pin->length > ptp->length) ptp->length = pin->length;
         }
      } /* End processing union */

   /* Scan for embedded structures */
   for (pit=ptp->pfit; pit; pit=pit->pnit)
      if (pin = pit->pinctbl) {  /* Assignment intended */
         WriteTTable(ptp, pin);
         ptp->length = keyszup(ptp,pin->mxtlen) + pin->length;
         }

   /* Make entry for size of this type.  Note that if a struct or
   *  union in a typedef is optimized out, the Tk_UNI or Tk_STR bits
   *  will remain off and the typedef name will be used in the sizeof
   *  operator that is written out.  If Tk_UEL key is set, this is a
   *  member of a union.  Use the name of the parent.  */
   ekey = ptp->key;
   pname = ptp->ptnm->typname;

   if (isunion) {
      /* Type is a union--put out max length and conversion index
      *  Rev, 07/11/16, GNR - Insert 'J' code to indicate type!
      *  Rev, 09/25/16, GNR - Move alignment info to word after 'J'. */
      WriteLength(ptp);
      fputs("   ( 1 << 8 ) | 'J',\n", pco);
      fputs(ssprintf(NULL, "   ( %ld << 8) | %d,\n",
         (ptp->joffset = joff++), (int)ptp->mxtlen), pco);
      /* Thread a new linked list of unions */
      *tailptut = ptp;
      tailptut = &ptp->ut.ptut;
      }
   else if (ekey & Tk_ENU) {
      /* Type is an enumeration--just needs a length record */
      fputs(ssprintf(NULL,"   %d,\t\t\t\t/* enum ",
         (int)pitlens[IT_ENUM]), pco);
      fputs(pname, pco);
      fputs(" */\n", pco);
      }
   else if (ekey & (Tk_STR|Tk_UEL|Tk_TDF)) {
      if (ekey & Tk_TDF) {
         /* Got a typedef -- needs included element scan below
         *  to handle condensed embedded structures */
         WriteLength(ptp);
         if (ptp->pfit != ptp->plit) fputs(ssprintf(NULL,
            "   ( %d << 8 ) | 's',\n", ptp->mxtlen), pco);
         }
      else {
         /* Got a structure or union element.  Output total
         *  length, then mxtlen */
         if (ptp->length <= 0) abexitm(354, ssprintf(NULL,
            "Conversion table needs size of %s", pname));
         WriteLength(ptp);
         if (!(ekey & Tk_UEL) && (ptp->pfit != ptp->plit))
            fputs(ssprintf(NULL,
            "   ( %d << 8 ) | 's',\n", ptp->mxtlen), pco);
         }

      /* Scan included elements and put out code for each.
      *  Change last code to upper case.  If code is 'X'
      *  pointing to a struct, append offset of referenced
      *  table.  If code is 'X' pointing to a union, create
      *  a 'J' entry, eliminating one level of recursion.  */
      for (pit=ptp->pfit; pit; pit=pit->pnit) {
         type *pxtp = pit->pinctbl;
         int  code, ityp = pit->itype;
         char codec;    /* Code as a character */
         if (ityp == IT_FUNC)
            abexitm(345, ssprintf(NULL,
               "Identifier %s refers to a function", pname));
         if (ityp == IT_VOID)
            abexitm(355, ssprintf(NULL, "Identifier %s contains "
               " an item of type void, not a pointer", pname));
         code = (int)intrtypecodes[ityp];
         if (pit == ptp->plit) code = toupper(code);
         codec = (char)code;
         fputs(ssprintf(NULL, "   ( %d << 8 ) | '%1s',\n",
            (int)pit->count, &codec), pco);
         if (ityp == IT_JUMP)       /* Print jump table index */
            fputs(ssprintf(NULL, "   ( %ld << 8) | %d,\n",
               pxtp->joffset, (int)pxtp->mxtlen), pco);
         else if (ityp == IT_STRUC)
            fputs(ssprintf(NULL, "   ( %ld ),\n", pxtp->offset), pco);
         } /* End loop over items included in structure */
      } /* End output for structure type */
   else {
      /* Type is a simple type, just describe it */
      pit = ptp->pfit;
      char tcode = (char)toupper(intrtypecodes[pit->itype]);
      fputs(ssprintf(NULL,"   ( %d ),\t\t\t\t/* ",
         (int)pitlens[pit->itype]), pco);
      fputs(pname, pco);
      fputs(" */\n", pco);
      fputs(ssprintf(NULL,"   ( %ld << 8 ) | '%1s',\n",
         pit->count, &tcode), pco);
      }

   /* Set flag to signal object already processed */
   ptp->nref = -1;

   } /* End WriteTTable() */

/*=====================================================================*
*                         nxdr2 main program                           *
*=====================================================================*/

int main(int argc, char *argv[], char *envp[]) {

   FILE   *ptmpc;          /* File for preprocessor script */
   keyrec *pkr;            /* Ptr to current keyrec */
   tynrec *ptyn;           /* Ptr to current tynrec */
   type   *ptp;            /* Ptr to current object */
   char   *bfnm = NULL;    /* Object list file name */
   char   *ffnm = NULL;    /* Include file name */
   char   *ifnm = NULL;    /* Name of .i file gen'd from ffnm */
   char   *cfnm = NULL;    /* Output C file name */
   char   *hfnm = NULL;    /* Output Header file name */
   char   *fext;           /* Ptr to file extension */
   char   *tpfx = NULL;    /* Table name prefix */
   char   *ttnm;           /* Global conversion table name */
   char   *utnm;           /* Union conversion table name */
   int    kzopt = ON;      /* Flag for single-ref optimization */
   int    iarg;            /* Argument scan index */
   int    lnm;             /* Length of a name */
   int    nobj;            /* Number of objects */

/* Establish rocks environment */

   nopage(RK_PGNONE);

/* Allocate space for temp names */

   cname = mallocv((lcname = MAXIDFR)+1, "Conversion name");
   dname = mallocv((ldname = MAXIDFR)+1, "Declarator name");
   token = mallocv((mxltoken = MAXIDFR)+1, "Program token");

/* Set to use default base-type length table */

   pitlens = itlendflt;

/* Scan command-line arguments */

   if (argc < 7 || argc > 20) {
      cryout(RK_P1, "0***Synopsis: ", RK_LN3, argv[0], RK_CCL,
         " -f incfile -b objfile -c cmd", RK_CCL,
         "    [-m32] [-m64] [-t tpfx] [-o outfile] [-h hdrfile]",
         RK_LN0+RK_FLUSH, NULL);
      abexit(330); }

   for (iarg=1; iarg<argc; ++iarg) {
      char optc;

      if (!strcmp(argv[iarg], "-m32")) {
         pitlens = itlen32; continue; }
      if (!strcmp(argv[iarg], "-m64")) {
         pitlens = itlen64; continue; }

      optc = (argv[iarg][0] == '-' && argv[iarg][2] == '\0') ?
         argv[iarg][1] : 'X';
      switch (optc) {

         case 'b':
            lnm = strlen(argv[++iarg]);
            bfnm = mallocv(lnm+1, "-b object list file name");
            strcpy(bfnm, argv[iarg]);
            break;

         case 'c': {
            int i;
            lnm = strlen(argv[++iarg]);
            ccmd = mallocv(lnm+1, "-c compiler command");
            strcpy(ccmd, argv[iarg]);

            /* Eliminate '-c' option from ccmd */
            for (i=0; i<lnm-2; i++)
               if (ccmd[i]==' ' && ccmd[i+1]=='-' && ccmd[i+2]=='c')
                  ccmd[i+1] = ccmd[i+2] = ' ';
            break;
            }

         case 'f':
            lnm = strlen(argv[++iarg]);
            ffnm = mallocv(lnm+1, "-f input file name");
            strcpy(ffnm, argv[iarg]);
            break;

         case 'g': {
            /* Undocumented debugging option.  Create an infinate
            *  loop waiting for the debugger.  After attaching dbx
            *  to the process, use it to set iii to zero, thus
            *  leaving the loop. */
            volatile int iii = 1;
            while (iii)
               sleep(1);
            break;
            }

         case 'h':
            lnm = strlen(argv[++iarg]);
            hfnm = mallocv(lnm+3, "-h output header file name");
            strcpy(hfnm, argv[iarg]);
            if (hfnm[lnm-2]!='.' || hfnm[lnm-1]!='h')
               strcat(hfnm, ".h");
            break;

         case 'o':
            lnm = strlen(argv[++iarg]);
            cfnm = mallocv(lnm+3, "-o output C file name");
            strcpy(cfnm, argv[iarg]);
            if (cfnm[lnm-2]!='.' || cfnm[lnm-1]!='c')
               strcat(cfnm, ".c");
            break;

         case 's':
            /* Undocumented debugging option.  Stop when a
            *  token matching the following string is found.  */
            lnm = strlen(argv[++iarg]);
            dbgtoken = mallocv(lnm+1, "debug token");
            strcpy(dbgtoken, argv[iarg]);
            break;

         case 't':
            lnm = strlen(argv[++iarg]);
            tpfx = mallocv(lnm+1, "-t table name prefix");
            strcpy(tpfx, argv[iarg]);
            break;

         case 'v':
            /* Undocumented debugging option.  Print a verbose
            *  report describing the program data analysis.  */
            kverb = ON;
            break;

         case 'z':
            /* Undocumented debugging option.  Turn off
            *  optimization of singly-referenced structures.  */
            kzopt = OFF;
            break;

         default:
            abexitm(331, ssprintf(NULL, "Unrecognized cmd line "
               "switch option %s", argv[iarg]));
         } /* End option switch */
      } /* End option loop */

/* Supply default names if output files not specified */

   if (!cfnm)
      cfnm = "./nxdrtab.c";
   if (!hfnm)
      hfnm = "./nxdrdef.h";

/* Verify include file and object file specified */

   if (!ffnm) abexitm(332,
      "An input file must be specified (-f option)");
   if (!bfnm) abexitm(333,
      "An object list file must be specified (-b option)");

/* Generate table names from default or tpfx option */

   {  int ltt,lut;
      if (!tpfx) tpfx = qDTPFX;
      lnm = strlen(tpfx);
      ltt = lnm + strlen(qTTSFX) + 1;
      lut = lnm + strlen(qUTSFX) + 1;
      ttnm = mallocv(ltt + lut, "output table names");
      utnm = ttnm + ltt;
      strcpy(ttnm, tpfx); strcat(ttnm, qTTSFX);
      strcpy(utnm, tpfx); strcat(utnm, qUTSFX);
      }

/* Print out names of output files to be generated */

   cryout(RK_P1, " Output C file name is ", RK_LN1,
      cfnm, RK_CCL, NULL);
   cryout(RK_P1, " Output header file name is ", RK_LN1,
      hfnm, RK_CCL, NULL);

/* Read the object list file and construct the headkeyrec list */

   if ((pif = fopen(bfnm, "r")) == NULL)
      abexitme(340, "Unable to open object list file (-b option)");
   nobj = ReadObjectList();
   fclose(pif);

/* Generate a name for the preprocessor output file.
*  N.B.  With acc and perhaps other compilers, the input file
*  must have a .c suffix or it will not be preprocessed.  */

   ifnm = mallocv(strlen(ffnm)+3, "preprocessor file name");
   strcpy(ifnm, ffnm);
   if (fext = strrchr(ifnm, '.')) {    /* Assignment intended */
      if (!strcmp(fext, ".c") || !strcmp(fext, ".h"))
         *fext = '\0';
      }
   strcat(ifnm, ".i");

/* Create and run a script file to preprocess the include file */

   if ((ptmpc = fopen("$runcpp$", "w")) == NULL)
      abexitme(334, "Cannot create cpp script file");
#ifdef GCC
   fprintf(ptmpc, "#!/bin/csh\n%s -E -P -ansi -o %s %s\n",
      ccmd, ifnm, ffnm);
#else
   fprintf(ptmpc, "#!/bin/csh\n%s -P %s\n", ccmd, ffnm);
#endif
   fclose(ptmpc);
   cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
   chmod("$runcpp$", 0700);
   switch (iarg = fork()) {
      case -1:
         abexitme(335, "Cannot fork cpp script");
      case 0:
         killstdout();
         execve("$runcpp$", argv, envp);
         regainstdout();
         abexitme(335, "Returned from execing cpp script");
      default:
         waitpid(iarg, &iarg, 0);
      }
   unlink("$runcpp$");

/* After this point, we are just interested in the name of
*  the compiler, not the -D options.  */

   {  int i = 0;
      /* Find the first non-white char in ccmd */
      while (ccmd[i] && isspace(ccmd[i])) ++i;
      /* Find the first white space after that */
      while (ccmd[i] && !isspace(ccmd[i])) ++i;
      /* And terminate the string right there */
      ccmd[i] = '\0';
      } /* End local scope */

/* Make sure it is possible to open the output files (pho first,
*  see below) before doing too much more work.  */

   if ((pho = fopen(hfnm,"w")) == NULL)
      abexitme(337, "Cannot open output .h file");
   if ((pco  = fopen(cfnm, "w")) == NULL)
      abexitme(336, "Cannot open output .c file");

/* Make a hash table for fast access to the tynrec list.  Allow
*  space for intrinsic and requested types, plus 50% of requested
*  types to allow for embedded unions, unnamed structs, etc.  The
*  hash table library functions will automatically add more space
*  if needed later.  */

   htynr = hashinit(Typhash,
      nobj + ((nobj + sizeof(intrtypes)/sizeof(char *))>>1),
      0,
      ((tynrec *)0)->typname - (char *)0,
      (char *)&((tynrec *)0)->hlink - (char *)0);

/* Initialize the headtynrec chain with the intrinsic types */

   Typinit();

/* Open output file generated by the preprocessor and parse it
*  to extract information on key types */

   if ((pif = fopen(ifnm, "r")) == NULL)
      abexitme(338, "Cannot open .i file");
   Scanhfile();
   fclose(pif);

/* If -v option was given, report what Scanhfile found.
*  This is purposefully done before SingleRefOptimize
*  so that we can see if basic parsing is correct.  */

   if (kverb) {
      for (ptp=headtyprec; ptp; ptp=ptp->pntp) {
         tynrec *pun;
         type   *put;
         item   *pit;
         int    ik,kk;
         static char *knames[] = { "Base ", "Key ", "Struct ",
            "Union ", "UnionEl ", "Enum ", "Typedef ",
            "Anonymous ", "Incomplete ", "InDef ", "Defined " };
         ptyn = ptp->ptnm;
         if (ptyn)
            cryout(RK_P3, "0Type ", RK_LN2, ptyn->typname, RK_CCL,
               " has keys: ", RK_CCL, NULL);
         else
            cryout(RK_P3, "0Unnamed type has keys: ", RK_LN2, NULL);
         for (ik=0,kk=1; ik<11; ik+=1,kk<<=1) if (ptp->key & kk)
            cryout(RK_P3, knames[ik], RK_CCL, NULL);
         convrt("(P3,'    nel = ',J0M5,', ninc = ',J0M5,', "
            "mxtlen = ',J0IC5)", &ptp->nel, &ptp->ninc,
            &ptp->mxtlen, NULL);
         if (ptp->pfit) {
            cryout(RK_P3,"    Members of this object are:",
               RK_LN1, NULL);
            for (pit=ptp->pfit; pit; pit=pit->pnit) {
               ik = pit->itype;
               convrt("(P3,'       type = ',A1,', count = ',J0IL6,"
                  "', align = ',J0IC5)", intrtypecodes+ik,
                  &pit->count, pitlens+ik, NULL);
               if ((put=pit->pinctbl) && (pun=put->ptnm))
                  cryout(RK_P3, ", pointing to ", RK_CCL,
                     pun->typname, RK_CCL, NULL);
               }
            } /* End listing object members */
         } /* End loop over type records */
         /* Flush output--this is a debugging tool */
         cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
      } /* End -v report */

/* Single-reference optimization:  If there is only one reference to
*  some object, this object is not a key object or union element, and
*  the container is not a union, then the type record for the object
*  can be telescoped into the type record for the container.  */

   if (kzopt) {
      for (ptp=headtyprec; ptp; ptp=ptp->pntp)
         if (!(ptp->key & (Tk_BASE|Tk_KEY|Tk_UEL|Tk_IBD|Tk_INC)))
            SingleRefOptimize(ptp);
      if (kverb)
         /* Flush output--this is a debugging tool */
         cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
      }

/* Match up user's keys with type definitions found in the scan.
*  Build a name for each anonymous type.  Calculate final table
*  offset for each type, key or anonymous.  Terminate if some
*  key types were not found.  */

   nobj = 0;               /* Count unmatched key types */
   for (pkr=headkeyrec; pkr; pkr=pkr->nxtkey) {
      if ((ptyn = tyrlkup(pkr->keyname)) &&  /* Assignment intended */
            (ptp = ptyn->ptype) &&           /* Assignment intended */
            (ptp->key & Tk_DEF)) {
         AssignOffsets(NULL, pkr->typdata = ptp);
         }
      else {
         if (nobj++ == 0)
            cryout(RK_P1+2, "0***Following key names not found in "
            "program being analyzed:", RK_LN2, " ", RK_LN1, NULL);
         cryout(RK_P1+1, pkr->keyname, RK_CCL, " ", 1, NULL);
         }
      } /* End loop over key types */

   if (kverb)
      /* Flush output--this is a debugging tool */
      cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);
   if (nobj) abexit(344);

/*---------------------------------------------------------------------*
*                     Generate the ouput .h file                       *
*---------------------------------------------------------------------*/

/* Write out some explanatory comments and the externs for the tables.
*  N.B.  On a parallel computer, we must output the extern for NXDRUT
*  even if there are no unions, to satisfy the reference in nncom.  */

   fputs("/* This file is generated automatically by nxdr2.  It\n"
         "*  contains extern statements and #defines needed to\n"
         "*  access tables (in generated .c file) that are used\n"
         "*  to convert binary data types to a standard format\n"
         "*  for interprocessor messages and external files. */\n"
         "\n", pho);
   fputs("extern long ", pho);
   fputs(ttnm, pho);
   fputs("[];\n"
         "extern unicvtf *", pho);
   fputs(utnm, pho);
   fputs("[];\n"
         "\n", pho);

/* Generate a '#define' for each kind of object for which the
*  application must be able to access the conversion table in
*  NXDRTT.  This includes named key types and all elements of
*  each union nested within a key type, whether named or not.
*  It does not include named or anonymous structs nested in a
*  key type, which only need access via 'X' codes, not by name.
*/

   obuf = mallocv(mxlname + 25 + 33, "MakePoundDef buffer");
   ofmt = obuf + mxlname + 25;
   lnm  = mxlname + 1;
   sconvrt(ofmt,"(15H(10H#define IXAJ0I3,13H,2H0xZJ8,1H\n))",
      &lnm,NULL);

   for (pkr=headkeyrec; pkr; pkr=pkr->nxtkey) {
      unsigned long negoff;
      ptp = pkr->typdata;  /* Can't fail--tested above */

      /* Make def for this object */
      negoff = UI32_SGN | ptp->offset;
      sconvrt(obuf, ofmt, pkr->keyname, &negoff, NULL);
      fputs(obuf, pho);

      /* Make defs for recursively included union members */
      MakeUnionDefs(ptp);
      }

/* Close the .h output file */

   fclose(pho);

/*---------------------------------------------------------------------*
*                     Generate the output .c file                      *
*---------------------------------------------------------------------*/

/* Generate the header part of the output .c file.
*  See notes above re generation of NXDRUT.  */

   fputs("/* This file is generated automatically by nxdr2.  It\n"
         "*  contains data conversion tables for interprocessor\n"
         "*  messaging.  Do not edit by hand!  */\n"
         "\n", pco);
   fputs("/* When compiled with -DPAR, the application must\n"
         "*  provide a definition of unicvtf, the interface\n"
         "*  to union conversion routines, in some header\n"
         "*  file that is included in the -f input file. */\n"
         "\n", pco);
   fputs("#include <stddef.h>\n"
         "#include <stdlib.h>\n"
         "#include <stdio.h>\n"
         "\n", pco);

/* Get user's include list by copying the ffnm file */

   {  FILE *phi;
      char incbuf[CDSIZE+1];
      if ((phi = fopen(ffnm, "r")) == NULL)
         abexitme(348, "Cannot open file specified by -f option");
      while (fgets(incbuf, CDSIZE+1, phi))
         fputs(incbuf, pco);
      if (ferror(phi))
         abexitme(348, "Error reading file specified by -f option");
      fputs("\n", pco);
      fclose(phi);
      } /* End local scope */

/* Now that all pointers have been removed from the conversion
*  tables, the code previously included here by ABP to deal with
*  a bug in the compiler supplied with the Inmos dev. kit D4219
*  has been removed.  */

/* Recursively build the actual conversion tables.  Make a
*  new threaded list of unions for use in building NXDRUT.  */

   fputs("long ", pco);
   fputs(ttnm, pco);
   fputs("[] = {\n", pco);

   joff = 0;               /* Counter for NXDRUT offset */
   tailptut = &headptut;
   for (pkr=headkeyrec; pkr; pkr=pkr->nxtkey)
      WriteTTable(NULL, pkr->typdata);
   fputs("   };\n\n", pco);
   *tailptut = NULL;       /* Finish off ptut list */

/* Write prototypes for all the NXF routines, then
*  write the table of pointers to the NXF routines.
*  The table must exist even if it is empty.  */

   fputs("#ifdef PAR\n", pco);
   if (joff > 0) {
      for (ptp=headptut; ptp; ptp=ptp->ut.ptut) {
         if ((ptyn = ptp->ptnm) == NULL) abexit(357);
         fputs("   extern unicvtf NXF", pco);
         fputs(ptyn->typname, pco);
         fputs("_u;\n", pco);
         }
      fputs("\n", pco);
      }
   fputs("unicvtf *", pco);
   fputs(utnm, pco);
   fputs("[] = {\n", pco);
   if (joff > 0) {
      for (ptp=headptut; ptp; ptp=ptp->ut.ptut) {
         ptyn = ptp->ptnm;
         fputs("   NXF", pco);
         fputs(ptyn->typname, pco);
         fputs("_u,\n", pco);
         }
      }
   else
      fputs("   NULL\n", pco);
   fputs("   };\n"
         "#else\n"
         "unicvtf *", pco);
   fputs(utnm, pco);
   fputs("[] = { NULL };\n"
         "#endif\n\n", pco);

/* Close the .c output file.  Since the .c output file #includes
*  the .h output file, it is important that the .c file be newer to
*  prevent an infinite loop when running 'make'.  Opening pco later,
*  doing all the writes later, and closing it later should take care
*  of this.  If there is a problem, move the pco open down below the
*  point where pho is closed.  */

   fclose(pco);
   unlink(ifnm);
   unlink(qKSONAME);
   cryout(RK_P1, "\0", RK_LN0+RK_FLUSH+1, NULL);

   return 0;
   } /* End nxdr2 main */

