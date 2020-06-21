/* (c) 1993 Neurosciences Research Foundation */
/* $Id: pltxgl.h 1 2008-12-19 16:56:22Z  $ */
/**********************************************************************
 *                                                                    *
 *                         PLTXGL.H Header File                       *
 *                                                                    *
 *   This header file contain funcyion prototypes for the library     *
 *   plotxgl.                                                         *
 *                                                                    *
 * Version 1, 04/16/93, ROZ                                           *
 * ==>, 07/01/02, GNR - Last date before committing to svn repository *
 **********************************************************************/
#ifndef _PLTXGL_H
#define _PLTXGL_H

#include "plotdefs.h"


void arc(float xc, float yc, float xs, float ys,float angle);
void circle(float xc, float yc, float radius, int krt);
void color(int color);
void ellips(float xc, float yc, float hw, float hh, float angle, int krt);
int  endplt(void);
void img_shw(unsigned char *array, float xc, float yc, int xofs,
                int yofs, int iwd, int iht, int type, int mode);
void line(float x1, float y1, float x2, float y2);
int  ngraph(float maxw, float maxh, float xorg, float yorg,
      char *pentyp, char *color, char *chart);
void pencol(char *str);
void penset(char *pentyp, char *color);
void plot(float x, float y, int ipen);
void plot_close();
void plot_init(Display *display,Window window,int screen,
         int init_width,int init_height);
void polyln(int kf, int np, float *x, float *y);
void symbol(float x, float y, float ht,
      char *text, float angle, int n);
void xgsymbol(float, float, float, char *, float, int);
void rect(float x, float y, float width, float height,int krt);
void repaint_proc ();
void resize_proc (int new_width,int new_height);
void reset_vdc();
#endif
