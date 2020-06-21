/* (c) Copyright 1992-2017, The Rockefeller University *11115* */
/* $Id: plots.h 65 2018-08-15 19:26:04Z  $ */
/***********************************************************************
*                         PLOTS.H Header File                          *
*                  ROCKS plot function declarations                    *
*                                                                      *
*  N.B.  Not all the functions defined here exist in the oldplot src.  *
************************************************************************
*  V1A, 10/08/90, GNR - Initial version                                *
*  Rev, 05/11/92, GNR - Remove plotdefs.h to separate file             *
*  Rev, 09/20/93, GNR - Correct axlin, kill new fns on NN (overflow)   *
*  Rev, 07/09/97, GNR - Add debug argument to setmfd                   *
*  Rev, 12/06/98, GNR - Add arc2, ctscol, ctsyn, ctload, frame, frameb,*
*                       framec, framex, gobjid, numbc, polygn, symbc,  *
*                       remove gscale, setmeta, setmfd                 *
*  Rev, 04/21/01, GNR - Add domvexp arg to newplt                      *
*  Rev, 05/25/02, GNR - Revise axlin, axlog, axpol, and arrow, remove  *
*                       arrow2, add mfbegatom and mfendatom            *
*  Rev, 02/27/08, GNR - Add finplt()                                   *
*  ==>, 03/12/08, GNR - Last date before committing to svn repository  *
*  Rev, 08/26/11, GNR - Change dbgmask long --> ui32                   *
*  Rev, 10/10/11, GNR - Make compatible with both plot and oldplot src *
*  Rev, 08/29/14, GNR - Updated new plot calls, add const to pointers  *
*  Rev, 03/31/17, GNR - Add bgrv typedef and deccol function           *
***********************************************************************/

#ifndef __PLOTS_HDR_INCLUDED__
#define __PLOTS_HDR_INCLUDED__

/* Bring in defined constants used in plot calls */
#include "plotdefs.h"

struct bgrv_t {
   si16  bb,gg,rr; };
typedef struct bgrv_t bgrv;

typedef struct rkg_inc RKGHOLD;

#ifdef __cplusplus
extern "C" {
#endif
void arc(float xc, float yc, float xs, float ys, float angle);
void arc2(float xc, float yc, float xs, float ys, float angle);
void arrow(float x1, float y1, float x2, float y2, float barb,
      float angle);
void axlin(float x, float y, const char *label, float axlen,
      float angle, float firstt, float deltat, float firstv,
      float deltav, float height, int ktk, int nd, int nmti);
void axlog(float x, float y, const char *label, float axlen,
      float angle, float firstt, float deltat, float firstv,
      float vmult, float height, int ktk, int nd, int nmti);
void axpol(float x, float y, const char *label, float radius,
      float firstc, float deltac, float firstv, float deltav,
      float angle, float cvoffx, float cvoffy, float height,
      int ns, int nl, int ktk, int nd, int nmti);
void bitmap(const unsigned char *array, int rowlen, int colht,
      float xc, float yc, int xoff, int yoff, int iwd, int iht,
      int type, int mode);
void bitmaps(const unsigned char *array, int rowlen, int colht,
      float xc, float yc, float bwd, float bht, int xoff,
      int yoff, int iwd, int iht, int type, int mode);
int  bmpxsz(int bmcm);
void chgwin(unsigned int iwin);
void circle(float xc, float yc, float radius, int kf);
void clswin(unsigned int iwin);
#ifdef __GLUH__                  /* Old prototype */
void color(COLOR index);
#else
void color(COLOR blue, COLOR green, COLOR red);
#endif
byte col2gr(byte bmcol);
/*** ctload(), ctscol(), ctsyn() are is obsolete--
*    remove when new plot library in use ***/
void ctload(COLOR index, int blue, int green, int red);
void ctscol(unsigned int icts);
unsigned int ctsyn(const char *expcol);
int  deccol(bgrv *pbgr, const char *pcol);
void ellips(float xc, float yc, float hw, float hh, float angle,
      int kf);
int  endplt(void);
void factor(float fac);
int  finplt(void);
void finwin(unsigned int iwin);
void font(const char *fname);
void frame(unsigned int frame);
void frmdbm(unsigned int frame);
void frmdef(unsigned int *pfrm, unsigned int mode,
   float width, float height, float vv[6]);
void frmdel(unsigned int frame);
int  frminq(unsigned int frame, float vv[6]);
void frmplt(unsigned int frame);
void gmode(int mode);
void gobjid(unsigned long id);
float gscale(void);
void line(float x1, float y1, float x2, float y2);
int  mfprtf(float x, float y, float ht, float angle,
      const char *fmt, ...);
int  newplt(float xsiz, float ysiz, float xorg, float yorg,
      const char *pentyp, const char *color, const char *chart,
      int kout);
int  newpltw(float xsiz, float ysiz, float xorg, float yorg,
      const char *pentyp, const char *color, const char *chart,
      int kout, unsigned int iwin, unsigned int fmode);
unsigned int newwin(void);
void nombc(float ht, float angle, double val, int ndec);
void nomber(float x, float y, float ht, float angle, double val,
      int ndec);
void numbc(float ht, double val, float angle, int ndec);
void number(float x, float y, float ht, double val,
      float angle, int ndec);
void pencol(const char *color);
void penset(const char *pentype, const char *color);
void pentyp(const char *pentyp);
void plot(float x, float y, int ipen);
char *plotvers(void);
void polygn(float xc, float yc, float radius, int nv,
      float angle, float dent, float spike, int kf);
void polyln(int kf, int np, const float *x, const float *y);
void rcolor(unsigned int jc);
void rect(float x, float y, float wd, float ht, int kf);
unsigned int regcol(const char *pencol);
unsigned int regfont(const char *font);
unsigned int regpen(const char *pentyp);
void retrace(int krt);
void rfont(unsigned int jf);
void rpen(unsigned int jp);
/* The following 4 setxxx() routines can be called from parallel
*  nodes, but the calls will be ignored.  */
void setmf(const char *fname, const char *station, const char *title,
      const char *icon, long buflen, ui32 dbgmask, int enc, int lci,
      int lcf);
/* setmeta,setmfd are obsolete--remove when new plot library in use */
void setmeta(char *fname, long buflen);
void setmfd(char *station, char *wtitl, long buflen, ui32 dbgmask);
void setmovie(int mmode, int device, int nexpose);
void square(float x, float y, float edge, int kf);
void simbc(float ht, float angle, const char *text, int n);
void simbol(float x, float y, float ht, float angle,
      const char *text, int n);
void symbc(float ht, const char *text, float angle, int n);
void symbol(float x, float y, float ht, const char *text,
      float angle, int n);
void where(float *x, float *y, float *fac);

/* The following are used only in a pic environment */
RKGHOLD *rkgcpy(void);
void rkgget(RKGHOLD *prkgh);
void rkgput(RKGHOLD *prkgh);
void rkgcls(RKGHOLD *prkgh);
#ifdef __cplusplus
}
#endif

#endif  /* __PLOTS_HDR_INCLUDED__ */
