/* (c) Copyright 1992-2009, The Rockefeller University *21115* */
/* $Id: nsitools.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                             NSITOOLS.H                               *
*  Application and machine independent structures, prototypes,         *
*  and definitions for applications using the hybrid tools library.    *
*                                                                      *
*  V1A, 12/18/92, ABP - Initial Version, from nsincube.h               *
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
***********************************************************************/

#ifndef NSITOOLS_HEADER_INCLUDED
#define NSITOOLS_HEADER_INCLUDED

#include <unistd.h>
#ifdef ICC
#include <channel.h>
#include <process.h>
#endif   /* ICC */

/* Number and definition for clients of the nio process.  This
*  process provides internode I/O services for processes on the
*  host machine */

#define NIOCL_HOST 0     /* Host process */
#define NIOCL_DRVR 1     /* "Driver" process */
#define NIOCL_MFSR 2     /* mfsr process */
#define NIOCL_TOPL 3     /* To_plat process */
#define NIOCL_TOCN 4     /* To_cns procss */
#define NUMNIOCLNT 5     /* Number of clients for nio process */

/* Macro for testing whether the source or destination of a
*  message is on the host (as opposed to the processor array).
*  Here is the "right" way to do it:
*  #define is_host(node)
*     ((node) >= NC.hostid && (node) < NC.hostid + NUMNIOCLNT)
*  But here is a faster way that is safe with present definitions:
*/
#define is_host(node) ((node) < 0)

/* The following structure contains information about the machine.
*  N.B.  This struct must define the same for all host processes and
*  the same for all node processes on any one kind of array.  However,
*  it need not be the same on host and array.
*  N.B.  The official way for a program to determine dynamically
*  whether it is running on a serial or parallel machine is to test
*  NC.dim, which will be negative only on a serial machine.  The main
*  program must set this at the beginning of execution.
*/

#ifndef MAIN
extern
#endif
   struct NCINFO {
   int node;         /* Relative node number of this processor */
   int procid;       /* Process ID of process on this processor */
   int hostid;       /* Destination for messages to host */
   int dim;          /* Dimension of hypercube */
   int total;        /* Number of nodes in hypercube = 1<<dim */
   int totalm1;      /* = total - 1 */
   int headnode;     /* Id of first (lowest) node on system */
   int debug;        /* Debug flag used for various purposes */
#define DBG_DBGWT 1  /* Wait for inquest or other debugger */
#define DBG_NOXIT 2  /* Loop rather than exit on error */
#ifndef PARn
/* These host items might not be used with all types of attached
*  processors, however, we leave them defined because we hope to
*  be able to write a universal driver.  */
   int dbx;          /* dbx debuging mask:  all 32 bits */
   int niorqp[NUMNIOCLNT]; /* fds for nio request pipes */
   int niorsp[NUMNIOCLNT]; /* Response fd for every nio client */
   int nmlh[2];      /* T/NM lo and hi sockets */
   int abrtpp;       /* Pipe for INIT_ABORT messages */
   int hmniof;       /* Host machine nio flags, defined below */
#define HNIO_NC  1   /* Machine is with ncube--obsolete */
#define HNIO_TR  2   /* Machine is with xputers */
#define HNIO_SE  4   /* Machine is single ended */
#define HNIO_DE  8   /* Machine is double ended */
#define HNIO_UP 32   /* Machine is upright */
#define HNIO_NP 64   /* Platform communication should not occur */
#endif
#if !defined (PARn) || defined(RING)
   int tailnode;     /* Id of last (highest) node on rign */
   int machid;       /* Id of machine (for future sub-machines) */
   int upperhalf;    /* Id of lowest node in upper half */
   int lowerhalf;    /* Id of highest node in lower half */
#endif

#if defined(PARn) && defined(RING)
   int bcstnbr;      /* Broadcast neighbour, see nio.c */
   int backnbr;      /* Broadcast prompt neighbour, see nio.c */
#ifdef INMOS8
   int hostsock;     /* Host socket descriptor on external nodes */
   int flags;        /* Flags, defined below */
#define NIO_FLG_DBLEND  0x1  /* Outer ring is double ended */
#define NIO_FLG_UPRGHT  0x2  /* This machine is "upright" */
#define NIO_FLG_HOST    0x4  /* This node has a direct host link */
   Channel *hostout; /* Host channel, on internal nodes */
   Channel *aorqch;  /* Application output request channel */
   Channel *aorsch;  /* Application output response channel */
   Channel *airqch;  /* Application input request channel */
   Channel *airsch;  /* Application input response channel */
   Channel *errqch;  /* En route message output req. channel */
   Channel *errsch;  /* En route message output res. channel */
   Process *ninproc; /* Pointer to nioin process */
   Process *nouproc; /* Pointer to nioout process */
   Process *hiproc;  /* Host input process, on external xputers */
#endif
#endif

   } NC;

/* The following structure contains information used by the nnget and
*  nnput routines to define node-to-node streams. */

   struct NNSTR {
      byte *buff;       /* Ptr to start of message buffer */
      byte *bptr;       /* Ptr to current location in msg buffer */
      long node;        /* Partner in the node-to-node stream */
      int lbsr;         /* Length of buffer space remaining */
      int type;         /* Message type for this stream */
      };

/*---------------------------------------------------------------------*
*                     Message passing definitions                      *
*---------------------------------------------------------------------*/

#define ANYNODE -1
#define ANYTYPE -1

/* A dummy node id used to denote a broadcast.  All anread/anwrite
*  (and their relatives) as well as buffered nio (nnput, nnget, nncom)
*  with this address as the partner node are actually broadcasts going
*  from the host to all computational nodes.  This value must be
*  positive, since negative node ids are reserved for host processes,
*  and a large number is chosen, under the assumption that we'll never
*  have that many nodes...  (famous last words) */

#define BCSTADDR 0x7FFFFFFF

/* A dummy node id for debug messages, which are forwarded to the host
*  and printed there to stderr.  */

#define DBGADDR (BCSTADDR-1)

/* A dummy node id for exchange messages.  These are handled by the
*  nioin process, by forwarding one copy to our downstream neighbour,
*  unless he is the origin of the message, and keeping a second copy
*  for local consumption.  Not implemented on hypercube geometries--
*  see d3exch() for typical application.  */

#define EXCHADDR  (BCSTADDR-3)

/* Messages passed with nncom, nnput, nnget are all not longer than
*  this value.  The value of 8 KB that was in use on the NCUBE is here
*  changed to 8000 Bytes, since 8KB is also the maximum length allowed
*  for a single socket write() on the current TCP/IP implementation on
*  our network.  Application messages longer than 8K after a routing
*  header is added are long enough to cause fragmented transmission.
*/

#define MAX_MSG_LENGTH  8000

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

#define ISYNCH_MSG       0x3110  /* Message used by isynch() */
#define VECOLL_MSG       0x3111  /* Collect/combine vectors */

/*---------------------------------------------------------------------*
*                  Exit message structures and codes                   *
*---------------------------------------------------------------------*/

struct ErrMsg {
   long ec;             /* Error Code */
   long src;            /* Id of process/node reporting the error */
   long iv;             /* Identifying integer value */
   long ltxt;           /* Length of message text, or 0 if none */
   };

struct exit_msg {       /* Msg used to pass parms to node 0 */
   long ec;             /* Error code */
   char *pit;           /* Ptr to identifying text */
   long iv;             /* Identifying integer value */
   };

/* Error code for driver exit (a general name for an exit request from
*  a "non CNS" process), moved here from d3global.h */

#define DRVEXIT_ERR         450   /* Exit requested by driver */

/*---------------------------------------------------------------------*
*                         Function prototypes                          *
*---------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif
void *angetp(long size);
void *angetpv(long, char *);
int  anread(void *buffer, int nbytes, int *src, int *type, char *cflg);
int  anreadp(void **buffer,int nbytes,int *src, int *type, char *flag);
int  anrelp(void *buffer);
void anrelpv(void *, char *);
int  antest(int *src, int *type);
int  anwrite(void *buffer, int nbytes, int dest, int type, char *cflg);
int  anwritep(void *buffer,int nbytes, int dest, int type, char *flag);
void awhoami(int *nodep, int *procp, int *hostp, int *dimp);
void bkpt(void);
void dbgprt(char *msg);
long imax(long);
long isum(long);
void isynch(void);
void ncinfo(void);
char *nsitvers(void);

#ifdef INMOS8
int nioinit(char *ARGV[]);
void nioerr(char *txt);    /* Wait for death */
#else
int nioinit(int sp);
#endif

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
short  nngeti2(struct NNSTR *pnn);
long   nngeti4(struct NNSTR *pnn);
si64   nngeti8(struct NNSTR *pnn);
ui64   nngetu8(struct NNSTR *pnn);
float  nngetr4(struct NNSTR *pnn);
double nngetr8(struct NNSTR *pnn);
void nngsk(struct NNSTR *pnn, int ilen);
void nnpcl(struct NNSTR *pnn);
void nnpcr(struct NNSTR *pnn, int node, int type);
void nnpfl(struct NNSTR *pnn);
void nnput(struct NNSTR *pnn, void *item, int ilen);
void nnputi2(struct NNSTR *pnn, short  i2);
void nnputi4(struct NNSTR *pnn, long   i4);
void nnputi8(struct NNSTR *pnn, si64   i8);
void nnputu8(struct NNSTR *pnn, ui64   u8);
void nnputr4(struct NNSTR *pnn, float  r4);
void nnputr8(struct NNSTR *pnn, double r8);
void nnpsk(struct NNSTR *pnn, int jlen);
char *nnputp(struct NNSTR *pnn, int ilen);
void waitdbx(void);

#ifndef PARn
void fatale(char *s1, char *s2);
void fatalpe(char *s1, char *s2);
#endif

#ifdef NEWPOOLS
typedef void unicvtf(struct NNSTR *, void **, void *, int);
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
#define D1G1_TMR  6     /* d1go() stage I timer   */
#define D1EX_TMR  7     /* d1exch() timer  */
#define D1G3_TMR  8     /* d1go() stage III timer */
#define DFLT_TMR  9     /* Default (overhead) timer */

#define NUMOFTIMERS 10 /* Number of timers */
#endif
#ifdef __cplusplus
}
#endif
#endif /* Not defined NSITOOLS_HEADER_INCLUDED */
