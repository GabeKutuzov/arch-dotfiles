/* (c) Copyright 1993-1999, Neurosciences Research Foundation, Inc. */
/* $Id: xpnio.h 51 2013-10-29 20:59:39Z  $ */
/***********************************************************************
*                      NeuMachine Library Header File                  *
*                               xpnio.h                                *
*                                                                      *
*  Structures and constants used by the transputer/NeuMachine          *
*  implementation of the internode I/O routines, part of the T/NM      *
*  nsitools library.                                                   *
*                                                                      *
*  Written by Ariel Ben-Porath                                         *
*  V1A, 03/24/93 Initial version                                       *
*  Rev, 10/18/93, GNR - Increase NQBUFSIZE to 256                      *
*  Rev, 05/23/99, GNR - Use new swapping scheme with init messages     *
*  ==>, 07/01/02, GNR - Last date before committing to svn repository  *
***********************************************************************/

#include "swap.h"

/* Header for nio messages exchanged between host and COMPXP--
*  using current processor's byte order */

/*** N.B.  WHEN REWRITING FOR MPI, CHANGE TYPES OF THESE
**** VARIABLES TO 32-bit OR 64-bit TYPES, NOT long  ****/

typedef struct _tag_ {
   struct _tag_ *nxt;   /* To make holding messages in queue easier */
   long type;
   long src;
   long dest;
   long len;    /* Length of data field which immediately follows */
   long flags;  /* Defined below */
#define NIOMSG_BCST 0x1  /* Message is a broadcast */
#define NIOMSG_INQN 0x2  /* Msg ready inquiry and negative response */
#define NIOMSG_DBGP 0x4  /* Msg is a debug print */
#define NIOMSG_EXCH 0x8  /* Msg is exchange */
   } niohdr;

/* Header for nio messages--using standard message formats.
*  This is the format in which the UNIX host expects to send or
*  receive messages to or from COMPXP processors.  Currently
*  used only on UNIX host--COMPXP nodes assume this is same as
*  niohdr above for historical reasons and do not swap.  So,
*  if nio.c is ever rewritten for some other type of processor
*  node, niohdrs should be converted to or from mfmtniohdr for
*  sending or receiving message headers to/from the UNIX host.  */

typedef struct _mtag_ {
   struct _mtag_ *nxt;
   char mtype[FMLSIZE];
   char msrc[FMLSIZE];
   char mdest[FMLSIZE];
   char mlen[FMLSIZE];
   char mflags[FMLSIZE];
   } mfmtniohdr;

/* Structure used by the application to send nio (ncube-type i/o)
*  requests to the i/o processes, nioin and nioout.  It contains
*  a field for the type of request, one of NIORQ_ codes below,
*  and a union for the arguments for each of the nio calls. */

struct nioreq {
   int reqtyp;    /* Type of request, one of the following codes */
#define NIORQ_NULL 0    /* Used to mark 'no request' */
#define NIORQ_GETP 1
#define NIORQ_RELP 2
#define NIORQ_TST  3
#define NIORQ_RD   4
#define NIORQ_RDP  5
#define NIORQ_WR   6
#define NIORQ_WRP  7

   /* Arguments for each of the nio requests defined above */
   union {
      struct {
         long size;
         } getp_a;
      struct {
         void *buffer;
         } relp_a;
      struct {
         int *src;
         int *type;
         } tst_a;
      struct {
         void *buffer;
         int nbytes;
         int *src;
         int *type;
         } rd_a;
      struct {
         void **buffer;
         int nbytes;
         int *src;
         int *type;
         } rdp_a;
      struct {
         void *buffer;
         int  nbytes;
         int  dst;
         int  type;
         } wr_a;
      struct {
         void *buffer;
         int  nbytes;
         int  dst;
         int  type;
         } wrp_a;
      } args;
   };

/* Structure used for nio requests between clients and the nio
*  process on the host machine.  The union above cannot be used,
*  since requests to/from nio on the host machine cannot pass pointer
*  arguments (memory may be in different address spaces).  There are
*  only three types of requests on the host:  read, write, and test.
*/

struct hnreq {
   int reqtyp;  /* Either NIORQ_WR or NIORQ_RD */
   int srcdst;  /* Destination, if WR, or source, if RD */
   int type;
   int nbytes;  /* Length of message body */
   };

/* Union used for return value from nioin/out processes, to
*  application */

union app_resp {
   int i;        /* For requests that return int */
   void *p;      /* For requests that return a pointer */
   };

/* Size of inbound message table.  This table is used for hashing
*  the list of buffers, for messages that have been received but
*  not yet read by the application.  Each entry points to a linked
*  list of message buffers, some static (sticky) and others dynamic.
*  The value of this constant will affect the speed of lookup, but
*  does not impose any restriction on the number of messages that
*  may be queued.  The initial value has been set to 64, for no
*  particular reason.
*  The key that is used as an argument to the hash function is the
*  type of the message.  Other keys could have been used (source,
*  length or a combination of them), and if a case can be made for
*  using another key, it should be changed. */

#define MAXIBM      64

/* Size of buffer for "short" messages.  This is worth a short
*  discussion:  Since every message on the INMOS links involves
*  a constant latency (setup time), and in our application, the
*  receiver does not usually have a-priori knowledge of the
*  incoming message length, it is efficient to have a minimal
*  message length.  Every read/write operation which contains
*  the header of an application (i.e. nio) message, is sent with
*  that minimun size.  If the length of the message requires
*  an additional transmission, as determined by the length count
*  in the message header, the remainder of the data will be sent
*  in a single operation, regardless of length.
*
*  If the minimun length is chosen properly, a substantial fraction
*  of all messages will be sent with a single operation.  To deter-
*  mine the value for this parameter, a "typical" CNS run was executed
*  (on the NCUBE), with a routine for gathering statistics about
*  message length.  The distribution of these lengths was collected
*  in log 2 bins.  The graph showed a peak at the 0-15 byte octave,
*  and a more or less uniform distribution (octave-wise) from there
*  to 511 bytes.
*
*  Unfortunately, this result includes rootbrdcst messages, which
*  on the NCUBE use logarithmic series, unlike the proposed
*  transputer implementation.  It is assumed that the distribution
*  on that machine will have a more noticable peak in the low end.
*  A serious calculation would require knowledge of the
*  communications latency vs. the channel throughput, which was
*  not done.  Instead, a "reasonable" value was chosen.  A
*  more elaborate test will be possible once the transputer
*  machine is available, and a "typical" CNS run is defined.
*
*  Messages of length <= minimum message length, are referred to
*  as short messages, hence the constant is named SHORTMESSAGELEN.
*/

#define SHORTMESSAGELEN 64

/* Each nioin process maintains a cache of buffers for incoming short
*  messages.  When longer messages arrive, or the cache is depleted,
*  malloc() is used.  The static buffers in the cache are reffered to
*  as "sticky" buffers.  Their length should be sufficient for a short
*  message + its nio header, and the constant SHORTBUFLEN below uses
*  that exact formula.  It is assumed that both terms in the
*  expression have been chosen as multiples of 4.
*
*  The number of sticky message buffers allocated is determined by
*  the parameter NUMSTICKMSG.   Currently this is a constant, but
*  optimization may require this to be a run time value, for the
*  following reason:  The sticky buffers in the cache are used for
*  messages bound for the local node, as well as those forwarded to
*  other nodes.  Some nodes (i.e. those closer to the edges of the
*  outer ring) handle more transient messages, and may require
*  a larger cache for load balancing. */

#define SHORTBUFLEN (SHORTMESSAGELEN + sizeof(niohdr))
#define NUMSTICKMSG 20

/* Bit flags for status variables ninstat (status of nioin process),
*  and noutstat (status of nioout process). */

#define NIO_ST_NREAD   0x1  /* Pending read request is nread() */
#define NIO_ST_NREADP  0x2  /* Pending read request is nreadp() */
#define NIO_ST_APP     0x4  /* A response to app. is pending */
#define NIO_ST_RELS    0x8  /* A buffer is pending release */

/* The array 'nchanl' contains the mapping of edges in the 'double
*  ring' architecture to the physical transputer links.  Creating
*  such a mapping, rather than using the hardcoded channel address,
*  lets us initialize so that a channel will point "up" (i.e. towards
*  the pipe tail) regardless of which side of a single ended machine
*  is connected to the host.  The set of defined constants that
*  follows is used to access the array elements.  */

#if defined(INMOS8) || defined(INMOS9)
#ifndef MAIN
extern
#endif
Channel *nchanl[8];   /* So far, all xputers have 4 links */
#endif

#define IRING_DN_IN   0
#define IRING_DN_OUT  1
#define ORING_DN_IN   2
#define ORING_DN_OUT  3
#define IRING_UP_IN   4
#define IRING_UP_OUT  5
#define ORING_UP_IN   6
#define ORING_UP_OUT  7

/* Indexes into the 'init' array, used by the initialization routine,
*  nioinit().  Three messages, called init0, init1, init3 are passed
*  as part of the initialization process.  Message 'init0' is sent
*  from the host (actually from the control process) to the external
*  node(s).  Messages 'init1' and 'init2' are passed among the
*  tranputers.  See nioinit() for more details. The messages are
*  arrays rather than structures, to simplify conversion when sending
*  from the host to the tranputers */

/* Indexes for access to init0 elements */
#define I0_ID1     0    /* Id of first (head) node */
#define I0_MCHN    1    /* Machine id. For future sub-partitioning */
#define I0_HOST    2    /* Host id */
#define I0_UPRIGHT 3    /* If non-zero, machine is "upright" */
#define I0_DBG     4    /* Debug word from host */
#define I0_LOWER   5    /* num nodes on lower half pf tnm */

/* Indexes for access to init1 elements */
#define I1_MYID    0    /* Id of local node */
#define I1_ID1     1    /* Id of first node (aka 'pipe head') */
#define I1_MCHN    2    /* Machine id. For future sub-partitioning */
#define I1_HOST    3    /* Host id */
#define I1_DBG     4    /* Debug word */
#define I1_DBL     5    /* If non-zero, machine is double-ended */

/* Indexes for access to init2 elements */
#define I2_NNOD    0    /* Number of nodes, including host */
#define I2_FLG     1    /* Value for NC.flags (a global struct) */

#define NUMINIT    6    /* Number of elements in init messages */

/* Stack sizes for the nioin and nioout processes.  Experiment */

#define NINSTACK  512
#define NOUTSTACK 512

/* Size of buffer used between nioin and nioout processes */

#define NQBUFSIZE 256

/* Delay, in milliseconds, for polling of host socket for nio
*  input.  The value will be translated to clock ticks and used
*  by nioin() process, to time out a call to ProcTimerAlt(). */

#define POLLDELAYMSEC  10

/* For some reason, could not find prototypes for socket calls in
*  any header file */

#if defined(INMOS8) && defined(EXTNODE)
extern int socket(int domain, int type, int protocol);
extern int connect(int s, struct sockaddr *name, int namelen);
#endif

extern int sowrite(int, char *, int);
extern int soread(int, char *, int);
