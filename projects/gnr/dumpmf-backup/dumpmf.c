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
#include "rkgdump.h"

#define dmpcmd(f,p...) dumpbits(&_mfs,f,p)

int main(int argc, char **argv) {

   ui32 opcount;           /* Number of executed operations     */
   ui32 win,frm;           /* Data for current window and frame */
   int  rop;               /* Current operation code            */
   char header[HDR_LEN];   /* Header string buffer              */
   char* filepath;         /* Metafile Filepath.                */

/*---------------------------------------------------------------------*
*        Initialize shared 'mfs data and read header from file         * 
*---------------------------------------------------------------------*/
   
   MFS _mfs;
   _mfs.bitpos    = 0;
   _mfs.bytesread = 0;
   _mfs.cnode     = 0;
   _mfs.nwins     = 0;
   _mfs.ahiv      = 0;
   _mfs.nnodes    = 1;

   /* Determine filepath. */
   if(argv[1] != NULL) filepath = argv[1];
   else filepath = DFLT_PTH;

   /* Prepare metafile */
   _mfs.rfptr = rfallo(IGNORE,READ,BINARY,SEQUENTIAL,TOP,NO_LOOKAHEAD,
      NO_REWIND,RELEASE_BUFF,MF_BUFF_SIZE,IGNORE,IGNORE,ABORT);
   /* Open metafile */
   rfopen(_mfs.rfptr,filepath,READ,BINARY,SEQUENTIAL,TOP,NO_LOOKAHEAD,
      NO_REWIND,RELEASE_BUFF,MF_BUFF_SIZE,IGNORE,IGNORE,ABORT);
   /* Read metafile */
   if(!rfread(_mfs.rfptr,header,HDR_LEN,ABORT)) {
      /* empty file? */
   }

   /* Store and print LCI,LCF */
   _mfs.lci = atoi(&header[16]);
   _mfs.lcf = atoi(&header[18]);
   printf("\nLCI: %d\nLCF: %d\n",_mfs.lci,_mfs.lcf);

   /* Check for 'additional header information' */
   if((rop = readbitsi(&_mfs,Lop)) == OpNNode) {
      _mfs.ahiv   = readbitsi(&_mfs,4);
      _mfs.nnodes = readbitsi(&_mfs,16)+1;
      printf("ahiv:   %d\n",_mfs.ahiv);
      printf("nnodes: %d\n",_mfs.nnodes);
   }
   else printf("No additional header information found.");

   /* Allocate space for node(s) */
   _mfs.nodes = malloc(sizeof(Node)*_mfs.nnodes);

   /* Setup first window and viewport(at least one is needed) */
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

/*---------------------------------------------------------------------*
*                   Read and dump next command record                  *
*---------------------------------------------------------------------*/

win=frm=0;
while (_mfs.eof != END_OF_FILE) {
   rop = readbitsi(&_mfs, Lop); /* Read 'Lop' bits for next cmd   */
   printf("\nNext Operation: %02x\n",rop);
   switch (rop) {
   default:
      /* No operation found */
      abexit(748);
      break;
   /* Reserved data record */
   case (0x00):
      printf("Do something here.\n");
      break;
   /* Start of plot record */
   case (OpStart):
      /* data: k,k,k,k,g,g,k,k,k,k6,cn*/
      dmpcmd("Start of plot record: window = %`k, frame = %`k, "
             "mfindex = %k, xgindex = %k, xsiz = %g, ysiz = %g, "
             "nexpose = %k, movie_device = %k, movie_mode = %k, "
             "nc = %k6, chart = %cn",
             &win,&frm);
      opcount++;
      printf("\n");
      break;
   /* New window record */
   case (OpNewWin):
      dmpcmd("New window record: iwin = %k", &win);
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
      dmpcmd("Arc record: type = %k2, xc = %x, yc = %y, xs = %x, ys = %y," 
             "angle = %a",NULL);
   /* Bitmap (unscaled) record */
      break;
   case (OpBMap):
      dmpcmd(
      "Bitmap record: type = %%, xc = %%, yc = %%, xs = %%, ys = %%," 
      "angle = %%",NULL);
      break;
   /* Bitmap (scaled) record */
   case (OpBMaps):
      break; // fix this
   /* Circle record */
   case (OpCirc):
      dmpcmd("Circle record: fill = %k2, xc = %x," 
         "yc = %y, radius = %r",NULL);
      break;
   /* Draw progressive line record */
   case (OpDraw):
      dmpcmd("Draw progressive line record: x = %x, y = %y",NULL);
   /* Ellipse Record */
   case (OpEllps):
      dmpcmd("Ellipse record: fill = %k2, xc = %x, yc = %y, wd = %w," 
         "ht = %h,angle = %a",NULL);
      break;
   /* Font record */
   case (OpFont):
      dmpcmd("Font record: jf = %k, n = %k6, font = %cn",NULL);
      break;
   /* Registered font record */
   case (OpRFont):
      dmpcmd("Registered font record: %k",NULL);
      break;
   /* Polygon/star record */
   case (OpStar):
      dmpcmd("Polygon/star record: fill = %k3, xc = %x, yc = %y,"
         "rv = %r, indent = %g, spike = %g, nv = %k, angle = %a",NULL);
      break;
   /* Retrace/thickness record */
   case (OpThick):
      dmpcmd("Retrace/thickness record: krt = %k4",NULL);
      break;
   /* Explicit numeric color record */
   case (OpColor):
      dmpcmd("Explicit numeric color record: "
         "blue = %k8, red = %k8, green = %k8",NULL);
      break;
   /* Registered color record */
   case (OpRCol):
      dmpcmd("Registered color record: jc = %k",NULL);
      break;
   /* Pen color record */
   case (OpPCol):
      dmpcmd("Pen color record: nc = %k4, cname = %cn",NULL);
      break;
   /* Register color record */   
   case (OpCtsyn):
      dmpcmd("Register color record: ",
         "jc = %k, nc = %k, cname = %cn",NULL);
      break;
   /* Line record */ 
   case (OpLine):
      dmpcmd("Line record: "
         "x1 = %x, y1 = %y, x2 = %x, y2 = %y",NULL);
      break;
   /* Thin line record */
   case (OpTLine):
      dmpcmd("Thin line record: "
         "x1 = %x, y1 = %y, x2 = %x, y2 = %y",NULL);
      break;
   /* Move record */
   case (OpMove):
      dmpcmd("Move record: x = %x, y = %y",NULL);
      break;
   /* Object identifier record */
   case (OpObjid):
      dmpcmd("Object identifier record: id = %k",NULL);
      break;    
   /* Polyline record */  
   case (OpPoly):
      /* Fix this one */
      dmpcmd("Polyline record: fill = %k3, np = %k, "
         "x1 = %x, y1 = %y, x2 = %x, y2 = %y",NULL);
      break;
   /* Square record */
   case (OpSqre):
      dmpcmd("Square record: "
         "fill = %k2, x1 = %x, y1 = %y, size = %r",NULL);
      break;
   /* Rectangle record */
   case (OpRect):
      dmpcmd("Rectangle record: "
         "fill = %k2, x1 = %x, y1 = %y, wd = %w, ht = %h",NULL);
      break;
   /* Symbol record */
   case (OpSymb):
      dmpcmd("Symbol record: "
         "x = %x, y = %y, ht = %h, angle = %a, n = %s, "
         "text = %cn",NULL);
      break;
   /* Continued symbol record */
   case (OpSymc):
      dmpcmd("Continued symbol record: "
         "ht = %h, angle = %a, n = %s, text = %cn",NULL);
      break;
   /* Pen type record */
   case (OpPTyp):
      dmpcmd("Pen type record: jt = %k, n = %k4, ptype = %cn",NULL);
      break;
   /* Registered pen type record */
   case (OpRPTyp):
      dmpcmd("Registered pen type record: rpen = %k",NULL);
      break;
   /* Viewport definition record */
   case (OpFrame):
      dmpcmd("Viewpoint definition record: modes = %k10, "
         "vn = %k, width = %w, height = %h, vxx = %g, vxy = %g,"
         "vxc = %g, vyx = %g, vyy = %g, vyc = %g",NULL);
      break;
   /* Viewport change record */
   case (OpChgFrm):
      dmpcmd("Viewport change record: ifrm = %k",&frm);
      break;
   /* Viewport update record */
   case (OpFrmCtl):
      dmpcmd("Viewport update record: fac = %k3, ifrm = %k",NULL);
      break;
   /* Window control record */
   case (OpWinCtl):
      dmpcmd("Window control record: action = %k2, iwin = %k",NULL);
      break;
   /* Window selection record */
   case (OpChgWin):
      dmpcmd("Window selection record: iwin = %k",&win);
      break;
   /* Graphics mode record */
   case (OpGMode):
      dmpcmd("Graphics mode record: gm = %k4",NULL);
      break;
   /* Alignment record */
   case (OpAlign):
      dmpcmd("Alignment record: nskip = %k4",NULL); // fix for additional alignment record
      break;
   /* State restoration record */
   case (OpState):
      dmpcmd("State restoration record: "
         "node = %k, iwin = %k, ifrm = %k",NULL);
      break;
   /* End-of-metafile record */
   case (OpEnd):
      dmpcmd("End-of-metafile record",NULL);
      _mfs.eof = END_OF_FILE;
      break;      
   }

   opcount++;

   if(opcount != 2){
      /*printf("\nWarning: 'OpStart' was not first plot command.\n");*/
   }
}
/*---------------------------------------------------------------------*
*                       End of dump program                            *
*---------------------------------------------------------------------*/
   
   rfclose(_mfs.rfptr,0,0,0);

   int w;
   for (w=0; w<_mfs.nwins; w++) {
      free(_mfs.windows[w].frames);
   }

   free(_mfs.nodes);
   free(_mfs.windows);
   
   printf("\n End of dump program. \n");
   return 0;
} /* End of main */
