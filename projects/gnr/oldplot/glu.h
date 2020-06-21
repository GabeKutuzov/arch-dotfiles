/* (c) Copyright 1993-2017, The Rockefeller University *21114* */
/* $Id: glu.h 34 2017-09-15 18:13:10Z  $ */
/***********************************************************************
*                            GLU.H                                     *
*                                                                      *
*             ROCKS-to-plot library glue routines header               *
*                                                                      *
***** THIS COPY KLUDGED TO PERMIT COMPILATION OF OLD PLOT LIBRARY ******
*                                                                      *
*  N.B.  Enhancements to oldplot to be implemented in new plot lib:    *
*     (1) Extended length of frame number in new frame ('[') record.   *
*     (2) Check all error codes vs errtab.                             *
*     (3) abexit when metafile exceeds 2GB except in 64-bit code.      *
*     (4) Separate control of mfdraw/metafile per frame.               *
*     (5) Bitmap 24-bit color and scaled bitmaps.                      *
*     (6) Printing text of error messages received from mfdraw.        *
*     (7) Allow early mfsynch direct from application                  *
*     (8) Allow _RKG to be saved across mex calls                      *
************************************************************************
*  V1A, 04/20/89, JWT                                                  *
*  Rev, 10/01/90, GNR - Add setmovie and movie-related variables       *
*  Rev, 06/19/92, ROZ - Add support for NSI graphic metafile           *
*  Rev, 08/12/92, ROZ - Add MFDRAW and SUN4 stuff                      *
*  Rev, 07/02/93, ABP - Change SUN4 conditional to PAR (SYNCH_MSG).    *
*  Rev, 07/16/93, GNR - Clean up, remove snapshot support              *
*  Rev, 01/03/94,  LC - Added currcolStr, currcolType fields           *
*  Rev, 02/29/08, GNR - Arrangements for finplt and kout               *
*  ==>, 02/29/08, GNR - Last mod before committing to svn repository   *
*  Rev, 10/11/11, GNR - Change _NCG to _RKG, I_AM_NGRAPH to I_AM_SETMF *
*                       Merge in contents of gmf.h and geomdefs.h      *
*                       Merge new plot glu.h changes except binary mf  *
*  Rev, 03/16/13, GNR - Add currcolType 'J' to for 'K' output          *
*  Rev, 07/20/16, GNR - Modify for MPI environment                     *
*  R32, 04/26/17, GNR - Fix bug: newplt sending finplt data 2nd time   *
*  R34, 08/01/17, GNR - Bug fix: endless wait mfckerr->abexit->endplt  *
***********************************************************************/

#ifndef __GLUH__
#define __GLUH__

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "sysdef.h"
#include "rkxtra.h"
#include "mfio.h"
#include "plotdefs.h"

/*** Define DEBUG as sum of values for desired debug outputs ***/
/* (These are compile-time tests so no time wasted in real runs) */
#define MFDBG_NWPLT  0x01  /* Debug output when new plot starts */
#define MFDBG_SYNCH  0x02  /* Debug output when a buffer is written */
#define MFDBG_CLOSE  0x04  /* Debug output from mfclose() */
#define MFDBG_FINPL  0x08  /* Debug o/p when endplt|finplt called */
#define MFDBG_WRITE  0x10  /* Debug output from mfwrite() */
#define DEBUG 0

/*---------------------------------------------------------------------*
*                              Constants                               *
*---------------------------------------------------------------------*/

/* IP address of local host */
#define LocalHostIP "127.0.0.1"

#define DFLTBUFLEN 4092    /* Default buffer length */
#define LEN_STATE_CMDS 25  /* Len of state variables in CmdBuff */

#define INTOPIX 69.8182    /* Inches to Pixels */
#define PPI INTOPIX        /* Pixels per inch (768/11.0) */
#define MAXSUFFIX 16       /* Maximum suffix length */
#define MAXSTRING 32       /* Maximum label size */

#define PI      3.141592654
#define TORADS  0.017453293/* Degrees to radians */
#define COSBS   0.866025   /* cosine pi/6 */
#define SINBS   0.5        /* sin pi/6 */

#define ASPECT   0.857143  /* Aspect ratio of standard symbols */
#define AXLBLH   1.333333  /* Aspect ratio of axis centered label */
#define EXPTIME  2.0       /* Minimum time for one movie exposure */
#define LABELHT  0.14      /* Height of axis labels */
#define LABELOFF 0.36      /* Offset of plot labels from axis */
#define MINORHT  0.10      /* Standard height for minor tick marks */
#define MAJORHT  0.15      /* Standard height for major tick marks */
#define MINARROW 0.02      /* Minimum arrow length */
#define MXTVLEN 12         /* Max tick value length */
#define POLARHT  0.0625    /* Half-length of polar plot tick marks */
#define POLARLS  0.10      /* Extra radius of polar long spokes */
#define SCALEHT  0.15      /* Standard height for axis scale ticks  */
#define SUPEROFF 0.333333  /* Allowance for height of superscript */
#define VALUGARD 0.05      /* Spacing of value labels from ticks */
#define VALUEHT  0.105     /* Standard height for axis value labels */
#define VALUEOFF 0.20      /* Standard offset of axis value labels  */

#define ON  1
#define OFF 0

#define STDCOL 10          /* Number of standard defined colors */

#define WHITE   0          /* "BLACK" on a CRT */
#define BLACK   1          /* "WHITE" on a CRT */
#define BLUE    2
#define CYAN    3
#define MAGENTA 4
#define VIOLET  5
#define ORANGE  6
#define GREEN   7
#define YELLOW  8
#define RED     9

/* Maximum lengths of some commands in 'A' format */
#define MFA_SIZE   30               /* Size of A NGM command */
#define MFB_SIZE   21               /* Size of B NGM command */
#define MFC_SIZE   18               /* Size of C NGM command */
#define MFD_SIZE   12               /* Size of D NGM command */
#define MFE_SIZE   30               /* Size of E NGM command */
#define MFI_SIZE    5               /* Size of I NGM command */
#define MFH_SIZE    3               /* Size of H NGM command */
#define MFK_SIZE(n) (3+n)           /* Size of K NGM command */
#define MFL_SIZE   22               /* Size of L NGM command */
#define MFP_SIZE(n) (10*n+5)        /* Size of P NGM command */
#define MFR_SIZE   23               /* Size of R NGM command */
#define MFS_SIZE(n) (27+n)          /* Size of S NGM command */
#define MFT_SIZE(n) (3+n)           /* Size of T NGM command */
#define MFSTSIZE(n) (27+n)          /* Size of [ NGM command */

/*---------------------------------------------------------------------*
*                               Macros                                 *
*---------------------------------------------------------------------*/

#define scale(c) ((int)((c)*_RKG.fact*PPI))
#define hexch_to_int(c) ( ((c) <= '9') ? (c) - '0' : (c) - 'A' + 10 )
/* Generate bcdout decimal conversion mode parameter:
*     a = number of decimals, b = total number of digits */
#define RKD(a,b) ((RK_IORF+RK_SNGL+(a)*RK_D+(b)-1))
/* Generate wbcdwt integer conversion mode parameter:
*     a = number of digits */
#define RKDI(a)  ((RK_IORF+RK_NINT+(a)-1))
#define RKDB(a)  ((RK_IORF+RK_NBYTE+(a)-1))
#define RKDH(a)  ((RK_IORF+RK_NHALF+(a)-1))
#define RKD32(a) ((RK_IORF+RK_NI32+(a)-1))

/*---------------------------------------------------------------------*
*                           Data structures                            *
*---------------------------------------------------------------------*/

/* The _RKG structure contains all the state of the graphics package.
*  The inner structure, s, contains the shared data that is broadcast
*  from node 0 to all other nodes at start up.  Types are ui32 rather
*  than int so swap4 can be used safely.  */

struct RKGSdef {
   float base_fact;        /* Initial factor value */
   ui32 dbgmask;           /* Debug flags for mfsr/mfdraw */
   ui32 MFBuffLen;         /* Length of metafile command buffer */
   ui32 MFMode;            /* Holds the mode of operation:  */
#define METF    0x04          /* Metafile */
#define SGX     0x08          /* X graphics */
#if METF != SKP_META || SGX != SKP_XG
#error METF != SKP_META || SGX != SKP_XG
#endif
#define MFSO    0x20          /* Metafile written to stdout */
#define ASCMF   0x40          /* Metafile written in ASCII mode */
#define MFD8C   0x80          /* mfdraw8 compatibility */
   ui32 MFActive;          /* MFMode filtered by skip bits  */
   short lcf;              /* Length of coordinate fractions */
   short lci;              /* Unused configuration parameter */
   };

struct RKGdef {
   struct RKGSdef s;       /* Shared data */
#ifndef PARn
   FILE *MFfdes;           /* Metafile file descriptor */
   char *MFfn;             /* Metafile file name */
   char *MFDdisplay;       /* X-Windows display */
   char *MFDwicon;         /* X-windows window icon */
   char *MFDwtitl;         /* Window title */
#endif
   unsigned char *MFCmdBuff;  /* Ptr to Metafile command buffer */
   unsigned char *MFCurrPos;  /* Ptr to MFCmdBuff current pos */
   unsigned char *MFTopPos;   /* Ptr to top of MFCmdBuf */
   unsigned char *MFFullBuff; /* Ptr to CountWord = MFCmdBuff - 4 */
   float xcurr, ycurr;     /* Current (x,y) */
   float fact;             /* Current factor value */
   ui32 MFCountWord;       /* Byte count and status bits */
   ui32 XGFrame;           /* XG frame number */
   ui32 MFFrame;           /* Metafile frame number */
   ui32 currcol;           /* Current plot color, color index `I` */
   int curretrac;          /* Current line retrace (krt) */
   int currtype;           /* Current pentype */
#ifndef PARn
   ui32 totcount;          /* Total metafile length */
   int  MFDsock;           /* mfdraw socket */
   int  MFDpid;            /* MFDraw process id */
#endif
   struct ackbutt ackb;    /* Button info from bpause or host */
   short nexp;             /* Number of exposures per frame */
   schr MFPending;         /* Count of sgx frames sent, not ack'ed */
   /* N.B.  MFF_MFDrawX indicates mfdraw has died, but mfsr remains
   *  alive until told to terminate by an MFB_CLOSE message.  This
   *  allows parallel app to complete writing metafile frame, then
   *  inform all nodes and do a graceful exit resulting in MFF_MEnded,
   *  whence mfsr may be assumed dead.  */
   byte MFFlags;           /* Status flags, initially 0 */
#define MFF_DidFinplt   1  /* TRUE if finplt called before newplt */
#define MFF_ModeSet     2  /* TRUE if MFMode setup done */
#define MFF_HdrSent     4  /* TRUE if metafile header sent */
#define MFF_DeferAck    8  /* TRUE if an mfwrite ack is deferred */
#define MFF_MFDrawX    16  /* TRUE if mfdraw already dead */
#define MFF_SetMFRan   32  /* TRUE if setmf has run */
#define MFF_EndPend    64  /* TRUE if endplt has been entered */
#define MFF_MEnded    128  /* TRUE if endplt() was executed */
   char movie_device;      /* 0 for Matrix, 1 for Beeper */
   char movie_mode;        /* Current movie mode */
   char currcolStr[9];     /* Current plot color, color string 'K' */
   char currcolType;       /* Switch for 'I' or 'K' value */
   };


#ifdef I_AM_SETMF
struct RKGdef _RKG = {
   {                       /* Begin RKGSdef struct */
      1.0,                    /* base_fact */
      0,                      /* dbgmask */
      DFLTBUFLEN,             /* MFBuffLen */
      0,                      /* MFMode */
      0,                      /* MFActive */
      0,0,                    /* lcf, lci */
      },
#ifndef PARn
   NULL,                   /* MFfdes */
   NULL,                   /* MFfn */
   NULL,                   /* MFDdisplay */
   NULL,                   /* MFDwicon */
   NULL,                   /* MFDwtitl */
#endif
   NULL,                   /* MFCmdBuff */
   NULL,                   /* MFCurrPos */
   NULL,                   /* MFTopPos */
   NULL,                   /* MFFullBuff */
   0.0, 0.0,               /* xcurr, ycurr */
   1.0,                    /* fact */
   0,                      /* MFCountWord */
   0, 0,                   /* XGFrame, MFFrame */
   BLACK,                  /* currcol */
   0,                      /* curretrac */
   0,                      /* currtype */
#ifndef PARn
   0,                      /* totcount */
   0,                      /* MFDsock */
   0,                      /* MFDpid */
#endif
   {                       /* Begin ackbutt struct */
      0,0,0,0,                /* ackc field */
      {  MM_MOVIE-1,          /* movie mode = movie */
         0,                   /* next frame */
         0,                   /* interrupt  */
         0,                   /* snapshot */
         0, 0 }               /* gfrm, gwin */
      },
   0,                      /* nexp */
   0,                      /* MFPending */
   0,                      /* MFFlags */
   0,                      /* movie_device */
   MM_MOVIE-1,             /* movie_mode   */
   "BLACK",                /* currcolStr */
   'I',                    /* currcolType */
   };
#else
extern struct RKGdef _RKG;
#endif

/* Prototype internal plot routines */
int bpause(void);
void i2a(unsigned char *, int, int, int);
#define DECENCD 0   /* 4th i2a arg--integer conversion */
#define FORCESZ 1   /* Force full length given in arg */

#endif   /* not defined __GLUH__ */
