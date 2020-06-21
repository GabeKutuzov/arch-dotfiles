/* (c) 1993 Neurosciences Research Foundation */
/* $Id: mfcv.h 1 2008-12-19 16:56:22Z  $ */
/**********************************************************************
 *                                                                    *
 *                              MFCV.H                                *
 *                                                                    *
 *  This header file contain function prototype for mfconv library.   *
 *                                                                    *
 * Version 1, 04/16/93, ROZ                                           *
 * ==>, 07/01/02, GNR - Last date before committing to svn repository *
 **********************************************************************/
#ifndef _MFCV_H
#define _MFCV_H


#define MFC_ALL     0
#define MFC_SAVE    1
#define MFC_SCREEN  2
#define MFC_PRINT   3
#define MFC_EPS     0
#define MFC_XGL     1
#define NAMEMAX     81

/* Function prototype */
int mfconv(FILE *fdes, int frame,int device,int type);
void mfc_attribute(char *outfn,int flgbdr,int flgcol);
void mfc_prn(char *alias,float tmargin,float lmargin,
   float width,float height,char *visual,int depth,int lwidth);
void mfc_pgdes(char *infn,int currframe);
static void PMFind(FILE *fdes,long frame);
long MFmaxfrms(FILE *fdes);
int addTile(unsigned char *sbuf, float xc, float yc,
            int rowlen, int colht, int xofs, int yofs,
            int iwd, int iht, int type, int mode);
void bitCopy(unsigned char *array1, int offset1, unsigned char *array2,
             int offset2, int len);

static int PMParse(FILE *fdes,int frame);
#endif
