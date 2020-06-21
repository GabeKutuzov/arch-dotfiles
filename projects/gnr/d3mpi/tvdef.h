/* (c) Copyright 1990-2018, The Rockefeller University *21115* */
/* $Id: tvdef.h 75 2017-10-13 19:38:52Z  $ */
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
************************************************************************
*  Rev, 08/10/90,  MC - Initial version                                *
*  V7C, 04/08/95, GNR - Add pointer to modality                        *
*  V8D, 02/09/07, GNR - Add items for BBD cameras                      *
*  ==>, 05/11/07, GNR - Last mod before committing to svn repository   *
*  V8F, 02/13/10, GNR - Add UPGM type and fn ptrs, icam, islot         *
*  Rev, 04/21/10, GNR - Add PREPROC and KERNEL structs and prototypes  *
*  Rev, 05/12/10, GNR - Move user-visible parts of PREPROC to UPREPR   *
*  R74, 05/27/17, GNR - New Matlab-compat color modes                  *
*  R78, 04/18/18, GNR - Add rounder seed, redefine tviflgs             *
***********************************************************************/

#include "utvdef.h"

#define qTV_MAXNM    "15"     /* Quoted max length of camera name */
#define qTV_MAXUD   "250"     /* Quoted max length of user data */
#define NRSHIST      256      /* Number of bins in TV_Scale histo */
#define RS_SEED     1111      /* Random rounding seed for rescale */

#ifndef PARn

/*---------------------------------------------------------------------*
*                                 Fif                                  *
*  This little structure is used to store interpolation tables for     *
*  image resizing in d3imgt.  It is defined here so d3snsa can know    *
*  its size in order to allocate it.                                   *
*---------------------------------------------------------------------*/

struct Fif { ui16 ipix; ui16 frac /* S16 */; };

/*---------------------------------------------------------------------*
*                   KERNEL (Data from KERNEL card)                     *
*  These are kernels for image preprocessing.  Note that when this     *
*  structure is allocated (node 0 only), enough space is provided      *
*  for 'kval' to contain the entire kernel data, not just one si32.    *
*                                                                      *
*  Kernel storage is arranged as follows for rapid run-time processing:*
*  To allow for nonrectangular kernels, each row contains (1) the off- *
*  set (in pixels, since this may be used with cameras with different  *
*  pixel sizes) from the left edge-nkx/2 to the first element to be    *
*  processed in that row, (2) the number of elements to be processed   *
*  in that row, and (3) the actual data in FS24 (FBik==24) format.     *
*                                                                      *
*  If there is more than one kernel in this KERNEL, each one starts    *
*  at offset ik*(nkx+2)*nky from kval so it can be found easily, even  *
*  though there will be some wasted space between them.                *
*--------------------------------------------------------------- -----*/

struct KERNEL {
   struct KERNEL *pnkn;       /* Ptr to next kernel */
   ui32  nkxy;                /* (nkx+2)*nky */
   txtid hknm;                /* Handle to kernel name */
   ui16  nker;                /* Number of kernels in this set */
   /* Code expects next two vars in order given */
   ui16  nkx;                 /* X dimension of kernels */
   ui16  nky;                 /* y dimension of kernels */
   ui16  kflgs;               /* Flags, with following values: */
#define IPK_BOX      0x0001      /* Box kernel */
#define IPK_DOG      0x0002      /* Dog kernel */
#define IPK_EDGE     0x0004      /* Edge kernel */
#define IPK_STGH     0x0008      /* Steerable filter */
#define IPK_USED     0x0100      /* Kernel is used */
#define IPK_CONV     0x0200      /* Kernel is a convolution */
#define IPK_NEGS     0x0400      /* Kernel has negatives */
   ui16  kpad;                /* Pad (future BC?) */
   si32  kval;                /* nker*((nkx+2)*nky+1) values (S24) */
   };
#endif

/*---------------------------------------------------------------------*
*               PREPROC (Image preprocessing parameters                *
*---------------------------------------------------------------------*/

struct PREPROC {
   struct PREPROC *pnip;      /* Ptr to next PREPROC */
   struct PREPROC *pnipc;     /* Ptr to next PREPROC on same camera */
   struct TVDEF *pipcam;      /* Ptr to source camera */
   int (*pipfn)(struct UPREPR *pupr, PIXEL *pin, PIXEL *pout);
                              /* Ptr to user processor function */
   struct UPREPR upr;         /* User-visible data */
   size_t oppoi;              /* Offset to output image in pbcst */
   size_t oppmx;              /* Offset to max over output image */
   size_t osfmx;              /* Offset to NOPVP max over angle */
   /* Display parameters: */
   float ipplx;               /* x displ in inches of TV from left */
   float ipply;               /* y displ in inches of TV from top  */
   float ipwd;                /* Width of image--0 for 1:1 map */
   float ipht;                /* Height of image (derived) */
   /* Derived constants used repeatedly in d3imgt(): */
   ui32  nytot;               /* Total y size of output */
   ui32  nyvis;               /* Y size visible to connections */
   txtid hipnm;               /* Handle to preprocess name */
   txtid hcamnm;              /* Handle to camera name */
   txtid hkernm;              /* Handle to kernel name */
   ui16  dcoff;               /* Display columns/rows separation */
   ui16  kpc;                 /* Kernels per plot column */
   byte  ipsflgs;             /* System flags */
/* Flags that may be stored in ipsflgs: */
#define PP_KERN        0x01      /* Use KERNEL interface */
#define PP_UPGM        0x02      /* Use UPGM interface */
#define PP_ON          0x04      /* This PP is used by somebody */
#define PP_STGH        0x08      /* This uses an STGH kernel */
#define PP_NEGS        0x10      /* Kernel has negatives */
#define PP_GETN        0x20      /* Get info for conntype norm */
#define PP_GETP        0x40      /* Get info for NOPVP treatment */
   byte  ipkbc;               /* Boundary condition option--
                              *  values defined in d3global.h */
   byte  dcols;               /* Number of display columns */
   byte  jkr0;                /* Value to control kernel repeat
                              *  according to color mode */
   };

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
   PIXEL *pmyim;              /* Ptr to image after averaging but before
                              *  cropping, in pbcst or just on Node 0 */
/* Parameters governing the image */
   struct UTVDEF utv;         /* Items passed to user program */
   size_t otvi;               /* Offset to image in pbcst or pipimin */
   size_t otvg;               /* Offset to gray-from-color image */
   size_t otvppgc;            /* Offset to GfmC for preprocessor */
   size_t otvmx;              /* Offset to max over output image */
/* Display parameters */
   float tvplx;               /* x displ in inches of TV from left */
   float tvply;               /* y displ in inches of TV from top  */
   float tvwd;                /* Width of image--0 for 1:1 map */
   float tvht;                /* Height of image (derived) */
/* Parameters for rescale option */
   si32  rsseed;              /* Random rounding seed (!PARn) */
   si16  rspct;               /* Target percentile for rescaling */
   ui16  rslvl[NColorDims];   /* Gray level rspct is scaled to (S14) */
/* Text locators */
   txtid hcamnm;              /* Handle to BBD,UPGM camera name */
   txtid hcamuf;              /* Handle to user file name */
/* tviflgs control the operations performed on a raw image before it
*  is moved to the broadcast area.  The numerical order of the flags
*  suggests the order in which the operations are performed.
*  N.B.  Time averaging is defined but not yet implemented.  */
   ui16  tviflgs;             /* Flags relating to image input */
#define TV_BigE      0x0100      /* Raw image is big-endian */
#define TV_Swap     TV_BigE      /* Swap byte order (After d3g1imin) */
#define TV_TAvg      0x0080      /* Perform temporal averaging */
#define TV_Reduc     0x0040      /* Raw image pix size > internal */
#define TV_SpAvg     0x0020      /* Perform spatial averaging */
#define TV_Scale     0x0010      /* Rescale gray levels */
#define TV_PPGC      0x0008      /* Gray from color for PP/vtouch */
#define TV_Filt      0x0004      /* Apply preprocess filter */
#define TV_IFit      0x0002      /* Pull tvx * tvy image for bcst */
#define TV_GfmC      0x0001      /* Gray from color */
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
