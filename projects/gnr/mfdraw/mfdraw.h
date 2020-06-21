// (c) Copyright 2012-2016, The Rockefeller University *21500*
//
// Rev, 05/17/17, GNR - Remove BM_C16, add BM_GS16, BM_C48
// Rev, 05/26/17, GNR - Construct colorname,Text w/string length

#ifndef MFDRAW_H
#define MFDRAW_H

//glut include needed for stroked fonts used to scale and rotate
// fonts in FrameView::drawText()
#include <FL/Fl.H>
#include <FL/glut.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/gl.h>
//glu.h  needed for tesselation (concave polygons)
#include <FL/glu.h>

#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_File_Chooser.H>
//<FL/x.H>  required for fl_open_display()
#include <FL/x.H>

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <math.h>
//include used in preference file access
#include <pwd.h>

// Constants defined for some metafile option codes
#include "plotdefs.h"

// Define runmode value
#define FILEMODE 0
#define SOCKMODE 1

// Global constants
const double TWOPI = 2.0*3.1415926535897;
const double TORADS =    0.01745329252;

// Define speed increment/decrement on click (msec)
#define DfltSpeedIncr 0.200   /* Default fastfwd/rev frame interval */
#define MinSpeedIncr  0.050   /* Minimum fastfwd/rev frame interval */
#define SpeedIncr     0.020   /* Increment for multiple clicks */

// Define maximum length a title can have (not checked in smx code)
#define MXLTTL 120

// Define DEBUG output options -- Add, 11/26/08, GNR
#ifndef DEBUG
#define DEBUG 0
#endif
#define DBGSTARTUP   0x0001
#define DBGCNSMSG    0x0002
#define DBGHANDLE    0x0004
#define DBGFRVIEW    0x0008
#define DBGDRAW      0x0010
#define DBGCKINPUT   0x0020
#define DBGPARSE     0x0040
#define DBGCLOSE     0x0080
#define DBGHOSTACC   0x0100
#define DBGLPREFS    0x0200
#define DBGRESIZE    0x0400
#define DBGSHOW      0x0800
#define DBGSOCKW     0x1000
#define DBGWAIT      0x8000

using namespace std;

extern const char *mfdrawversion;  //defined in mfdrawmain.cxx
extern string preffileinuse;

enum COMMANDS {ENDOFFILE = -1, NEWFRAME, DRWLINE, DRWCIRCLE,
   DRWFILLEDCIRCLE, DRWOUTLINEELLIPSE, DRWFILLEDELLIPSE,
   DRWFILLCLOSEPOLYLINE, DRWOUTLINECLOSEPOLYLINE, DRWOPENPOLYLINE,
   DRWOUTLINESQUARE, DRWFILLEDSQUARE, DRWOUTLINERECTANGLE,
   DRWFILLEDRECTANGLE, DRWBITMAP, DRWBITMAPGRAYSCALESCALABLE,
   COLORNAME, COLORBGR, COLORBBGGRR, DRWTEXT, LINETHICK};

extern enum COMMANDS CURRENTCMD;
class Command;

//stl double linked list of all drawing commands
typedef list<Command*> LISTOFCOMMANDS;
extern LISTOFCOMMANDS sListofCommands;
extern LISTOFCOMMANDS::iterator listit;

extern int atframe;
extern int previousframe;
extern int mmode, initmmode;

//extern int commandcount; //used for debug only

extern bool inframe;
extern bool advanceframe;

//extern bool frameorder_view;

//support for >> and << buttons (time delay on frame sequence)
extern bool fastforward;
extern bool fastprevious;
extern double fwdspeedup;
extern double bckspeedup;

//supports run time sized frametracker and MAXNUMFRAMES  new
extern LISTOFCOMMANDS::iterator *frametracker;

extern int lastframe;
extern bool moreframes;

extern bool connectionclosed;
extern bool quitpressed;

extern Fl_Value_Slider *ptrzoom;
extern Fl_Light_Button *ptrorigin;
extern string header1, header2, header3;

extern Fl_Button* ptrnxtfrm;
extern Fl_Button* ptrprevfrm;
extern Fl_Double_Window* ptrnetdialog;
extern Fl_Group* ptrcontrols;
extern Fl_Double_Window* ptrsettings;
extern Fl_Double_Window* ptrexitcontinue;

class  FrameViewUI;
extern FrameViewUI *pfvui;

class  Circledata;
extern Circledata circledata;

extern int MAXNUMFRAMES;
const  int MAXCOMMANDSIZE = 6000000;

const  int MSGLOGSIZE = 300;

extern char msglog[];

//opengl screen width, height resolution--also now part of preferences
extern int WSCREENPIX;
extern int HSCREENPIX;

//used to indicate that the s key has been pressed and it is the
//most recent event.  used in mainWindow to avoid closing app.
//on s command.
extern bool skey;

extern int XPOSWINDOW;   //overriding window manager for positioning
extern int YPOSWINDOW;
extern int MAINWINDOWWIDTHSIZE;

class FrameView : public Fl_Gl_Window {

public:
   // Background color is remembered but not inherited from pages
   // of the same dimension
   bool *bkgrndwhite;
   GLdouble left, right, bottom, top; //defines ortho projection
   //flag that indicates next mouse press is at the user requested
   //location for a new origin
   bool setorigin;
   // size determines the scaling factor for drawing in frameview
   double *size;
   double sizestep;      //step size for size and size's range
   double sizemin;
   double sizemax;
   double *oldsize;
   float *sizew;
   float *sizeh;
   double *pt0x;
   double *pt0y;

   int fl_up_count;      //support left/right up/down arrow keys
   //we need to delete key release and 1 repeat key event
   int fl_down_count;

   // used to update the state of mfdraw drawing parameters
   void init(void);
   // used to reset window to initial drawing parameters
   void home(void);

   //align frametracker viewing parameters with framebuilder[]
   // upon frame deletes. called in buildframetracker()
   void leftshift_frame_props(void);

   void zoomfunc();
   FrameView(int x,int y,int w,int h,const char *l=0);
   void draw();
   void resize(int xp, int yp, int wd, int ht);
   int handle(int);
   void modelviewsetorigin(int xpos, int ypos);

private:
   int *origwinwidth;
   int *origwinheight;
   int *lastwinwidth;
   int *lastwinheight;
   int *origmainwinwidth;
   int *origmainwinheight;
   bool *firsttime; //initialized in contructor initialization list
public:
   //zoomfactor made public for displaying size relative to
   //current window rather than to original window 3/10/2008
   double *zoomfactor;
private:
   double *oldzoomfactor;
   float *xshift;
   float *yshift;
   double *previouspagewidth;
   double *previouspageheight;

   int current_line_thickness;

   void drawFrame();
   void drawCircle(double x, double y, double r);
   void drawFilledcircle(double x, double y, double r);

   void drawOutlineellipse(double x, double y, double w, double h,
      double angle);
   void drawFilledellipse(double x, double y, double w, double h,
      double angle);

   void drawLine(double x1, double y1, double x2, double y2);
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

class Command {
public:
   enum COMMANDS cmd;
   Command(enum COMMANDS command) : cmd(command) {}
   enum COMMANDS getcmd(void) { return cmd;}
   };

class Newframe : public Command {
public:
   int frameindex;
   GLdouble left;
   GLdouble right;
   GLdouble bottom;
   GLdouble top;
   string frametype;
   Newframe(int frameindex, GLdouble lft, GLdouble rt, GLdouble bot,
      GLdouble tp, string frmtype = "") : Command(NEWFRAME),
      frameindex(frameindex), left(lft), right(rt),
      bottom(bot), top(tp), frametype(frmtype) {}
   };

class Circle : public Command {
public:
   double x;
   double y;
   double r;
   Circle(double x,double y,double radius) :
      Command(DRWCIRCLE), x(x), y(y), r(radius) {}
   };

class Filledcircle : public Command {
public:
   double x;
   double y;
   double r;
   Filledcircle(double x,double y,double radius) :
      Command(DRWFILLEDCIRCLE), x(x), y(y), r(radius) {}
   };

class Outlineellipse : public Command {
public:
   double x;
   double y;
   double w;
   double h;
   double angle;
   Outlineellipse(double x, double y, double w, double h, double a) :
      Command(DRWOUTLINEELLIPSE), x(x), y(y), w(w), h(h), angle(a) {}
   };

class Filledellipse : public Command {
public:
   double x;
   double y;
   double w;
   double h;
   double angle;
   Filledellipse(double x, double y, double w, double h, double a) :
      Command(DRWFILLEDELLIPSE), x(x), y(y), w(w), h(h), angle(a) {}
   };

class Line : public Command {
public:
   double  x1;
   double  y1;
   double  x2;
   double  y2;
   Line(double x1, double y1, double x2, double y2) :
      Command(DRWLINE), x1(x1), y1(y1), x2(x2), y2(y2) {}
   };

class Colorname : public Command {
public:
   string colorname;
   Colorname(const char *c, size_t nc) :
      Command(COLORNAME), colorname(c,nc) {}
   };

class Colorbgr : public Command {
public:
   int blue;
   int green;
   int red;
   Colorbgr(int b, int g, int r) :
      Command(COLORBGR), blue(b), green(g), red(r) {}
   };

class Colorbbggrr : public Command {
public:
   int blue;
   int green;
   int red;
   Colorbbggrr(int b, int g, int r) :
      Command(COLORBBGGRR), blue(b), green(g), red(r) {}
   };

class Fillclosepolyline : public Command {
public:
   int np;
   double *c;
   Fillclosepolyline(int np, double *c) :
      Command(DRWFILLCLOSEPOLYLINE), np(np), c(c) {}
   };

class Outlineclosepolyline : public Command {
public:
   int np;
   double *c;
   Outlineclosepolyline(int np, double *c) :
      Command(DRWOUTLINECLOSEPOLYLINE), np(np), c(c) {}
   };

class Openpolyline : public Command {
public:
   int np;
   double *c;
   Openpolyline(int np, double *c) :
      Command(DRWOPENPOLYLINE), np(np), c(c) {}
   };

class Outlinesquare : public Command {
public:
   double x;
   double y;
   double e;
   Outlinesquare(double x, double y, double e) :
      Command(DRWOUTLINESQUARE), x(x), y(y), e(e) {}
   };

class Filledsquare : public Command {
public:
   double x;
   double y;
   double e;
   Filledsquare(double x, double y, double e) :
      Command(DRWFILLEDSQUARE), x(x), y(y), e(e) {}
   };

class Outlinerectangle : public Command {
public:
   double x;
   double y;
   double e1;
   double e2;
   Outlinerectangle(double x, double y, double e1, double e2) :
      Command(DRWOUTLINERECTANGLE), x(x), y(y), e1(e1), e2(e2) {}
   };

class Filledrectangle : public Command {
public:
   double x;
   double y;
   double e1;
   double e2;
   Filledrectangle(double x, double y, double e1, double e2) :
      Command(DRWFILLEDRECTANGLE), x(x), y(y), e1(e1), e2(e2) {}
   };

class Text : public Command {
public:
   double x;
   double y;
   double ht;
   double angle;
   int length;
   string text;
   Text(double x, double y, double ht, double angle, int length,
      const char *txt) : Command(DRWTEXT), x(x), y(y), ht(ht),
      angle(angle), length(length), text(txt,(size_t)length) {}
   };

class Linethick : public Command {
public:
   int thickness; //range is currently 0 to 3 with 0 the thinest
   Linethick(int t) : Command(LINETHICK), thickness(t) {}
   };

class Bitmap : public Command {  // bitmap
public:
   int type, mode;
   float xc, yc;
   int rolen, colht, xofs, yofs, iwd, iht, length;
   char *bitdata;
   Bitmap(int type, int mode, float xc, float yc, int rolen, int colht,
         int xofs, int yofs, int iwd, int iht, int length,
         char *buffer) : Command(DRWBITMAP), type(type), mode(mode),
         xc(xc), yc(yc), rolen(rolen), colht(colht), xofs(xofs),
         yofs(yofs), iwd(iwd), iht(iht), length(length) {
      bitdata = new char[length];
      /* Rev, 05/02/10, GNR - Use memcpy when iht < 0 */
      if (iht > 0) {
         char *pbd = bitdata, *pbf = buffer + length;
         int  rowlen = (iwd+7+(xofs&7)) >> 3;
         for (int irow=0; irow<iht; ++irow) {
            pbf -= rowlen;
            memcpy(pbd, pbf, rowlen);
            pbd += rowlen;
            }
         }
      else
         memcpy(bitdata, buffer, length);
      }
   ~Bitmap() {
      delete [] bitdata;
      // printf("deleted a Bitmap object using Bitmap destructor\n");
      }
   };

class Bitmapgrayscalescalable : public Command {
public:
   int type, mode;
   float xc, yc, bwd, bht;
   int rolen, colht, xofs, yofs, iwd, iht, length;
   char *bitdata;
   Bitmapgrayscalescalable(int type, int mode, float xc, float yc,
         float bwd, float bht, int rolen, int colht, int xofs, int yofs,
         int iwd, int iht, int length, char* buffer) :
         Command(DRWBITMAPGRAYSCALESCALABLE), type(type), mode(mode),
         xc(xc), yc(yc), bwd(bwd), bht(bht), rolen(rolen), colht(colht),
         xofs(xofs), yofs(yofs), iwd(iwd), iht(abs(iht)), length(length)
         {
      // Use this to create a 'jumbo' Bitmapgrayscalescalable object
      //    to demonstrate various mem mgt.issues
      //    length =  MAXCOMMANDSIZE - 10;
      bitdata = new char[length];
      /* Rev, 04/30/10, GNR - Use memcpy when iht < 0 */
      if (iht > 0) {
         char *pbd = bitdata, *pbf = buffer + length;
         int  rowlen;
         switch (type) {
         case BM_BW:
            rowlen = (iwd+7+(xofs&7)) >> 3; break;
         case BM_GS:
         case BM_C8:
            rowlen = iwd; break;
         case BM_GS16:
         case BM_C16:
            rowlen = 2*iwd; break;
         case BM_C24:
            rowlen = 3*iwd; break;
         case BM_C48:
            rowlen = 6*iwd; break;
            } /* End type switch */
         for (int irow=0; irow<iht; ++irow) {
            pbf -= rowlen;
            memcpy(pbd, pbf, rowlen);
            pbd += rowlen;
            }
         }
      else
         memcpy(bitdata, buffer, length);
      }
   ~Bitmapgrayscalescalable() {
      delete [] bitdata;
      // printf("Deleted a Bitmapgrayscalescalable object using
      //    Bitmapgrayscalescalable destructor\n");
      }
   };

class Circledata {
private:
   double circlepoints;
   double *mycos;
   double *mysin;
   public:
   Circledata(int n = 360)  {
      circlepoints = n; //trade off between speed and resolution
      mycos = new double[n];
      mysin = new double[n];
      double angle;
      for (int i = 0; i < circlepoints; i++) {
         angle = TWOPI*i/circlepoints;
         mycos[i] = cos(angle);
         mysin[i] = sin(angle);
         }
      }
   double getcirclepoints(void)      { return circlepoints; }
   inline double getmycos(int index) { return mycos[index]; }
   inline double getmysin(int index) { return mysin[index]; }
   };

// Populate the frametracker array of iterators
void buildframetracker(void);
// Check that xhost +localhost command has been executed on workstation
//    (only needed in socket mode)
void checklocalhostxaccess(int, char **);
// Normal CNS socket read
void cnsckinput(void);
// Read header from socket file
void cnsstartup(void);
// Input graphics commands from a meta file
void createdata(const char *inputdatafile);
// Driver to create some test data of circles, lines, etc.
void createtestdata(void);
// Clear current data, if any, in order to load new data
void deletedata(void);
// Delete frame(s)
int  deleteframe(void);
// Code used in many places where a frame is drawn
void drawFrameCommon(FrameView *glView, int incr, int dozoom);
// Obtain value (string &) of a preferences key/value pair
void extractval(string &);
// Organize fastforward (>> button) callback
void fastforward_cb(FrameView*);
// Organize fastprevious (<< button) callback
void fastprevious_cb(FrameView*);
// Obtain screen width/height in pixels
void getscreensize(void);
// Load preferences from .mfdrawrc in home dir or user else defaults
void loadpreferences(void);
// Report on mfdraw memory and input file size
void memfile(void);
// Organize next-frame callback
void nextframe_cb(FrameView*);
// Execute previous-frame callback
void prevframe_cb(FrameView*);
// Utility to provide high resolution elapsed time between calls
void stopwatch(void);

// end of include guard
#endif
