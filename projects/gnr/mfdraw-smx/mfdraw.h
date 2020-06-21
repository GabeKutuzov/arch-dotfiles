#ifndef MFDRAW_H
#define MFDRAW_H

//#include <GL/glut.h>
#include <FL/glut.H>
//glut include needed for stroked fonts used to scale and rotate fonts in FrameView::drawText()

#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/gl.h>

#include </usr/include/GL/glu.h>
//glu.h  needed for tesselation (concave polygons)

#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_File_Chooser.H>

#include <FL/x.H>                               
//<FL/x.H>  required for fl_open_display()

#include <stdlib.h> 

#include<iostream>
#include<fstream>

#include<list>
//#include<deque>  //experiment


#include<string>

#include <math.h>

#include <pwd.h> 
//include used in preference file access

using namespace std;
 
extern const char *mfdrawversion;  //defined in mfdrawmain.cxx

extern string preffileinuse;


enum COMMANDS {ENDOFFILE = -1, NEWFRAME, DRWLINE, DRWCIRCLE,DRWFILLEDCIRCLE, DRWOUTLINEELLIPSE, DRWFILLEDELLIPSE, DRWFILLCLOSEPOLYLINE, DRWOUTLINECLOSEPOLYLINE,DRWOPENPOLYLINE, DRWOUTLINESQUARE, DRWFILLEDSQUARE, DRWOUTLINERECTANGLE, DRWFILLEDRECTANGLE, DRWBITMAP, DRWBITMAPGRAYSCALESCALABLE,  COLORNAME, COLORBGR, COLORBBGGRR, DRWTEXT, LINETHICK};
//added COLORBBGGRR on 6/20/2008



extern  enum COMMANDS CURRENTCMD;
class Command;

typedef list<Command*> LISTOFCOMMANDS; //stl double linked list of all drawing commands
//typedef deque<Command*> LISTOFCOMMANDS;  //experiment


extern  LISTOFCOMMANDS sListofCommands;
extern  LISTOFCOMMANDS::iterator listit;

extern  int atframe;
extern  int previousframe;

//extern int commandcount; //used for debug only

extern  bool inframe;
extern  bool advanceframe;

//extern bool frameorder_view;

extern bool fastforward;
extern bool fastprevious; //support for << button (time delay on reverse frame sequence)

//extern  LISTOFCOMMANDS::iterator frametracker[]; //orig commented out for following run time sized frametracker new
extern  LISTOFCOMMANDS::iterator *frametracker;    //supports run time sized frametracker and MAXNUMFRAMES  new

extern  int lastframe;
extern  bool moreframes;

extern bool connectionclosed;  //++++++++++++++++++++++++++++ 4/18/2008
extern bool quitpressed;       //++++++++++++++++++++++++++++ 4/18/2008


extern  Fl_Value_Slider*  ptrzoom;
extern  Fl_Light_Button*  ptrorigin;
extern  string header1, header2, header3;

extern  Fl_Button* ptrnxtfrm;
extern  Fl_Button* ptrprevfrm;
extern  Fl_Double_Window* ptrnetdialog;
extern  Fl_Group* ptrcontrols;
extern  Fl_Double_Window* ptrsettings;
extern  Fl_Double_Window* ptrexitcontinue;

class   FrameViewUI;
extern  FrameViewUI *pfvui;

class Circledata;
extern Circledata circledata; 

//const   int MAXNUMFRAMES =   15000;
//const   int MAXNUMFRAMES =     601;  // actual max number of frames is 1 less than MAXNUMFRAMES. mfdraw keeps the last MAXNUMFRAMES - 1 frames
//const   int  MAXNUMFRAMES =     81; 
//const   int MAXNUMFRAMES =       7;   // example for MANXNUMFRAMES = 7: max num of frames = 6 after 6th frame we delete a frame for every new frame that is added

extern int  MAXNUMFRAMES;


//const  int MAXCOMMANDSIZE =   60000; //see createdata()
const    int MAXCOMMANDSIZE = 6000000;
//const  int MAXCOMMANDSIZE =  200000;

//const   int MSGLOGSIZE    =   60000;
const     int MSGLOGSIZE    = 6000000;
//const   int MSGLOGSIZE    =  200000;

extern  char msglog[];

extern int WSCREENPIX;  //developer's opengl design design time opengl screen width resolution  also now part of preferences
extern int HSCREENPIX;  //developer's opengl design design time opengl screen height resolution also now part of preferences

extern bool skey;      //used to indicate that the s key has been pressed and it is the most recent event.  used in mainWindow to avoid closing app. on s command.

extern int XPOSWINDOW;   //overriding window manager for positioning
extern int YPOSWINDOW;

extern int  MAINWINDOWWIDTHSIZE;



class FrameView : public Fl_Gl_Window {
 public:
  
  //bool bkgrndwhite[MAXNUMFRAMES];    //back ground color is remembered but not inherited from pages of the same dimension
  bool *bkgrndwhite;
  GLdouble left, right, bottom, top; //defines ortho projection
  bool setorigin;                    //flag that indicates next mouse press is at the user requested location for a new origin
  //double size[MAXNUMFRAMES];         // size determines the scaling factor for drawing in frameview
  double *size;
  double sizestep;                   //step size for size and size's range
  double sizemin;
  double sizemax;
  //double oldsize[MAXNUMFRAMES];      // as size changes we need the difference of change via oldisze - size
  double *oldsize;
  //float  sizew[MAXNUMFRAMES], sizeh[MAXNUMFRAMES];               // scaling factor for width and height respectively
  float *sizew;
  float *sizeh;
  //double pt0x[MAXNUMFRAMES], pt0y[MAXNUMFRAMES];                 // user selected origin to provide a reference for sizing (i.e. zoom)
  double *pt0x;
  double *pt0y;

  int fl_up_count;                   //support left/right up/down arrow keys
  int fl_down_count;                 //we need to delete key release and 1 repeat key event

  //obsolete version of frame advance in glView::handle()
  //int fl_left_count;              
  //int fl_right_count;
  //bool first_right;

  void init(void);  // used to update the state of mfdraw drawing parameters
  void home(void);  // used to reset window to initial drawing parameters

  void leftshift_frame_props(void); //align frametracker viewing parameters with framebuilder[] upon frame deletes. called in buildframetracker()
 
  void zoomfunc();

  FrameView(int x,int y,int w,int h,const char *l=0);

  void draw(); 

  void resize(int xp, int yp, int wd, int ht);

  int handle(int);

  void modelviewsetorigin(int xpos, int ypos);

private:
  //int origwinwidth[MAXNUMFRAMES];       //for glView
  int *origwinwidth;
  //int origwinheight[MAXNUMFRAMES];      //for glView
  int *origwinheight;
  //int lastwinwidth[MAXNUMFRAMES];       //used to determine if glView has changed size i.e. resized
  int *lastwinwidth;
  //int lastwinheight[MAXNUMFRAMES];      //used to determine if glView has changed size i.e. resized
  int *lastwinheight;
  //int origmainwinwidth[MAXNUMFRAMES];   //for mainWindow
  int *origmainwinwidth;
  //int origmainwinheight[MAXNUMFRAMES];  //for mainWindow
  int *origmainwinheight;
  //bool firsttime[MAXNUMFRAMES];
  bool *firsttime; //initialized in contructor initialization list
  //double zoomfactor[MAXNUMFRAMES];      //for resize
 public:
  double *zoomfactor; //zoomfactor made public for displaying size relative to current window rather than to original window 3/10/2008
 private:
  //double oldzoomfactor[MAXNUMFRAMES];   //for resize
  double *oldzoomfactor;  
  //float xshift[MAXNUMFRAMES];
  float *xshift;
  //float yshift[MAXNUMFRAMES];
  float *yshift;
  //double previouspagewidth[MAXNUMFRAMES];
  double *previouspagewidth;
  //double previouspageheight[MAXNUMFRAMES];
  double *previouspageheight;

  int current_line_thickness;

  void drawFrame();
  void drawCircle(double x, double y, double r);
  void drawFilledcircle(double x, double y, double r);

  void drawOutlineellipse(double x, double y, double w, double h, double angle);
  void drawFilledellipse(double x, double y, double w, double h, double angle);

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
  //added colorbbggrr() on 6/20/2008
	   


  void drawText(double x, double y, double ht, double angle, int len, string text);
  void linethick(int t);

  void drawBitmap(int type, int mode, float xc, float yc, int rowlen, int colht, int xofs, int yofs, int iwd, int iht, int length, char* buf);   
  void drawBitmapgrayscalescalable(int type, int mode, float xc, float yc, float bwd, float bht, int rowlen, int colht, int xofs, int yofs, int iwd, int iht, int length, char* buf);
};


class Command {
 public:
  enum COMMANDS cmd;
  Command(enum COMMANDS command) : cmd(command) {}
  enum COMMANDS getcmd(void) { return cmd;}
};


#if 0
// support for unique command id numbers experiments - replaces above Command class 
class Command {
  public:
  enum COMMANDS cmd;
  static int cmdidnum;
  Command(enum COMMANDS command) : cmd(command) {++cmdidnum;}
  enum COMMANDS getcmd(void) { return cmd;}
};
#endif


class Newframe : public Command {
 public:
  int frameindex;
  GLdouble left;
  GLdouble right;
  GLdouble bottom;
  GLdouble top;
  string frametype;
  Newframe(int frameindex, GLdouble lft, GLdouble rt, GLdouble bot, GLdouble tp, string frmtype = "") : Command(NEWFRAME),
    frameindex(frameindex), left(lft), right(rt), bottom(bot), top(tp), frametype(frmtype) {}
};
class Circle : public Command {
 public:
  double x; 
  double y;
  double r;
  Circle(double x,double y,double radius) : Command(DRWCIRCLE), x(x), y(y), r(radius) {} 
};
class Filledcircle : public Command {
 public:
  double x; 
  double y;
  double r;
  Filledcircle(double x,double y,double radius) : Command(DRWFILLEDCIRCLE), x(x), y(y), r(radius) {} 
};

class Outlineellipse : public Command {
 public:
  double x;
  double y;
  double w;
  double h;
  double angle;
  Outlineellipse(double x, double y, double w, double h, double a) : Command(DRWOUTLINEELLIPSE), x(x), y(y), w(w), h(h), angle(a) {}
};
class Filledellipse : public Command {
 public:
  double x;
  double y;
  double w;
  double h;
  double angle;
  Filledellipse(double x, double y, double w, double h, double a) : Command(DRWFILLEDELLIPSE), x(x), y(y), w(w), h(h), angle(a) {}
};

class Line : public Command {
 public:
  double  x1;
  double  y1;
  double  x2;
  double  y2;
  Line(double x1, double y1, double x2, double y2) : Command(DRWLINE), x1(x1),y1(y1),x2(x2),y2(y2) {}
};





class Colorname : public Command {
 public:
  string colorname;
  Colorname(string c) : Command(COLORNAME), colorname(c) {}
};
class Colorbgr : public Command {
 public:
  int blue;
  int green;
  int red;
  Colorbgr(int b, int g, int r) : Command(COLORBGR), blue(b), green(g), red(r) {}
};
class Colorbbggrr : public Command {
 public:
  int blue;
  int green;
  int red;
  Colorbbggrr(int b, int g, int r) : Command(COLORBBGGRR), blue(b), green(g), red(r) {}
};
//added class Colorbbggrr on 6/20/2008





class Fillclosepolyline : public Command {
 public:
  int np;
  double *c;
  Fillclosepolyline(int np, double *c) : Command(DRWFILLCLOSEPOLYLINE), np(np), c(c) {}
};
class Outlineclosepolyline : public Command {
 public:
  int np;
  double *c;
  Outlineclosepolyline(int np, double *c) : Command(DRWOUTLINECLOSEPOLYLINE), np(np), c(c) {}
};
class Openpolyline : public Command {
 public:
  int np;
  double *c;
  Openpolyline(int np, double *c) : Command(DRWOPENPOLYLINE), np(np), c(c) {}
};
class Outlinesquare : public Command {
 public:
  double x;
  double y;
  double e;
  Outlinesquare(double x, double y, double e) : Command(DRWOUTLINESQUARE), x(x), y(y), e(e) {}
};
class Filledsquare : public Command {
 public:
  double x;
  double y;
  double e;
  Filledsquare(double x, double y, double e) : Command(DRWFILLEDSQUARE), x(x), y(y), e(e) {}
};
class Outlinerectangle : public Command {
 public:
  double x;
  double y;
  double e1;
  double e2;
  Outlinerectangle(double x, double y, double e1, double e2) : Command(DRWOUTLINERECTANGLE), x(x), y(y), e1(e1), e2(e2) {}
};
class Filledrectangle : public Command {
 public:
  double x;
  double y;
  double e1;
  double e2;
  Filledrectangle(double x, double y, double e1, double e2) : Command(DRWFILLEDRECTANGLE), x(x), y(y), e1(e1), e2(e2) {}
};
class Text : public Command {
 public:
  double x;
  double y;
  double ht;
  double angle;
  int length;
  string text;
  Text( double x, double y, double ht, double angle, int length, string txt) :
    Command(DRWTEXT), x(x), y(y), ht(ht), angle(angle), length(length), text(txt) {}
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
  Bitmap(int type, int mode, float xc, float yc, int rolen, int colht, int xofs, int yofs, int iwd, int iht, int length, char* buffer) : 
    Command(DRWBITMAP), type(type), mode(mode), xc(xc), yc(yc), rolen(rolen), colht(colht), xofs(xofs), yofs(yofs), iwd(iwd), iht(iht), length(length) 
    {
      bitdata = new char[length];     
      for(int i = 0; i < length; i++)
	bitdata[i] = buffer[i];
    } 
  ~Bitmap() {
    delete [] bitdata;    
    //printf("deleted a Bitmap object using Bitmap destructor\n");
  }

  //-------------------- based on test of the following doesnt appear i need a copy constructor -----------------------------
  /*  Bitmap(const Bitmap & b) : Command(b.cmd) {  */
  /*     type = b.type; */
  /*     // ++++++++++++++++++++++++ do the other elements */
  /*     printf("Bitmap copy constructor called\n"); */
  /*   } */
  //--------------------------------------------------------------------------------------------------------------------------  
};

class Bitmapgrayscalescalable : public Command {
 public:
  int type, mode;
  float xc, yc, bwd, bht;
  int rolen, colht, xofs, yofs, iwd, iht, length;
  char *bitdata;
  Bitmapgrayscalescalable(int type, int mode, float xc, float yc, float bwd, float bht, int rolen, int colht, int xofs, int yofs, int iwd, int iht,int length, char* buffer) :
    Command(DRWBITMAPGRAYSCALESCALABLE), type(type), mode(mode), xc(xc), yc(yc), bwd(bwd), bht(bht), rolen(rolen),colht(colht), xofs(xofs), yofs(yofs), iwd(iwd), iht(iht), 
    length(length) 
    {      
      //length =  MAXCOMMANDSIZE - 10;    //use this to create a 'jumbo' Bitmapgrayscalescalable object to demonstrate various mem mgt.issues
      bitdata = new char[length];
      for(int i = 0; i < length; i++)
	bitdata[i] = buffer[i];
    }
  ~Bitmapgrayscalescalable() {
    delete [] bitdata;
    //printf("deleted a Bitmapgrayscalescalable object using Bitmapgrayscalescalable destructor\n");
  }
};


#if 0
// replacement of the above class for unique command id numbers experiments
class Bitmapgrayscalescalable : public Command {
 public:
  int type, mode;
  float xc, yc, bwd, bht;
  int rolen, colht, xofs, yofs, iwd, iht, length;
  char *bitdata;

  int xcmdidnum; // command id num for this instance obtained from the initialization list in this instances 

  Bitmapgrayscalescalable(int type, int mode, float xc, float yc, float bwd, float bht, int rolen, int colht, int xofs, int yofs, int iwd, int iht,int length, char* buffer) :
    Command(DRWBITMAPGRAYSCALESCALABLE), type(type), mode(mode), xc(xc), yc(yc), bwd(bwd), bht(bht), rolen(rolen),colht(colht), xofs(xofs), yofs(yofs), iwd(iwd), iht(iht), 
    length(length), xcmdidnum(cmdidnum) //xcmdidnum +++ new
    {      
      //length =  MAXCOMMANDSIZE - 10;    //use this to create a 'jumbo' Bitmapgrayscalescalable object to demonstrate various mem mgt. 
      bitdata = new char[length];
      for(int i = 0; i < length; i++)
	bitdata[i] = buffer[i];
    }
  ~Bitmapgrayscalescalable() {
    delete [] bitdata;
    //printf("deleted a Bitmapgrayscalescalable object using Bitmapgrayscalescalable destructor\n");
  }
};
#endif


const double TWOPI= 2.0*3.1415926535897;
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

void deletedata(void);                       //clear current data, if any, in order to load new data
void createtestdata(void);                   //driver to create some test data of Circles, Lines, etc.
void buildframetracker(void);                //populate the frametracker array of iterators which point to start of frame
void createdata(const char *inputdatafile);  //input graphics commands from a meta file
void moviemode_cb(void*);                    //when in movie mode we want to call next frame call back during initialization
void checklocalhostxaccess(int, char **);    //check that xhost +localhost command has been executed on gui workstation (for socket mode only)
void nextframe_cb(FrameView*);               //used to organize nxtfrm callback

void fastforward_cb(FrameView*);             //used to organize fastforward (>> button) callback
void fastprevious_cb(FrameView*);            //used to organize fastprevious (<< button) callback

void getscreensize(void);                    //obtains screen width/height in pixels, used to dynamically adjust mfdraw mainwindow to accomodate monitor resolution
int  deleteframe(void);                      //deletes frame(s) so new frame can be added thereby preventing frame count from exceeding maximum number of frames


void memfile(void);                         //reports on mfdraw memory and input file size to support memory mamangement tests (see mfdraw.cxx end of file) 


void loadpreferences(void);                 //looks in home directory of current user for .mfdrawrc if not found uses compile time defaults for preferences
void extractval(string &);                  //called by loadpreferences to obtain value (string &) of the key/value pair

void stopwatch(void);                       //utility to provide high resolution elapsed time between calls

//end of include guard
#endif
