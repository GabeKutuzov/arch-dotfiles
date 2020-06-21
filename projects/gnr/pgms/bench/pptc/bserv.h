/***********************************************************************
*                               bserv.h                                *
*                                                                      *
*  Type and parameter definitions for TI MVP implementation of d3bench *
*                                                                      *
*  Rev, 11/21/95, GNR - Add debug print task & host communications     *
***********************************************************************/

/* Define usual constants and macros */
#define FALSE  0
#define TRUE   1
#define OFF    0
#define ON     1

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

/* Define software interrupt level used by PPs to signal completion */
#define PPMsgLevel 1

/* Define parameters that aren't worth varying in input file
      PT = positive firing threshold for a cell
      MTI = modification threshold on output of a cell
      MTJ = modification threshold on input to a connection
      ET = excitatory threshold for an input to a connection          */

#define  PT    0.25           /* Positive firing threshold */
#define MTI    0.3            /* Presynaptic modification thresh */
#define MTJ    0.4            /* Postsynaptic modification thresh */
#define  ET    0.2            /* Effectiveness threshold */

/* Memory allocation parameters */
#define NDAS             4    /* Number of words of DAS stats */
#define NumHMsgs         4    /* Number of host msg buffers */
#define NumPTReq         4    /* Max number of PTReq's needed */
#define NLijGuides     256    /* Max number of "Guide Table" entries
                              *  that will fit below .pbss in RAM2 */
#define NConnsPerBatch 256    /* Currently must equal NLijGuides */
#define StackSize     2048    /* Stacks for subsidiary processes */

/* Symbolic locations of PTReq blocks */
#define TRWorkUnitIn (pPTReq)
#define TRConnDatIn  (pPTReq+1)
#define TRLijToLUT   (pPTReq+2)
#define TRSjIn       (pPTReq+3)

#ifndef _MVP_PP
/* Prototype task functions */
void Dispatch(void *);
void HostMsg(void *);
void TransferControl(void *);
void ProcessorSched(void *);
void ProcessorFinish(void *);

/* Task priorities for the above tasks--
*  Note:  GIC startup code makes UserMain have priority 15 */
#define DispatchPrio    17    /* Priority for dispatch task */
#define PPSchedPrio     19    /* Priority for PP Schedulers */
#define PPFinishPrio    21    /* Priority for PP Clean Up */
#define TransferPrio    23    /* Priority for transfer control */
#define HostMsgPrio     25    /* Priority for host messages */

/* Priority bits for assigning WORKUNITS to processors */
#define WBackPrio    1        /* Write Back is queued */
#define LoadPrio     4        /* Load is queued */
#define CalcPrio    16        /* Calculation is queued */
#define NumCmdBufs   2        /* Number of command buffers per PP */
#endif

/* Define struct types used in the simulation.  Pointers in these
   structs refer to off-chip memory unless prefaced with "My".  */

   typedef unsigned char byte;
   typedef void * Message;
   typedef long Port;
   typedef long Semaphore;
   typedef long TaskId;

   typedef struct WORKUNIT {  /* Define a unit of work */
      struct WORKUNIT *pnwu;  /* Ptr to next work unit */
      struct CONNBLK *pconnb; /* Ptr to assigned CONNBLK */
      struct CELLSUM *pcsum;  /* Ptr to assigned CELLSUM */
      struct CONNDATA *Myic;  /* Ptr to my copy of CONNDATA */
      struct WORKOUT *Mywo;   /* Ptr to my work output area */
      struct PTReq *pTCList;  /* Ptr to transfer control list */
      long int scale;         /* Scale factor (S8) */
      int MyPP;               /* PP assigned */
      int nbatch;             /* Size of current connection batch */
      /* Following items are shorts to allow 16-bit multiply */
      short delta;            /* Amplification factor (S10) */
      short et;               /* Effectiveness threshold (S8) */
      short mti;              /* Postsynaptic amp threshold (S8) */
      short mtj;              /* Presynaptic amp threshold (S8) */
      short si;               /* s(i) of current cell */
      short pad;
      } WORKUNIT;

   typedef struct WORKOUT {   /* Define output from a WORKUNIT */
      long int das[NDAS];     /* Detailed amp stats */
      long int ax;            /* Afferent sum (S15) */
      } WORKOUT;

   typedef struct CELLSUM {   /* Summation of input to a cell */
      byte *pnsi;             /* Where new s(i) is to be stored */
      long wksum;             /* Working accumulator */
      long wknum;             /* Number of batches remaining */
      } CELLSUM;

   typedef struct CONNDATA {  /* Data for a single connection */
      short int lij;          /* Number of source cell */
      byte cij;               /* Strength of connection (S7) */
      byte sj;                /* Looked up by transfer controller */
      } CONNDATA;

   typedef struct CONNBLK {   /* Connection-type information block */
      struct CONNBLK *pct;    /* Pointer to next CONNBLK */
      struct CELLBLK *psrc;   /* Pointer to source cell type */
      byte *psj;              /* Working storage for psj */
#ifdef _MVP_PP
      double pad[2];          /* Fill out to size of chunksize on MP */
#else
      double chunksize;       /* Range of a generated connection */
#endif
      long int nconns;        /* Number of connections */
      int nbatch;             /* Number of batches to evaluate */
      int source;             /* Serial number of source cell type */
      int rule;               /* Connection generating rule */
      long int scale;         /* Scale factor (S8) */
      long int das[NDAS];     /* Detailed amp stats */
      long int ax;            /* Intermediate afferent sum (S15) */
      } CONNBLK;

   typedef struct CELLBLK {   /* Cell-type information block */
      struct CELLBLK  *play;  /* Pointer to next CELLBLK */
      struct CONNBLK  *pct1;  /* Pointer to first CONNBLK */
      struct CONNDATA *prd;   /* Pointer to actual connection data */
      long int delta;         /* Amplification factor (S10) */
      long int ncells;        /* Number of cells of this type */
      long int nconntypes;    /* Number of connection types */
      long int nconntotal;    /* Total number of connections */
      int nbatches;           /* Number of batches to compute */
      byte  *psi;             /* Pointer to s(i) data */
      byte  *ps2;             /* Pointer to alternate (new) s(i) */
      } CELLBLK;
