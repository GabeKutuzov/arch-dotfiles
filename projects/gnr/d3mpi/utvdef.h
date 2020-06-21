/* (c) Copyright 2018, The Rockefeller University *11115* */
/* $Id: utvdef.h 77 2018-03-15 21:08:14Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                         UTVDEF Header File                           *
*                                                                      *
*  This parameter block contains camera parameters that can be passed  *
*  to a user program providing images to CNS.  It is a subset of the   *
*  information in the TVDEF header that is sanitized to remove types   *
*  and defs that the user program need not have access to.  There is   *
*  space for the user code to store a few things (e.g. maybe a pointer *
*  to an RFDEF) where state information can be stored between calls.   *
*                                                                      *
*  Additionally, a UPREPR parameter block is defined that will be      *
*  passed to user-written image preprocessors for similar purposes.    *
*                                                                      *
*  Information regarding image size is now encoded as follows in       *
*  TVDEF->utv in pixels (accessible to rest of CNS).  Each of these    *
*  sizes is equal to or smaller than the previous one:                 *
*  tvrxe,tvrye    Entered (max) values for tvrx,tvry (for mem alloc)   *
*  tvrx,tvry      Actual raw image size                                *
*  tvix,tviy      Size after spatial averaging                         *
*  tvx,tvy        Final size seen by connection types                  *
*                                                                      *
*  tvix,tviy will be the size of the array passed as input to any      *
*  preprocessors; tvx,tvy will be the size broadcast to other nodes.   *
*                                                                      *
*  Dealing with images of different sizes:  The quantities above will  *
*  refer to the image(s) expected in a run--larger ones will be crop-  *
*  ped and smaller ones will be centered with a border added (so pre-  *
*  processors have max access to edges).  tvx,tvy are thus constant so *
*  our present Lij setup can work.  tvix,tviy will be left with the    *
*  size of the image before any cropping, fitting, or expansion.       *
*                                                                      *
*  Reminder:  Pixel sizes here are always before any gray-from-color   *
*  calculations, as those may coexist with 3-color pixels in same run. *
************************************************************************
*  V8F, 02/13/10, GNR - New header file                                *
*  ==>, 02/13/10, GNR - Last mod before committing to svn repository   *
*  Rev, 05/12/10, GNR - Add UPREPR                                     *
*  Rev, 06/01/10, GNR - Add utvinfo                                    *
*  R74, 05/16/17, GNR - Update tvkcol for additional color options     *
*  R74, 06/05/17, GNR - New size parameters as described above         *
***********************************************************************/

typedef byte PIXEL;           /* Pixel type */
#define NUTVINFO          4   /* Bytes of info for SAVE UTVINFO */
#define NPDHISTB         16   /* Number bins in PREPR density histo */
#if NPDHISTB != 16
#error Pixel density histo assumes 16 bins--recode in d3imgt to change
#endif

/*---------------------------------------------------------------------*
*       Define information for user-written image preprocessor         *
*---------------------------------------------------------------------*/

struct UPREPR {
   struct KERNEL *pipk;       /* Ptr to kernel block */
   void *pupd;                /* Ptr to arbitrary user data */
   ui16  nipx,nipy;           /* Size of preproc input (pixels) */
   ui16  nppx,nppy;           /* Size of preproc output (pixels) */
   ui16  nkx,nky;             /* Combined size of all kernels */
   ui16  oipx,oipy;           /* X,Y offsets of output in input */
   /* Derived constants used repeatedly in d3imgt() */
   ui32  lppi3;               /* HWords in one PP output image */
   ui16  nker;                /* Number of kernels */
   byte  ipuflgs;             /* User option flags */
/* Flags that may be stored in ipuflgs: */
#define PP_PLOT        0x01      /* 'P' Opt--Draw image supers */
#define PP_OTITL       0x02      /* 'O' Opt--Omit image title */
#define PP_2SYMM       0x04      /* '2' Opt--Apply twofold symmetry */
#define PP_GfmC        0x08      /* 'G' Opt--Gray from color input */
#define PP_HIST        0x10      /* 'D' Opt--Print density histogram */
#define PP_ROOT        0x20      /* 'R' Opt--Use sqrt(STGH2 data) */
/* ipkcol describes color mode of output from this preprocessor.
*  If PP_GfmC is set, it is GS or GS16, otherwise color of camera */
   byte  ipkcol;              /* Color mode (BM_GS16 or BM_C48) */
   byte  sncol;               /* Number of colors at setup */
   byte  tncol;               /* Number of colors--current image */
   } ;

/*---------------------------------------------------------------------*
*               Define a camera (could be still or TV)                 *
*---------------------------------------------------------------------*/

/* Use this macro to determine whether an image (after d3imgt returns)
*  has byte or ui16 pixels (odd pixel size has to be bytes)  */
#define qBytePixels(cam) (cam->utv.tvopxsz & 1)

struct UTVDEF {
   void  *ptvsrc;             /* Ptr to camera source info */
   byte  *p1row;              /* Space for user pgm to work on a row */
   /* ltvrxy is tvrxe*tvrye*pixsz during setup to allow memory alloc
   *  for largest possible image, then is reset to actual value when
   *  each new image is read in d3imgt().  Similarly for ltvixy.  */
   size_t ltvrxy;             /* Length of raw image (bytes) */
   size_t ltvixy;             /* Length of processed image (bytes) */
   size_t ltvxy;              /* Length of image seen by network */
   size_t tvrlen;             /* Length of one camera row--stored
                              *  here when tvinitfn runs, if needed */
   ui32  ntvxy;               /* tvx*tvy */
   ui32  tvsflgs;             /* System flags */
/* Flags that may be stored in tvsflgs: */
#define TV_ON        0x0001      /* This TV is used or plotted */
#define TV_ONDIR     0x0002      /* This TV is used directly */
#define TV_ONPP      0x0004      /* This TV is used via preprocessor */
#define TV_ONGfmC    0x0008      /* This TV is used via EXGfmC */
/* TV_VVTV,TV_BBD,TV_UPGM must match EVTVTV,EVTBTV,EVTUTV, bzw. */
#define TV_VVTV      0x0010      /* Use VVTV interface */
#define TV_BBD       0x0020      /* Use BBD interface */
#define TV_UPGM      0x0040      /* Use UPGM interface */
/* Combos of frequency and interface for faster run-time tests.
*  N.B.  These are generated by shifts in d3tchk and must not
*  be redefined without modifying that code accordingly.  */
#define TV_TriVVTV   0x0080      /* Series freq VVTV, etc. */
#define TV_TriBBD    0x0100
#define TV_TriUPGM   0x0200
#define TV_EvtVVTV   0x0400
#define TV_EvtBBD    0x0800
#define TV_EvtUPGM   0x1000
#define TV_SerVVTV   0x2000
#define TV_SerBBD    0x4000
#define TV_SerUPGM   0x8000
#define TV_ONPPGC  0x010000      /* GfmC used as preproc/vitp input */
#define TV_GETN    0x040000      /* Conntype wants scale normalized */
   ui16  tvrx,tvry;           /* Size of raw image (pixels) AFTER any
                              *  tvr[xy]o crop--may change per image */
   ui16  tvrxe,tvrye;         /* Entered (rqst) values of tvr[xy] */
   ui16  tvix,tviy;           /* Size after averaging (pixels) */
   ui16  tvx,tvy;             /* Size of image sent to networks */
   /* tvr[xy]o are offsets of samples from left/top edges of image to
   *  be applied in VVTV or UPGM image readers to allow efficient crop-
   *  ping at the source.  May change for each image if tvuflgs
   *  TV_RCTR is set.  */
   ui16  tvrxo,tvryo;         /* x,y source cropping offsets */
   /* tv[xy]o are offsets of tvx*tvy samples from reduced, averaged
   *  images that are passed to networks (PARn nodes if parallel).
   *  Different CONNTYPEs can perform different cropping using offsets
   *  in the KGEN rules.  */
   ui16  tvxo,tvyo;           /* Cropping offsets to extract tv[xy]  */
   ui16  tvuflgs;             /* User flags */
/* Flags that may be stored in tvuflgs: */
#define TV_UFL1      0x0001      /* '1' Opt--User-defined code 1 */
#define TV_UFL2      0x0002      /* '2' Opt--User-defined code 2 */
#define TV_PLOT      0x0004      /* 'P' Opt--Plot image supers */
#define TV_PISG      0x0008      /* 'I' Opt--Plot isn,ign */
#define TV_OTITL     0x0010      /* 'O' Opt--Omit image title */
#define TV_RESCG     0x0020      /* 'R' Opt--Rescale gray levels */
#define TV_RESC3     0x0040      /* '3' Opt--Rescale 3 colors indiv */
#define TV_RESCL     0x0060      /* 'R' or '3' */
#define TV_BYPR      0x0080      /* 'B' Opt--Bypass reset */
#define TV_ADJB      0x0100      /* 'A' Opt--Adjacent border */
#define TV_ICTR      0x0200      /* 'C' Opt--Cntr tv[xy] on tvi[xy] */
#define TV_RCTR      0x0400      /* 'D' Opt--Cntr tvr[xy] on image  */
#define TV_IFIT      0x0800      /* 'F' Opt--Fit tvi[xy] to tv[xy]  */
   ui16  isn;                 /* Stimulus number returned by camera */
   ui16  ign;                 /* Group number */
   short iadapt;              /* Adaptiveness value (S8) */
   byte  utvinfo[NUTVINFO];   /* Arbitrary data for SIMDATA */
#define UTVInf_INST       0      /* Instance of the stimulus */
#define UTVInf_ELEV       1      /* Elevation of the stimulus */
#define UTVInf_AZIM       2      /* Azimuth of the stimulus */
#define UTVInf_LGHT       3      /* Lighting condition */
   byte  icam;                /* Camera number (from 1) */
   byte  islot;               /* Slot number (for multicam cards) */
   byte  tvsa;                /* Spatial averaging factor */
   byte  tvta;                /* Temporal averaging factor */
   byte  tvncol;              /* 1 if grayscale, 3 if colored */
/* tvkicol describes feed from camera into CNS and uses codes given
*  here to provide a wide variety of BBD-and Matlab-compatible modes.
*  When used with BBDs, these definitions must match the color modes
*  defined in bbd.h (max 5 bits available).  d3imgt is expected to
*  convert any single color or gray inputs to byte or ui16 and
*  describe in tvkocol as one of the plotdefs.h BM_xxxx modes.  Note
*  that code 0 is interpreted as the default TVC_GS|TVC_Int|TVC_B1.
*  Fiat:  If a UPGM interface can read images with TVC_ACHAN, they
*  are expected to remove it and store just 3-color data that the rest
*  of CNS can handle.  A possible future change would be to code all
*  the d3imgt/d3sj/d3gvin routines to index colors skipping A channel
*  so A channel could be read into memory and just left there.  */
   byte  tvkicol;                /* Camera color mode supplied to CNS */
#define TVC_PNE        0x40      /* Not entered on PREPROC */
#define TVC_ACHAN      0x20      /* A channel present */
#define TVC_C8         0x10      /* Legacy C8 mode (2-3-3 color) */
#define TVC_C16        0x11      /* Legacy C16 mode (5-5-5 color) */
#define TVC_Pal        0x12      /* Legacy Palette mode (color index) */
#define TVC_GS         0x00      /* Grayscale image */
#define TVC_Col        0x08      /* Colored image */
#define TVC_Int        0x00      /* Unsigned integer data */
#define TVC_Flt        0x04      /* Real, i.e. floating point data */
#define TVC_B8         0x03      /* 8 bytes per color */
#define TVC_B4         0x02      /* 4 bytes per color */
#define TVC_B2         0x01      /* 2 bytes per color */
#define TVC_B1         0x00      /* 1 byte per color */
#define TVC_MASK       0x1f      /* Mask to extract color def bits */
/* tvkocol describes images output by d3imgt and used internally in
*  CNS.  Values are BM_xxxx codes from plotdefs.h.  Some code
*  assumes we cannot create color if it is not in the image.  */
   byte  tvkocol;             /* d3imgt() output color mode */
/* tvipxsz is size of input pixels in bytes.  Does not include any
*  information, e.g. alpha channel, removed by input routine.  */
   byte  tvipxsz;
   byte  tvopxsz;             /* Output (storage) pixel size (bytes) */
   byte  tvkfreq;             /* Access frequency */
/* Flags that may be stored in tvkfreq (these are the same as the
*  corresponding BBDCFr options, but are given different names
*  to allow future differentiation of these options):  */
#define UTVCFr_TRIAL   0x01      /* Grab on every trial */
#define UTVCFr_EVENT   0x02      /* Grab on designated event */
#define UTVCFr_SERIES  0x03      /* Grab on new series */
   } ;

/* Prototype routines that use UTVDEF struct */
/* Values for kcall argument: */
enum tvinit_kcall {
   TV_INIT = 0,               /* UPGM initialization call */
   TV_RESET,                  /* UPGM reset call */
   TV_RESTART,                /* UPGM restart (new UDATA) call */
   TV_CLOSE };                /* UPGM close files call */
int tvinitfn(struct UTVDEF *putv, char *camnm,
   const char *ufile, const char *udata, enum tvinit_kcall kcall);
int tvgrabfn(struct UTVDEF *putv, PIXEL *pimage);
/* Prototype routine that uses UPREPR struct */
int preprfn(struct UPREPR *pupr, PIXEL *pin, PIXEL *pout);
