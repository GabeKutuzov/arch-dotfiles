/* (c) Copyright 2007-2017, The Rockefeller University *11115* */
/* $Id: bbd.h 62 2017-09-15 17:47:40Z  $ */
/***********************************************************************
*                       BBD Package Header File                        *
*                                bbd.h                                 *
*                                                                      *
*  The BBD package allows a real or virtual BBD to communicate with    *
*  the CNS cortical network simulator (or some other software) in      *
*  order that CNS may serve as a nervous system simulator for the BBD. *
*                                                                      *
*  This header file provides both client-side and server-side          *
*  definitions and prototypes in order that definitions that need      *
*  to be the same in both libraries are defined in one place.          *
*  Server routines may use rksubs-level ROCKS routines, client         *
*  routines will not.  Define preprocessor variable BBDS for server    *
*  compilations, otherwise client definitions will be generated.       *
*                                                                      *
*  All multi-byte data (numeric setup data, value, 16-bit color,       *
*  effector commands) are transmitted in big-endian form between       *
*  processes so client and server machines can use different byte      *
*  orderings.  Other "neural" camera and sense signals are trans-      *
*  ferred as 8-bit bytes and therefore processor byte ordering is      *
*  not significant for these data.                                     *
*                                                                      *
************************************************************************
*  V1A, 02/01/07, GNR - New program                                    *
*  Rev, 11/23/07, GNR - Add event signalling                           *
*  V1B, 05/02/08, GNR - Add wait argument to bbdsevnt                  *
*  Rev, 08/14/08, GNR - Break hnm down into chnm and shnm              *
*  ==>, 08/14/08, GNR - Last mods before committing to svn repository  *
*  Rev, 09/02/08, GNR - Add bbdciniv, bbdsputm, and bbdcgetm calls     *
*  Rev, 04/09/10, GNR - Minor type changes for 64-bit compilation      *
*  V1C, 11/05/10, GNR - Add fields for BBDMEX and isn,ign return       *
*  Rev, 01/15/11, GNR - Add bbdssvrc() routine                         *
*  Rev, 04/18/13, GNR - Add external access to bbdcquit, bbdsquit      *
*  R64, 12/10/15, GNR - Add x,y dims for effectors                     *
*  R66, 03/25/16, GNR - Change BBDPortInit to 10002, rm BBDPortData    *
*  R29, 05/31/17, GNR - Update bdflgs for additional color options     *
***********************************************************************/

#ifndef BBDhdr
#define BBDhdr

#include <sys/types.h>
#include <semaphore.h>
#include "sysdef.h"
#include "swap.h"

/*---------------------------------------------------------------------*
*               Definitions common to client and server                *
*---------------------------------------------------------------------*/

#define CkIntIncr      11        /* Check integer increment */
#define MaxDevNameLen  15        /* Max length of a device name */
#define qMDNL         "15"       /* Quoted MaxDevNameLen */
/* This is not defined for SUNOS */
#ifdef SUN4
#define ssize_t int
#endif

/* IP address for local loopback */
#define LocalHostIP "127.0.0.1"

/* Following default port number is from IANA unassigned range.
*  Rev, 03/25/16, GNR - Old value 49998 did not work on Mac OS,
*     went to smaller value 10002.  Removed BBDPortData, made
*     code always use (possibly input) BBDPortInit + 1.  */
#define BBDPortInit 10002        /* Port for CNS startup */

/* Codes used to signal events to the N/W server */
#define BBDSigReset     1        /* The N/W should reset */
#define BBDSigImage     2        /* An event image is ready */
#define BBDSigQuit      4        /* Server should terminate normally */

/* One data type to hold them all ... */
typedef struct BBDDev_t {
   struct BBDDev_t *pndev;       /* Ptr to next device */
   ui16 bdflgs;                  /* BBD device flags */
/* Type-specific bits may be defined in the upper byte of bdflgs.
*  The lower byte is common to all device types and the BBDTMask
*  portion defines which element of the UD union is in use.
*  Common bdflgs */
#define BBDTMask       0x0007       /* Mask for type codes */
#define BBDTSns        0x0001       /* Device is a sensor */
#define BBDTCam        0x0002       /* Device is a camera */
#define BBDTVal        0x0003       /* Device is a value */
#define BBDTEff        0x0004       /* Device is an effector */
#define BBDMdlt        0x0008       /* Device feeds a modality */
#define BBDUMask       0x0070       /* Bits that may differ */
#define BBDUsed        0x0010       /* Device is used */
#define BBDLink        0x0020       /* Device is linked */
#define BBDOpen        0x0040       /* Device path is open */
#define BBDBig         0x0080       /* Data are big-endian */
/* Sensor bdflgs */
#define BBDSns_Byte         0       /* Transmit 1-byte data */
#define BBDSns_R4      0x0400       /* Transmit float  data */
/* Camera bdflgs */
#define BBDColMask     0x001f       /* Mask for color codes */
#define BBDColShft          8       /* Shift color<-->bdflgs */
#define BBDCol_C8        0x10       /* Legacy C8 mode (2-3-3 color) */
#define BBDCol_C16       0x11       /* Legacy C16 mode (5-5-5 color) */
#define BBDCol_Pal       0x12       /* Legacy Palette (color index) */
#define BBDCol_GS        0x00       /* Grayscale image */
#define BBDCol_Col       0x08       /* Colored image */
#define BBDCol_C24       0x08       /* Combo code for 24-bit color */
#define BBDCol_Int       0x00       /* Unsigned integer data */
#define BBDCol_Flt       0x04       /* Real, i.e. floating point data */
#define BBDCol_B8        0x03       /* 8 bytes per color */
#define BBDCol_B4        0x02       /* 4 bytes per color */
#define BBDCol_B2        0x01       /* 2 bytes per color */
#define BBDCol_B1        0x00       /* 1 byte per color */
#define BBDCFrMask     0x0003       /* Camera grab frequency mask */
#define BBDCFrShft         13       /* Shift frequency<-->bdflgs */
#define BBDCFr_TRIAL     0x01       /* Grab on every trial */
#define BBDCFr_EVENT     0x02       /* Grab on designated event */
#define BBDCFr_SERIES    0x03       /* Grab on new series */
   ui16 bdid;                    /* Id code for link checking */
#define BBDLC_MASK     0x0fff       /* Mask for valid bdids */
/* Note:  The protocol now is that all BBDLC_TERM messages from
*  server to client will have a 4-byte error code following.
*  Messages from client to server will not have a code.  */
#define BBDLC_TERM     0x1000       /* Terminate server */
#define BBDLC_EVNT     0x2000       /* Signal code follows */
#define BBDLC_MSG      0x4000       /* Message length and text
                                    *  follows */
   si32 ldat;                    /* Number of data items/cycle
                                 *  (or bytes of camera data) */
   char devnm[MaxDevNameLen+1];  /* Name of the device */
/* Communications section */
#ifdef BBDS
   struct BBDsHost *pshost;      /* Ptr to host info */
#endif
/* Device-specific section */
   union {                       /* Section for each device type */
      struct {                   /* SENSOR section */
         byte *psa;                 /* Ptr to sense data (static) */
         si32 snsx,snsy;            /* Sense array size */
         ui16 *pisgn;               /* Ptr to stim. cat. info */
         } Sns;
      struct {                   /* CAMERA section */
         byte *pim;                 /* Ptr to the image (static) */
         si32 camx,camy;            /* Image size (pixels) */
         ui16 *pisgn;               /* Ptr to stim. cat. info */
         } Cam;
      struct {                   /* VALUE section */
         float *pval;               /* Ptr to the value (static) */
         } Val;
      struct {                   /* EFFECTOR section */
         float *pmno;               /* Ptr to motor neuron output */
         si32 effx,effy;            /* Cells x ntc dims */
         } Eff;
      } UD;
   } BBDDev;

struct BBDComData {              /* Communications structure */
   BBDDev *pfeff;                /* Ptr to first effector */
   BBDDev *pfsns;                /* Ptr to first virtual sense */
   BBDDev *pfttv;                /* Ptr to first trial camera */
   BBDDev *pfetv;                /* Ptr to first event camera */
   BBDDev *pfstv;                /* Ptr to first series camera */
   BBDDev *pfval;                /* Ptr to first value */
   /* Following pointers used to manipulate list order */
   BBDDev **ppfeff;              /* Ptr to end of effector list */
   BBDDev **ppfsns;              /* Ptr to end of sense list */
   BBDDev **ppfttv;              /* Ptr to end of tcam list */
   BBDDev **ppfetv;              /* Ptr to end of ecam list */
   BBDDev **ppfstv;              /* Ptr to end of scam list */
   BBDDev **ppfval;              /* Ptr to end of value list */
#ifdef BBDS                      /* Stuff only used on the server */
   struct BBDsHost *phlist;      /* Ptr to list of hosts */
   ui32 BBDSigCode;              /* Signal passed to server */
   int svrc;                     /* Return code for bbdsquit */
#else                            /* Stuff only used on the client */
   struct RFdef *ndsend;         /* Neural data send file */
   struct RFdef *ndrecv;         /* Neural data receive file */
   sem_t *pMinpSem;              /* Ptr to bbdcminp semaphore */
   sem_t *pMlogSem;              /* Ptr to bbdcmlog semaphore */
   char *chnm;                   /* Client host name or IP */
   char *shnm;                   /* Server host name or IP */
   char *exsi;                   /* Extra server input info */
   char **ppvdefs;               /* bbdcminp arg ptrs and vardefs */
#ifndef I_AM_MEX
   char *smsg;                   /* Message received from server */
   si32  lmsg;                   /* Length allocated to smsg */
#endif
   pid_t LogProc;                /* ID of log process */
   pid_t InpProc;                /* ID of input process */
   volatile int Minprc;          /* Return code from bbdcminp */
   volatile int Mlogrc;          /* Return code from bbdcmlog */
   int AbexLoop;                 /* Termination loop detector */
   int Cnssock;                  /* Socket for CNS communication */
   int DataPort;                 /* Port for data exchange */
   int Vclients;                 /* Clients that should get VMSG */
#ifdef I_AM_MEX
#define NOMINPLOG 2                 /* 'retver' bit for no minp log */
   int neffs;                    /* Number of effectors */
#else
#define NOMINPLOG 0x010000          /* 'who' bit for no minp log */
#endif
#endif
   };

/* Function prototypes */
#ifdef __cplusplus
extern "C" {
#endif
char *bbdvers(void);
#ifdef __cplusplus
}
#endif

#ifdef BBDS
/*---------------------------------------------------------------------*
*                     Definitions for server only                      *
*---------------------------------------------------------------------*/

/* ABEXIT codes--Range 135-139 is assigned to server-side package */
#define BBDsErrSelect  135    /* Select error checking socket */
#define BBDsErrParams  136    /* Parameter values out of range */
#define BBDsErrNoHost  137    /* BBDHOST was not received */
#define BBDsErrBadPort 138    /* Bad character in port number */
#define BBDsErrBadData 139    /* Problem exchanging data */

struct BBDsHost {             /* Information about one client host */
   struct BBDsHost *pnhost;      /* Ptr to next host */
   struct RFdef *ndsend;         /* Neural data send file */
   struct RFdef *ndrecv;         /* Neural data receive file */
   char *pbbdnm;                 /* Ptr to host name */
   int  bbdport;                 /* BBD device port */
   ui16 labdid;                  /* Look-ahead bdid from bbdsevnt */
   byte qlabd;                   /* TRUE if labdid has data */
   byte kpute;                   /* Client file flags: */
#define BBDsClosed   1              /* Path to client closed */
#define BBDsGotEff   2              /* TRUE if effectors here */
   };

/* Function prototypes */
#ifdef __cplusplus
extern "C" {
#endif
void   bbdsinit(void);
struct BBDsHost *bbdsfhnm(char *phnm);
BBDDev *bbdsrege(char *effnm, char *phnm, int mnad);
BBDDev *bbdsregs(char *snsnm, char *phnm, si32 snsx, si32 snsy,
   int ksns);
BBDDev *bbdsregt(char *camnm, char *phnm, si32 camx, si32 camy,
   int kcol, int kfreq);
BBDDev *bbdsregv(char *valnm, char *phnm);
int    bbdschk(void);
int    bbdsgets(void);
int    bbdsgett(int kfreq);
int    bbdsgetv(void);
ui32   bbdsevnt(int wait);
void   bbdspute(int quit);
void   bbdsputm(char *msg, int who);
void   bbdsquit(void);
void   bbdssvrc(int rc);
#ifdef __cplusplus
}
#endif

#else
/*---------------------------------------------------------------------*
*                     Definitions for client only                      *
*---------------------------------------------------------------------*/

#define LMxClSPAA        8    /* Length of mex client subproc args */
#define qMxClSPAfmt    "%7"   /* ssprintf length for subproc args */
#define MinpSemNm "/BBDMEX_MINP" /* Semaphore for bbdcminp */
#define MlogSemNm "/BBDMEX_MLOG" /* Semaphore for bbdcmlog */
#define NoEXSI    "NoBBDCEXSI"   /* Indicate no BBDCEXSI data */
#ifdef _ISOC99_SOURCE
#define URW_Mode S_IRUSR|S_IWUSR
#else
#define URW_Mode S_IREAD|S_IWRITE
#endif

/* ABEXIT codes--Ranges 133-134+140-149+560-569 are assigned to
*  client package */
#define BBDcErrExsi     133   /* bbdcexsi called twice or too late */
#define BBDcErrSema     134   /* Error using POSIX semaphore */
#define BBDcErrServer   140   /* Server (CNS) abended */
#define BBDcErrSocket   141   /* Failed to create socket */
#define BBDcErrConnect  142   /* Failed to open connection */
#define BBDcErrFork     143   /* Failed to fork child process */
#define BBDcErrLogFile  144   /* Unable to read/write/close log */
#define BBDcErrControl  145   /* Unable to copy control file */
#define BBDcErrParams   146   /* Parameter values out of range */
#define BBDcErrChkConf  147   /* Configuration does not check */
#define BBDcErrBadData  148   /* Problem exchanging data */
#define BBDcErrMexCall  149   /* Bad arguments in a mex call */
#define BBDcErrNoHome   150   /* getenv("HOME") returned NULL */

#define BBDcErrInitArgs 560   /* Wrong number of args to bbdcinit */
#define BBDcErrValStr   561   /* Vardef value must be a string */
#define BBDcErrVarNLong 562   /* A variable name is too long */
#define BBDcErrVarValue 563   /* A variable value is too long */
#define BBDcErrCtlArg   564   /* Bad control file arg */
#define BBDcErrLogArg   565   /* Bad log file arg */
#define BBDcErrRetVer   566   /* Bad type or dimension of retval */
#define BBDcErrHostName 567   /* Bad host name argument */
#define BBDcErrPortNum  568   /* Bad port number argument */
#define BBDcErrVDLong   569   /* Variable definition exceeds CDSIZE */

/* Function prototypes */
#ifdef __cplusplus
extern "C" {
#endif
int  bbdcinit(char *phost, ui32 port, char *pctrl, char *plog);
int  bbdciniv(char *phost, ui32 port, char *pctrl, char *plog,
   int who);
BBDDev *bbdcrege(char *effnm, float *pmno, int mnad);
BBDDev *bbdcregs(char *snsnm, byte *psa, si32 snsx, si32 snsy,
   int ksns);
BBDDev *bbdcrsid(char *snsnm, byte *psa, ui16 *pisgn,
   si32 snsx, si32 snsy, int ksns);
BBDDev *bbdcregt(char *camnm, byte *pim, si32 camx, si32 camy,
   int kcol, int kfreq);
BBDDev *bbdcrtid(char *camnm, byte *pim, ui16 *pisgn,
   si32 camx, si32 camy, int kcol, int kfreq);
BBDDev *bbdcregv(char *valnm, float *pval);
int  bbdcchk(void);
int  bbdcgete(BBDDev *pdev);
char *bbdcgetm(void);
void bbdcevnt(ui32 ecode);
void bbdcexsi(const char *exstring);
void bbdcputs(BBDDev *pdev);
void bbdcputt(BBDDev *pdev);
void bbdcputv(BBDDev *pdev);
#ifdef I_AM_MEX
void bbdcquit(struct BBDComData *BBDcd);
#else
void bbdcquit(void);
#endif
void bbdcwait(void);
#ifdef __cplusplus
}
#endif

#endif /* !BBDS */

#endif /* BBDhdr */
