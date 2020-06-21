/* (c) 1993 Neurosciences Research Foundation */
/* $Id: pltps.h 1 2008-12-19 16:56:22Z  $ */
/**********************************************************************
 *                                                                    *
 *                         PLTPS.H Header File                        *
 *                                                                    *
 *   This header file contain function prototypes and defined         *
 *   constant that are used in the library plotps.                    *
 *                                                                    *
 * Version 1, 04/16/93, ROZ                                           *
 * ==>, 07/01/02, GNR - Last date before committing to svn repository *
 **********************************************************************/
#ifndef _PLTPS_H
#define _PLTPS_H

#include "plotdefs.h"


/* Set of attributes */
#define PPS_BACKGROUND     1
#define PPS_BORDER         2
#define PPS_VISUAL         3
#define PPS_FILE           4
#define PPS_TOP_MARG       5
#define PPS_LEFT_MARG      6
#define PPS_WIDTH          7
#define PPS_HEIGHT         8
#define PPS_DEPTH          9
#define PPS_TITLE_LINE1    11
#define PPS_TITLE_LINE2    12
#define PPS_HEADER         13

/* Set of values */
#define PPS_WHITE          1
#define PPS_COLOR          1
#define PPS_GRAY           2
#define PPS_MONO           0
#define PPS_BLACK          0
#define PPS_ON             1
#define PPS_OFF            0

#define NAMEMAX     81

struct bitmap {
   float xc;
   float yc;
   int count;
   unsigned char *array;
   } *bitmaps;

int f2a(char *buff, float num, int dec);
void ps_new_page(float maxx, float maxy);
void ps_set(int firstint, ...);
void psarc(float xc,float yc,float xs,float ys,float angle);
void psarrow(float x1,float y1,float x2,float y2);
void psarrow2(float x1,float y1,float x2,float y2,float barb);
void psaxis(float x,float y,char *label,int n,
      float axlen,float angle,float firstv,float deltav);
void psaxlin(float x,float y,char *label,int n,
      float axlen,float angle,float firstt,
      float deltat,float firstv,float deltav,int nd);
void psaxlog(float x,float y,char *label,int n,
      float axlen,float angle,float declen,
      float firstv,float heght,int nd);
void psaxpol(float x,float y,char *label,int n,
      float radius,float firstv,float deltav,
      float height,int nc,int ns,int nl,int nt,int nd);
void psbitmap(unsigned char *, float xc, float yc,
              int iwd, int iht, int type, int mode);
void pscircle(float x,float y,float r,int krt);
void psellips(float x,float y,float w,float h,float angle,int krt);
void pscolor(int col);
void psline(float x1, float y1, float x2, float y2);
void psplot(float x,float y,int ipen);
void pspolyln(int kf, int np, float *x, float *y);
void pspencol(char *color);
void psrect(float x1,float y1,float x2,float y2,int krt);
void psretrace(int krt);
void pssymbol(float x,float y,float ht,char *text,float angle,int n);
void pspentyp(char *type);
void showpage();
int addTile(unsigned char *sbuf, float xc,
                    float yc, int colht, int rowlen, int xofs,
                    int yofs, int iwd, int iht, int type, int mode);
void bitCopy(unsigned char *array1, int offset1, unsigned char *array2,
             int offset2, int len);

#endif
