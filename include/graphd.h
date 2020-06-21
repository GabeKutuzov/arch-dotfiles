/* (c) Copyright 1997-2008, The Rockefeller University *11114* */
/* $Id: graphd.h 61 2017-04-13 20:31:56Z  $ */
/***********************************************************************
*                              GRAPHD.H                                *
*                                                                      *
*  Definitions for working with NSI "GRAPH" files, now known as        *
*  "SIMDATA", version 3                                                *
*                                                                      *
*  V1A, 09/08/97, GNR - New program                                    *
*  Rev, 02/01/08, GNR - Increase selector length to 16                 *
*  ==>, 03/22/08, GNR - Last date before committing to svn repository  *
*  V1B, 09/18/08, GNR - Revisions for 64-bit systems                   *
*  Rev, 03/01/17, GNR - Add new GDV type codes                         *
***********************************************************************/
#include "rfdef.h"

#define EndMark (-999999)        /* Time mark at end of file */
#define LIdRec       20          /* Length of ID record */
#define LTitRec      60          /* Length of title record */
#define LTimeStamp   12          /* Length of time stamp */
#define LLevName     12          /* Max length of a level name */
#define qLLN        "12"         /* Quoted LLevName */
#define LVarName      8          /* Max length of variable name */
#define qLVN         "8"         /* Quoted LVarName */
#define LSelName     16          /* Max length of selector name--
                                 *  spec says 8, this is generous */
#define qLSN        "16"         /* Quoted LSelName */

enum GDTERM {                    /* Terminators found on selectors */
   Blank=0, Comma, Plus, Minus, EndOfCard };

struct GDVAR {                   /* Data variable description */
   char vname[LVarName];         /* Name of the variable */
   short vtype;                  /* Type of the variable */
#define GDV_Fix       0             /* Signed fixed-point type */
#define GDV_UFix      1             /* Unsigned fixed-point type */
#define GDV_Real      2             /* Real type */
#define GDV_Dble      3             /* Double-precision real type */
#define GDV_Color     4             /* Colored pixel (8, 16, 24-bit) */
#define GDV_ASCII     5             /* ASCII text data */
#define GDV_Hex       6             /* Hexadecimal data */
#define GDV_FixMZero  7             /* Fix point, minus zero allowed */
   short vlev;                   /* Level of this variable */
   short vscl;                   /* Binary scale of this variable */
   short vlen;                   /* Length of this variable */
   long  vdim;                   /* Dimension if item is an array */
   ui32  vic;                    /* Code for output conversion */
   };
typedef struct GDVAR GDVar;

struct GDNODE {                  /* Selector node data */
   struct GDNODE *pns;           /* Ptr to next selector node */
   struct GDNODE *pls;           /* Ptr to lower-level selector */
   struct GDNODE *par;           /* Ptr to parent of this node */
   struct GDSEL  *psel;          /* Ptr to array of selectors */
   unsigned short *pvin;         /* Ptr to array of variable indexes */
   size_t ldata;                 /* Length of data for one selection
                                 *  at and below this node */
   size_t nitms;                 /* Number of items selected here */
   size_t odata;                 /* Origin of data at this node */
   int level;                    /* Level of this node */
   int levid;                    /* Id number of level name (counting
                                 *  from 1) or 0 if no name given */
   int nsallo;                   /* Number of selectors allocated */
   int nsel;                     /* Number of selectors stored */
   int nvar;                     /* Number of variables found */
   };
typedef struct GDNODE GDNode;

struct GDSEL {                   /* Individual selector data */
   int ks;                       /* Kind of selector */
#define GDS_NUM   0                 /* A number */
#define GDS_RANGE 1                 /* A range */
#define GDS_NAME  2                 /* A name */
#define GDS_OFFR  3                 /* Offset range */
   union {
      long it;                   /* Item number */
      struct {                   /* A range */
         long is;                   /* Start of range */
         long ie;                   /* End of range */
         long ii;                   /* Range increment */
         } r;
      char in[LSelName];         /* A name */
      } us;
   };
typedef struct GDSEL GDSel;

/* Prototypes for gditools module */
struct RFdef *gdiopen(char *fname);
char *gdickid(void);
int  gdinlvls(void);
int  gdinvars(void);
char *gdititle(void);
char *gditime(void);
char ***gdilvlnm(void (*gcblevnm)(int ilvl, int klvl, char *name));
#define GDL_FIRST 1              /* klvl:  name is first on level */
#define GDL_NLAST 2              /* klvl:  name is not last on level */
GDVar *gdivars(void (*gcbvar)(int ivar, char *pd, int ld, GDVar *pv));
int  gdiqlvl(char *item, char **levnms);
int  gdiqseg(void);
int  gdiqvar(char *varnm);
void gdigoseg(int iseg);
size_t gdiparse(GDNode **pptree, void (*gcbparse)(char *pd, int ld));
int  gdinits(void);
size_t gdirecl(void);
void gdiclear(GDNode **pptree);
void gdiclose(void);

