/* (c) Copyright 1992-2018, The Rockefeller University *11117* */
/* $Id: mpitools.h 65 2018-08-15 19:26:04Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                             MPITOOLS.H                               *
*  Application and machine independent structures, prototypes,         *
*  and definitions for parallel processing (now MPI) applications.     *
*                                                                      *
*    DESIGN CONSIDERATIONS REGARDING MESSAGE LENGTH VARIABLE TYPES     *
*                                                                      *
*  Most data length calculations are done with type 'long', matching   *
*  data in the nxdr2 infrastructure and most likely the same as size_t.*
*  Arithmetic on these types will not be checked for overflow, as this *
*  will be very unlikely in 64-bit systems.  However, lengths that are *
*  arguments to message-passing functions will be checked for overflow *
*  when the function argument type is 'int' and this is a shorter type.*
*                                                                      *
*  Traditionally, length arguments to the allocp routines were ints.   *
*  This has been preserved for compatibility, with the thought that    *
*  it is very unlikely to want to allocate blocks > 2GB across nodes.  *
*----------------------------------------------------------------------*
*  WARNING:  Throughout long history, we have assumed that a process   *
*  number always fits in an integer.  If this ever becomes a problem,  *
*  all variables containing process numbers should be redeclared as    *
*  pid_t (but then this typedef may not exist on all processor nodes). *
*----------------------------------------------------------------------*
*  V1A, 12/18/92, ABP - Initial Version of nsitools.h, from nsincube.h *
*  Rev, 05/28/94, GNR - Remove application-specific error codes,       *
*                       add swapping prototypes and macros, reformat   *
*  Rev, 11/16/96, GNR - Remove nreadr, nreadl, nwriter, nwritel,       *
*                       revise ifdefs--assume all systems are hybrid,  *
*                       allow host to support Xptr, NCUBE, or both.    *
*  Rev, 11/13/97, GNR - Remove swapping protos and macros to swap.h    *
*  V2A, 06/27/98, GNR - Remove memory management to memshare.h         *
*  Rev, 07/25/98, GNR - anread[p], anwrite[p] now routines, not macros *
*  Rev, 06/05/99, GNR - Add new nn[get|put][i2|i4|r4|r8] routines      *
*  Rev, 09/04/99, GNR - Eliminate NCUBE support                        *
*  ==>, 08/29/07, GNR - Last date before committing to svn repository  *
*  Rev, 12/30/09, GNR - Add nsitvers, C++ wrapper                      *
*  ==>, 04/11/16, GNR - Renamed mpitools.h, changes for use with MPI   *
*  Rev, 04/23/16, GNR - Eliminate unneeded longs, add nn[get|put]i[lz] *
*  Rev, 06/19/16, GNR - Add auwrite, auread, remove nioinit            *
*  Rev, 08/13/16, GNR - Add NXDRTT,NXDRUT tables pushdown stack        *
*  Rev, 08/06/18, GNR - Make inclusion of mpi.h depend on PAR defined  *
*  R66, 09/01/18, GNR - Add mpirex, mpiwex                             *
***********************************************************************/

#ifndef MPITOOLS_HEADER_INCLUDED
#define MPITOOLS_HEADER_INCLUDED

#include <unistd.h>
#include "sysdef.h"
#include "rkarith.h"

/* Here are a few definitions that will make it easier to work
*  with a different parallel library than MPI if that becomes
*  necessary:  */
#if defined(USE_MPI) && defined(PAR)
#include "mpi.h"
typedef MPI_Request AN_Request;
typedef MPI_Status  AN_Status;
typedef MPI_Comm    AN_Comm;
#define ANYNODE MPI_ANY_SOURCE
#define ANYTYPE MPI_ANY_TAG
#define is_host(node) ((node) == 0)
#else
/* Just leaving some old junk here 'JIC' */
typedef void AN_Request;
typedef int  AN_Status;
typedef int  AN_Comm;
#define ANYNODE -1
#define ANYTYPE -1
#define is_host(node) ((node) < 0)
#endif

/* "Official" way to determine if running multinode */
#define IAmMulti  (NC.dim > 0)

/* Relative node (including host) from absolute node */
#define rnode(anode) \
   ((anode) == NC.hostid ? 0 : (anode) - NC.headnode + 1)
/* Absolute node from relative node */
#define anode(rnode) \
   ((rnode) == 0 ? NC.hostid : (rnode) + NC.headnode - 1)

/* Length overflow checking */
#if LSIZE > ISIZE
#define ckul2i(il,fnm) (il > INT_MAX ? \
   e64act(fnm,EAabx(86)), INT_MAX : (int)il)
#else
#define ckul2i(il,fnm) ((int)il)
#endif
#if ZSIZE > ISIZE
#define ckz2i(iz,fnm) (iz > INT_MAX ? \
   e64act(fnm,EAabx(86)), INT_MAX : (int)iz)
#else
#define ckz2i(iz,fnm) ((int)iz)
#endif

/* The following structure contains information used by the nnget and
*  nnput routines to define node-to-node streams.  Note:  lengths in
*  most mpi get/send routines are int, so lbsr is int, not size_t.  */

struct NNSTR {
   byte *buff;       /* Ptr to start of message buffer */
   byte *bptr;       /* Ptr to current location in msg buffer */
   AN_Request *preq; /* Ptr to request handle */
   int lbsr;         /* Length of buffer space remaining */
   int nnode;        /* Partner in the node-to-node stream */
   int type;         /* Message type for this stream */
   int flags;        /* Flags */
   };
typedef struct NNSTR NNST;

/* Items used for accessing the pushdown conversion tables */
#define MBTMask  0x3fffffff /* Mask for isolating mbtype */
#define ttloc(ix) (NC.pnxtt[NC.jnxtt]+(ix&MBTMask))
#define NTTPD  3     /* Number of NXDRTT pushdowns allowed */
typedef void unicvtf(struct NNSTR *, void **, void *, int);

/*---------------------------------------------------------------------*
*                   NODE CONFIGURATION ASSUMPTIONS                     *
*  The following structure contains information about the machine.     *
*  For some sort of compatibility with earlier versions of this code,  *
*  we will use the following conventions:                              *
*  NC.total = total number of nodes (processes) on whole execution.    *
*  NC.hostid = node number for PAR0 node, sometimes called host = 0.   *
*  NC.headnode = node number of lowest PARn node = 1.                  *
*  NC.tailnode = node number of highest PARn node.                     *
*                                                                      *
*  N.B.  The official way for a program to determine dynamically       *
*  whether it is running on a serial or parallel machine is to test    *
*  NC.dim, which will be negative only on a serial machine.  The main  *
*  program must set this at the beginning of execution.                *
*---------------------------------------------------------------------*/

#ifndef MAIN
extern
#endif
   struct NCINFO {
   long *pnxtt[NTTPD+1];     /* Pushdown stack for NXDRTT tables */
   unicvtf **pnxut[NTTPD+1]; /* Pushdown stack for NXDRUT tables */
   AN_Comm commc;    /* Communicator for computational cluster */
   AN_Comm commd;    /* Communicator for debug/abort messages */
   AN_Comm commmf;   /* Communicator for metafile graphics */
   int procid;       /* Process ID of process on this processor */
   int total;        /* Number of nodes in complete system */
   int hostid;       /* Destination for messages to host (PAR0) */
   int mfsrid;       /* Destination for messages to mfsr */
   int dmsgid;       /* Destination for messages to debug/abort node */
   int headnode;     /* Id of lowest comp node (!PAR0, !mfsr) */
   int tailnode;     /* Id of highest comp node */
   int cnodes;       /* Number of comp nodes = tail - head + 1 */
   int dim;          /* Was dimension of hypercube, now is bitsize
                     *  of (tailnode-headnode) (<0 if serial or
                     *  parallel and there are no comp nodes) */
   int node;         /* Absolute node number of this processor */
   int jnxtt;        /* Index into pnxtt,pnxut stacks */
   ui32 debug;       /* Debug flag used for various purposes */
#define DBG_DBGWT 1     /* Wait for debugger at node (debug >> 8) */
#define DBG_NOXIT 2     /* Loop rather than exit on error */
#define DBG_START 4     /* Print messages during startup */
#define DBG_ERLOG 8     /* Divert stderr messages to andlog */
#define DBG_WTALL 0x80  /* Wait for debugger at all nodes */
   } NC;

/*---------------------------------------------------------------------*
*                     Message passing definitions                      *
*---------------------------------------------------------------------*/

/* Name of the main program (process) that is to be spawned on Node 0
*  to receive amd print debug messages, halt messages, anything else
*  "out of stream".  */

#define DMSGPGM "andmsg"

/* Name of the main program (process) that is to be spawned on Node 0
*  to receive and consolidate graphics command buffers.  If needed,
*  this could be moved to another node, e.g. to get an outside socket.
*/

#define MFSRPGM "mfsr"

/* A dummy node id used to denote a broadcast.  All buffered nio
*  (nnput, nnget, nncom) with this address as the partner node are
*  actually broadcasts going from the host to all computational nodes.
*  Do not use with plain anread/anwrite pairs!  A large number is
*  chosen, under the assumption that we'll never have that many
*  nodes...  (famous last words) */

#define BCSTADDR 0x7FFFFFFF

/* Messages passed with nncom, nnput, nnget are all not longer than
*  this value.  The value of 8 KB that was in use on the NCUBE is here
*  changed to 8000 Bytes to allow room for possible header info added
*  by MPI library functions.  */

#define MAX_MSG_LENGTH  8000     /* Longest data in one message */
#define MAX_EMSG_LENGTH  161     /* Longest text of an error msg
                                 *  (including final '\0') */

/*---------------------------------------------------------------------*
*                      Message type definitions                        *
*                                                                      *
*  Message types defined here are generic to all applications using    *
*  this library, and hence should not be preempted by any application. *
*  A complete list of all known message types across all operating     *
*  systems, libraries, and applications is supposed to be kept in      *
*  the file "msgtypes.doc" in the nsi include directory.               *
*---------------------------------------------------------------------*/

/* Numeric values are in application range for historical reasons... */
#define INIT_ABORT_MSG   0x3100  /* Initiate abort sequence */
#define EXEC_ABORT_MSG   0x3101  /* Execute abort */
#define DEBUG_MSG        0x3102  /* Send a debug message to host */
#define SHUTDOWN_ANDMSG  0x3103  /* Tell andmsg to shut down */
#define CLOSING_ANDMSG   0x3104  /* andmsg acks shutdown */
#define ISYNCH_MSG       0x3110  /* Message used by isynch() */
#define VECOLL_MSG       0x3111  /* Collect/combine vectors */
#define CPPACK_MSG       0x3112  /* COPPACK -- pack and combine */
#define IMAX_MSG         0x3113  /* Integer max over array */
#define JMAX_MSG         0x3114  /* max over array */
#define WMAX_MSG         0x3115  /* si64 max over array */
#define ISUM_MSG         0x3116  /* Integer sum over array */
#define JSUM_MSG         0x3117  /* si32 sum over array */
#define WSUM_MSG         0x3118  /* si64 sum over array */

/*---------------------------------------------------------------------*
*                  Exit message structures and codes                   *
*---------------------------------------------------------------------*/

struct ErrMsg {
   int ec;              /* Error Code */
   int src;             /* Id of process/node reporting the error */
   int iv;              /* Identifying integer value */
   int ltxt;            /* Length of message text, or 0 if none */
   };

struct exit_msg {       /* Msg used to pass parms to node 0 */
   char *pit;           /* Ptr to identifying text */
   int  ec;             /* Error code */
   int  iv;             /* Identifying integer value */
   };

/* Error code for driver exit (a general name for an exit request from
*  a "non CNS" process), moved here from d3global.h */

#define DRVEXIT_ERR 450 /* Exit requested by driver */

/*---------------------------------------------------------------------*
*                         Function prototypes                          *
*---------------------------------------------------------------------*/

/* N.B.  I am leaving as ints those lengths that were traditionally
*  declared as ints in the NCUBE library (and are still ints in MPI)--
*  those that were longs are now size_t.  */

#ifdef __cplusplus
extern "C" {
#endif
void *angetp(size_t size);
void *angetpv(size_t, char *);
int  anread(void *buffer, int nbytes, int *src, int *type, char *cflg);
int  anreadp(void **buffer,int nbytes,int *src, int *type, char *flag);
int  anrelp(void *buffer);
void anrelpv(void *, char *);
int  antest(int *src, int *type);
int  anwait(AN_Request *prqst, AN_Status *pstat);
int  anwrite(void *buffer, int nbytes, int dest, int type, char *cflg);
int  anwritep(void *buffer,int nbytes, int dest, int type, char *flag);
/* auread/auwrite not yet implemented, planned unblocking read/write */
int  auread(void *buffer, int nbytes, int *src, int *type, char *cflg);
int  auwrite(void *buffer, int nbytes, int dest, int type, char *cflg);
#ifdef GCC
void appexit(char *txt, int err, int ival) __attribute__ ((noreturn));
#else
void appexit(char *txt, int err, int ival)
#endif
void aninit(ui32 opts, ui32 debug, ui32 mfdbg);
#define ANI_DBMSG    0x01  /* opts code to install debug msg comm */
#define ANI_MFSR     0x02  /* opts code to install mfsr */
#define ANI_HOST     0x04  /* Specify host in MPI_Comm_spawn calls */
void dbgprt(char *msg);
int  imax(int);
int  isum(int);
void isynch(void);
si32 jmax(si32);
si32 jsum(si32);
void mpirex(int type, int node, int rc,
   char *fnm) __attribute__ ((noreturn));
void mpiwex(int type, int node, int rc,
   char *fnm) __attribute__ ((noreturn));
char *mpitvers(void);
void nxtupush(long *qnxtt, unicvtf **qnxut);
void nxtupop(void);
si64 wmax(si64);
si64 wsum(si64);

void nncom(struct NNSTR *pnn, void **ppobj, long *pnxtab, int flags);
/* Values for nncom flags argument: */
#define NNC_Send        0  /* Send data to partner node */
#define NNC_Rcv         1  /* Receive and translate normally */
#define NNC_SendPtrs    2  /* Send only pointers */
#define NNC_RcvPtrs     3  /* Receive only pointers */
#define NNC_RcvUnint    5  /* Receive uninitialized data--
                           *  ignore untranslatable pointers */
void nngcl(struct NNSTR *pnn);
void nngcr(struct NNSTR *pnn, int node, int type);
void nnget(struct NNSTR *pnn, void *item, int ilen);
int  nngetp(struct NNSTR *pnn, void **pbfr);
char   nngetch(struct NNSTR *pnn);
short  nngeti2(struct NNSTR *pnn);
si32   nngeti4(struct NNSTR *pnn);
si64   nngeti8(struct NNSTR *pnn);
ui64   nngetu8(struct NNSTR *pnn);
long   nngetil(struct NNSTR *pnn);
ulng   nngetul(struct NNSTR *pnn);
size_t nngetiz(struct NNSTR *pnn);
float  nngetr4(struct NNSTR *pnn);
double nngetr8(struct NNSTR *pnn);
void nngsk(struct NNSTR *pnn, int ilen);
void nnpcl(struct NNSTR *pnn);
void nnpcr(struct NNSTR *pnn, int node, int type);
void nnpfl(struct NNSTR *pnn);
void nnput(struct NNSTR *pnn, void *item, int ilen);
void nnputch(struct NNSTR *pnn, char   ch);
void nnputi2(struct NNSTR *pnn, short  i2);
void nnputi4(struct NNSTR *pnn, si32   i4);
void nnputi8(struct NNSTR *pnn, si64   i8);
void nnputu8(struct NNSTR *pnn, ui64   u8);
void nnputil(struct NNSTR *pnn, long   il);
void nnputul(struct NNSTR *pnn, ulng   ul);
void nnputiz(struct NNSTR *pnn, size_t iz);
void nnputr4(struct NNSTR *pnn, float  r4);
void nnputr8(struct NNSTR *pnn, double r8);
void nnpsk(struct NNSTR *pnn, int jlen);
char *nnputp(struct NNSTR *pnn, int ilen);

#ifndef PARn
void fatale(char *s1, char *s2);
void fatalpe(char *s1, char *s2);
#endif

/*---------------------------------------------------------------------*
*    Definitions and prototypes for the timing statistics utility      *
*---------------------------------------------------------------------*/

#ifndef TIMERS
#define stattmr(a,b)  /* */

#else
extern void stattmr(int,int);

/* Op codes for calls to stattmr */
#define OP_PUSH_TMR  21 /* Start a new timer on top of timer stack */
#define OP_POP_TMR   22 /* Pop timer from head of stack */
#define OP_STOP_ALL  23 /* Stop all timers (end of stat gathering) */
#define OP_RESET_ALL 24 /* Clear all readings, stop all timers */
#define OP_PRINT_ALL 25 /* Printf reading of all timers */

/* Identifiers for timers.  Note:  Some of these are application-
*  specific, some are library-specific.  For now, we just put them
*  here, where they are visible to everybody.  */
#define GRPH_TMR  0     /* Graphichs timer */
#define CALC_TMR  1     /* Number crunching timer */
#define COMM_TMR  2     /* Inter node communications timer */
#define GDFL_TMR  3     /* Graph data file timer */
#define SVFL_TMR  4     /* Network save file timer */
#define TVPL_TMR  5     /* TV plot timer */
#define DFLT_TMR  6     /* Default (overhead) timer */

#define NUMOFTIMERS 7 /* Number of timers */
#endif
#ifdef __cplusplus
}
#endif
#endif /* Not defined MPITOOLS_HEADER_INCLUDED */
