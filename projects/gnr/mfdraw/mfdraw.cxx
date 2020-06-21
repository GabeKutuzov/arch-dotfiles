// (c) Copyright 2008-2015, The Rockefeller University *21114*
/***********************************************************************
*                             mfdraw.cxx                               *
*  Version 1.111               6/8/2005             Steven Marx        *
*                                                                      *
*  Definitions for the FrameView class its constructor and methods. In *
*  addition this file contains functions for tracking start of frame   *
*  data, deleting all data and creating data in file mode.             *
*                                                                      *
*  Note:  arc was not implemented by smarx.  Look into                 *
*  Fl_Graphics_Driver::arc(x, y, r, start, end)                        *
***********************************************************************/

// Rev, 11/28/08, GNR - Added #if DEBUG tests, reformatted comments,
//                      deleted a whole bunch of old versions in #if 0
// Rev, 01/14/09, GNR - Handle BATCH mode
// Rev, 03/10/12, GNR - Reformat to GNR style
// Rev, 03/10/12, GNR - Make fast forward/reverse timing work...maybe
// Rev, 10/22/15, GNR - Implement letter height from spec

#include <math.h>
#include <stdio.h>

#include "sysdef.h"
#include "mfdraw.h"
#include "mfdrawsockserver.h"
#include "mfdrawUI.h"
#include "plotdefs.h"

extern char msglog[];

// Precompute  cos(x) and sin(x) for circle or elliptical boundaries
Circledata circledata;

/*=====================================================================*
*                        FrameView::FrameView                          *
*=====================================================================*/

FrameView::FrameView(int x,int y,int w,int h,const char *l) : \
   Fl_Gl_Window(x,y,w,h,l), bkgrndwhite(new bool[MAXNUMFRAMES]), \
   size(new double[MAXNUMFRAMES]), oldsize(new double[MAXNUMFRAMES]), \
   sizew(new float[MAXNUMFRAMES]), sizeh(new float[MAXNUMFRAMES]), \
   pt0x(new double[MAXNUMFRAMES]), pt0y(new double[MAXNUMFRAMES]), \
   origwinwidth(new int[MAXNUMFRAMES]), \
   origwinheight(new int[MAXNUMFRAMES]), \
   lastwinwidth(new int[MAXNUMFRAMES]), \
   lastwinheight(new int[MAXNUMFRAMES]), \
   origmainwinwidth(new int[MAXNUMFRAMES]), \
   origmainwinheight(new int[MAXNUMFRAMES]), \
   firsttime(new bool[MAXNUMFRAMES]), \
   zoomfactor(new double[MAXNUMFRAMES]), \
   oldzoomfactor(new double[MAXNUMFRAMES]), \
   xshift(new float[MAXNUMFRAMES]), \
   yshift(new float[MAXNUMFRAMES]), \
   previouspagewidth(new double[MAXNUMFRAMES]), \
   previouspageheight(new double[MAXNUMFRAMES]) {

   double adjust;

   if (HSCREENPIX/1024.0 <= WSCREENPIX/1280.0)
      adjust = ((double)HSCREENPIX)/1024.0;
   else
      adjust = ((double)WSCREENPIX)/1280.0;

#if DEBUG & DBGFRVIEW
   sprintf(msglog, "Entered FrameView, adjust = %g and "
      "MAXNUMFRAMES = %d", adjust, MAXNUMFRAMES);
   writetolog(msglog);
#endif

   for (int i=0; i < MAXNUMFRAMES; i++) {

      bkgrndwhite[i] = false;
      firsttime[i] =   true;
      origwinwidth[i]  = int(MAINWINDOWWIDTHSIZE*adjust);
      origwinheight[i] = int(MAINWINDOWWIDTHSIZE*adjust);
      lastwinwidth[i]  = origwinwidth[i];
      lastwinheight[i] = origwinheight[i];
      origmainwinwidth[i]  =  origwinwidth[i];
      // Numerical value 25 in next line should match height of
      //    control group in mfdrawUI.fl (smx had 22 here with
      //    comment it should be 25)
      origmainwinheight[i] =  origwinheight[i] + int(25*adjust);

      pt0x[i] = 0.0;
      pt0y[i] = 0.0;
      zoomfactor[atframe] = oldzoomfactor[atframe] = 1.0;
      size[i]  = oldsize[i] = 1.0;
      xshift[i] = 0.0;
      yshift[i] = 0.0;
      }

#if DEBUG & DBGFRVIEW
   writetolog("Finished initialization of frame parameters");
#endif

   if (runmode == SOCKMODE) {

      // Read the science model's drawing commands from stdin
      // (in tcp/ip socket mode) which is provided via xinetd
      cnsckinput();

      LISTOFCOMMANDS::iterator frit;
      // Frame pointed to by the iterators in frametracker[n]
      frit = frametracker[atframe];
      left = ((Newframe*)(*frit))->left;
      right = ((Newframe*)(*frit))->right;
      bottom = ((Newframe*)(*frit))->bottom;
      top = ((Newframe*)(*frit))->top;
      }

   } // End of FrameView::FrameView

/*=====================================================================*
*                           FrameView::home                            *
*=====================================================================*/

void FrameView::home(void) {

   bool skipsize;
   int deltax = abs(lastwinwidth[atframe] - origwinwidth[atframe]);
   int deltay = abs(lastwinheight[atframe] - origwinheight[atframe]);

#if DEBUG & DBGFRVIEW
   sprintf(msglog, "In FrameView::home, deltax = %d, deltay = %d",
      deltax, deltay);
   writetolog(msglog);
#endif
   /* Choose to redraw but not resize if resize is very small--
   *  tolerance 9 is arbitrary.  */
   skipsize = (deltax < 9 && deltay < 9);

   pt0x[atframe] = 0.5*(right - left);
   pt0y[atframe] = 0.5*(top - bottom);

   size[atframe] = oldsize[atframe] = zoomfactor[atframe] =
      oldzoomfactor[atframe];

   xshift[atframe] = 0.0;
   yshift[atframe] = 0.0;

   ptrzoom->value(1.0);                  // Initial zoom -- none
   ptrzoom->value(size[atframe]);

   pfvui->zoomadjuster->value(size[atframe]);
   pfvui->zoomadjuster->do_callback();

   xshift[atframe] = 0.0;
   yshift[atframe] = 0.0;

   setorigin = false;
   ptrorigin->value(0);

   ptrnetdialog->hide();
   fl_up_count    = 0;
   fl_down_count  = 0;
   current_line_thickness = 0;
   cursor(FL_CURSOR_DEFAULT);

   if (skipsize == false) {
      int mainlesswinht =
         parent()->parent()->parent()->h() - this->h();
      // Let the desktop manager position the main window
      parent()->parent()->parent()->size(lastwinwidth[atframe],
         lastwinheight[atframe] + mainlesswinht);
      }

   redraw();
   } /* End FrameView::home */

/*=====================================================================*
*                           FrameView::init                            *
*                                                                      *
*  init() sets frame viewing parameters, leaving the user in the frame *
*  currently being viewed.  If NOT at the first frame at the start of  *
*  the program then propagate the visual settings to the atframe frame *
*  from the previous frame if the previous frame has the same page     *
*  dimensions as the atframe frame.                                    *
*=====================================================================*/

void FrameView::init(void) {

   GLdouble twid = right - left;
   GLdouble thgt = top - bottom;

#if DEBUG & DBGFRVIEW
   sprintf(msglog, "In FrameView::init, atframe = %d, pagewidth = %g, "
      "pageheight = %g and previousframe = %d", atframe, twid, thgt,
      previousframe);
   writetolog(msglog);
#endif

   if (!firsttime[1]) {

      // Get (from stored ?) left/right and top/bottom for previous and
      // at frames and determine if we should propagate or continue
      // with the rest of init
      if ((previouspagewidth[previousframe] == twid) &&
         (previouspageheight[previousframe] == thgt)) {
         // Copy the previous page's view settings into current page
#if DEBUG & DBGFRVIEW
         writetolog("Not first time and inheritance is required");
#endif
         xshift[atframe]           = xshift[previousframe];
         yshift[atframe]           = yshift[previousframe];
         pt0x[atframe]             = pt0x[previousframe];
         pt0y[atframe]             = pt0y[previousframe];

         origwinwidth[atframe]     = origwinwidth[previousframe];
         origwinheight[atframe]    = origwinheight[previousframe];
         origmainwinwidth[atframe] = origmainwinwidth[previousframe];

         lastwinwidth[atframe]     = lastwinwidth[previousframe];
         lastwinheight[atframe]    = lastwinheight[previousframe];
         size[atframe]             = size[previousframe];
         oldsize[atframe]          = oldsize[previousframe];
         zoomfactor[atframe]       = zoomfactor[previousframe];
         oldzoomfactor[atframe]    = oldzoomfactor[previousframe];

         sizew[atframe] = sizew[previousframe];
         sizeh[atframe] = sizeh[previousframe];

#if DEBUG & DBGFRVIEW
         sprintf(msglog, "In init() at line %d inheritance atframe %d "
            "with sizew = %g and sizeh = %g\n",  __LINE__, atframe,
            sizew[atframe], sizeh[atframe]);
         writetolog(msglog);
         ptrzoom->value(size[atframe]); // Needed for exponential slider
         sprintf(msglog,"mfdraw atframe = %d, size = %f, with sizestep "
            "= %g, lastwinwidth = %d\n", atframe, size[atframe],
            sizestep, lastwinwidth[atframe]);
         writetolog(msglog);
#endif
         // We need to add the controls group's height to opengl window
         // height for main window height
         int mainlesswinht =
            parent()->parent()->parent()->h() - this->h();
         parent()->parent()->parent()->size(lastwinwidth[atframe],
            lastwinheight[atframe] + mainlesswinht);

         previouspagewidth[atframe]  = twid;
         previouspageheight[atframe] = thgt;
         firsttime[atframe] = false;
         return;
         } // End of page size same as previous page
      } // End of not firsttime

   previouspagewidth[atframe]  = twid;
   previouspageheight[atframe] = thgt;

   if (firsttime[atframe] == true) {

      if (twid > thgt)      origwinwidth[atframe] =
         int(origwinwidth[atframe] * twid/thgt);
      else if (twid < thgt) origwinheight[atframe] =
         int(origwinheight[atframe] * thgt/twid);

#if DEBUG & DBGFRVIEW
      sprintf(msglog,"first time atframe %d origwinheight = %d",
         atframe, origwinheight[atframe]);
      writetolog(msglog);
#endif
      origmainwinwidth[atframe] =  origwinwidth[atframe];

      int mainlesswinht =
         parent()->parent()->parent()->h() - this->h();
      origmainwinheight[atframe] =
         origwinheight[atframe] + mainlesswinht;

      lastwinwidth[atframe]  = origwinwidth[atframe];
      lastwinheight[atframe] = origwinheight[atframe];
      size[atframe] = oldsize[atframe] = 1.0;
      zoomfactor[atframe] = oldzoomfactor[atframe] = 1.0;

      // When initializing the zoom widget should reflect the initial
      //  size which is 1.0
      ptrzoom->value(1.0);
      // Support for adjuster widget
      pfvui->zoomadjuster->value(1.0);
      pfvui->zoomdisplay->label("1.0");

      xshift[atframe]=0.0;
      yshift[atframe]=0.0;
      pt0x[atframe] = 0.5*twid;     // Reset to center of the window
      pt0y[atframe] = 0.5*thgt;     // Reset to center of the window

#if DEBUG & DBGFRVIEW
      sprintf(msglog,"Exiting first time atframe %d size = %g",
         atframe, size[atframe]);
      writetolog(msglog);
#endif

      } // End of first time

   if (atframe <= 1 && firsttime[atframe]) {
      // printf("kick the video card at frame %d\n", atframe);
      Fl::damage(1);
      Fl::flush();
      }

   bkgrndwhite[atframe] = false;
   setorigin = false;

   // Linear slider
   ptrzoom->value(size[atframe]);
   sizestep = 0.02;
   sizemin =  0.5;
   sizemax =  100;

   // New exponential zoom parameters:
   // sizestep = 0.02; sizemin =  0.5; sizemax =  5.0;

   sizew[atframe] = size[atframe]*
      (float(origwinwidth[atframe])/this->w());
   sizeh[atframe] = size[atframe]*
      (float(origwinheight[atframe])/this->h());
#if DEBUG & DBGFRVIEW
   sprintf(msglog, "in init()at line %d atframe = %d, sizew = %g, "
      " sizeh = %g", __LINE__, atframe, sizew[atframe], sizeh[atframe]);
   writetolog(msglog);                                                                                                               //++++++++++++++++++++++++++=11/07/07
#endif

   ptrorigin->value(0);
   ptrnetdialog->hide();
   fl_up_count    =  0;
   fl_down_count  =  0;
   current_line_thickness = 0;
   cursor(FL_CURSOR_DEFAULT);

   int mainlesswinht =  parent()->parent()->parent()->h() - this->h();
   parent()->parent()->parent()->size(lastwinwidth[atframe],
      lastwinheight[atframe] + mainlesswinht);

   previouspagewidth[atframe]  = twid;
   previouspageheight[atframe] = thgt;

   firsttime[atframe] = false;

   } // End of init()

/***********************************************************************
*                                                                      *
*                   opengl drawing commands follow                     *
*                                                                      *
***********************************************************************/

void FrameView::drawCircle(double x1, double y1, double r) {
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
   } /* End drawCircle() */

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

void FrameView::drawLine(double x1, double y1, double x2, double y2) {
   glLineWidth(1.0 + current_line_thickness);
   glBegin(GL_LINES);
   glVertex2f(x1,y1);
   glVertex2f(x2,y2);
   glEnd();
   } /* End drawLine() */

//======= start tesselation work area ========
/*  the callback routines registered by gluTessCallback() */
void vertexCallback(GLdouble *v) {
   glVertex2dv(v);
   }
void beginCallback(GLenum which) {
   glBegin(which);
   }
void endCallback(void) {
   glEnd();
   }
void errorCallback(GLenum errorCode) {
   const GLubyte *estring;
   estring = gluErrorString(errorCode);
   fprintf (stderr, "Tessellation Error: %s\n", estring);
   exit (0);
   }

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

/*=====================================================================*
*                        FrameView::linethick                          *
*=====================================================================*/

void FrameView::linethick(int t) {
   current_line_thickness = t;
   } /* End linethick() */

/*=====================================================================*
*                          FrameView::resize                           *
*=====================================================================*/

void FrameView::resize(int xp, int yp, int wd, int ht) {
   // Linear version of resize
   // Drawing is resized, if possible, to fill new window size

   double zoomfactor_w = (double(wd)/origwinwidth[atframe]);
   double zoomfactor_h = (double(ht)/origwinheight[atframe]);
   zoomfactor[atframe] = (zoomfactor_w <= zoomfactor_h) ?
      zoomfactor_w : zoomfactor_h;
   size[atframe] += (zoomfactor[atframe] - oldzoomfactor[atframe]);

#if DEBUG & DBGRESIZE
   sprintf(msglog,"resize at frame %d in line %d xp = %d, yp = %d, "
      "wd = %d, ht = %d", atframe, __LINE__, xp, yp, wd, ht);
   writetolog(msglog);
   sprintf(msglog,"   oldzoomfactor = %g, zoomfactor = %g, "
      "size[atframe] = %g, oldsize[atframe] = %g",
      oldzoomfactor[atframe], zoomfactor[atframe],
      size[atframe], oldsize[atframe]);
   writetolog(msglog);
#endif

   if (size[atframe] != oldsize[atframe]) {
      ptrzoom->value(size[atframe]);
#if DEBUG & DBGRESIZE
      sprintf(msglog,"calling zoomfunc");
      writetolog(msglog);
#endif
      zoomfunc();

      // Support for adjuster widget
      pfvui->zoomadjuster->value(size[atframe]);
      pfvui->zoomadjuster->do_callback();
      }

   double sw = size[atframe]*double(origwinwidth[atframe])/double(wd);
   double sh = size[atframe]*double(origwinheight[atframe])/double(ht);

   xshift[atframe] += sw*(pt0x[atframe])*((double(wd -
      lastwinwidth[atframe]))/(double)lastwinwidth[atframe]);
   yshift[atframe] += sh*(pt0y[atframe])*((double(ht -
      lastwinheight[atframe]))/(double)lastwinheight[atframe]);

#if DEBUG & DBGRESIZE
   writetolog("calling Fl_Gl_Window::resize w/above xp, yp, wd, ht");
#endif
   Fl_Gl_Window::resize(xp, yp, wd, ht);

   oldzoomfactor[atframe] = zoomfactor[atframe];
   lastwinwidth[atframe]  = w();
   lastwinheight[atframe] = h();
   } /* End resize() */

/*=====================================================================*
*                           FrameView::draw                            *
*=====================================================================*/

void FrameView::draw() {

   if (!valid()) {
      glLoadIdentity();

      // Resolves bug - no more image clipping on zoom at origin
      glViewport(-w(),-h(),2*w(),2*h());
      glOrtho(left,right,bottom,top,-1,1);
      glScalef(.5,.5, 1.0);
      glTranslatef((right-left), (top-bottom), 0.0);
      }

   if (bkgrndwhite[atframe])
      glClearColor(1.0,1.0,1.0,1.0); // Make bkgrnd white
  else
      glClearColor(0.0,0.0,0.0,1.0); // Make bkgrnd black

   // Initializes the screen
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   // If (sListofCommands.size() == 0) {
   if (sListofCommands.empty()) return;

   glPushMatrix();

   glTranslatef(xshift[atframe], yshift[atframe], 0);

   //--------- maintain original gl window aspect ratio ---------
#if DEBUG & DBGDRAW
   sprintf(msglog, "in draw() line %d atframe = %d and sizeh = %g "
      "and origwinheight = %d", __LINE__, atframe, sizeh[atframe],
      origwinheight[atframe]);
   writetolog(msglog);
#endif

   sizew[atframe] = size[atframe]*(float(origwinwidth[atframe])/w());
   sizeh[atframe] = size[atframe]*(float(origwinheight[atframe])/h());

   // We have collapsed the z axis!
   glScalef(sizew[atframe], sizeh[atframe], 0);

   // It appears that we should use -DMESA when under os x (MAC) even
   // though it appears that we are not using MESA opengl
#ifndef MESA
   glDrawBuffer(GL_FRONT_AND_BACK);
   // cout<<"don't have mesa front and back"<<endl;
#endif // !MESA

   drawFrame();
   // It appears that we should use -DMESA when under os x (MAC) even
   // though it appears that we are not using MESA opengl
#ifndef MESA
   glDrawBuffer(GL_BACK);
   // cout<<"don't have mesa back"<<endl;
#endif // !MESA

   glFlush();
   glPopMatrix();

   } /* End draw() */

/*=====================================================================*
*                           drawFrameCommon                            *
*                                                                      *
*  This routine added by GNR, 02/25/12.  All it does is capture some   *
*  code that was repeated numerous times in what follows in the smx    *
*  code -- maybe this will help clarify the design.  The following     *
*  comment appeared at one instance of this code:    Initialize        *
*  orthographic projection from the framenumber frame data record      *
*=====================================================================*/

void drawFrameCommon(FrameView* glView, int incr, int dozoom) {

   char lbl[MXLTTL];

   atframe += incr;
   LISTOFCOMMANDS::iterator frit;
   // frame is pointed to by the iterator in frametracker[n]
   frit = frametracker[atframe];
   glView->left   = ((Newframe*)(*frit))->left;
   glView->right  = ((Newframe*)(*frit))->right;
   glView->bottom = ((Newframe*)(*frit))->bottom;
   glView->top    = ((Newframe*)(*frit))->top;
   glView->invalidate();
   // Reset zoom to reflect size of frame 1
   if (dozoom) pfvui->zoomdisplay->do_callback();
   glView->init();
   glView->redraw();
   glView->show();
   Fl::check();
   // Display title at main window
   sprintf(lbl, "%s", header2.c_str());
   pfvui->mainWindow->label(lbl);
   pfvui->mainWindow->redraw();
   Fl::check();

   } /* End drawFrameCommon() */

/*=====================================================================*
*                        FrameView::drawFrame                          *
*=====================================================================*/

void FrameView::drawFrame(void) {

   static LISTOFCOMMANDS::iterator listit;
   if (listit == sListofCommands.begin())
      listit = sListofCommands.end();
   else
      listit = frametracker[atframe];
   while(listit-- != sListofCommands.begin()) {

#if DEBUG & DBGDRAW
      sprintf(msglog, "Debug: inframe is %d drawing frame %d and "
         "current drawing command is %d\n", inframe, atframe,
         (*listit)->cmd);
      writetolog(msglog);
#endif

      // We want the NEWFRAME record if we just advanced the frame
      if (advanceframe)
         listit++;
      switch ((*listit)->cmd) {

      case ENDOFFILE : {
         // printf("Debug: Hit an end of file in frame %d\n", atframe);
         // Reprocess this last frame next time we draw, advance frame
         // only through gui.
         listit = frametracker[atframe];
         inframe = true;
         return;
         }
      case NEWFRAME : {
         if (advanceframe) {
#if DEBUG & DBGDRAW
            writetolog("drawFrame advanced the frame via the gui.");
#endif
            LISTOFCOMMANDS::iterator frit;
            // Iterator frametracker[1] points to the first frame
            frit = frametracker[atframe];
            left = ((Newframe*)(*frit))->left;
            right = ((Newframe*)(*frit))->right;
            bottom = ((Newframe*)(*frit))->bottom;
            top = ((Newframe*)(*frit))->top;
            advanceframe = false;
            inframe = true;
            // Continue processing draw commands for this frame
            break;
            }
         if (!inframe) {
            inframe = true;
            // Continue processing draw commands for this frame
            break;
            }
         if (inframe) {
            // printf("Got newframe command while in frame %d\n",
            //    atframe);
            // Reprocess current frame next time we draw and
            // do not advance
            listit = frametracker[atframe];
            return;
            }
         }
      case DRWLINE : {
         drawLine(((Line*)(*listit))->x1, ((Line*)(*listit))->y1,
            ((Line*)(*listit))->x2, ((Line*)(*listit))->y2);
         break;
         }
      case DRWCIRCLE : {
         drawCircle(((Circle*)(*listit))->x,((Circle*)(*listit))->y,
            ((Circle*)(*listit))->r);
         break;
         }
      case DRWFILLEDCIRCLE : {
         drawFilledcircle(((Filledcircle*)(*listit))->x,
            ((Filledcircle*)(*listit))->y,
            ((Filledcircle*)(*listit))->r);
         break;
         }
      case DRWOUTLINEELLIPSE : {
         drawOutlineellipse(((Outlineellipse*)(*listit))->x,
            ((Outlineellipse*)(*listit))->y,
            ((Outlineellipse*)(*listit))->w,
            ((Outlineellipse*)(*listit))->h,
            ((Outlineellipse*)(*listit))->angle);
         break;
         }
      case DRWFILLEDELLIPSE : {
         drawFilledellipse(((Filledellipse*)(*listit))->x,
            ((Filledellipse*)(*listit))->y,
            ((Filledellipse*)(*listit))->w,
            ((Filledellipse*)(*listit))->h,
            ((Filledellipse*)(*listit))->angle);
         break;
         }
      case DRWFILLCLOSEPOLYLINE : {
         drawFillclosepolyline(((Fillclosepolyline*)(*listit))->np,
            ((Fillclosepolyline*)(*listit))->c);
         break;
         }
      case DRWOUTLINECLOSEPOLYLINE : {
         drawOutlineclosepolyline(
            ((Outlineclosepolyline*)(*listit))->np,
            ((Outlineclosepolyline*)(*listit))->c);
         break;
         }
      case DRWOPENPOLYLINE : {
         drawOpenpolyline(((Openpolyline*)(*listit))->np,
            ((Openpolyline*)(*listit))->c);
         break;
         }
      case DRWOUTLINESQUARE : {
         drawOutlinesquare(((Outlinesquare*)(*listit))->x,
            ((Outlinesquare*)(*listit))->y,
            ((Outlinesquare*)(*listit))->e);
         break;
         }
      case DRWFILLEDSQUARE : {
         drawFilledsquare(((Filledsquare*)(*listit))->x,
            ((Filledsquare*)(*listit))->y,
            ((Filledsquare*)(*listit))->e);
         break;
         }
      case DRWOUTLINERECTANGLE : {
         drawOutlinerectangle(((Outlinerectangle*)(*listit))->x,
            ((Outlinerectangle*)(*listit))->y,
            ((Outlinerectangle*)(*listit))->e1,
            ((Outlinerectangle*)(*listit))->e2);
         break;
         }
      case DRWFILLEDRECTANGLE : {
         drawFilledrectangle(((Filledrectangle*)(*listit))->x,
            ((Filledrectangle*)(*listit))->y,
            ((Filledrectangle*)(*listit))->e1,
            ((Filledrectangle*)(*listit))->e2);
         break;
         }
      case DRWBITMAP : {
         drawBitmap(((Bitmap*)(*listit))->type,
            ((Bitmap*)(*listit))->mode,
            ((Bitmap*)(*listit))->xc, ((Bitmap*)(*listit))->yc,
            ((Bitmap*)(*listit))->rolen, ((Bitmap*)(*listit))->colht,
            ((Bitmap*)(*listit))->xofs,  ((Bitmap*)(*listit))->yofs,
            ((Bitmap*)(*listit))->iwd,   ((Bitmap*)(*listit))->iht,
            ((Bitmap*)(*listit))->length,((Bitmap*)(*listit))->bitdata);
         break;
         }
      case DRWBITMAPGRAYSCALESCALABLE : {
         drawBitmapgrayscalescalable(
            ((Bitmapgrayscalescalable*)(*listit))->type,
            ((Bitmapgrayscalescalable*)(*listit))->mode,
            ((Bitmapgrayscalescalable*)(*listit))->xc,
            ((Bitmapgrayscalescalable*)(*listit))->yc,
            ((Bitmapgrayscalescalable*)(*listit))->bwd,
            ((Bitmapgrayscalescalable*)(*listit))->bht,
            ((Bitmapgrayscalescalable*)(*listit))->rolen,
            ((Bitmapgrayscalescalable*)(*listit))->colht,
            ((Bitmapgrayscalescalable*)(*listit))->xofs,
            ((Bitmapgrayscalescalable*)(*listit))->yofs,
            ((Bitmapgrayscalescalable*)(*listit))->iwd,
            ((Bitmapgrayscalescalable*)(*listit))->iht,
            ((Bitmapgrayscalescalable*)(*listit))->length,
            ((Bitmapgrayscalescalable*)(*listit))->bitdata);
         break;
         }
      case COLORNAME : {
         colorname(((Colorname*)(*listit))->colorname);
         break;
         }
      case COLORBGR : {
         colorbgr(((Colorbgr*)(*listit))->blue,
            ((Colorbgr*)(*listit))->green,
            ((Colorbgr*)(*listit))->red);
         break;
         }
      case COLORBBGGRR : {
         colorbbggrr(((Colorbbggrr*)(*listit))->blue,
         ((Colorbbggrr*)(*listit))->green,
         ((Colorbbggrr*)(*listit))->red);
         break;
         }
      case DRWTEXT : {
         drawText(((Text*)(*listit))->x, ((Text*)(*listit))->y,
            ((Text*)(*listit))->ht, ((Text*)(*listit))->angle,
            ((Text*)(*listit))->length, ((Text*)(*listit))->text);
         break;
         }
      case LINETHICK : {
         linethick(((Linethick*)(*listit))->thickness);
         break;
         }
      default :
         writetolog("Unknown cmd in drawframe, terminating.");
         exit(EXIT_FAILURE);
         } /* End cmd switch */

      } /* End while listit */
   } /* End drawFrame() */

/***********************************************************************
*                            Event Handler                             *
***********************************************************************/

int FrameView::handle(int event) {

   static int startx, endx, starty, endy;

#if DEBUG & DBGHANDLE
   char hmsglog[MSGLOGSIZE];
   writetolog("In FrameView::handle (GNRTEST2)");
   sprintf(hmsglog, " at frame %d got event %d = Fl::event "
      "%d and Fl::event_key %d ", atframe, event, Fl::event(),
      Fl::event_key());
   writetolog(hmsglog);
#endif

   switch (event) {

   case FL_KEYBOARD:
      if ((Fl::event_key(FL_Control_L) || Fl::event_key(FL_Control_R)) &&
         (Fl::event_key('q') || Fl::event_key('Q'))) {
#if DEBUG & DBGHANDLE
         writetolog("Exit on event key Q");
#endif
         exit(0);
         }
      if ((Fl::event_key(FL_Alt_L) || Fl::event_key(FL_Alt_R)) &&
         Fl::event_key(4+FL_F)) {
#if DEBUG & DBGHANDLE
         writetolog("Exit on event key FL_Alt_4");
#endif
         exit(0);
         }
      if (!(Fl::event_key(FL_Left) || Fl::event_key(FL_Right)))
         ptrzoom->activate();
      else
         ptrzoom->deactivate();

      // Keyboard up and down arrow control for zoom
      if (Fl::event_key(FL_Up) && fl_up_count > 0) {
#if DEBUG & DBGHANDLE
         writetolog("Event key is FL_Up, fl_up_count > 0, "
            "will return 1");
#endif
         if (++fl_up_count == 2) {
            fl_up_count = 0;
            return 1;
            }
         }
      if (Fl::event_key(FL_Down) && fl_down_count > 0) {
#if DEBUG & DBGHANDLE
         writetolog("Event key is FL_Down, fl_down_count > 0, "
            "will return 1");
#endif
         if (++fl_down_count == 2) {
            fl_down_count = 0;
            return 1;
            }
         }

      // Linear slider with 'exponential step size'
      if (Fl::event_key(FL_Up) && fl_up_count == 0) {
#if DEBUG & DBGHANDLE
         writetolog("Event key is FL_Up, fl_up_count == 0, "
            "calls zoomfunc");
#endif
         fl_up_count++;
         if (size[atframe] <= (sizemax  - sizestep*size[atframe])) {
            // This updates widget appearance but does not invoke
            // callback
            ptrzoom->value(size[atframe] + sizestep*size[atframe]);
            size[atframe] += sizestep*size[atframe];
            zoomfunc();
            }
         return 1;
         }
      if (Fl::event_key(FL_Down) && fl_down_count == 0) {
#if DEBUG & DBGHANDLE
         writetolog("Event key is FL_Down, fl_down_counnt == 0, "
            "calls zoomfunc");
#endif
         fl_down_count++;
         if (size[atframe] >= (sizemin + sizestep*size[atframe])) {
            // This updates widget appearance but does not invoke
            // callback
            ptrzoom->value(size[atframe] - sizestep*size[atframe]);
            size[atframe] -= sizestep*size[atframe];
            zoomfunc();
            }
         return 1;
         }
      if ( Fl::event_key(FL_Down) || Fl::event_key(FL_Up)) {
#if DEBUG & DBGHANDLE
         writetolog("Event key is FL_Up or FL_Down, just return 1");
#endif
         // Do nothing it's either a release or an unneeded typematic
         return 1;
         }
      //======end keyboard up and down arrow control for zoom =======

   case FL_KEYUP:
      // Keyboard left and right arrow control for frame movement
      if (Fl::event_key() == FL_Right) {
#if DEBUG & DBGHANDLE
         writetolog("Event is FL_KEYUP, key is FL_Right");
#endif
         if (sListofCommands.empty()) return 1;
#if DEBUG & DBGHANDLE
         writetolog("Calling ptrnxtfrm->do_callback");
#endif
         ptrnxtfrm->do_callback();
         return 1;
         } // End FL_Right
      if (Fl::event_key() == FL_Left) {
#if DEBUG & DBGHANDLE
      writetolog("Event is FL_KEYUP, key is RL_Left");
#endif
         if (sListofCommands.empty())
         return 1;
#if DEBUG & DBGHANDLE
         writetolog("Calling ptrprevfrm->do_callback");
#endif
         ptrprevfrm->do_callback();
         return 1;
         } // End FL_Left
      // End keyboard left and right arrow control for frame movement

   case FL_SHORTCUT:
      // Response to 'S' key deleted here--no idea what it is

      if (Fl::event_key() == 'm' ||  Fl::event_key() == 'M') {
         if (runmode == FILEMODE) {
            // writetolog("The mode toggle pertains only to socket "
            //   "data input");
            return 1;
            }
#if DEBUG & DBGHANDLE
         writetolog("Event is FL_SHORTCUT, key is m or M, "
            "toggle movie mode");
#endif
         if (mmode >= MM_MOVIE) {
            mmode = MM_STILL;
            fastforward = false;
            return 1; }
         else if (mmode == MM_STILL) {
            // If there is no more connection to the science model then
            // there can be no movie
            if (moreframes == false) return 1;
            fastforward = true;
            // Note 01/14/09, GNR - If we are interacting, for now I
            //    deem it acceptable to revert to MM_MOVIE if the
            //    original setting was MM_BATCH.
            mmode = MM_MOVIE;
#if DEBUG & DBGHANDLE
            writetolog("Calling ptrnxtfrm->do_callback");
#endif
            ptrnxtfrm->do_callback();
            }
         return 1;
         } /* End dealing with m key */

      if (Fl::event_key() == 'n' || Fl::event_key() == 'N') {
#if DEBUG & DBGHANDLE
         writetolog("Event is FL_SHORTCUT, key is n or N");
#endif
         if (runmode == FILEMODE) {
#if DEBUG & DBGHANDLE
            writetolog("Calling ptrexitcontinue->show");
#endif
            ptrexitcontinue->show();
            return 1;
            }
#if DEBUG & DBGHANDLE
         writetolog("Calling ptrnetdialog->show");
#endif
         ptrnetdialog->show();
         // ptrcontrols->deactivate();
         // ptrzoom->take_focus();
         return 1;
         } /* End dealing with 'n' */

      if (Fl::event_key() == 'b' || Fl::event_key() == 'B') {
#if DEBUG & DBGHANDLE
         writetolog("Event is FL_SHORTCUT, key is b or B");
#endif
         setorigin = true;
         if (bkgrndwhite[atframe])
            bkgrndwhite[atframe] = false;
         else
            bkgrndwhite[atframe] = true;
         redraw();
         // ptrzoom->take_focus();
         return 1;
         } /* End dealing with 'b' */

      if (Fl::event_key() == 'o' || Fl::event_key() == 'O') {
#if DEBUG & DBGHANDLE
         writetolog("Event is FL_SHORTCUT, key is o or O, set origin");
#endif
         setorigin = true;
         ptrorigin->value(1);
         ptrorigin->do_callback();
         return 1;
         } /* End dealing with 'o' */

      if (Fl::event_key() == 'h' ||  Fl::event_key() == 'H') {
#if DEBUG & DBGHANDLE
         writetolog("Event is FL_SHORTCUT, key is h or H, call home");
#endif
         home();
         return 1;
         } /* End dealing with 'h' */

      if (Fl::event_key() == 'f' ||  Fl::event_key() == 'F') {
         if (runmode == SOCKMODE) {
#if DEBUG & DBGHANDLE
            writetolog("Event is FL_SHORTCUT, key is f or F");
#endif
            ptrexitcontinue->show(); // replacement for fl_choice
            return 1;
            }
         } /* End dealing with 'f' */

      if (Fl::event_key() == 'a' ||  Fl::event_key() == 'A') {
#if DEBUG & DBGHANDLE
         writetolog("Event is FL_SHORTCUT, key is a or A");
#endif
         fl_message("%s\n%s\n%s",header1.c_str(), header2.c_str(),
            header3.c_str());
         return 1;
         } /* End dealing with 'a' */

#if DEBUG & DBGHANDLE
      writetolog("Event is FL_SHORTCUT, unrecognized key, "
         "calling parent handler");
#endif
      return Fl_Gl_Window::handle(event);
      // End dealing with SHORTCUT keys

   case FL_DRAG:
#if DEBUG & DBGHANDLE
      writetolog("Event is FL_DRAG");
#endif
      endx = Fl::event_x();
      endy = Fl::event_y();
      // printf("you shift x by %d and shift y by %d\n",
      // endx - startx, endy - starty);
      xshift[atframe] += (right - left)*((double)(endx - startx))/w();
      yshift[atframe] -= (top - bottom)*((double)(endy - starty))/h();
      startx = endx;
      starty = endy;
      // cout<<"calling modelviewsetorigin from handle drag:  "
      // "with xpos = "<<(this->w())/2<<" and ypos = "<<
      // (this->h())/2<<endl;
      // When we translate display we move pt0x[atframe] and
      // pt0y[atframe] back to center of the screen
      // This will provide zooming which originates at screen center
      redraw();
      return 1;

   case FL_PUSH:
#if DEBUG & DBGHANDLE
      writetolog("Event is FL_PUSH");
#endif
      // printf("PUSH: x orig screen = %d y orig screen = %d w() = %d "
      //  "h() = %d mouse xpos = %d mouse ypos = %d\n",
      //  x(), y(), w(), h(), Fl::event_x(), Fl::event_y());
      startx  = Fl::event_x();
      starty  = Fl::event_y();
      return 1;

   case FL_RELEASE:
      //----get user desired location of the new origin ----
#if DEBUG & DBGHANDLE
      writetolog("Event is FL_RELEASE");
#endif
      int xpos; // addded as work around for ubuntu 11/7/2004
      int ypos; // addded as work around for ubuntu 11/7/2004

      if (Fl::event_button() == FL_RIGHT_MOUSE) {
#if DEBUG & DBGHANDLE
         writetolog("Event is FL_RELEASE, event_button is "
            "FL_RIGHT_MOUSE");
#endif
         if (runmode == SOCKMODE) {
            ptrnetdialog->position(Fl::event_x(), Fl::event_y());
            ptrnetdialog->show();
            }
         else {
            ptrexitcontinue->show(); // Replacement for fl_choice
            }
         return 1;
         }

      xpos =  Fl::event_x();  // addded as work around for ubuntu 11/7/2004
      ypos =  Fl::event_y();  // addded as work around for ubuntu 11/7/2004
#if DEBUG & DBGHANDLE
      sprintf(hmsglog, "called modelviewsetorigin on release with "
         "xpos = %d and ypos = %d", xpos, ypos);
      writetolog(hmsglog);
#endif
      modelviewsetorigin(xpos,ypos);
      return 1;

   default:
#if DEBUG & DBGHANDLE
      writetolog("In FrameView::handle and didn't find anything to do:");
      writetolog("  letting Fl_Gl_Window::handle process the event as "
         "default");
#endif
      return Fl_Gl_Window::handle(event);
      } /* End event switch */

   } /* End event handler */

/***********************************************************************
*                         modelviewsetorigin                           *
***********************************************************************/

void FrameView::modelviewsetorigin(int xpos, int ypos) {
   // This function is called by case FL_RELEASE or case FL_DRAG in
   // FrameView::handle()
   // printf("in modelviewsetorigin(): setorigin = %d and  mouse "
   //    "released at x = %d y = %d\n",setorigin, xpos, ypos);
   // printf("left = %g right = %g bottom = %g top = %g\n", left, "
   //    "right, bottom, top);

   pt0x[atframe] = -xshift[atframe] + ((double)xpos/w())*(right-left);
   pt0y[atframe] = -yshift[atframe] + (top - bottom) -
      ((double)ypos/h())*(top - bottom);

   pt0x[atframe] = pt0x[atframe]/sizew[atframe];
   pt0y[atframe] = pt0y[atframe]/sizeh[atframe];

   setorigin = false;
   ptrorigin->value(0);

   // printf("setorigin is %d new origin location in inches relative "
   //    "to the current origin: x = %g y = %g\n", setorigin,
   //    pt0x[atframe], pt0y[atframe]);
   // fflush(stdout);

   cursor(FL_CURSOR_DEFAULT);

   return;
   } /* End modelviewsetorigin() */

/*=====================================================================*
*                         FrameView::zoomfunc                          *
*=====================================================================*/

void FrameView::zoomfunc() {
   xshift[atframe] -= pt0x[atframe]*(size[atframe] - oldsize[atframe])*
      (float(origwinwidth[atframe])/this->w());
   yshift[atframe] -= pt0y[atframe]*(size[atframe] - oldsize[atframe])*
      (float(origwinheight[atframe])/this->h());
   oldsize[atframe] = size[atframe];

   // Support for adjuster widget
   // printf("in zoomfunc with size = %g\n",size[atframe]);
   pfvui->zoomadjuster->value(size[atframe]);
   pfvui->zoomdisplay->do_callback();

   redraw();
   } /* End zoomfunc() */

/*=====================================================================*
*                           createtestdata                             *
*=====================================================================*/

void createtestdata(void) {
   // createtestdata() is for debugging purposes only
   Command *ptrendoffile;
   Newframe *ptrnewframe;
   Line *ptrline;
   Circle *ptrcircle;
   Filledcircle *ptrfilledcircle;
   Colorname *ptrcolorname;
   Outlinesquare *ptroutlinesquare;

   // Record #1
   ptrnewframe = new Newframe(1,0,11,0,11,"CLIP");
   sListofCommands.push_front(ptrnewframe);
   // Record #2
   ptroutlinesquare = new Outlinesquare(3,3,1);
   sListofCommands.push_front(ptroutlinesquare);
   // Record #3
   ptrline = new Line(0,0,.5,.5);
   sListofCommands.push_front(ptrline);
   // Record #4
   ptrcircle = new Circle(0,0,.25);
   sListofCommands.push_front(ptrcircle);
   // Record #5
   ptrcircle = new Circle(0,0,1.0);
   sListofCommands.push_front(ptrcircle);
   // Record #6
   ptrnewframe = new Newframe(2,-10,10,-10 ,10,"CLIP");
   sListofCommands.push_front(ptrnewframe);
   // Record #7
   ptrcircle = new Circle(2,2,1);
   sListofCommands.push_front(ptrcircle);
   // Record #8
   ptrcolorname = new Colorname("RED", 3);
   sListofCommands.push_front(ptrcolorname);
   // Record #9
   ptrfilledcircle = new Filledcircle(4,4,1);
   sListofCommands.push_front(ptrfilledcircle);
   // Record #10
   ptrendoffile = new Command(ENDOFFILE);
   sListofCommands.push_front(ptrendoffile);
   } /* End createtestdata() */

/*=====================================================================*
*                          buildframetracker                           *
*=====================================================================*/

void buildframetracker(void) {
   // maxframe control version but assumes that meta file is read one
   // frame at a time as in socket mode
   // printf("using maxframe version of buildframetracker");
   // fflush(stdout);

   if (sListofCommands.empty())
      return;

   int deletedsofar = deleteframe();
   // Deletes frames so that only the last MAXNUMFRAMES -1 are present
   // in run mode 0 (meta file) amd run mode 1 (sockets)
   if (deletedsofar < 0) {
      writetolog("Error: terminate deletedsofar return value < 0");
      exit(1);
      }

   // Used by sListofCommands implemented as a list container -
   // note that the list's iterators are not invalidated by
   // addition of frames
   if (deletedsofar == 0) {
      // Can only be used when we do not have to delete frames
      // following if empty not needed it is performed above
      // if (sListofCommands.empty())
      // return;

      static LISTOFCOMMANDS::iterator itX = sListofCommands.end();

      while(itX-- != sListofCommands.begin()) {
         if ((*itX)->cmd == NEWFRAME) {
            // Frametracker values are iterator which points to the
            //    frame's NEWFRAME record
            frametracker[((Newframe*)(*itX))->frameindex] = itX;
            lastframe =  ((Newframe*)(*itX))->frameindex;
            // sprintf(msglog,"buildframetracker() newframes index is "
            //    "%d and lastframe = %d\n",
            //    ((Newframe*)(*itX))->frameindex, lastframe);
            // writetolog(msglog);
            break;  // Comment out for deque version 4/4/2008
            }
         // writetolog("drawing command but not frame command");
         }
      return;
      }

   LISTOFCOMMANDS::iterator it;
   for (int fr = 2; fr < MAXNUMFRAMES; fr++) {
      frametracker[fr-1] = frametracker[fr];
      }

   // Support for synchronizing delete with frame properties
   // Align frame properties with buildframetracker after frame delete
   pfvui->glView->leftshift_frame_props();

   it = sListofCommands.begin();
   while(1) {
      it++;
      if ((*it)->cmd == NEWFRAME) {
         frametracker[MAXNUMFRAMES -1] = it;
         lastframe = MAXNUMFRAMES - 1;
         // sprintf(msglog, "in file %s at line %d atframe = %d got "
         // "newframe with frame index = %d  and loaded an iterator "
         // "to it in frametracker[%d]\n", __FILE__, __LINE__, atframe,
         // ((Newframe*)(*it))->frameindex,  MAXNUMFRAMES -1);
         // printf(msglog);
         // writetolog(msglog);
         break;
         }
      }

   } // End of buildframetracker()

/*=====================================================================*
*                  FrameView::leftshift_frame_props                    *
*=====================================================================*/

void FrameView::leftshift_frame_props() {
   // Align frame properties with buildframetracker after frame deletes

   // char debugmsg[300];
   // sprintf(debugmsg,"in leftshift_frame_props: mode = %d fastforward "
   // "= %d atframe = %d lastframe = %d moreframes = %d",
   //  mmode, fastforward, atframe, lastframe, moreframes);
   // writetolog(debugmsg);

   for (int fr = 2; fr < MAXNUMFRAMES; fr++) {
      previouspagewidth[fr-1]  =  previouspagewidth[fr];
      previouspageheight[fr-1] =  previouspageheight[fr];
      xshift[fr-1]             = xshift[fr];
      yshift[fr-1]             = yshift[fr];
      pt0x[fr-1]               = pt0x[fr];
      pt0y[fr-1]               = pt0y[fr];
      origwinwidth[fr-1]       = origwinwidth[fr];
      origwinheight[fr-1]      = origwinheight[fr];
      origmainwinwidth[fr-1]   = origmainwinwidth[fr];
      origmainwinheight[fr-1]  = origmainwinheight[fr];
      lastwinwidth[fr-1]       = lastwinwidth[fr];
      lastwinheight[fr-1]      = lastwinheight[fr];
      size[fr-1]               = size[fr];
      oldsize[fr-1]            = oldsize[fr];
      zoomfactor[fr-1]         = zoomfactor[fr];
      oldzoomfactor[fr-1]      = oldzoomfactor[fr];
      sizew[fr-1]              = sizew[fr];
      sizeh[fr-1]              = sizeh[fr];
      }

   // Set lastframe, that is #  MAXNUMFRAMES -1, to initial values
   firsttime[MAXNUMFRAMES - 1]        = true;
   previouspagewidth[MAXNUMFRAMES-1]  = previouspagewidth[0];
   previouspageheight[MAXNUMFRAMES-1] = previouspageheight[0];
   xshift[MAXNUMFRAMES-1]             = xshift[0];
   yshift[MAXNUMFRAMES-1]             = yshift[0];
   pt0x[MAXNUMFRAMES-1]               = pt0x[0];
   pt0y[MAXNUMFRAMES-1]               = pt0y[0];
   origwinwidth[MAXNUMFRAMES-1]       = origwinwidth[0];
   origwinheight[MAXNUMFRAMES-1]      = origwinheight[0];
   origmainwinwidth[MAXNUMFRAMES-1]   = origmainwinwidth[0];
   origmainwinheight[MAXNUMFRAMES-1]  = origmainwinheight[0];
   lastwinwidth[MAXNUMFRAMES-1]       = lastwinwidth[0];
   lastwinheight[MAXNUMFRAMES-1]      = lastwinheight[0];
   size[MAXNUMFRAMES-1]               = size[0];
   oldsize[MAXNUMFRAMES-1]            = oldsize[0];
   zoomfactor[MAXNUMFRAMES-1]         = zoomfactor[0];
   oldzoomfactor[MAXNUMFRAMES-1]      = oldzoomfactor[0];
   sizew[MAXNUMFRAMES-1]              = sizew[0];
   sizeh[MAXNUMFRAMES-1]              = sizeh[0];

   } /* End leftshift_frame_props() */

string header1, header2, header3;

/*=====================================================================*
*                             createdata                               *
*=====================================================================*/

void createdata(const char *inputdatafile) {
   char buffer[MAXCOMMANDSIZE];
   static int frmcnt = 0;
   static int firsttime = true;

   static ifstream inputStream(inputdatafile);
   if (!inputStream) {
      writetolog("Error opening input stream in createdata()");
      cerr << "Error opening input stream" << endl;
      exit(1);
      }

   if (firsttime == true) {
      firsttime = false;
      while (1) {
         inputStream.getline(buffer, MAXCOMMANDSIZE );
         int num  = inputStream.gcount();
         if (num <= 0) {
            // break;
            // cout<<"marker that we are at the end of input meta "
            //    "file - no more frames"<<endl;
            inputStream.close();
            return;
            }
         buffer[num] = '\n';
         // buffer[num + 1] = '\0';
         // cout<<"the buffer has "<<num<<" characters not including "
         //    "the null terminator"<<buffer<<endl;
         parsebuffer(buffer,num);
         int next = inputStream.peek();
         if (next == '[') {
            frmcnt++;

            // if (frmcnt == MAXNUMFRAMES) {
            if (frmcnt == 2) {
               // cout<<"about to process frame # "<<frmcnt<<
               //    " we will not load this frame"<<endl;
               frmcnt--;
               return;
               }
            // cout<<"* at createdata() in meta file mode will load "
            // "frame # "<<frmcnt<<endl;
            continue;
            }
         if (next == ']') {
            // cout<<"next is ]"<<endl;
            moreframes = false;
            continue;
            }
         } // end while
      return;
      } // End of firsttime is true

   // Add one frame and its drawing commands to sListofCommands

   while (1) {
      inputStream.getline(buffer, MAXCOMMANDSIZE );
      int num  = inputStream.gcount();
      if (num <= 0) {
         inputStream.close();
         // cout<<"file "<<__FILE__<<" at line "<<__LINE__<<" : marker "
         // "that we are at the end of input meta file - no more frames"
         // <<endl<<fflush(stdout); //+++++++++ 1/10/08
         moreframes = false;
         return;;
         }
      buffer[num] = '\n';
      // buffer[num + 1] = '\0';
      // cout<<"the buffer has "<<num<<" characters not including the "
      // "null terminator"<<buffer<<endl;
      // cout<<"the buffer has "<<num<<" characters not including the "
      // "null terminator"<<endl;
      parsebuffer(buffer,num);

      int next = inputStream.peek();
      if (next == '[') {
         frmcnt++;
         // cout<<"at createdata() came to but will not load frame # = "
         // <<frmcnt<<endl;
         break;
         }
      if (next == ']') {
         // cout<<"next is ]"<<endl;
         moreframes = false;
         continue;
         }
      } // End while

   } // End new createdata 10/23/2007

/*=====================================================================*
*                             deletedata                               *
*=====================================================================*/

void deletedata(void) {
   if (sListofCommands.empty())
      return;
   writetolog("warning: mfdraw.cxx deletedata() called and there were "
      "commands on sListofCommands");
   sListofCommands.clear();
   atframe = 1;
   inframe = false;
   } /* End deletedata() */

/*=====================================================================*
*                            nextframe_cb                              *
*                                                                      *
*  Heavily revised, 03/11/12, GNR.  Note:  This code can now be called *
*  whether or not in fast-forward mode, so code to turn off that mode  *
*  has been placed in the UI entry for the button or keyboard > click, *
*  but it is still done here when reaching the end of the data.        *
*                                                                      *
*  Rev, 08/25/16, GNR - Removed checking of moreframes from inside     *
*  the two read cases--this apparently was reason why last frame was   *
*  never drawn.  Now the test occurs when user tries to go beyone to   *
*  another, nonexistent, frame.                                        *
*=====================================================================*/

void nextframe_cb(FrameView* glView) {

#if DEBUG & DBGDRAW
   sprintf(msglog, "Entered nextframe_cb: lastframe = %d, atframe = "
      "%d, mmode = %d\n", lastframe, atframe, mmode);
   writetolog(msglog);
#endif

   if (sListofCommands.empty())
      return;

   /* If not at end of loaded frames, just advance to next one */
   if (atframe < lastframe) {
      previousframe = atframe++;
      advanceframe = true;
      }

   /* Display is at end of loaded frames, so this code is concerned
   *  with loading more frames.  */
   else if (!moreframes) {
      advanceframe = false;
      fastforward = false;
      return;
      }

   else if (runmode == FILEMODE) {
      createdata(mf_filename.c_str());

      buildframetracker();
      if (atframe < (MAXNUMFRAMES-1))
         previousframe = atframe++;
      } /* End if FILEMODE */

   else {   /* SOCKMODE */
      /* This code should not care whether in movie or still mode,
      *  either way we want to see if another frame is available.
      *  In SOCKMODE, unlike the case with FILEMODE, turn off fast
      *  forward when end of loaded frames is reached. */
      cnsckinput();
      previousframe = atframe++;
      } /* End else SOCKMODE */

   advanceframe = true;
   drawFrameCommon(glView, 0, 0);

#if DEBUG & DBGDRAW
   sprintf(msglog,"In nxtfrm callback: atframe = %d, lastframe = %d,"
      " fastforward = %d", atframe, lastframe, fastforward);
   writetolog(msglog);
#endif

   if (fastforward) Fl::repeat_timeout(fwdspeedup,
      (Fl_Timeout_Handler)nextframe_cb, glView);

   else if (runmode == SOCKMODE && moreframes && atframe == lastframe &&
         mmode >= MM_MOVIE) {
      ptrnxtfrm->do_callback(); // Automatic frame display advance
      }

   } // End nextframe_cb

/*=====================================================================*
*                           fastforward_cb                             *
*                                                                      *
*  Rev, 03/10/12, GNR - Do not try to wait in here for the timer --    *
*     instead, use the add_timeout call next_frame                     *
*=====================================================================*/

void fastforward_cb(FrameView* glView) {

   if (sListofCommands.empty())
      return;

   // If in fastprevious mode, just increase the delay time
   if (fastprevious)
      bckspeedup += SpeedIncr;

   // If already in fastforward mode, decrement the delay time
   else if (fastforward) {
      fwdspeedup -= SpeedIncr;
      if (fwdspeedup < MinSpeedIncr) fwdspeedup = MinSpeedIncr;
      }

   else {
      // Set initial delay from prefs file
      fwdspeedup = MINFRAMEVIEWTIMENEXT;
      fastforward  = true;
      // Draw the first frame after the specified delay
      if (atframe <= lastframe) {
         previousframe = atframe;
         Fl::add_timeout(fwdspeedup,
            (Fl_Timeout_Handler)nextframe_cb, glView);
         }
      }

   } // End fastforward_cb()

/*=====================================================================*
*                            prevframe_cb                              *
*                                                                      *
*  New routine, 03/13/12, GNR - Based on nextframe_cb and code for     *
*  previous frame that smx had in mfdrawUI.fl.  Moving it here makes   *
*  it accessible from the fast_previous_cb.  As with nextframe_cb,     *
*  note that this code can be called whether or not in fast-previous   *
*  mode, so code to turn off that mode should be in the UI entry for   *
*  the button or keyboard < click, but it is still done here when      *
*  reaching the start of the data.                                     *
*=====================================================================*/

void prevframe_cb(FrameView* glView) {

#if DEBUG & DBGDRAW
   sprintf(msglog, "Entered prevframe_cb: lastframe = %d, atframe = "
      "%d, mmode = %d\n", lastframe, atframe, mmode);
   writetolog(msglog);
#endif

   if (sListofCommands.empty())
      return;

   /* If not at start of loaded frames, just back up to previous one.
   *  (In current version (not in spec), we do not attempt to reload
   *  frames that were deleted -- this is for another day.)  */
   if (atframe > 1) {
      previousframe = atframe--;
      advanceframe = true;
      drawFrameCommon(glView, 0, 0);
      if (fastprevious)
         Fl::repeat_timeout(bckspeedup,
            (Fl_Timeout_Handler)prevframe_cb, glView);
      }

   } // End prevframe_cb()

/*=====================================================================*
*                          fast_previous_cb                            *
*                                                                      *
*  Rev, 03/10/12, GNR - Do not try to wait in here for the timer --    *
*     instead, use the add_timeout call next_frame                     *
*=====================================================================*/

void fastprevious_cb(FrameView* glView) {

   if (sListofCommands.empty())
      return;

   // If in fastforward mode, just increase the delay time
   if (fastforward)
      fwdspeedup += SpeedIncr;

   // If already in fastprevious mode, decrement the delay time
   else if (fastprevious) {
      bckspeedup -= SpeedIncr;
      if (bckspeedup < MinSpeedIncr) bckspeedup = MinSpeedIncr;
      }

   else {
      // Set initial delay from prefs file
      bckspeedup = MINFRAMEVIEWTIMEPREV;
      fastprevious = true;
      // Draw the first frame in about 1/10 secon
      if (atframe > 1) {
         previousframe = atframe;
         Fl::add_timeout(bckspeedup,
            (Fl_Timeout_Handler)prevframe_cb, glView);
         }
      }

   } // End fastprevious_cb()

/*=====================================================================*
*                             deleteframe                              *
*=====================================================================*/

int deleteframe(void) {
   // deleteframe will insure that number of frames on sListofCommands
   // is < MAXNUMFRAMES;

   // int listsize = sListofCommands.size();
   // Note that sListofCommands.size() is O(N)
   // printf("listsize pre-delete = %d\n", listsize);

   // Note that this is an alternate method
   // (alternate to buildframetracker approach)
   // of finding the index of the last frame in constant time
   static LISTOFCOMMANDS::iterator listit;
   static int deletedsofar = 0;
   listit = sListofCommands.begin();
   while(listit++ != sListofCommands.end()) {
      if ((*listit)->cmd == NEWFRAME) {
         // sprintf(msglog, "last frame on list has index = %d",
         // ((Newframe*)(*listit))->frameindex);
         // writetolog(msglog);
         break;
         }
      } // End find last frame

   // generalized to inlude runmode = 1 (sockets)
   int numdeleteframe    = (((Newframe*)(*listit))->frameindex) -
      MAXNUMFRAMES + 1 - deletedsofar;
   // sprintf(msglog,"number of frames to delete: %d",numdeleteframe);
   // cout<<"in file "<<__FILE__<<" at line "<<__LINE__<<" delete "
   //<<numdeleteframe<<" frames"<<endl;
   // writetolog(msglog);

   if (numdeleteframe < 1) {
      // memfile();                          // for testing
      // sprintf(msglog,"atframe %d - did not delete frame", atframe);
      // writetolog(msglog);
      // printf(" did not delete frame\n");  // for  testing
      return 0;
      }
   else {
      // printf("will now delete %d frames\n", numdeleteframe);
      deletedsofar += numdeleteframe;
      } // end find the index of the last frame  (meta file mode)

   listit = sListofCommands.end();
   while (1) {
      --listit;
      switch ((*listit)->cmd) {
      case NEWFRAME : {
         if (numdeleteframe--  > 0) {
            // sprintf(msglog, "in mfdraw.cxx / deleteframe(): "
            // "runmode = %d and delete frame. Deleted so far %d",
            // runmode, deletedsofar);
            // writetolog(msglog);
            }
         else {
            // listsize = sListofCommands.size();
            // printf("listsize post-delete just before return from "
            // "deleteframe() = %d\n", listsize);
            // writetolog("no more frames to delete return from "
            // "deleteframe\n");
            // memfile(); // for memfile testing
            // printf("listsize post-delete just before return from "
            // "deleteframe() = %d\n", listsize);
            return deletedsofar;
            }

#if DEBUG & DBGDRAW
         sprintf(msglog,"about to delete frame of index = %d in %s at "
            "line %d: runmode = %d atframe = %d previousframe = %d, "
            "lastframe = %d\n", ((Newframe*)(*listit))->frameindex,
            __FILE__ , __LINE__ , runmode, atframe, previousframe,
            lastframe);
         writetolog(msglog);
#endif

         delete (Newframe*)(*listit);
         sListofCommands.erase(listit);

         break;
         } /* End case NEWFRAME */

      case DRWLINE : {
         delete (Line*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Line\n");
         break;
         }
      case DRWCIRCLE : {
         delete (Circle*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Circle\n");
         break;
         }
      case DRWFILLEDCIRCLE : {
         delete (Filledcircle*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Filledcircle\n");
         break;
         }
      case DRWOUTLINEELLIPSE : {
         delete (Outlineellipse*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Outlineellipse\n");
         break;
         }
      case DRWFILLEDELLIPSE : {
         delete (Filledellipse*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Filledellipse\n");
         break;
         }
      case DRWFILLCLOSEPOLYLINE : {
         delete (Fillclosepolyline*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Fillclosepolyline\n");
         break;
         }
      case DRWOUTLINECLOSEPOLYLINE : {
         delete (Outlineclosepolyline*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Outlineclosepolyline\n");
         break;
         }
      case DRWOPENPOLYLINE : {
         delete (Openpolyline*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Openpolyline\n");
         break;
         }
      case DRWOUTLINESQUARE : {
         delete (Outlinesquare*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Outlinesquare\n");
         break;
         }
      case DRWFILLEDSQUARE : {
         delete (Filledsquare*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Filledsquare\n");
         break;
         }
      case DRWOUTLINERECTANGLE : {
         delete (Outlinerectangle*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Outlinerectangle\n");
         break;
         }
      case DRWFILLEDRECTANGLE : {
         delete (Filledrectangle*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Filledrectangle\n");
         break;
         }
      case DRWBITMAP : {
         delete (Bitmap*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Bitmap\n");
         break;
         }
      case DRWBITMAPGRAYSCALESCALABLE : {
         delete (Bitmapgrayscalescalable*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Bitmapgrayscalescalable\n");
         break;
         }
      case COLORNAME : {
         delete (Colorname*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Colorname\n");
         break;
         }
      case COLORBGR : {
         delete (Colorbgr*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Colorbgr\n");
         break;
         }
      case COLORBBGGRR : {
         delete (Colorbbggrr*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Colorbbggrr\n");
         break;
         }
      case DRWTEXT : {
         delete (Text*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Text\n");
         break;
         }
      case LINETHICK : {
         delete (Linethick*)(*listit);
         sListofCommands.erase(listit);
         // printf("Deleted command = Linethick\n");
         break;
         }
      default :
         sprintf(msglog, "Error in %d in %s:  Did not delete drawing "
            "command  %d\n", __LINE__ , __FILE__ , (*listit)->getcmd());
         writetolog(msglog);
         exit(EXIT_FAILURE);
      } // End cmd switch
   } // End of while

   return deletedsofar;
   } // End deletframe()

/*=====================================================================*
*                               memfile                                *
*=====================================================================*/

void memfile() {

   // Obtain memory usage statistics using proc file statm
   char buf[30];
   snprintf(buf, 30, "/proc/%u/statm", (unsigned)getpid());
   FILE* pf = fopen(buf, "r");
   if (pf) {
      unsigned size;       //       total program size
      unsigned resident;   //       resident set size
      unsigned share;      //       shared pages
      unsigned text;       //       text (code)
      unsigned lib;        //       library
      unsigned data;       //       data/stack
      // unsigned dt;         //       dirty pages (unused in Linux 2.6)
      // fscanf(pf, "%u" /* %u %u %u %u %u"*/, &size/*, &resident,
      // &share, &text, &lib, &data*/);
      fscanf(pf, "%u %u %u %u %u %u", &size, &resident, &share,
         &text, &lib, &data);
      }
   fclose(pf);
   } /* End memfile() */
