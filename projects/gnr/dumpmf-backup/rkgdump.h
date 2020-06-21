/* (c) Copyright 20xx, The Rockefeller University *nnnnn* */
/***********************************************************************
*                      Dumpmf for ROCKS plot library                   *
*                               RKGDUMP.H                              *
*                                                                      *
*  Header file for dumpmf program                                      *
*                                                                      *
************************************************************************
*  Version 1, 00/00/19, GMK                                            *
***********************************************************************/

#ifndef __RKGDUMP__
#define __RKGDUMP__

#include "rfdef.h"

#define DFLT_PTH "/home/gmk/src/plot/rkgtest.mf"

/*---------------------------------------------------------------------*
*                              Constants                               *
*---------------------------------------------------------------------*/

#define MF_BUFF_SIZE  4096        /* Metafile buffer size       */
#define HDR_LEN         94        /* Length of metafile header  */
#define END_OF_FILE      1        /* End of file definition     */

/* Defs for formatted printing */
#define UIVAL 0
#define SIVAL 1
#define FLVAL 2
#define STVAL 3

/* Readbits definitions */
#define INTEGER  0
#define FLOATING 1

/*---------------------------------------------------------------------*
*                           Data structures                            *
*---------------------------------------------------------------------*/

/* Node */
struct Nodedef {
   ui16 cwin;
};
typedef struct Nodedef Node;

/* Frame */
struct Frmdef {
   float x,y,r,w,h;
   float px,py,pr,pw,ph;
};
typedef struct Frmdef Frame;

/* Window */
struct Windef {
   Frame *frames;         /* Ptr to first frame on this window     */
   ui16 cframe,nframes;   /* Current and number of drawing frames  */ 
   ui16 winnum;           /* User-defined number of this window    */
};
typedef struct Windef Window;

/* Shared metafile data */
struct Mfsdef {
   /* Metafile data */
   struct RFdef *rfptr; /* Pointer to RFdef struct                */
   int bitpos;          /* Current bit position in metafile       */
   int bytesread;       /* Current amount of bytes that were read */  
   int eof;             /* End of file                            */
   int lci,lcf;         /* LCI/LCF header information             */
   int ahiv,nnodes;     /* ahiv/nnodes header information         */
   byte currbyte;       /* Current byte to be read                */
   /* Node data */
   Node   *nodes;    /* Pointer first node        */
   Window *windows;  /* Pointer to first window   */
   int nwins;        /* Total number of windows   */
   int cnode;        /* Currently active node     */
};
typedef struct Mfsdef MFS;

/* function prototypes */
int   readbitsi(MFS *mfs, size_t nbits);
float readbitsf(MFS *mfs, size_t nbits);
char* readbytes(MFS *mfs, size_t nbytes);
void  dumpbits(MFS *mfs, char *fmt, ...);

#endif /* ifndef __RKGDUMP__ */
