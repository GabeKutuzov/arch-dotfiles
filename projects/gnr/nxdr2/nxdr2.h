/* (c) Copyright 1999-2016, The Rockefeller University *11113* */
/***********************************************************************
*              NXDR2.H -- Header file for nxdr2 utility                *
*                                                                      *
***********************************************************************/

#include "rkhash.h"

/* Configuration parameters */

#define qDTPFX "NXDR"   /* Default table name prefix */
#define qTTSFX "TT"     /* Suffix for global conversion table name */
#define qUTSFX "UT"     /* Suffix for union conversion table name */

#define MAXIDFR  32     /* Initial max length of an identifier, used
                        *  to allocate space for the first token.
                        *  When a longer identifier is found, token
                        *  storage is expanded accordingly, so there
                        *  is no intrinsic limit in this program to
                        *  how long a token can be.  */

#define IDFRINCR  4     /* Increment to add to allocated token size
                        *  when a longer token is found.  */

#define ITEMINCR 50     /* Pool size for allocation of item and tab3
                        *  records.  Since these records are small and
                        *  many are needed, they are allocated in sets
                        *  of ITEMINCR to cut down on malloc calls.  */

#define NCONTEXT  3     /* Number of lines of context to print when
                        *  a syntax error is found in the input.  */

#define qKSONAME "$nxdr$.tt"  /* Name for redirected stdout file */

/* Token classes, returned by nexttkn() in ttype.
*  Initial value of ttype is 0--do not assign 0 as a class.  */

#define TKN_CHR   1     /* Token is a single character */
#define TKN_NUM   2     /* Token is a number */
#define TKN_IDF   3     /* Token is an identifier */
#define TKN_EOF   4     /* No more tokens--end of file */

/* Action codes for declaration scan routines */

#define TOPLEVEL  0     /* Top level declaration--make a new type
                        *  if named, otherwise skip */
#define INSTRUCT  1     /* Embedded in a struct--append via pinctbl
                        *  to outer-level table passed as 2nd arg */
#define INUNION   2     /* Embedded in a union--make a new type
                        *  for each declarator, link via pnut */
#define INTYPDEF  3     /* Embedded in a typedef--make a new type
                        *  if named, otherwise skip */

/* An idfr (identifier) is a dynamically allocated, NULL-terminated
*  string derived from a C identifier in the input file.  */

typedef char *idfr;

/* An item structure describes a single data item */

typedef struct itm {
   struct itm *pnit;    /* Ptr to next item in current object */
   struct typ *pinctbl; /* Pointer to type rec for included type */
   long   count;        /* Repeat count */
   int    itype;        /* Index of type in intrtypes */
/* Definitions of type indexes for special types--must match
*  tables at top of nxdr2.c  */
#define IT_PTR    0        /* Object is a pointer */
#define IT_FUNC   1        /* Object is a function */
#define IT_JUMP   2        /* Objet is a union */
#define IT_KJUMP  3        /* Object is a hand-coded jump */
#define IT_STRUC  4        /* Object is a structure */
#define IT_VOID   5        /* Object is void, first real type */
#define IT_ENUM   7        /* Object is enum type */
   } item;

/* A type record is associated with each type, whether named or
*  unnamed.  A type record acts as a header for a chain of item
*  records (struct or simple type) or of types (union).  All type
*  records are kept in a linked list headed at headtyprec.  */

typedef struct typ {
   struct typ *pntp;    /* Ptr to next type in main list */
   struct typ *pnut;    /* Ptr to next element in union chain */
   union {
      struct typ **pplut;  /* Ptr to last pnut */
      struct typ *ptut;    /* Ptr to threaded union table */
      } ut;
   struct itm *pfit;    /* Ptr to first item in chain */
   struct itm *plit;    /* Ptr to last item in chain */
   struct tyn *ptnm;    /* Ptr to tynrec with name of this struct */
   long   length;       /* Length of struct or union */
   long   offset;       /* Offset of this table in NXDRTT output */
   long   joffset;      /* Index of union conversion routine */
   short  nel;          /* Number of elements in the item chain */
   short  ninc;         /* Number of included elements in union *
                        *     -1 when #define has been written */
   short  nref;         /* Number of references to this object--
                        *     -1 when table has been written */
   short  key;          /* One or more of the following:  */
#define Tk_BASE  0x0001    /* Object is a built-in base type */
#define Tk_KEY   0x0002    /* Object is a key type */
#define Tk_STR   0x0004    /* Object is a structure */
#define Tk_UNI   0x0008    /* Object is a union */
#define Tk_UEL   0x0010    /* Object is a union element */
#define Tk_ENU   0x0020    /* Object is an enum */
#define Tk_TDF   0x0040    /* Object is a typedef */
#define Tk_ANON  0x0080     /* Object has manufactured name
                           *  (used only in output phase) */
#define Tk_INC   0x0100    /* Object is an incomplete type */
#define Tk_IBD   0x0200    /* Object is being defined */
#define Tk_DEF   0x0400    /* Object is fully defined */
   char   mxtlen;       /* Maximum tlen of any embedded type */
   } type;

/* A tynrec holds the name of a type and its hash linker. */

typedef struct tyn {
   struct typ *ptype;   /* Pointer to type record */
   struct tyn *hlink;   /* Link for resolving hash collisions */
   char   typname[1];   /* Expanded to hold full name */
   } tynrec;

/* Data structure to hold key types.  Starting at 'headkeyrec' is a
*  linked list of keyrecs for all "key" types, i.e.  types for which
*  conversion tables must be produced.  The user supplies this list
*  via the "-b" option.  Enum, union, or struct types are given the
*  names enu_name, uni_name, or str_name, respectively.  */

typedef struct kyr {
   struct kyr *nxtkey;  /* Ptr to next keyrec in linked list */
   struct kyr *hlink;   /* Link for resolving hash collisions */
   struct typ *typdata; /* Ptr to conversion data for this key */
   char   keyname[1];   /* Expanded to hold the full name */
   } keyrec;

/* Global variables */

#ifndef MAIN
extern FILE   *pif;     /* Stream for .i file */
extern item   *pavit;   /* Ptr to first available item block */
extern type   *pavtp;   /* Ptr to first available type block */
extern type   *headtyprec;
extern type   **tailtyprec;
extern keyrec *headkeyrec;
extern char   *pitlens; /* Ptr to type lengths table to be used */
extern htbl   *hkeyr;   /* Hash table for key records */
extern htbl   *htynr;   /* Hash table for tyn records */
extern char   *ccmd;    /* Command to compile a C program */
extern long   siln[NCONTEXT]; /* Seek info for context */
extern idfr   cname;    /* Conversion name */
extern idfr   dname;    /* Declarator name */
extern idfr   token;    /* Current token */
extern idfr   dbgtoken; /* Wait for debugger at this token */
extern size_t lcname;   /* High-water length of cname */
extern size_t ldname;   /* High-water length of dname */
extern size_t ltoken;   /* Length of current token */
extern size_t mxlname;  /* Maximum length of an identifier name */
extern size_t mxltoken; /* Maximum length of any token */
extern int    oldstdout;/* Holds stdout handle during forks */
extern int    ttype;    /* Type of current token */
#endif

/* Function prototypes */

extern item *additem(type *ptyp, type *pinc, long count,
   int ityp);
extern void addtype(idfr typname, type *ptyp);
extern type *allotype(int keys);
extern void findmatch(int kbra);
#define FM_CURLY  0
#define FM_ROUND  1
#define FM_SQUARE 2
extern void freeitem(item *pit);
extern void freetype(type *ptp);
extern long getdim(idfr dname);
extern void ignqualif(void);
extern void killstdout(void);
extern type *needname(int level);
extern void nexttkn(void);
extern void regainstdout(void);
extern void savedname(type *inner, idfr dname);
extern void show_context(void);
extern void skipstmt(void);
extern void Scan_decls(int level, type *outer, type *inner);
extern void Scan_basetype(int level, type *ptp);
extern void Scan_enum(int level, type *ptp);
extern void Scan_struct(int level, type *ptp);
extern void Scan_union(int level, type *ptp);
extern void Scan_typedef(void);

/* Inline functions, implemented as macros */

#define hwmidfr(name,lold,lnew,msg) \
   if (lnew>lold) name = reallocv(name,(lold=lnew+IDFRINCR)+1,msg)
#define keylkup(name) hashlkup(hkeyr, name)
#define keyszup(ptp,len) ((ptp->length + (long)((len) - 1)) & \
   ~((long)(len) - 1))
#define tyrlkup(name) hashlkup(htynr, name)
#define is_simple(ptp) (ptp->pfit == ptp->plit)
#define token_is(string) (!strcmp(token, string))
#define token_is_not(string) (strcmp(token, string))
