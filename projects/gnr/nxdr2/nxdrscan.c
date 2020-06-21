/* (c) Copyright 1999-2018, The Rockefeller University *11113* */
/***********************************************************************
*                             nxdrscan.c                               *
*                                                                      *
*  This file is part of the nxdr2 utility.  It contains routines used  *
*  to parse typedef, enum, structure, and union declarations.  These   *
*  routines replace the generic basetype() routine written by ABP.     *
*  Some effort (imperfect!) is now made to follow the ANSI C language  *
*  grammar so that most program constructs can be handled correctly.   *
*  Syntax errors are only detected if they affect the operation of     *
*  nxdr--users should depend on their C compiler for full diagnostics. *
*                                                                      *
*----------------------------------------------------------------------*
*  Rev, 04/07/08, GNR - Recognize "long unsigned int" from x86_64      *
*  Rev, 09/05/13, GNR - Recognize an array of pointers (*name[nn])     *
*  V2D, 02/05/16, GNR - Object lengths computed, alignment sizes       *
*                       from default or '-m32' or '-m64' options.      *
*  Rev, 08/06/18, GNR - Use ifdef GCC instead of checking ccmd for gcc *
*                       so code will work when ccmd is mpicc           *
***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "nxdr2.h"

/*=====================================================================*
*                             Scan_decls                               *
*                                                                      *
*  Generate table entries for a list of declarators describing a set   *
*  of objects of a known type.  The current token must be the first    *
*  token in the declarator list.                                       *
*                                                                      *
*  Arguments:                                                          *
*     level       Level of declaration--TOPLEVEL, INSTRUCT, INUNION,   *
*                 or INTYPDEF--controls storage of item definition.    *
*     outer       Ptr to type record describing the container object.  *
*     inner       Ptr to type record describing the embedded object.   *
*=====================================================================*/

void Scan_decls(int level, type *outer, type *inner) {

   type *ptp = NULL;       /* Ptr to container object */

/* Process declarators until reach end of statement.
*  N.B.  We want to give an error when a declarator is expected but
*  not found.  This case is not an error at the end of a definition
*  (Tk_IBD bit not yet cleared).  Also, ANSI C explicitly allows the
*  construct "struct-or-union identifier;" (Tk_INC bit set), which
*  is used to invalidate an identifier from an outer scope.  This
*  occurs in certain gcc header files.  Since nxdr2 has no concept
*  of a local scope of a name, it should be safe to just skip over
*  these statements--they should not refer to key types.  Also, for
*  simplicity, we don't trap the case of an incomplete enum with no
*  declarators, which should be an error.  */

   if (token_is(";")) {
      if (!(inner->key & (Tk_IBD|Tk_INC))) {
         show_context();
         abexitm(342, "An identifier was expected, "
            "but end of statement was found");
         }
      }
   else while (1) {

      long nitm = 1;       /* Number of items in array in parens */
      int kptr = FALSE;    /* TRUE if item is any kind of pointer */
      int kppn = FALSE;    /* TRUE if item is ptr in parens */

/* Search for asterisk indicating item is a pointer */

      while (1) {
         ignqualif();
         if (token_is("*")) {
            kptr = TRUE;
            nexttkn();
            ignqualif();
            }
         else
            break;
         }

/* If the name is in parentheses, it may be preceded by an asterisk,
*  indicating it is a pointer.  If there is no asterisk, it is just
*  a plain name and the parentheses are superfluous.  There may be
*  a dimension after the name if this is an array of pointers to
*  functions.  If there are no parentheses, just a plain name is
*  expected.  In both cases, check that an identifier rather than
*  some other kind of token is found where the name is expected.  */

      if (token_is("(")) {
         nexttkn();
         if (token_is("*")) {
            kppn = TRUE; kptr = TRUE; nexttkn();
            }
         /* Make sure an identifier comes next, then store it */
         if (ttype != TKN_IDF) {
            show_context();
            abexitm(342, ssprintf(NULL, "An identifier was "
               "expected, %s was found", token));
            }
         hwmidfr(dname, ldname, ltoken, "Declarator name");
         strcpy(dname, token);
         nexttkn();
         /* Here we can have a dimension (array of ptrs) */
         if (token_is("[")) nitm = getdim(dname);
         /* Make sure the parens are closed here */
         if (token_is_not(")")) {
            show_context();
            abexitm(342, ssprintf(NULL, "A right paren was "
               "expected, %s was found", token));
            }
         } /* End checking parenthesized name */
      else {
         /* Name is not parenthesized, just check and store it */
         if (ttype != TKN_IDF) {
            show_context();
            abexitm(342, ssprintf(NULL, "An identifier was "
               "expected, %s was found", token));
            }
         hwmidfr(dname, ldname, ltoken, "Declarator name");
         strcpy(dname, token);
         }
      nexttkn();

/* Decide where to store the information for this identifier.
*  If declaring an anonymous struct or union, the first identifier
*  is used to make a name for use if needed in comments in output
*  table.  */

      switch (level) {
      case TOPLEVEL:       /** Top level object **/
         /* Ignore all declarators--
         *  basically, we are just checking syntax now.  */
         ptp = NULL;
         break;
      case INSTRUCT:       /** Structure **/
         /* If object is anonymous, save declarator name */
         if (!inner->ptnm) savedname(inner, dname);
         /* Append current declarator to outer type record */
         ptp = outer;
         break;
      case INUNION:        /** Union **/
         /* If object is anonymous, save declarator name */
         if (!inner->ptnm) savedname(inner, dname);
         /* Make a new type record and link it to the pnut chain
         *  in the outer type record.  It gets a name later.  Set
         *  special key struct + union.  */
         ptp = allotype(Tk_STR|Tk_UEL);
         *outer->ut.pplut = ptp;
         outer->ut.pplut = &ptp->pnut;
         ptp->ninc = ++outer->ninc;
         break;
      case INTYPDEF:       /** Typedef **/
         /* Create a new type record for the named type, even if
         *  it is not a key type, because it might be used inside
         *  some other type (e.g. byte).  */
         ptp = allotype(Tk_TDF);
         addtype(dname, ptp);
         } /* End level switch */

/* If the name was a pointer in parentheses, there are three
*  cases:
*  (1) The parens are followed by square brackets.  The item
*      is a pointer to an array, and the dimension is one
*      regardless of the dimensions in the brackets.
*  (3) The parens are followed by another set of parens.  The
*      item is a pointer to a function and for our purposes
*      it can be handled the same as any other single pointer.
*      All the stuff before the parens just describes what the
*      function returns, which is of no interest to us.
*  (3) Anything else is a syntax error in the user program.
*/

      if (kppn) {
         if (token_is("[")) {
            while (token_is("[")) findmatch(FM_SQUARE);
            }
         else if (token_is("(")) {
            findmatch(FM_ROUND);
            }
         else {
            show_context();
            abexitm(342, ssprintf(NULL, "A pointer declaration "
               "in parens (%s) is expected to be an array or "
               "function pointer", dname));
            }
         if (ptp) additem(ptp,NULL,nitm,IT_PTR);
         }

/* Otherwise, if the name is not in parentheses, or the
*  parens were superfluous, there are again three cases:
*  (1) The item is a name followed by one or more argument
*      declarations in parentheses.  The item is a function,
*      which cannot be copied between unlike processors.  It
*      is an error for the user to request a conversion table
*      entry for this item.  However, the item may occur in a
*      typedef that is never referenced.  Hence, the function
*      is marked here with a special code and the error is
*      deferred until NXDRTT tables are being written.
*  (2) The item is a name followed by one or more constant
*      expressions in brackets.  The item is an array of the
*      inner type or of pointers to the inner type.
*  (3) The item is a simple name.  One item of the inner type,
*      or a pointer to one such item, is embedded in the outer
*      type.  If the inner type is "void", not a pointer, the
*      construct is in error.  However, the item may occur in
*      a typedef that is never referenced.  Hence, the void
*      item is marked here with a special code and the error
*      is deferred until NXDRTT tables are being written.
*      Likewise, an incomplete type is explicitly allowed by
*      the ANSI standard in a typedef, and this feature is
*      used in LINUX headers.
*/

      else if (ptp) {     /* Storing the information */
         long ll = token_is("[") ? getdim(dname) : 1;
         if (kptr)
            additem(ptp, NULL, ll, IT_PTR);
         else if (token_is("(")) {
            additem(ptp, NULL, ll, IT_FUNC);
            findmatch(FM_ROUND); }
         else if (level != INTYPDEF &&
               !(inner->key & (Tk_DEF|Tk_BASE))) {
            show_context();
            abexitm(342, ssprintf(NULL, "Identifier "
               "%s refers to an incomplete type", dname));
            }
         else if (is_simple(inner) && inner->pfit) {
            item *eit = inner->pfit;
            if (level != INTYPDEF && eit->itype == IT_VOID) {
               show_context();
               abexitm(342, ssprintf(NULL, "Identifier "
                  "%s is of type void, not a pointer", dname));
               }
            additem(ptp, eit->pinctbl, ll*eit->count, eit->itype);
            }
         /* Redundant brackets added here--possible gcc bug skips
         *  'else' clause when this test is false.  */
         else if (inner->key & (Tk_UNI|Tk_UEL)) {
            additem(ptp, inner, ll, IT_JUMP); }
         else {
            additem(ptp, inner, ll, IT_STRUC); }
         }
      else {         /* Not storing the information */
         if (token_is("(")) findmatch(FM_ROUND);
         while (token_is("[")) findmatch(FM_SQUARE);
         }

/* If in a typedef, mark it as defined */

      if (level == INTYPDEF && ptp->pfit) {
         ptp->key = (ptp->key & ~Tk_IBD) | Tk_DEF;
         }

/* If in a union, pass the mxtlen up to the parent */

      if (level == INUNION  && ptp->mxtlen > outer->mxtlen)
         outer->mxtlen = ptp->mxtlen;

/* At this point, there may be initializers.  These are of no
*  concern to this program, so we just skip over them until a
*  comma or semicolon is found.  But if we hit curly braces,
*  we must ignore any commas inside them.  */

      if (token_is("=")) {
         do {
            nexttkn();
            if (token_is("{")) findmatch(FM_CURLY);
            }
         while (token_is_not(",") && token_is_not(";"));
         }

/* Anything but a comma or a semicolon here is a syntax error,
*  except in gcc header files, we can skip over __attribute__ */

      if (token_is(";"))
         break;
      if (token_is(",")) {
         nexttkn();
         continue;
         }
#ifdef GCC
      if (token_is("__attribute__")) {
         nexttkn();
         findmatch(FM_ROUND);
         break;
         }
#endif
      show_context();
      abexitm(342, ssprintf(NULL, "Declarator %s is not "
         "followed by the expected comma or semicolon", dname));

      } /* End declarator loop */

   nexttkn();              /* Swallow the semicolon */

   } /* End Scan_decls */

/*=====================================================================*
*                            Scan_basetype                             *
*                                                                      *
*                    Parse a base type declaration                     *
*                                                                      *
*  Create a table entry corresponding to a base type, which may be     *
*  the word "signed" or "unsigned" followed by a C-defined base type   *
*  name, or it may be a previously encountered typedef name.  The      *
*  modifiers "const" and "volatile", when encountered, are ignored.    *
*=====================================================================*/

void Scan_basetype(int level, type *ptp) {

   type *etp;           /* Ptr to type record for existing type */
   tynrec *etyn;        /* Ptr to matching type record */
   int ksgn = FALSE;    /* TRUE if "signed" token present */
   int kuns = FALSE;    /* TRUE if "unsigned" token present */

/* A base type can start with "signed" or "unsigned", but not both */

   while (1) {
      ignqualif();
      if (token_is("signed")) {
         ksgn = TRUE;
         nexttkn();
         }
      else if (token_is("unsigned")) {
         kuns = TRUE;
         nexttkn();
         }
      else
         break;
      }
   if (ksgn && kuns) {
      show_context();
      abexitm(342, ssprintf(NULL,
         "Name %s is both signed and unsigned.", token));
      }

/* The next token should match a base type, except that "signed"
*  alone is equivalent to "int" and "unsigned" alone is equivalent
*  to "unsigned int".  Also check for "long long", "long long int",
*  "long int", "short int", long unsigned int, etc.  */

   hwmidfr(dname, ldname, ltoken, "Base type name");
   strcpy(dname, token);
   if (etyn = tyrlkup(dname)) {   /* Assignment intended */
      /* Type is found, swallow the type name */
      nexttkn();
      /* Check for "long long", "long int", or "long long int" */
      if (!strcmp(dname, "long")) {
         if (token_is("unsigned")) {
            kuns = TRUE;
            nexttkn();
            }
         else if (token_is("long")) {
            static char long_long[] = "long long";
            int lll = strlen(long_long);
            hwmidfr(dname, ldname, lll, "Base type name");
            strcpy(dname, long_long);
            etyn = tyrlkup(dname);
            nexttkn();
            }
         if (token_is("int"))
            nexttkn();
         }
      else if (!strcmp(dname, "short")) {
         if (token_is("int"))
            nexttkn();
         }
      /* Prepend "unsigned" if relevant and search again.
      *  In this situation, failure to match is an error.
      *  Temp idfr cname also used in needname(), getdim().  */
      if (kuns) {
         static char unspfx[] = "unsigned ";
         hwmidfr(cname, lcname, strlen(unspfx) + strlen(dname),
            "Unsigned type name");
         strcpy(cname, unspfx);
         strcat(cname, dname);
         etyn = tyrlkup(cname);
         if (!etyn) {
            show_context();
            abexitm(342, ssprintf(NULL, "Unsigned %s "
               "encountered, has not been defined.", dname));
            }
         }
      }
   else {
      /* Name not found, check for "signed" or "unsigned" */
      if (ksgn)
         etyn = tyrlkup("int");  /* Cannot fail */
      else if (kuns)
         etyn = tyrlkup("unsigned int");
      else {
         show_context();
         abexitm(342, ssprintf(NULL, "Name %s "
            "encountered where type name expected.", dname));
         }
      }
   etp = etyn->ptype;   /* Locate item codes for this type */

/* Next comes a list of declarators */

   Scan_decls(level, ptp, etp);

   } /* End Scan_basetype() */

/*=====================================================================*
*                              Scan_enum                               *
*                                                                      *
*                  Parse an enumeration declaration                    *
*                                                                      *
*  Note that the conversion code emitted for an enum assumes that enum *
*  variables will be expanded to 4 bytes in messages and files.  This  *
*  is the responsibility of the routine that performs the conversion.  *
*=====================================================================*/

void Scan_enum(int level, type *ptp) {

   type *pntp;          /* Ptr to new type record */

   if (pntp = needname(level)) {    /* Assignment intended */

/* Create a new conversion table if enum not already defined */

      if (!(pntp->key & (Tk_DEF|Tk_IBD|Tk_INC))) {
         pntp->key |= (Tk_ENU|Tk_IBD);

         additem(pntp, NULL, 1, IT_ENUM);

         /* Ignore the named enum constants, get next token */
         findmatch(FM_CURLY);
         pntp->key |= Tk_DEF;
         }

/* Next comes a list of declarators */

      Scan_decls(level, ptp, pntp);
      pntp->key &= (pntp->key & Tk_INC) ? ~Tk_INC : ~Tk_IBD;

      } /* End if needname */

   } /* End Scan_enum() */

/*=====================================================================*
*                             Scan_struct                              *
*                                                                      *
*                    Parse a structure declaration                     *
*                                                                      *
*  Scan_struct calls itself recursively to handle embedded structures. *
*                                                                      *
*  Note that any "static" or "extern" qualifier preceding the "struct" *
*  keyword will automatically be swallowed by the Scanhfile() routine. *
*=====================================================================*/

void Scan_struct(int level, type *ptp) {

   type *pntp;          /* Ptr to new type record */

   if (pntp = needname(level)) {    /* Assignment intended */

/* Create a new conversion table if struct not already defined */

      if (!(pntp->key & (Tk_DEF|Tk_IBD|Tk_INC))) {

         pntp->key |= (Tk_STR|Tk_IBD);

/* Read tokens and search for one of "enum", "struct", or "union".
*  When found, call the appropriate Scan_ routine to process the
*  embedded object.  Anything else must be a base type.  */

         while (token_is_not("}")) {

            /* Error if hit end of file */
            if (ttype == TKN_EOF) abexitm(342, "Reached end-of-"
               "file while parsing a struct declaration");

            /* Ignore qualifiers */
            ignqualif();

            /* Parse the type definition */
            if (token_is("enum"))
               Scan_enum(INSTRUCT, pntp);
            else if (token_is("struct"))
               Scan_struct(INSTRUCT, pntp);
            else if (token_is("union"))
               Scan_union(INSTRUCT, pntp);
            else
               Scan_basetype(INSTRUCT, pntp);
            }
         nexttkn();     /* Swallow the "}" */
         pntp->key |= Tk_DEF;
         }

/* Next comes a list of declarators for the structure as a whole */

      Scan_decls(level, ptp, pntp);
      pntp->key &= (pntp->key & Tk_INC) ? ~Tk_INC : ~Tk_IBD;

      } /* End if needname */

   } /* End Scan_struct() */

/*=====================================================================*
*                             Scan_union                               *
*                                                                      *
*                      Parse a union declaration                       *
*                                                                      *
*  This routine differs from Scan_struct in that each member of a      *
*  union gets its own type.  Scan_union calls itself recursively to    *
*  handle embedded unions.                                             *
*                                                                      *
*  N.B.  When a struct or another union is embedded in a union, it     *
*  ends up with a type record for the union member which points to     *
*  a type record for the embedded object.  This seems wasteful, but    *
*  is required to deal with multiple declarators of the same object.   *
*  The extra type can be optimized away when only a single declarator  *
*  is found.                                                           *
*                                                                      *
*  Note that any "static" or "extern" qualifier preceding the "union"  *
*  keyword will automatically be swallowed by the Scanhfile() routine. *
*=====================================================================*/

void Scan_union(int level, type *ptp) {

   type *pntp;                      /* Ptr to new type record */

   if (pntp = needname(level)) {    /* Assignment intended */

/* Create a new conversion table if union not already defined */

      if (!(pntp->key & (Tk_DEF|Tk_IBD|Tk_INC))) {

         pntp->ut.pplut = &pntp->pnut;
         pntp->key |= (Tk_UNI|Tk_IBD);

/* Read tokens and search for one of "enum", "struct", or "union".
*  When found, call the appropriate Scan_ routine to process the
*  embedded object.  Anything else must be a base type.  */

         while (token_is_not("}")) {

            /* Error if hit end of file */
            if (ttype == TKN_EOF) abexitm(342, "Reached end-of-"
               "file while parsing a union declaration");

            /* Ignore qualifiers */
            ignqualif();

            /* Parse the type definition */
            if (token_is("enum"))
               Scan_enum(INUNION, pntp);
            else if (token_is("struct"))
               Scan_struct(INUNION, pntp);
            else if (token_is("union"))
               Scan_union(INUNION, pntp);
            else
               Scan_basetype(INUNION, pntp);
            }
         nexttkn();     /* Swallow the "}" */
         pntp->key |= Tk_DEF;
         }

/* Next comes a list of declarators for the union as a whole */

      Scan_decls(level, ptp, pntp);
      pntp->key &= (pntp->key & Tk_INC) ? ~Tk_INC : ~Tk_IBD;

      } /* End if needname */

   } /* End Scan_union() */

/*=====================================================================*
*                           Scan_typedef()                             *
*                                                                      *
*                           Parse a typedef                            *
*                                                                      *
*  A typedef is always top level, so it doesn't need level and ptp     *
*  args.  Note that function types are ignored, as they cannot be      *
*  moved between processors and therefore should never appear on the   *
*  key list.  Any typedef with parens in it is taken to be a function  *
*  def--some fancy constructs may not be handled correctly.            *
*=====================================================================*/

void Scan_typedef(void) {

   type *pntp;           /* Ptr to new type record */

   /* Swallow the "typedef" token */
   nexttkn();

   /* Ignore "constant" or "volatile" qualifiers */
   ignqualif();

   /* Next should come the type-specifier.  If it is a structure
   *  union, or enum, call the appropriate scan routine.  Otherwise,
   *  process it as a base type (which might be a name from an
   *  earlier typedef).  */
   if (token_is("struct"))
      Scan_struct(INTYPDEF, NULL);
   else if (token_is("union"))
      Scan_union(INTYPDEF, NULL);
   else if (token_is("enum"))
      Scan_enum(INTYPDEF, NULL);
   else
      Scan_basetype(INTYPDEF, NULL);

   } /* End Scan_typedef() */

