/***********************************************************************
*                                                                      *
*                             Viewport.cxx                             *
*                                                                      *
***********************************************************************/

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include "sysdef.h"
#include "rocks.h"
#include "plotops.h"
#include "mfdraw.h"

/*---------------------------------------------------------------------*
*                 FrameView OpenGL methods implementation              *
*---------------------------------------------------------------------*/

void Viewport::Viewport(int width, int height) {
   
   Fl_Window *window = new Fl_Window(width*96,height*96);
   /*
   Fl_Box *box = new Fl_Box(20,40,300,100,"I am a box.");
   box->box(FL_UP_BOX);
   box->labelfont(FL_BOLD+FL_ITALIC);
   box->labelsize(36);
   box->labeltype(FL_SHADOW_LABEL);
   */
   window->end();
   window->show(argc, argv);
}

void Viewport::drawCircle(double x1, double y1, double r) {
   // (length of circumference)*(pixels per inch)*zoom ==>
   //    num pixels needed to draw circle
   int numpix =
      int(TWOPI*r*(this->h()/(top-bottom))*(ptrzoom->value()));
   if (numpix < 3)
      numpix = 3;
   else if (numpix > 360)
      numpix = 360;
   int increment = 360/numpix;
   // cout<<"cricle increment = "<<increment<<" with r = "<<r<<endl;
   glBegin(GL_LINE_LOOP);
   for (int i=0; i < circledata.getcirclepoints(); i += increment)
      glVertex2f(
         r*circledata.getmycos(i)+x1, r*circledata.getmysin(i)+y1);
   glEnd();
}

void FrameView::drawFilledcircle(double x1, double y1, double r) {
   // (length of circumference)*(pixels per inch)*zoom ==>
   //    num pixels needed to draw circle
   int numpix =
      int(TWOPI*r*(this->h()/(top-bottom))*(ptrzoom->value()));
   if (numpix < 3)
      numpix = 3;
   else if (numpix > 360)
      numpix = 360;
   int increment = 360/numpix;
   // cout<<"filled cricle increment = "<<increment<<" with r = "<<r<<endl;
   glBegin(GL_POLYGON);
   for (int i=0; i < circledata.getcirclepoints(); i += increment) {
      glVertex2f(
         r*circledata.getmycos(i)+x1, r*circledata.getmysin(i)+y1);
      }
   glEnd();
   } /* End drawFilledcircle */

void Viewport::drawLine(double x1, double y1, double x2, double y2) {
   glLineWidth(1.0 + current_line_thickness);
   glBegin(GL_LINES);
   glVertex2f(x1,y1);
   glVertex2f(x2,y2);
   glEnd();
   } /* End drawLine() */

void FrameView::drawOutlineellipse(double x, double y, double w,
      double h, double inclineangle) {
   // Incline angle not yet implemented
   double r;
   // Approx. ellipse with a circle instead of using first terms of an
   // infinite series that approximates perimeter of ellipse
   r = (w >= h) ? w : h;
   int numpix =
      int(TWOPI*r*(this->h()/(top-bottom))*(ptrzoom->value()));
   if (numpix < 3)
      numpix = 3;
   else if (numpix > 360)
      numpix = 360;
   int increment = 360/numpix;
   glBegin(GL_LINE_LOOP);
      for (int i=0; i < circledata.getcirclepoints(); i += increment) {
         glVertex2f(
            x + w*circledata.getmycos(i), y + h*circledata.getmysin(i));
         }
   glEnd();
   } /* End drawOutlineelipse() */

void FrameView::drawFilledellipse(double x, double y, double w,
      double h, double inclineangle) {
   // Incline angle not yet implemented
   double r;
   // Approx. ellipse with a circle instead of using first terms of an
   // infinite series that approximates perimenter of ellipse
   r = (w >= h) ? w : h;
   int numpix =
      int(TWOPI*r*(this->h()/(top-bottom))*(ptrzoom->value()));
   if (numpix < 3)
      numpix = 3;
   else if (numpix > 360)
      numpix = 360;
   int increment = 360/numpix;
   glBegin(GL_POLYGON);
   for (int i = 0; i < circledata.getcirclepoints(); i += increment) {
      glVertex2f(
         x + w*circledata.getmycos(i), y + h*circledata.getmysin(i));
      }
   glEnd();
   } /* End drawFilledellipse() */

void FrameView::drawFillclosepolyline(int np, double *c) {

#if DEBUG & DBGDRAW
   writetolog("**In mfdraw.cxx in drawFillclosepolyline() drawing "
      "convex or concave polygons using polygon tesellation ");
#endif

   GLUtesselator * tobj;
   tobj = gluNewTess();
   // Registering Tessellation Callbacks
   gluTessCallback(tobj, GLU_TESS_VERTEX,(GLvoid (*) ()) &glVertex3dv);
   gluTessCallback(tobj, GLU_TESS_BEGIN, (GLvoid (*) ()) &beginCallback);
   gluTessCallback(tobj, GLU_TESS_END,   (GLvoid (*) ()) &endCallback);
   gluTessCallback(tobj, GLU_TESS_ERROR, (GLvoid (*) ()) &errorCallback);
   gluTessProperty(tobj,GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);

   // Begins and ends the specification of a polygon to be tessellated
   //    and associates a tessellation object, tobj, with it.
   // c points to a user-defined data structure, which is passed along
   //    to all the GLU_TESS_*_DATA callback functions that have been
   //    bound.
   gluTessBeginPolygon (tobj, c);
   gluTessBeginContour(tobj);

   for (int i=0; i <np; i++) { // Clockwise
      gluTessVertex(tobj, c+3*i, c+3*i);
      }

   gluTessEndContour(tobj);
   gluTessEndPolygon (tobj);

   } /* End FrameView::drawFillclosepolyline */

//=======end tesselation work area =========

void FrameView::drawOutlineclosepolyline(int np, double *c) {
   // writetolog("in mfdraw.cxx: in drawOutlineclosepolyline()");
   glBegin(GL_LINE_LOOP);
   for (int i=0; i < 2*np; i+=2)
      glVertex2d(c[i], c[i+1]);
   glEnd();
   } /* End drawOutlineclosepolyline() */

void FrameView::drawOpenpolyline(int np, double *c) {
   // writetolog("in drawOpenpolyline()");
   glBegin(GL_LINE_STRIP);
   for (int i=0; i < 2*np; i+=2)
      glVertex2d(c[i], c[i+1]);
   glEnd();
   } /* End drawOpenpolyline() */

void FrameView::drawOutlinesquare(double x, double y, double e) {
   // x,y coordinates of lower left-hand corner of square
   // e = edge of square
   glBegin(GL_LINE_LOOP);
   glVertex2f(x,y);
   glVertex2f(x+e,y);
   glVertex2f(x+e,y+e);
   glVertex2f(x,y+e);
   glEnd();
   } /* End drawOutlinesquare() */

void FrameView::drawFilledsquare(double x, double y, double e) {
   // x,y coordinates of lower left-hand corner of square
   // e = edge of square
   glRectd(x,y,x+e,y+e);
   } /* End drawFilledsquare() */

void FrameView::drawOutlinerectangle(double x, double y, double e1,
      double e2) {
   // x,y coordinates of lower left-hand corner of square
   // e1 = edge of x side of rectangle
   // e2 = edge of y side of rectangle
   glBegin(GL_LINE_LOOP);
   glVertex2f(x,y);
   glVertex2f(x+e1,y);
   glVertex2f(x+e1,y+e2);
   glVertex2f(x,y+e2);
   glEnd();
   } /* End drawOutlinerectangle() */

void FrameView::drawFilledrectangle(double x, double y, double e1,
      double e2) {
   // x,y coordinates of lower left-hand corner of square
   // e1 = edge of x side of rectangle
   // e2 = edge of y side of rectangle
   glRectd(x,y,x+e1,y+e2);
   } /* End drawFilledrectangle() */

void FrameView::colorname(string s) {
   if (s == "RED") {
      glColor3f(1.0, 0.0, 0.0);
      return;
      }
   if (s == "YELLOW") {
      glColor3f(1.0, 1.0, 0.0);
      return;
      }
   if (s == "MAGENTA") {
      glColor3f(1.0, 0.0, 1.0);
      return;
      }
   if (s == "CYAN") {
      glColor3f(0.0, 1.0, 1.0);
      return;
      }
   if (s == "ORANGE") {
      glColor3f(1.0, .65, 0.0);
      return;
      }
   if (s == "BLACK") {
      // If background is not white (i.e. it is black) then do not
      //    color in black but color in white
      if (bkgrndwhite[atframe])
         glColor3f(0.0, 0.0, 0.0);
      else
         glColor3f(1.0, 1.0, 1.0);
      return;
      }
   if (s == "BLUE") {
      glColor3f(0.0, 0.0, 1.0);
      return;
      }
   if (s == "GREEN") {
      glColor3f(0.0, 1.0, 0.0);
      return;
      }
   if (s == "WHITE") {
      if (bkgrndwhite[atframe])
         glColor3f(0.0, 0.0, 0.0);
      else
         glColor3f(1.0, 1.0, 1.0);
      return;
      }
   if (s == "VIOLET") {
      glColor3f(.50, 0.0, .75);
      return;
      }
   // cout<<"Error: color "<<s<<" in colorname not defined"<<endl;
   writetolog("Error: colorname not defined");
   exit(1);
   } /* End colorname() */

void FrameView::colorbgr(int b, int g, int r) {

#if DEBUG & DBGDRAW
   sprintf(msglog,"colorbgr: b = %d g = %d r = %d\n", b, g, r);
   writetolog(msglog);
#endif

   // If drawing color is black (or white) and background is black
   // (or white) make drawing color the opposite
   if ((b | g | r) == 0 && !bkgrndwhite[atframe]) {
      glColor3f(1.0, 1.0, 1.0);
      return;
      }
   if ((b & g & r) == 15 && bkgrndwhite[atframe]) {
      glColor3f(0.0, 0.0, 0.0);
      return;
      }
   float blue  = b/15.0;
   float green = g/15.0;
   float red   = r/15.0;
   glColor3f(red, green, blue);
   } /* End colorbgr() */

void FrameView::colorbbggrr(int b, int g, int r) {

#if DEBUG & DBGDRAW
   sprintf(msglog,"colorbbggrr: b = %d g = %d r = %d\n", b, g, r);
   writetolog(msglog);
#endif

   // If drawing color is black (or white) and background is black
   // (or white) make drawing color the opposite
   if ((b | g | r) == 0 && !bkgrndwhite[atframe]) {
      glColor3f(1.0, 1.0, 1.0);
      return;
      }
   if ((b  & g & r) == 255 && bkgrndwhite[atframe]) {
      glColor3f(0.0, 0.0, 0.0);
      return;
      }
   float blue  = b/255.0;
   float green = g/255.0;
   float red   = r/255.0;
   glColor3f(red, green, blue);
   } /* End colorbbggrr() */

void FrameView::drawText(double x, double y, double ht, double angle,
   int len, string text) {

   // glut stroke approach - requires libglut in make
   /* Rev, 05/25/17, GNR - smarx did not implement the special
   *  characters listed in the documentation.  Here I am adding
   *  just superscript/subscript as needed in axlog.  (Can't use
   *  PushMatrix/PopMatrix to restore normal text, as origin is
   *  moved.  */
   int ssstate = 0;     /* Current super/subscript state */
   GLfloat mht = glutStrokeHeight(GLUT_STROKE_MONO_ROMAN);
   GLfloat tscale = (float)ht/mht;
   GLfloat hscale = 0.5*mht;
   glPushMatrix();
   glTranslatef((float)x,(float)y,(float)0);
   if (angle != 0) glRotated(angle,0,0,1);
   glScalef(tscale, tscale, 0.0);
   for (const char *c = text.c_str(); *c; c++) {
      if (*c == 0x12) {       /* Superscript */
         if (ssstate > 0) continue;
         if (ssstate < 0) glScalef(1.5, 1.5, 0);
         glTranslatef(0, hscale, 0);
         if (ssstate == 0) glScalef(0.666667, 0.666667, 0);
         ssstate += 1;
         } /* End superscript */
      else if (*c == 0x14) {  /* Subscript */
         if (ssstate < 0) continue;
         if (ssstate > 0) glScalef(1.5, 1.5, 0);
         glTranslatef(0, -hscale, 0);
         if (ssstate == 0) glScalef(0.666667, 0.666667, 0);
         ssstate -= 1;
         } /* End subscript */
      else glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *c);
      } /* End loop over char string */
   glPopMatrix();
   } /* End drawText() */

/* N.B.  08/24/16, GNR - The glBitmap routine used here is only good
*  for actual bitmaps, i.e.  no grayscale or color.  Thus, the routine
*  drawBitmapgrayscalescalable() is set in mfdraw.cxx to be called for
*  anything that is not a binary map, whether scalable or not.  */

void FrameView::drawBitmap(int type, int mode, float xc, float yc,
   int rowlen, int colht, int xofs, int yofs, int iwd, int iht,
   int length, char* buf) {

#if DEBUG & DBGDRAW
   sprintf(msglog,"in FrameView::drawBitmap() for nonscalable bitmap "
      "at frame = %d", atframe);
   writetolog(msglog);
#endif

   glPushMatrix();

/* Rev, 08/29/16, GNR - Calls to glColor3f here to set color
*  according to bkgrndwhite flag removed so bitmaps can be
*  plotted in the color set by most recent penset() call.  */

   // Adjust for bitmap tile positioning when zooming
   double dxofs = xofs/sizew[atframe];
   double dyofs = yofs/sizeh[atframe];

   float xpixtoinches = (right-left)/(float)this->w();
   float ypixtoinches = (top - bottom)/(float)this->h();

   glRasterPos2f(xc+dxofs*xpixtoinches, yc+dyofs*ypixtoinches);

   switch (mode) {

   case GM_SET:      // Set frame data to buf data
      glBitmap(iwd,iht,0,0,0,0,(const GLubyte*)buf);
      break;

   case GM_XOR:      // Xor buf data with existing frame data
      glEnable(GL_COLOR_LOGIC_OP);
      glLogicOp(GL_XOR);
      glBitmap(iwd,iht,0,0,0,0,(const GLubyte*)buf);
      glDisable(GL_COLOR_LOGIC_OP);
      break;

   case GM_AND:      // And buf data with existing frame data
      glEnable(GL_COLOR_LOGIC_OP);
      glLogicOp(GL_AND);
      glBitmap(iwd,iht,0,0,0,0,(const GLubyte*)buf);
      glDisable(GL_COLOR_LOGIC_OP);
      break;

   case GM_CLR:      // If buf bit == 1 clear frame bit
      glEnable(GL_COLOR_LOGIC_OP);
      glLogicOp(GL_AND_INVERTED);
      glBitmap(iwd,iht,0,0,0,0,(const GLubyte*)buf);
      glDisable(GL_COLOR_LOGIC_OP);
      break;

   default:
      writetolog("Error, bitmap mode not recognized");
      exit(1);
      } /* End mode switch */
   glPopMatrix();
   } /* End drawBitmap() */

// This code should be used for any map that needs to be scalable
// (bitmaps() call in plot library) or for any map that is not
// binary, whether it needs to be scaled or not (bitmap() with
// type other than BM_BW).  The latter case is signalled by
// setting bwd = bht = 0.
void FrameView::drawBitmapgrayscalescalable(int type, int mode,
   float xc, float yc, float bwd, float bht, int rowlen, int colht,
   int xofs, int yofs, int iwd, int iht, int length, char* buf) {

#if DEBUG & DBGDRAW
   writetolog("in FrameView::drawBitmapgrayscalescalable()");
#endif

   glPushMatrix();

   if (bwd != 0 && bht != 0) {
      // SCALEABLE IMAGES, ANY COLOR MODE

      // Alternative order of calculation to improve precision
      float sizewide   = (bwd*(float)(this->w())*sizew[atframe])/
         ((right-left)*((float)rowlen));
      float sizeheight = (bht*(float)(this->h())*sizeh[atframe])/
         ((top-bottom)*((float)colht));

      glPixelZoom(sizewide, sizeheight);

      float xinchesperpixel = bwd/(float)rowlen;
      float yinchesperpixel = bht/(float)colht;

      // Note no correction for zooming when computing offset.
      // glPixelZoom has already been called.
      glRasterPos2f(
         xc + xofs*xinchesperpixel, yc + yofs*yinchesperpixel);
      }
   else {
      //  ______ SCALEABLE BINARY MAPS _______

      float xpixtoinches =
         (right - left)/((float)origwinwidth[atframe]);
      float ypixtoinches =
         (top - bottom)/((float)origwinheight[atframe]);
      glRasterPos2f(xc +  xofs*xpixtoinches/size[atframe],
         yc + yofs*ypixtoinches/size[atframe]);
      }


   /* Rev, 04/30/10, GNR - Fix bug when rowlen not a mult of 4 */
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glShadeModel(GL_FLAT);

   /* See opengl redbook, version 1.2, third edition, page 296 & 301.
   *  Note, 08/26/16, GNR -- Apparently other color modes were never
   *  programmed here...  Colored images do not need pixel maps, and
   *  the RGB values are coded directly in the data.  Bitmap case
   *  added here.
   *  Rev, 05/18/17, GNR - Switch and plot the seven image types now
   *  defined in plotdefs.h (even though BM_COLOR is not supported
   *  in the oldplot library).  */

   switch (type) {
   case BM_BW: {           /* Black and white (binary) bitmap */
/* Rev, 08/29/16, GNR - Calls to glColor3f here to set color according
*  to bkgrndwhite flag were removed so bitmaps can be plotted in the
*  color set by most recent penset() call.  Later it was understood
*  that for glDrawPixels() with format GL_COLOR_ INDEX, we must make
*  Pixel Maps for index-to-red, index-to-green, and index-to-blue.
*  These maps have two entries, corresponding to the 0 and 1 bits of
*  a bitmap.  The 0 entry is the old bkgrndwhite test result, the 1
*  entry is the current drawing color.  I decided not to try to do
*  these manipulations back when the drawing color is changed, as that
*  is done far more often than a call to bitmaps().  */
      float ccolor[4];     /* Current color values */
      float ccred[2],ccgrn[2],ccblu[2];
      glGetFloatv(GL_CURRENT_COLOR, ccolor);
      ccred[0] = ccgrn[0] = ccblu[0] = bkgrndwhite[atframe] ? 1.0 : 0;
      ccred[1] = ccolor[0];
      ccgrn[1] = ccolor[1];
      ccblu[1] = ccolor[2];
      glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 2, ccred);
      glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 2, ccgrn);
      glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 2, ccblu);
      glDrawPixels(iwd, iht, GL_COLOR_INDEX, GL_BITMAP,
         (const GLubyte*)buf);
      break;
      } /* End BM_BW local scope */

   case BM_GS:             /* 8-bit grayscale */
      glDrawPixels(iwd, iht, GL_LUMINANCE, GL_UNSIGNED_BYTE,
         (const GLubyte*)buf);
      break;

   case BM_GS16:           /* 16-bit grayscale */
      glDrawPixels(iwd, iht, GL_LUMINANCE, GL_UNSIGNED_SHORT,
         (const GLubyte*)buf);
      break;

   case BM_C48:            /* 48-bit color */
      glDrawPixels(iwd, iht, GL_RGB, GL_UNSIGNED_SHORT,
         (const GLubyte*)buf);
      break;

   case BM_COLOR:          /* 8-bit color index */
      glDrawPixels(iwd, iht, GL_COLOR_INDEX, GL_UNSIGNED_BYTE,
         (const GLubyte*)buf);
      break;

   case BM_C8:             /* 8 bit color */
      glDrawPixels(iwd, iht, GL_RGB, GL_UNSIGNED_BYTE_2_3_3_REV,
         (const GLubyte*)buf);
      break;

   case BM_C16:            /* 16-bit color */
      glDrawPixels(iwd, iht, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV,
         (const GLubyte*)buf);
      break;

   case BM_C24:            /* 24-bit color */
      glDrawPixels(iwd, iht, GL_RGB, GL_UNSIGNED_BYTE,
         (const GLubyte*)buf);
      break;

      } /* End type switch */

   glFlush();
   glPixelZoom(1.0,1.0);
   glPopMatrix();
   } /* End drawBitmapgrayscalescalable() */
