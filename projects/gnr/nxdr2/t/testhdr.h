/* (c) Copyright 1999, George N. Reeke, Jr. */
/***********************************************************************
*              NXDR2.H -- Header file for nxdr2 utility                *
*  THIS VERSION MODIFIED FOR TEST PURPOSES--DO NOT USE IN NXDR2 BUILD  *
*                                                                      *
*  Rev, 09/05/13, GNR - Add test for array of pointers to functions    *
***********************************************************************/

#include "rkhash.h"

/* Configuration parameters */

#define MAXIDFR  32     /* Initial max length of an identifier, used
                        *  to allocate space for the first token.
                        *  When a longer identifier is found, token
                        *  storage is expanded accordingly, so there
                        *  is no intrinsic limit in this program to
                        *  how long a token can be.  */

#define IDFRINCR  4     /* Increment to add to allocated token size
                        *  when a longer token is found.  */

#define ITEMINCR 50     /* Pool size for allocation of item records.
                        *  Since these records are small and many
                        *  are needed, they are allocated in sets of
                        *  ITEMINCR to cut down on malloc calls.  */

#define NCONTEXT  3     /* Number of lines of context to print when
                        *  a syntax error is found in the input.  */

/* Token classes, returned by nexttkn() in ttype.
*  Initial value of ttype is 0--do not assign 0 as a class.  */

#define TKN_CHR  1      /* Token is a single character */
#define TKN_NUM  2      /* Token is a number */
#define TKN_IDF  3      /* Token is an identifier */
#define TKN_EOF  4      /* No more tokens--end of file */

/* Action codes for declaration scan routines */

#define TOPLEVEL 0      /* Top level declaration--make a new typrec
                        *  if named, otherwise skip */
#define INSTRUCT 1      /* Embedded in a struct--append via pinctbl
                        *  to outer-level table passed as 2nd arg */
#define INUNION  2      /* Embedded in a union--make a new typrec
                        *  for each declarator, link via pfut */
#define INTYPDEF 3      /* Embedded in a typedef--make a new typrec
                        *  if named, otherwise skip */

/* Length precedence codes for base types that are recognized
*  other than by matching their names with intrtypes--must
*  match intrtypeseqs values in main pgm.  Code for enum is
*  not fixed--depends on compiler.  */

#define ITSptr  10

/* An idfr (identifier) is a dynamically allocated, NULL-terminated
*  string derived from a C identifier in the input file.  */

typedef char *idfr;

/* An item structure describes a single data item */

typedef struct itm {
   struct itm *pnit;    /* Ptr to next item in current object */
   struct tb3 *pinctbl; /* Pointer to tab3 for included type */
   long   count;        /* Repeat count */
   char   type;         /* A type code from intrtypecodes */
   char   tseq;         /* Size precedence of the base type */
   } item;

/* A triplet table of type tab3 is associated with each type record,
*  whether named or unnamed.  A tab3 acts as a header for a chain
*  of item records (struct or simple type) or of tab3's (union).  */

typedef struct tb3 {
   struct tb3 *pnut;    /* Ptr to next tab3 in union chain */
   struct tb3 **ppnut;  /* Ptr to ptr to next tab3 in union chain */
   struct itm *pfit;    /* Ptr to first item in chain */
   struct itm *plit;    /* Ptr to last item in chain */
   struct tyr *ptnm;    /* Ptr to typrec with name of this struct */
   long   offset;       /* Offset of this table in NXDRTT output */
   short  nel;          /* Number of elements in the item chain */
   short  ninc;         /* Number of included elements in union */
   char   key;          /* One or more of the following:  */
#define T3_BASE 1          /* Object is a built-in base type */
#define T3_KEY  2          /* Object is a key type */
#define T3_STR  4          /* Object is a structure */
#define T3_UNI  8          /* Object is a union */
#define T3_ENU 16          /* Object is an enum */
#define T3_OUT 32          /* Object table already written out */
   char   mxtseq;       /* Maximum tseq of any embedded type */
   } tab3;

/* A typrec is needed only for a named type.  All type records are
*  kept in a linked list headed at headtyprec.  */

typedef struct tyr {
   struct tyr *nxtyp;   /* Ptr to next typrec in linked list */
   struct tyr *hlink;   /* Link for resolving hash collisions */
   struct tb3 *ptab3;   /* Pointer to triplet table */
   char   typname[1];   /* Expanded to hold full name */
   } typrec;

/* Data structure to hold key types.  Starting at 'headkeyrec' is a
*  linked list of keyrecs for all "key" types, i.e.  types for which
*  conversion tables must be produced.  The user supplies this list
*  via the "-b" option.  Enum, union, or struct types are given the
*  names enu_name, uni_name, or str_name, respectively.  */

typedef struct kyr {
   struct kyr *nxtkey;  /* Ptr to next keyrec in linked list */
   struct kyr *hlink;   /* Link for resolving hash collisions */
   struct tb3 *typdata; /* Ptr to conversion data for this key */
   char   keyname[1];   /* Expanded to hold the full name */
   } keyrec;

/* Global variables */

#ifndef MAIN
extern FILE   *pif;     /* Stream for .i file */
extern item   *pavit;   /* Ptr to first available item block */
extern tab3   *pavt3;   /* Ptr to first available tab3 block */
extern htbl   *hkeyr;   /* Hash table for key records */
extern htbl   *htypr;   /* Hash table for type records */
extern char   *ccmd;    /* Command to compile a C program */
extern long   siln[NCONTEXT]; /* Seek info for context */
extern idfr   cname;    /* Conversion name */
extern idfr   dname;    /* Declarator name */
extern idfr   token;    /* Current token */
extern size_t lcname;   /* High-water length of cname */
extern size_t ldname;   /* High-water length of dname */
extern size_t ltoken;   /* Length of current token */
extern size_t mxlname;  /* Maximum length of an identifier name */
extern size_t mxltoken; /* Maximum length of any token */
extern int    ttype;    /* Type of current token */
extern keyrec *headkeyrec;
extern typrec *headtyprec;
extern typrec **tailtyprec;
#endif

/* Function prototypes */

extern void addanon(tab3 *inner, tab3 *outer, idfr dname);
extern item *additem(tab3 *ptab, tab3 *pinc, long count,
   int code, int tseq);
extern void addtype(idfr typname, tab3 *ptab);
extern tab3 *allotab3(int is_key);
extern void findmatch(int kbra);
#define FM_CURLY  0
#define FM_ROUND  1
#define FM_SQUARE 2
extern void freeitem(item *pit);
extern void freetab3(tab3 *pt3);
extern long getdim(idfr dname);
extern void ignqualif(void);
extern tab3 *needname(int level);
extern void nexttkn(void);
extern void show_context(void);
extern void skipstmt(void);
extern void Scan_decls(int level, tab3 *outer, tab3 *inner);
extern void Scan_basetype(int level, tab3 *pt3);
extern void Scan_enum(int level, tab3 *pt3);
extern void Scan_struct(int level, tab3 *pt3);
extern void Scan_union(int level, tab3 *pt3);
extern void Scan_typedef(void);

/* Inline functions, implemented as macros */

#define hwmidfr(name,lold,lnew,msg) \
   if (lnew>lold) name = reallocv(name,(lold=lnew+IDFRINCR)+1,msg)
#define keylkup(name) hashlkup(hkeyr, name)
#define tyrlkup(name) hashlkup(htypr, name)
#define is_simple(pt3) (pt3->pfit == pt3->plit)
#define token_is(string) (!strcmp(token, string))
#define token_is_not(string) (strcmp(token, string))

typedef union u1 {
   tab3 u1t3;
   struct itm u1it;
   long u1l;
   } uni1;

struct t1 {
   int it1,*pit2,it2;
   enum et1 {et1a,et1b,et1c} ett;
   si32 (*aptrfun[3])(double, float, int);
   int at1[3],aat1[4][7],ast1[NCONTEXT+3/2];
   };

#ifdef TX   /* Define value of TX to test abexit code TX */
/* Some compilers (e.g. gcc) don't allow floating-point
*  numbers in #ifs, so use straight integers. */
#if (TX == 34201)
struct tx {
   int i;
   struct j;
   };
#endif

#if (TX == 34202)
struct tx {
   int (*34)(int i);
   };
#endif

#if (TX == 34203)
struct tx {
   int (*pix&34)(int i);
   };
#endif

#if (TX == 34204)
struct tx {
   int *34;
   };
#endif

#if (TX == 34205)
struct tx {
   int (*pix);
   };
#endif

#if (TX == 34206)
struct tx {
   int pix(int i);
   };
#endif

#if (TX == 34207)
struct tx {
   struct tx i;
   };
#endif

#if (TX == 34208)
struct tx {
   void i;
   };
#endif

#if (TX == 34209)
struct tx {
   int i&j;
   };
#endif

#if (TX == 34210)
struct tx {
   signed unsigned int i;
   };
#endif

#if (TX == 34211)
struct tx {
   unsigned double dd;
   };
#endif

#if (TX == 34212)
struct tx {
   undeftype i;
   };
#endif

#if (TX == 34213)
typedef long;
#endif

#if (TX == 34601)
struct tx {
   long tnmp[];
   };
#endif

#if (TX == 34602)
struct tx {
   long tnmp[45;
   };
#endif

#if (TX == 34603)
struct tx {
   long tnmp[a];
   };
#endif

#if (TX == 34604)
struct tx {
   long tnmp[343987329874023892397234987529874378];
   };
#endif

#if (TX == 351)
struct itm {
   long another_def;
   };
#endif

#if (TX == 34301)
struct tx {
   long *tnmp[45);
   };
#endif

#if (TX == 34302)
struct tx {
   int i;
/* Need to have end of file here--struct unfinished for test */
#endif
#endif
