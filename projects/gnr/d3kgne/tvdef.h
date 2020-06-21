/* (c) Copyright 1991-2010, Neurosciences Research Foundation, Inc. */
/* $Id: tvdef.h 32 2010-10-19 15:53:04Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                          TVDEF Header File                           *
*                                                                      *
*  Definitions of structures (TVDEF, KERNEL, and PREPROC) and function *
*  prototypes that are used in conjunction with image inputs.          *
*                                                                      *
*  Note:  There is just one linked list for all camera interfaces and  *
*  frequencies.  In any given run, there will rarely be more than one  *
*  or two TVDEF blocks, so little time is wasted skipping irrelevant   *
*  ones, and then only at trial-level loops.                           *
*                                                                      *
*  Rev, 08/10/90,  MC - Initial version                                *
*  V7C, 04/08/95, GNR - Add pointer to modality                        *
*  V8D, 02/09/07, GNR - Add items for BBD cameras                      *
*  ==>, 05/11/07, GNR - Last mod before committing to svn repository   *
*  V8F, 02/13/10, GNR - Add UPGM type and fn ptrs, icam, islot         *
*  Rev, 04/21/10, GNR - Add PREPROC and KERNEL structs and prototypes  *
*  Rev, 05/12/10, GNR - Move user-visible parts of PREPROC to UPREPR   *
***********************************************************************/

#include "utvdef.h"

#define qTV_MAXNM "15"        /* Quoted max length of camera name */
#define qTV_MAXUD "250"       /* Quoted max length of user data */
#define RS_SEED   1111        /* Random rounding seed for rescale */

#ifndef PARn
/*---------------------------------------------------------------------*
*                   KERNEL (Data from KERNEL card)                     *
*  These are kernels for image preprocessing.  Note that when this     *
*  structure is allocated (node 0 only), enough space is provided      *
*  for 'kval' to contain the entire kernel data, not just one si32.    *
*---------------------------------------------------------------------*/

   struct KERNEL {
      struct KERNEL *pnkn;    /* Ptr to next kernel */
      txtid hknm;             /* Handle to kernel name */
      ui16  nker;             /* Number of kernels in this set */
      /* d3grp1 code expects next two vars in order given */
      ui16  nkx;              /* X dimension of kernels */
      ui16  nky;              /* y dimension of kernels */
      si32  kval;             /* nker*((nkx+2)*nky+1) values (S24) */
      };

/*---------------------------------------------------------------------*
*               PREPROC (Image preprocessing parameters                *
*---------------------------------------------------------------------*/

   struct PREPROC {
      struct PREPROC *pnip;   /* Ptr to next PREPROC */
      struct PREPROC *pnipc;  /* Ptr to next PREPROC on same camera */
      struct TVDEF *pipcam;   /* Ptr to source camera */
      int (*pipfn)(struct UPREPR *pupr, PIXEL *pin, PIXEL *pout);
                              /* Ptr to user processor function */
      struct UPREPR upr;      /* User-visible data */
      long  otvi;             /* Offset to output image in pbcst */
      /* Display parameters */
      float ipplx;            /* x displ in inches of TV from left */
      float ipply;            /* y displ in inches of TV from top  */
      float ipwd;             /* Width of image--0 for 1:1 map */
      float ipht;             /* Height of image (derived) */
      float dcoff;            /* Display columns offset */
      /* Derived constants used repeatedly in d3imgt() */
      ui32  lbkup;            /* Backup to start of kernel input */
      ui32  nytot;            /* Total y size of output */
      ui32  rppc;             /* Rows per plot column =
                              *  nipy*((nker+dcols-1)/dcols) */
      txtid hipnm;            /* Handle to preprocess name */
      txtid hcamnm;           /* Handle to camera name */
      txtid hkernm;           /* Handle to kernel name */
      ui16  kpc;              /* Kernels per plot column */
      ui16  nkx,nky;          /* X,Y dimensions of kernels */
      byte  ipsflgs;          /* System flags */
/* Flags that may be stored in ipsflgs: */
#define PP_KERN   0x01           /* Use KERNEL interface */
#define PP_UPGM   0x02           /* Use UPGM interface */
#define PP_ON     0x04           /* This PP is used by somebody */
      byte  ipkbc;            /* Boundary condition option--
                              *  values defined in d3global.h */
      byte  dcols;            /* Display columns option */
      byte  jkr0,jkri;        /* Values to control kernel repeat
                              *  according to color mode */
      };
#endif

/*---------------------------------------------------------------------*
*                      TVDEF (Camera parameters)                       *
*  Reminder:  On comp nodes, use otvi offset into pbcst, not pointer,  *
*  because of membcst.  TVDEF is allocated w/MBT_Unint so host node    *
*  can use untranslated pointers to input and PREPROCs.  This arrange- *
*  ment must be changed if preprocessing is to be done on comp nodes.  *
*---------------------------------------------------------------------*/

struct TVDEF {
   struct TVDEF *pnxtv;       /* Ptr to next TVDEF struct */
   struct MODALITY *ptvmdlt;  /* Pointer to modality of this TV--
                              *  NULL value means has no modality */
/* Following pointers will be valid on host node only: */
/* Pointers to user functions if this is a UPGM camera, else NULL */
   int (*tvinitfn)(struct UTVDEF *putv, char *camnm,
      const char *ufile, const char *udata, enum tvinit_kcall kcall);
   int (*tvgrabfn)(struct UTVDEF *putv, PIXEL *pimage);
/* Pointers used to facilitate processing in trial loop */
   struct PREPROC *pmyip;     /* List of PREPROCS using this camera */
   PIXEL *pmyin;              /* Ptr to image in pbcst or pipimin */
/* Parameters governing the image */
   struct UTVDEF utv;         /* Items passed to user program */
   long  otvi;                /* Offset to image in pbcst or pipimin */
/* Display parameters */
   float tvplx;               /* x displ in inches of TV from left */
   float tvply;               /* y displ in inches of TV from top  */
   float tvwd;                /* Width of image--0 for 1:1 map */
   float tvht;                /* Height of image */
/* Parameters for rescale option */
   si16  rspct;               /* Target percentile for rescaling */
   si16  rslvl;               /* Gray level rspct is rescaled to */
/* Text locators */
   txtid hcamnm;              /* Handle to BBD,UPGM camera name */
   txtid hcamuf;              /* Handle to user file name */
   } ;

/* Prototype routines that use the above structures */
#ifndef PARn
struct TVDEF *findtv(txtid hcam, char *erstring);
struct KERNEL *findkn(txtid hker, char *erstring);
struct PREPROC *findpp(txtid hip, char *erstring);
void tvdflt(struct TVDEF *cam);
void prepdflt(struct PREPROC *pip);
#endif
void tvplot(void);
