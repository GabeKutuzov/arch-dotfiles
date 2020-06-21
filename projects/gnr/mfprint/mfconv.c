/* (c) 1992 Neurosciences Research Foundation */
/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/* int mfconv(FILE *fdes, int frame,int device,int type)              */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   mfconv() displays the content of an NSI graphics metafile.       */
/*   It reads the metafile and translates it to drawing commands.     */
/*   In order to use the the function , first open the                */
/*   metafile and then call the function with four arguments:         */
/*   Arguments are:                                                   */
/*    fdes    = file descriptor,                                      */
/*    frame   = the frame number to be shown, if 0, all frames        */
/*              will be displayed.                                    */
/*    device  = the type of device to be drawn                        */
/*    type    = the package type (XGL,PS...)                          */
/*                                                                    */
/* RETURN VALUES:                                                     */
/*   0=successful completion,                                         */
/*   1=invalid command line,                                          */
/*   2=invalid file header,                                           */
/*   3=frame not found,                                               */
/*   4=error in file open,                                            */
/*                                                                    */
/* Version 1, 08/24/92, ROZ                                           */
/*    Rev.,   07/21/94,  LC  Added retrace.                           */
/**********************************************************************/


#define  _MFCONV_C
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>

#include "xglut.h" 
#include "pltxgl.h" 
#include "pltps.h" 
#include "mfconv.h"
#include "mfcv.h"
#include "gmf.h"
#include "gluxgl.h"

#define PM_OK        0          /* Successful completion */ 
#define INVAL_HEADER 2          /* Invalid file header */
#define NOT_FOUND    3          /* Error open the file */
#define ERR_OPEN     4          /* Frame not found */
#define FOUND        5          /* Frame exists */
#define ON           1          /* Color table mode is on */
#define OFF          0          /* Color table mode is off */

#define MAXLINE      2048       /* Maximun length of line in the 
                                   NGM file. */

/* Allocate memory for target string and copy source into target. */
#define allo_cpy(target,source) { \
   if((target = (char *)malloc(strlen(source))) == NULL) { \
      printf("can not allocate memory \n"); \
      exit(1); \
      } \
   strcpy(target,source); \
   } \

float tab[] = {1.0,10.0,100.0,1000.0,10000.0,100000.0};

#define a2f(value,field,size,scale,type) { \
   char cc;\
   float sign=1.0; \
   value = 0.0; \
   if (*field == '-') { \
      field++; \
      sign = -1.0; \
      } \
   do { \
      cc = *field++; \
      if (cc >= 'a') { \
         value = sign * (float)(value*10 + (cc - 'a'))/tab[scale]; \
         break; \
         } \
      value = value*10 + (cc - '0'); \
      } while (1); \
   } 

#define a2i(value,field,size,type) { \
   char cc;\
   int sign = 1; \
   value = 0; \
   if (*field == '-') { \
      field++; \
      sign = -1; \
      } \
   do { \
      cc = *field++; \
      if (cc >= 'a') { \
         value = sign * (float)(value*10 + (cc - 'a')); \
         break; \
         } \
      value = value*10 + (cc - '0'); \
      } while (1); \
   } 


unsigned char *PMBuff;            /* pointer to a line of input */
char buffer[MAXLINE];    /* Buffer that holds a line of input */
extern int adjust;

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*     void mfc_prn(char *alias,float tmargin,float lmargin,          */
/*     float width,float height,char *visual,int depth,int lwidth)    */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*    Get information on the default printer.                         */
/* RETURN VALUES:                                                     */
/*    None                                                            */
/**********************************************************************/
void mfc_prn(char *alias,float tmargin,float lmargin,
   float width,float height,char *visual,int depth,int lwidth) {
   allo_cpy(CPD.alias,alias);        /* Alias as appears in printcap */
   CPD.tmargin = tmargin;           /* Top margin of printed area */
   CPD.lmargin = lmargin;           /* Left margin of printed area */
   CPD.width = width;               /* Max width of printed area */
   CPD.height = height;             /* Max height of printed area */
   allo_cpy(CPD.visual,visual);     /* Visual properties */
   CPD.depth = depth;               /* Depth of color */
   CPD.lwidth = lwidth;             /* Line width */
   ps_set(PPS_TOP_MARG,tmargin,
                PPS_LEFT_MARG,lmargin,
                PPS_WIDTH,width,
                PPS_HEIGHT, height,
                NULL);
   }
 
/**********************************************************************/
/*                                                                    */
/*                         mfc_attribute                              */
/*                                                                    */
/*  mfc_attribute() Sets the attribute of the following variables:    */
/*  MFC.outfn.                                                        */
/*  Border (on/off).                                                  */
/*  Color (on/off).                                                   */
/*  Background (black/white).                                         */
/*                                                                    */
/**********************************************************************/

void mfc_attribute(char *outfn,int flgbdr,int flgcol) {

   strcpy(MFC.outfn ,outfn);
   MFC.flgbdr = flgbdr;
   MFC.flgcol = flgcol;
   }


/**********************************************************************/
/*                                                                    */
/*                         mfc_pgdes                                  */
/*                                                                    */
/*  mfc_attribute() Sets the header on the printed page:              */
/*  infn - Current metafile file name.                                */
/*  currframe - Current metafile frame number.                        */
/*                                                                    */
/**********************************************************************/

void mfc_pgdes(char *infn,int currframe) {
   strcpy(MFC.infn,infn);
   MFF.currframe = currframe;
   MFC.pgdes = 1;
   }


/**********************************************************************/
/*                                                                    */
/*                              PMFind                                */
/*                                                                    */
/*  PMFind() Finds a specific frame indicated by the argumrnt         */
/*  frame.                                                            */
/*                                                                    */
/*  Synopsis: void PMFind(FILES *des,long frame)                      */
/*    fdes  = file descriptor                                         */
/*    frame = the frame number to be shown.                           */
/*                                                                    */
/**********************************************************************/

static void PMFind(FILE *fdes,long frame) {

   int index;
   MFF.currframe = frame;
   clearerr(fdes);
   fseek(fdes,(long)MFF.index[frame],SEEK_SET);
   fgets(buffer,80,fdes);
   PMBuff = (unsigned char *)buffer;
   }/* End of PMFind */


/**********************************************************************/
/*                                                                    */
/*                              MFmaxfrms                             */
/*                                                                    */
/*  MFmaxfrms() Returns the number of frames in a metafile.           */
/*                                                                    */
/*  Prototype: long MFmaxfrms(FILES *des)                             */
/*    fdes  = file descriptor                                         */
/*  Return value:                                                     */
/*       0= no frame not exists.                                      */
/*       else = number of frames exist.                               */
/*                                                                    */
/**********************************************************************/

long MFmaxfrms(FILE *fdes) {

   long currsize=0;
   int index=0;
   char buffer[81];

   if (fdes == MFC.infd) {
      return MFF.maxfrms;
      }

   rewind(fdes);
   fgets(buffer,80,fdes);
  
   /* Check file header for the correct version */
   if (strncmp(buffer,"PLOTDATA V1C",12) !=0)
      return 0;

   MFC.infd = fdes;
   currsize = 256*sizeof(long);
   MFF.index = (long *)malloc(currsize);
   clearerr(MFC.infd);
   while(!feof(MFC.infd)) {
      fgets(buffer,80,fdes);
      if (buffer[0] == '[') {
         if (index*sizeof(long) >= currsize) {
            currsize += 256*sizeof(long);
            MFF.index = (long *)realloc(MFF.index,currsize);
            }
         MFF.index[index++]= (long)ftell(MFC.infd)-strlen(buffer);
         }
      else if (buffer[0] == ']')
         return (MFF.maxfrms=index);
      }
   clearerr(MFC.infd);
   return (MFF.maxfrms=index-1);
   }/* End of PMFind */


/**********************************************************************/
/*                                                                    */
/*                            addTile                                 */
/*                                                                    */
/*    Inserts the partial bitmap data from bitmap records in a        */
/*    metafile into a complete bitmap stored in the 'bitmaps'         */
/*    structure.                                                      */
/*                                                                    */
/*    Return value:                                                   */
/*      -1 if bitmap is still incomplete.                             */
/*      Index into 'bitmaps' structure for a completed bitmap.        */ 
/*                                                                    */
/**********************************************************************/

int addTile(unsigned char *sbuf, float xc, float yc,
            int rowlen, int colht, int xofs, int yofs, 
            int iwd, int iht, int type, int mode) {

   int i, j, iw, iBM, xoffBits, stride, rlen;
   unsigned char *parray;
   static int totBitmaps;

   if (type == BM_BW) {
      rlen = (rowlen+7)>>3;
      iw = (iwd+7)>>3;
      }
   else if (type == BM_COLOR)
      rlen = rowlen;

/* new bitmap? */
   for (iBM=0; iBM<totBitmaps; iBM++)
      if ((bitmaps[iBM].xc == xc) && (bitmaps[iBM].yc == yc)) break;

   if (iBM == totBitmaps) {      /* new bitmap */
      totBitmaps++;
      if (totBitmaps == 1)       /* 1st bitmap */
          bitmaps = (struct bitmap *) malloc(
                                  sizeof(struct bitmap)*totBitmaps);
      else
          bitmaps = (struct bitmap *) realloc(bitmaps,
                                  sizeof(struct bitmap)*totBitmaps);
      bitmaps[iBM].array = (unsigned char *)
                           malloc(sizeof(unsigned char) *
                                  colht*rlen); /* bytes of full BM  */
      bitmaps[iBM].count = 0;
      bitmaps[iBM].xc = xc;
      bitmaps[iBM].yc = yc;
      }
/* insert tile into full bitmap */

   parray = bitmaps[iBM].array;
   parray += yofs*rlen;          /* rowlen in bytes times rows */

   if (type == BM_COLOR) {       
      if (iht>1) sbuf += (iht-1)*iwd; 
      parray += xofs;
      stride = rlen-iwd;
      for (i=0; i<iht; i++) {
          for (j=0; j<iwd; j++) 
              *parray++ = *sbuf++;
          parray += stride;
          sbuf -= iwd*2;
          }
      }
/* BW may not lie on byte boundary, do a bitCopy */
   else if (type == BM_BW) {
      if (iht>1) sbuf += (iht-1)*iw;
      for (i=0; i< iht; i++) {
         bitCopy(sbuf, 0, parray, xofs, iwd);
         parray += rlen;
         sbuf -= iw;
         }
      }

   bitmaps[iBM].count += iwd*iht; /* pixels */
   if (bitmaps[iBM].count == colht*rowlen)
       return iBM;
   else
       return -1;

   } /* end addTile */  

/**********************************************************************/
/*                                                                    */
/*                            bitCopy                                 */
/*                                                                    */
/* Copies 'len' bits from 'array1' at bit offset 'off1' to 'array2'   */
/* at bit offset 'off2'.                                              */
/*                                                                    */
/**********************************************************************/

void bitCopy(unsigned char *array1, int offset1, unsigned char *array2, 
             int offset2, int len) {

   static unsigned char mask[8] = {0x80, 0xc0, 0xe0, 0xf0, 
                          0xf8, 0xfc, 0xfe, 0xff};
   int diff, l, m;
   int bitoff1, bitoff2;

   array1 += offset1>>3; 
   array2 += offset2>>3; 

   bitoff1 = offset1&7;
   bitoff2 = offset2&7;

   while (len) {
      diff = bitoff1 - bitoff2;

      if (diff>0) {
         l = (len<(8-bitoff1)) ? len: (8-bitoff1);
         m = mask[l-1]>>bitoff2;
         *array2 = (*array2 & ~m) | ((*array1<<diff) & m);
         array1++; bitoff1 = 0;
         bitoff2 += l;
         }
      else if (diff<0) {   
         l = (len<(8-bitoff2)) ? len: (8-bitoff2);
         m = mask[l-1]>>bitoff2;
         *array2 = (*array2 & ~m) | ((*array1>>(-diff)) & m);
         array2++; bitoff2 = 0;
         bitoff1 += l;
         }
      else {
         l = (len<(8-bitoff1)) ? len: (8-bitoff1);
         m = mask[l-1]>>bitoff2;
         *array2 = (*array2 & ~m) | (*array1 & m);
         array1++; bitoff1 = 0;
         array2++; bitoff2 = 0;
         }
      len -= l;
      }
   } /* end bitcopy */

/**********************************************************************/
/*                                                                    */
/*                            PMParse                                 */
/*                                                                    */
/*  PMParse() reads an NSI graphics metafile and translates it to     */
/*  drawing commands.                                                 */
/*                                                                    */
/*  Prototype: int PMParse(FILE *fdes,int frame)                      */
/*    fdes  = file descriptor                                         */
/*    frame   = the frame number to be shown, if 0 then all frames    */
/*    will be displayed.                                              */
/* Return value:                                                      */
/*       0=successful completion,                                     */
/*       3=frame not found,                                           */
/*                                                                    */
/**********************************************************************/

static int PMParse(FILE *fdes,int frame) {

   int mode,krt = 0,End = OFF,loc;
   int first_pass =1;
   int skip=ON;
   char c1,c2;

   /* Find a specific frame */
   PMFind(fdes,frame);        

   while (!End) {

      /* Skip fgets only once. */
      if (skip) 
         skip = OFF;
      else {
         fgets(buffer,MAXLINE,fdes);
         PMBuff = (unsigned char *)buffer;
         }

      c1 = toupper(PMBuff[0]);           
      c2 = toupper(PMBuff[1]); 
         
      switch (c1) {
         case '\n':
            break;
         case '[':
            {

            int index,n;
            float xsz, ysz, xi, yi;

            PMBuff += 1;
            /* Get integer argument */
            a2i(index,PMBuff,4,DECENCD);
            a2f(xsz,PMBuff,5,3,DECENCD);
            a2f(ysz,PMBuff,5,3,DECENCD);
            a2f(xi,PMBuff,5,3,DECENCD);
            a2f(yi,PMBuff,5,3,DECENCD);
            a2i(n,PMBuff,1,DECENCD);
            PMBuff[n] = '\0';
    
               /* Initialize graphic system, start a new frame */
               if (first_pass){
                  if (MFC.device != MFC_SCREEN) {
                     ps_new_page(xsz,ysz);
                     if (SXGL.ngcallno==1)
                        psretrace(SXGL.curretrac+adjust);
                     }
                  else  {
#ifdef PLOT
                     ngraph(xsz, ysz, xi,yi,
                        "SOLID","BLACK",(char *)PMBuff);
                     if (SXGL.ngcallno==1)
                        retrace(SXGL.curretrac+adjust);
                     }
#endif
                  first_pass = 0;
                  }
               else {
                  if (MFC.device != MFC_SCREEN)
                     showpage();
                  else 
#ifdef PLOT
                     endplt();
#endif
                  End = ON;         /* Exit while (End) loop */
                  }
            }
            break;
         case ']':
            if (MFC.device != MFC_SCREEN) {
               showpage();
               }
            else {
#ifdef PLOT
               endplt();
#endif
               }
            End = ON;         /* Exit while (End) loop */
            break;
         case 'A':
            if (c2 == 'C') {
                 
               float xc,yc,xs,ys,angle;

               PMBuff += 2;
               /* Get float arguments */
               a2f(xc,PMBuff,5,3,DECENCD);
               a2f(yc,PMBuff,5,3,DECENCD);
               a2f(xs,PMBuff,5,3,DECENCD);
               a2f(ys,PMBuff,5,3,DECENCD);
               a2f(angle,PMBuff,7,3,DECENCD);


               /* Draw arc */
               if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
                  arc(xc,yc,xs,ys,angle);
#endif
                  }
               else {
                  psarc(xc,yc,xs,ys,angle);                  
                  }
               }                
            break;
         case 'B': 
            {
            float xc,yc;
            int iwd,iht,xofs,yofs,type,mode,length,len,i,ls,ms;
            int colht, rowlen, xbitOffs, ibitmap;
            unsigned char *sbuf;

            PMBuff += 1;
            type = (int)((char)*PMBuff < 'a')?((char)*PMBuff-'0'):
                   ((char)*PMBuff-'a');
            PMBuff += 1;
            mode = (int)((char)*PMBuff < 'a')?((char)*PMBuff-'0'):
                   ((char)*PMBuff-'a');
            PMBuff += 1;
            a2f(xc,PMBuff,5,3,DECENCD);
            a2f(yc,PMBuff,5,3,DECENCD);
            a2i(rowlen,PMBuff,4,DECENCD); /* in pixels */
            a2i(colht,PMBuff,4,DECENCD);
            a2i(xofs,PMBuff,4,DECENCD);
            a2i(yofs,PMBuff,4,DECENCD);
            a2i(iwd,PMBuff,4,DECENCD);
            a2i(iht,PMBuff,4,DECENCD);

            PMBuff += 1;
            if (type == BM_BW) {
              xbitOffs = xofs&7;
              length = iht*((iwd+7+xbitOffs)>>3);
              length<<=1; /* bytes */
               }
           else if (type == BM_COLOR) 
               length = ((iwd*iht)<<1)  + 1;
               

            sbuf = (unsigned char *)malloc(length*sizeof(unsigned char));
            fread(sbuf,sizeof(unsigned char),length,fdes);

/* Repack from hex. This is required for xgl, but not for PS. I repack it for
   both cases to simplify the reassembly of partial bitmaps into the 
   complete bitmap that is passed to PS. This becomes important when the samples
   don't fall on  byte boundaries.   */

               for (i=0;i<length;i+=2) {
                  ms = sbuf[i] < 'a' ? sbuf[i] - '0' : 
                      sbuf[i] - ('a'-10);
                  ls = sbuf[i+1] < 'a' ? sbuf[i+1] - '0' : 
                       sbuf[i+1] - ('a'-10);
                  sbuf[i/2] = ls | (ms << 4);
                  }
               PMBuff += 1;
            if (MFC.device == MFC_SCREEN)  
#ifdef PLOT          
            /* Draw bitmap */
               img_shw(sbuf, xc, yc, xofs, yofs, iwd, iht,
                        type, mode);
#endif
               
            else 
               if ((ibitmap = addTile(sbuf, 
                         xc, yc, rowlen, colht, xofs, yofs, 
                        iwd, iht, type, mode))>=0) {

                  psbitmap(bitmaps[ibitmap].array, xc, yc, rowlen,
                           colht, type, mode);
                  free(bitmaps[ibitmap].array);
                  } 
            free(sbuf);
            }
            break;
         case 'C':
            {
            float x,y,r;

            PMBuff += 2;
            /* Get float arguments */
            a2f(x,PMBuff,5,3,DECENCD);
            a2f(y,PMBuff,5,3,DECENCD);
            a2f(r,PMBuff,5,3,DECENCD);

            if (c2 == 'F') {
               /* draw filled circle */
               if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
                  circle(x,y,r,-1);
#endif
                  }
               else {
                  pscircle(x,y,r,-1);
                  }
               }
            else if (c2 == 'O') {
               /* draw filled circle */
               if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
                  circle(x,y,r,1);
#endif
                  }
               else {
                  pscircle(x,y,r,1);
                  }
               }
            }
            break;
         case 'E': 
            {
            float x,y,w,h,angle;

            PMBuff += 2;
            /* Get float arguments */
            a2f(x,PMBuff,5,3,DECENCD);
            a2f(y,PMBuff,5,3,DECENCD);
            a2f(w,PMBuff,5,3,DECENCD);
            a2f(h,PMBuff,5,3,DECENCD);
            a2f(angle,PMBuff,7,3,DECENCD);

            if (c2 == 'F') {
               /* draw filled ellipse */
               if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
                  ellips(x,y,w,h,angle,FILLED);
#endif
                  }
               else {
                  psellips(x,y,w,h,angle,FILLED);
                  }
               }
            else if (c2 == 'O') {
               /* draw filled ellipse */
               if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
                  ellips(x,y,w,h,angle,1);
#endif
                  }
               else {
                  psellips(x,y,w,h,angle,1);
                  }
               }
            }
            break;
         case 'D':
            {
 
            float x,y;

            PMBuff += 1;
            /* Get float arguments */
            a2f(x,PMBuff,5,3,DECENCD);
            a2f(y,PMBuff,5,3,DECENCD);

               /* Plot pen up */
               if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
                  plot(x,y,PENDOWN);
#endif
                  }
               else {
                  psplot(x,y,PENDOWN);
                  }
            }
            break;
         case 'H': 
            {
            int krt;

            PMBuff += 1;
            a2i(krt,PMBuff,1,DECENCD);
               if (MFC.device == MFC_SCREEN) 
#ifdef PLOT
                  retrace(krt+adjust);
#endif
               else 
                  psretrace(krt+adjust);
                  
            }
            break;
         case 'P':
            {
 
            float *x,*y;
            int np,i,kf;

            PMBuff += 2;

            /* Get float arguments */
            a2i(np,PMBuff,3,DECENCD);

            if (!(x = (float *) malloc(np*sizeof(float)))) {
               printf("(MFXpar)Memory allocation failed!\n");
               exit(1);
               }
            if (!(y = (float *) malloc(np*sizeof(float)))) {
               printf("(MFXpar)Memory allocation failed!\n");
               exit(1);
               }

            for (i=0;i<np;i++) {
               a2f(x[i],PMBuff,5,3,DECENCD);
               a2f(y[i],PMBuff,5,3,DECENCD);
               }

            /* Draw polyln */
            switch (c2) {
               case 'O':
                  kf = 1;
                  break;
               case 'F':
                  kf = -1;
                  break;
               case 'L':
                  kf = 3;
                  break;
               }
            if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
               polyln(kf,np,x,y);
#endif
               }
            else {
               pspolyln(kf,np,x,y);
               }
            free(x);
            free(y);
            }
            break;
         case 'I': 
            {
            int col;

            PMBuff += 1;
            /* Get integer argument */
            a2i(col,PMBuff,3,DECENCD);

            /* Set color */
            if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
               color(col);
#endif
               }
            else {
               pscolor(col);
               }
            }    
            break;
         case 'K':
            {
            int n;

            PMBuff += 1;
            a2i(n,PMBuff,1,DECENCD);

            /* Get string argument */
            PMBuff[n] = '\0';

            /* Set pen color */
            if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
               pencol((char *)PMBuff);
#endif
               }
            else {
               pspencol((char *)PMBuff);
               }
            }
            break;
         case 'L': 
            {

            float x1,y1,x2,y2;
            int n;

            PMBuff += 1;
            /* Get float arguments */
            a2f(x1,PMBuff,5,3,DECENCD);
            a2f(y1,PMBuff,5,3,DECENCD);
            a2f(x2,PMBuff,5,3,DECENCD);
            a2f(y2,PMBuff,5,3,DECENCD);

            /* Draw a line */
            if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
               line(x1,y1,x2,y2);
#endif
               }
            else {
               psline(x1,y1,x2,y2);
               }
            }   
            break;
         case 'M':
            {
            float x,y;

            PMBuff += 1;
            /* Get float arguments */
            a2f(x,PMBuff,5,3,DECENCD);
            a2f(y,PMBuff,5,3,DECENCD);

            /* Plot pen up */
            if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
               plot(x,y,PENUP);
#endif
               }
            else {
               psplot(x,y,PENUP);
               }
            }
            break;
         case 'Q':
            {
            float x,y,e;

            PMBuff += 2;
            /* Get float arguments */
            a2f(x,PMBuff,5,3,DECENCD);
            a2f(y,PMBuff,5,3,DECENCD);
            a2f(e,PMBuff,5,3,DECENCD);

            if (c2 == 'F') {
               /* Draw filled box */
               if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
                  rect(x,y,e,e,-1);
#endif
                  }
               else {
                  psrect(x,y,e,e,-1);
                  }
               }
            else if (c2 == 'O') {
               /* Draw filled box */
               if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
                  rect(x,y,e,e,1);
#endif
                  }
               else {
                  psrect(x,y,e,e,1);
                  }
               }
            }
            break;
         case 'R':
            {
            float x1,y1,x2,y2;

            PMBuff += 2;
            /* Get float arguments */
            a2f(x1,PMBuff,5,3,DECENCD);
            a2f(y1,PMBuff,5,3,DECENCD);
            a2f(x2,PMBuff,5,3,DECENCD);
            a2f(y2,PMBuff,5,3,DECENCD);

            if (c2 == 'F') {
               /* Draw filled box */
               if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
                  rect(x1,y1,x2,y2,-1);
#endif
                  }
               else {
                  psrect(x1,y1,x2,y2,-1);
                  }
               }
            else if (c2 == 'O') {
               /* Draw  box */
               if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
                  rect(x1,y1,x2,y2, 1);
#endif
                  }
               else {
                  psrect(x1,y1,x2,y2, 1);
                  }
               }
            }
            break;
         case 'S':
            {

            float x,y,ht,angle;
            int n;

            PMBuff += 1;
            /* Get float arguments */
            a2f(x,PMBuff,5,3,DECENCD);
            a2f(y,PMBuff,5,3,DECENCD);
            a2f(ht,PMBuff,5,3,DECENCD);
            a2f(angle,PMBuff,7,3,DECENCD);

            /* Get integer argument */
            a2i(n,PMBuff,3,DECENCD);

            PMBuff[n] = '\0';

               /* Draw text */
               if (MFC.device == MFC_SCREEN) {
#ifdef PLOT
                  xgsymbol( x,y,ht,(char *)PMBuff,angle,n);
#endif
                  }
               else {
                  pssymbol( x,y,ht,(char *)PMBuff,angle,n);
                  }

            }    
            break;
         case 'T': 
            {
             
            int n;
            char type[9];

            PMBuff += 1;
            a2i(n,PMBuff,1,DECENCD);

            /* Get string argument */
            PMBuff[n] = '\0';

            /* Set pen type */
            /*printf("pentyp(%s) \n",PMBuff);*/

            break;
            }
         default:
            fprintf(stderr,"(mfconv) Unsupported command!\n");
            fprintf(stderr, "PMBuff[0] is %c \n ", PMBuff[0]);
            break;
         }/* End of Switch()           */
      }   /* End of while (End)        */
   }      /* End of function PMParse() */


int mfconv(FILE *fdes, int frame,int device,int type)  {

   int j;
   char tmpnam_buff[L_tmpnam];
   char header[81],cmdbuff[256];
   struct tm *tmstmp;
   time_t tp;

   time(&tp);
   tmstmp = localtime(&tp);

   rewind(fdes);
   fgets(header,80,fdes);
  
   /* Check file header for the correct type */
   if (strncmp(header,"PLOTDATA V1C",12) !=0)
      return INVAL_HEADER;

   if (device != MFC_SCREEN) {
      strcpy(MFC.tmpfn,tmpnam(tmpnam_buff));
      if ((MFC.tmpfd = fopen(MFC.tmpfn,"w+")) == NULL) {
        return ERR_OPEN;
        }
      ps_set(PPS_FILE,MFC.tmpfd,
             PPS_BORDER,MFC.flgbdr,
             PPS_VISUAL,MFC.flgcol,
             NULL);
      }
  
   /* Advance two more lines */
   fgets(MFC.title,80,fdes);
   fgets(MFC.date,80,fdes);

   MFC.device =  device;
   MFC.type =  type;
   MFC.infd =  fdes;

   /* Select the right device */
   switch (MFC.device) {
      case MFC_PRINT:
         /* Print current frame from metafile */
         if (MFC.pgdes) {
            char title2[80];

            sprintf(title2,"File name: %s     Frame #: %d    Created: %c%c/%c%c/%c%c    Printed: %2d/%2d/%2d",
            MFC.infn,MFF.currframe+1,MFC.date[2],
            MFC.date[3],MFC.date[4],MFC.date[5],MFC.date[0],MFC.date[1],
            tmstmp->tm_mon+1,tmstmp->tm_mday,tmstmp->tm_year);

            ps_set(PPS_HEADER,PPS_ON,
                PPS_TITLE_LINE1, MFC.title,
                PPS_TITLE_LINE2, title2,
                NULL);
            MFC.pgdes = 0;
            }
         if (PMParse(MFC.infd,frame) == NOT_FOUND)
            return NOT_FOUND;
         fclose(MFC.tmpfd);
         strcpy(cmdbuff,"lpr -P");
         strcat(cmdbuff,CPD.alias);
         strcat(cmdbuff," ");
         strcat(cmdbuff,MFC.tmpfn);
         system(cmdbuff);
         unlink(MFC.tmpfn);
         break;
      case MFC_SAVE:
         /* Save current frame in EPS form */
         if (MFC.pgdes) {
            char title2[80];

            sprintf(title2,"File name: %s     Frame #: %d    Created: %c%c/%c%c/%c%c    Printed: %2d/%2d/%2d",
            MFC.infn,MFF.currframe+1,MFC.date[2],
            MFC.date[3],MFC.date[4],MFC.date[5],MFC.date[0],MFC.date[1],
            tmstmp->tm_mon+1,tmstmp->tm_mday,tmstmp->tm_year);

            ps_set(PPS_HEADER,PPS_ON,
                PPS_TITLE_LINE1, MFC.title,
                PPS_TITLE_LINE2, title2,
                NULL);
            MFC.pgdes = 0;
            }
         if (PMParse(MFC.infd,frame) == NOT_FOUND)
            return NOT_FOUND;
         fclose(MFC.tmpfd);
         strcpy(cmdbuff,"cp ");
         strcat(cmdbuff,MFC.tmpfn);
         strcat(cmdbuff," ");
         strcat(cmdbuff,MFC.outfn);
         system(cmdbuff);
         unlink(MFC.tmpfn);
         break;
      case MFC_SCREEN:
         /* Display current frame on screen */
         if (PMParse(MFC.infd,frame) == NOT_FOUND)
            return NOT_FOUND;
         break;
      }

   return PM_OK;

   } /* End of mfconv() routine */







