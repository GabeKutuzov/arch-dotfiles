/***********************************************************************
*                                                                      *
*                               mfdraw.h                               *
*                                                                      *
***********************************************************************/

#ifndef __MFDRAW_H__
#define __MFDRAW_H__

#include "sysdef.h"

#define WIDTH  8
#define HEIGHT 8

class Viewport {
/*---------------------------------------------------------------------*
*                        opengl drawing commands                       *
*---------------------------------------------------------------------*/
   public:
   void drawCircle(double x1, double y1, double r);
   void drawFilledCircle(double x1, double y1, double r);

   void drawLine(double x1, double y1, double x2, double y2);

   void drawOutlineellipse(double x, double y, double w,
      double h, double inclineangle);

   void drawOutlinesquare(double x, double y, double e);
   void drawFilledsquare(double x, double y, double e);

   void drawFillclosepolyline(int np, double *v);
   void drawOutlineclosepolyline(int np, double *v);
   void drawOpenpolyline(int np, double *v);

   void drawOutlinerectangle(double x, double y, double e1, double e2);
   void drawFilledrectangle(double x, double y, double e1, double e2);

   void colorname(string s);
   void colorbgr(int b, int g, int r);
   void colorbbggrr(int b, int g, int r);

   void drawText(double x, double y, double ht, double angle,
      int len, string text);
   void linethick(int t);

   void drawBitmap(int type, int mode, float xc, float yc,
      int rowlen, int colht, int xofs, int yofs, int iwd, int iht,
      int length, char* buf);
   void drawBitmapgrayscalescalable(int type, int mode,
      float xc, float yc, float bwd, float bht, int rowlen, int colht,
      int xofs, int yofs, int iwd, int iht, int length, char* buf);
};

#endif /* Not defined __MFDRAW_H__ */
