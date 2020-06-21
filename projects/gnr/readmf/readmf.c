/* (c) Copyright 20xx, The Rockefeller University *nnnnn* */
/*********************************************************************
*                                                                    *
* SYNOPSIS:                                                          *
*    void readmf();                                                  * 
*                                                                    *
* DESCRIPTION:                                                       *
*                                                                    *
* DESIGN NOTES:                                                      *
*     As of right now, this only handles reading operations,         *
*     which is incorrect. Should handle all metafile reading         *
*     operations. Presumabely move dumpbits.c here, change dumpmf    *
*     printing to use only dumbsub();                                *
*                                                                    *
ARGUMENTS:                                                           *
*  mfs      Pointer to metafile shared data.                         *
*  fmt      Pointer to formatted string.                             *
*  ...      Variadic parameters (should be addresses) that are       *
*           updated when value is requested.                         *
*                                                                    *
**********************************************************************
*  Version 1, 00/00/19, GMK                                          *
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "rocks.h"
#include "plotops.h"
#include "readmf.h"

#define dmpcmd(f,p...) dumpbits(&_mfs,f,p)

/*---------------------------------------------------------------------*
*        Initialize shared 'mfs data and read header from file         * 
*---------------------------------------------------------------------*/
MFS initrmf(char* fp) {

   char header[HDR_LEN];   /* Header string buffer    */

   MFS _mfs;
   _mfs.bitpos    = 0;
   _mfs.bytesread = 0;
   _mfs.cnode     = 0;
   _mfs.nwins     = 0;
   _mfs.ahiv      = 0;
   _mfs.nnodes    = 1;
   _mfs.rop       = 0;

   /* Determine filepath. */
   if (fp == NULL) {
      abexit(0);
   }

   /* Prepare metafile */
   _mfs.rfptr = rfallo(IGNORE,READ,BINARY,SEQUENTIAL,TOP,NO_LOOKAHEAD,
      NO_REWIND,RELEASE_BUFF,MF_BUFF_SIZE,IGNORE,IGNORE,ABORT);
   /* Open metafile */
   rfopen(_mfs.rfptr,fp,READ,BINARY,SEQUENTIAL,TOP,NO_LOOKAHEAD,
      NO_REWIND,RELEASE_BUFF,MF_BUFF_SIZE,IGNORE,IGNORE,ABORT);
   /* Read metafile */
   if(!rfread(_mfs.rfptr,header,HDR_LEN,ABORT)) {
      /* empty file? */
   }

   /* Store and print LCI,LCF */
   _mfs.lci = atoi(&header[LCI_INDEX]);
   _mfs.lcf = atoi(&header[LCF_INDEX]);

   /* Check for 'additional header information' */
   if((_mfs.rop = readbitsi(&_mfs,Lop)) == OpNNode) {
      _mfs.ahiv   = readbitsi(&_mfs,4);
      _mfs.nnodes = readbitsi(&_mfs,16)+1;
      printf("ahiv:   %d\n",_mfs.ahiv);
      printf("nnodes: %d\n",_mfs.nnodes);
   }
   else printf("No additional header information found.");

   /* Allocate space for node(s) */
   _mfs.nodes = malloc(sizeof(Node)*_mfs.nnodes);

   /* Setup first window and viewport (at least one is needed) */
   _mfs.nwins++;
   _mfs.windows = malloc(sizeof(Window));
   _mfs.windows[0].frames = malloc(sizeof(Frame));
   _mfs.windows[0].cframe  = 0;
   _mfs.windows[0].nframes = 1;
   _mfs.windows[0].winnum  = 0;
   _mfs.windows[0].frames[0].x = 0;
   _mfs.windows[0].frames[0].y = 0;
   _mfs.windows[0].frames[0].r = 0;
   _mfs.windows[0].frames[0].w = 0;
   _mfs.windows[0].frames[0].h = 0;

   return _mfs;
}

void closermf(MFS* mfs) {
   /* free each frame on each window */
   int w;
   for (w=0; w<mfs->nwins; w++) {
      free(mfs->windows[w].frames);
   }
   /* free nodes and windows */
   free(mfs->nodes);
   free(mfs->windows);
   /* close file */
   rfclose(mfs->rfptr,0,0,0);
}

int nextop(MFS* mfs) {
   readbitsi(mfs, Lop); /* Read 'LOP' bits for next operation */
   return 0;
}
