/* (c) Copyright 20xx, The Rockefeller University *nnnnn* */
/*********************************************************************
*                                                                    *
* SYNOPSIS:                                                          *
*    dumpmf [filename]                                               *
*                                                                    *
* DESCRIPTION:                                                       *
*     This program reads a metafile and prints the values in a       *
*     formatted and human-readable way.                              *
*                                                                    *
* RETURN VALUES:                                                     *
*    0 = Successful execution                                        *
*                                                                    *
* NOTE:                                                              *
*    If argv[1] is NULL, the filepath will be pulled from DFLT_PTH.  *
*                                                                    *
**********************************************************************
*  Version 1, 00/00/19, GMK                                          *
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#define MAIN          /* Required by main progs using rocks routines */
#include "sysdef.h"
#include "rocks.h"
#include "plotops.h"
#include "readmf.h"

#define dmpcmd(f,p...) dumpbits(&mfs,f,p)
#define DFLT_PATH "/home/gmk/src/plot/rkgtest.mf"

int main(int argc, char **argv) {
   ui32 opcount;           /* Number of executed operations     */
   ui32 win,frm;           /* Data for current window and frame */
   MFS mfs;                /* Metafile shared-data structure    */

   /* Initialize readmf, specify filepath */
   mfs = initrmf(argv[1]);  

   //printf("\nLCI: %d\nLCF: %d\n",_mfs.lci,_mfs.lcf);

/*---------------------------------------------------------------------*
*                   Read and dump next command record                  *
*---------------------------------------------------------------------*/

win=frm=0;
while (mfs.eof != END_OF_FILE) {
   mfs.rop = readbitsi(&mfs,Lop); /* Read 'Lop' bits for next cmd   */
   printf("\n[0x%02x] ",mfs.rop);
   switch (mfs.rop) {
   default:
      /* No operation found */
      abexit(748);
      break;
   /* Reserved data record */
   case (0x00):
      break;
   /* Start of plot record */
   case (OpStart):
      /* data: k,k,k,k,g,g,k,k,k,k6,cn*/
      dmpcmd("Start of plot record:\n\twindow = %`k\n\tframe = %`k\n\t"
             "mfindex = %k\n\txgindex = %k\n\txsiz = %g\n\tysiz = %g\n\t"
             "nexpose = %k\n\tmovie_device = %k\n\tmovie_mode = %k\n\t"
             "nc = %k6\n\tchart = %cn",
             &win,&frm);
      opcount++;
      printf("\n\t");
      break;
   /* New window record */
   case (OpNewWin):
      dmpcmd("New window record:\n\tiwin = %k",&win);
      #if 0
      _mfs.nwins++;
      _mfs.windows = realloc(_mfs.nwins+sizeof(Window));
      _mfs.windows[win].frames = realloc(sizeof(Frame));
      _mfs.windows[win].cframe  = 0;
      _mfs.windows[win].nframes = 1;
      _mfs.windows[win].winnum  = win;
      _mfs.windows[win].frames[0].x = 0;
      _mfs.windows[win].frames[0].y = 0;
      _mfs.windows[win].frames[0].r = 0;
      _mfs.windows[win].frames[0].w = 0;
      _mfs.windows[win].frames[0].h = 0;
      #endif
      break;
   /* Arc record */
   case (OpArc):
      dmpcmd("Arc record:\n\ttype = %k2\n\txc = %x\n\tyc = %y\n\txs = %x\n\tys = %y\n\t" 
             "angle = %a",NULL);
   /* Bitmap (unscaled) record */
      break;
   case (OpBMap):
      dmpcmd(
      "Bitmap record:\n\ttype = %%\n\txc = %%\n\tyc = %%\n\txs = %%\n\tys = %%," 
      "angle = %%",NULL);
      break;
   /* Bitmap (scaled) record */
   case (OpBMaps):
      break; // fix this
   /* Circle record */
   case (OpCirc):
      dmpcmd("Circle record:\n\tfill = %k2\n\txc = %x\n\t" 
         "yc = %y\n\tradius = %r",NULL);
      break;
   /* Draw progressive line record */
   case (OpDraw):
      dmpcmd("Draw progressive line record:\n\tx = %x\n\ty = %y",NULL);
      break;
   /* Ellipse Record */
   case (OpEllps):
      dmpcmd("Ellipse record:\n\tfill = %k2\n\txc = %x\n\tyc = %y\n\twd = %w," 
         "ht = %h,angle = %a",NULL);
      break;
   /* Font record */
   case (OpFont):
      dmpcmd("Font record:\n\tjf = %k\n\tn = %k6\n\tfont = %cn",NULL);
      break;
   /* Registered font record */
   case (OpRFont):
      dmpcmd("Registered font record:\n\t%k",NULL);
      break;
   /* Polygon/star record */
   case (OpStar):
      dmpcmd("Polygon/star record:\n\tfill = %k3\n\txc = %x\n\tyc = %y,"
         "rv = %r\n\tindent = %g\n\tspike = %g\n\tnv = %k\n\tangle = %a",NULL);
      break;
   /* Retrace/thickness record */
   case (OpThick):
      dmpcmd("Retrace/thickness record:\n\tkrt = %k4",NULL);
      break;
   /* Explicit numeric color record */
   case (OpColor):
      dmpcmd("Explicit numeric color record:\n\t"
         "blue = %k8\n\tred = %k8\n\tgreen = %k8",NULL);
      break;
   /* Registered color record */
   case (OpRCol):
      dmpcmd("Registered color record:\n\tjc = %k",NULL);
      break;
   /* Pen color record */
   case (OpPCol):
      dmpcmd("Pen color record:\n\tnc = %k4\n\tcname = %cn",NULL);
      break;
   /* Register color record */   
   case (OpCtsyn):
      dmpcmd("Register color record:\n\t",
         "jc = %k\n\tnc = %k\n\tcname = %cn",NULL);
      break;
   /* Line record */ 
   case (OpLine):
      dmpcmd("Line record:\n\t"
         "x1 = %x\n\ty1 = %y\n\tx2 = %x\n\ty2 = %y",NULL);
      break;
   /* Thin line record */
   case (OpTLine):
      dmpcmd("Thin line record:\n\t"
         "x1 = %x\n\ty1 = %y\n\tx2 = %x\n\ty2 = %y",NULL);
      break;
   /* Move record */
   case (OpMove):
      dmpcmd("Move record:\n\tx = %x\n\ty = %y",NULL);
      break;
   /* Object identifier record */
   case (OpObjid):
      dmpcmd("Object identifier record:\n\tid = %k",NULL);
      break;    
   /* Polyline record */  
   case (OpPoly):
      /* Fix this one */
      dmpcmd("Polyline record:\n\tfill = %k3\n\tnp = %k\n\t"
         "x1 = %x\n\ty1 = %y\n\tx2 = %x\n\ty2 = %y",NULL);
      break;
   /* Square record */
   case (OpSqre):
      dmpcmd("Square record:\n\t"
         "fill = %k2\n\tx1 = %x\n\ty1 = %y\n\tsize = %r",NULL);
      break;
   /* Rectangle record */
   case (OpRect):
      dmpcmd("Rectangle record:\n\t"
         "fill = %k2\n\tx1 = %x\n\ty1 = %y\n\twd = %w\n\tht = %h",NULL);
      break;
   /* Symbol record */
   case (OpSymb):
      dmpcmd("Symbol record:\n\t"
         "x = %x\n\ty = %y\n\tht = %h\n\tangle = %a\n\tn = %s\n\t"
         "text = %cn",NULL);
      break;
   /* Continued symbol record */
   case (OpSymc):
      dmpcmd("Continued symbol record:\n\t"
         "ht = %h\n\tangle = %a\n\tn = %s\n\ttext = %cn",NULL);
      break;
   /* Pen type record */
   case (OpPTyp):
      dmpcmd("Pen type record:\n\tjt = %k\n\tn = %k4\n\tptype = %cn",NULL);
      break;
   /* Registered pen type record */
   case (OpRPTyp):
      dmpcmd("Registered pen type record:\n\trpen = %k",NULL);
      break;
   /* Viewport definition record */
   case (OpFrame):
      dmpcmd("Viewport definition record:\n\tmodes = %k10\n\t"
         "vn = %k\n\twidth = %w\n\theight = %h\n\tvxx = %g\n\tvxy = %g\n\t"
         "vxc = %g\n\tvyx = %g\n\tvyy = %g\n\tvyc = %g",NULL);
      break;
   /* Viewport change record */
   case (OpChgFrm):
      dmpcmd("Viewport change record:\n\tifrm = %k",&frm);
      break;
   /* Viewport update record */
   case (OpFrmCtl):
      dmpcmd("Viewport update record:\n\tfac = %k3\n\tifrm = %k",NULL);
      break;
   /* Window control record */
   case (OpWinCtl):
      dmpcmd("Window control record:\n\taction = %k2\n\tiwin = %k",NULL);
      break;
   /* Window selection record */
   case (OpChgWin):
      dmpcmd("Window selection record:\n\tiwin = %k",&win);
      break;
   /* Graphics mode record */
   case (OpGMode):
      dmpcmd("Graphics mode record:\n\tgm = %k4",NULL);
      break;
   /* Alignment record */
   case (OpAlign):
      dmpcmd("Alignment record:\n\tnskip = %k4",NULL); // fix for additional alignment record
      break;
   /* State restoration record */
   case (OpState):
      dmpcmd("State restoration record:\n\t"
         "node = %k\n\tiwin = %k\n\tifrm = %k",NULL);
      break;
   /* End-of-metafile record */
   case (OpEnd):
      dmpcmd("End-of-metafile record",NULL);
      mfs.eof = END_OF_FILE;
      break;      
   }

   opcount++;

   if (opcount != 2){
      /*printf("\n\tWarning:\n\t'OpStart' was not first plot command.\n\t");*/
   }
}
/*---------------------------------------------------------------------*
*                       End of dump program                            *
*---------------------------------------------------------------------*/
   
   closermf(&mfs);
   printf("\n End of dump program. \n");
   return 0;
} /* End of main */
