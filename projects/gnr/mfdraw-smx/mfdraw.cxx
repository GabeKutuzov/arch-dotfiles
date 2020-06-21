/*##################################################################
  #               File mfdraw.cxx               		   #
  #								   #
  # Version 1.111               6/8/2005             Steven Marx   #          
  ##################################################################
    
  Definitions for the FrameView class its constructor and methods. In
  addition this file contains functions for tracking start of frame
  data, deleting all data and creating data in file mode. In
  mfdraw there are two run modes: file mode (runmode = 0)
  and socket mode (runmode = 1). mfdraw distinguishes between
  the two by simply detecting if main has any arguments, if it
  does then it is in run mode 1 else it is in run mode 0 (see
  mfdrawmain.cxx for details.

  ###################################################################*/

#include "mfdraw.h"
#include "mfdrawsockserver.h"

#include "mfdrawUI.h"
//mfdrawUI can be used to provide access to FrameViewUI *pfvui for example pfvui->glView

#include <math.h>
#include <stdio.h> //for printf style debugging

Circledata circledata; //precompute  cos(x) and sin(x) for circle or elliptical boundaries

#if 0
FrameView::FrameView(int x,int y,int w,int h,const char *l)
  : Fl_Gl_Window(x,y,w,h,l)
{
  double adjust;  
  if(HSCREENPIX/1024.0 <= WSCREENPIX/1280)   
      adjust = ((double)HSCREENPIX)/1024.0; 
  else
      adjust = ((double)WSCREENPIX)/1280.0;
  //char debugmsg[300];
  //sprintf(debugmsg, "adjust = %g and MAXNUMFRAMES = %d", adjust, MAXNUMFRAMES);
  //writetolog(debugmsg);


  //writetolog("ready to start initialization for frame parameters");
  //int sz = sizeof(firsttime);
  //printf("sizeof firsttime = %d\n", sz);
  for(int i = 0; i < MAXNUMFRAMES; i++) {
    firsttime[i] = true; 
     
    origwinwidth[i]  = int(this->w()*adjust); 
    origwinheight[i] = int(this->h()*adjust); 
  
    lastwinwidth[i]  = origwinwidth[i];
    lastwinheight[i] = origwinheight[i];

    //origmainwinwidth[i]  =  parent()->parent()->parent()->w();
    //origmainwinheight[i] =  parent()->parent()->parent()->h(); 
    origmainwinwidth[i]  =  (int)((parent()->parent()->parent()->w())*adjust);
    origmainwinheight[i] =  (int)((parent()->parent()->parent()->h())*adjust);
    
    pt0x[i] = 0.0;
    pt0y[i] = 0.0;

    size[i]   = oldsize[i] = 1.0;
    xshift[i] = 0.0;       
    yshift[i] = 0.0;             
  }
  //writetolog("finished initialization for frame parameters");

  if(runmode == 1) {   
    cnsstartup();  //Reads the science model's drawing commands from stdin (in tcp/ip socket mode) which is provided via xinetd
    LISTOFCOMMANDS::iterator frit;
    frit = frametracker[atframe]; //frame pointed to by the iterators in frametracker[n]
    left = ((Newframe*)(*frit))->left;
    right = ((Newframe*)(*frit))->right;
    bottom = ((Newframe*)(*frit))->bottom;
    top = ((Newframe*)(*frit))->top;
  }
}//end of FrameView::FrameView
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ start replacement for above
//FrameView::FrameView(int x,int y,int w,int h,const char *l) : firsttime(new bool[MAXNUMFRAMES]), Fl_Gl_Window(x,y,w,h,l)
FrameView::FrameView(int x,int y,int w,int h,const char *l) : Fl_Gl_Window(x,y,w,h,l), \
  bkgrndwhite(new bool[MAXNUMFRAMES]), size(new double[MAXNUMFRAMES]), oldsize(new double[MAXNUMFRAMES]), sizew(new float[MAXNUMFRAMES]), sizeh(new float[MAXNUMFRAMES]),\
  pt0x(new double[MAXNUMFRAMES]),  pt0y(new double[MAXNUMFRAMES]), \
  origwinwidth(new int[MAXNUMFRAMES]), origwinheight(new int[MAXNUMFRAMES]), lastwinwidth(new int[MAXNUMFRAMES]), lastwinheight(new int[MAXNUMFRAMES]),\
  origmainwinwidth(new int[MAXNUMFRAMES]), origmainwinheight(new int[MAXNUMFRAMES]), firsttime(new bool[MAXNUMFRAMES]), zoomfactor(new double[MAXNUMFRAMES]), \
  oldzoomfactor(new double[MAXNUMFRAMES]), xshift(new float[MAXNUMFRAMES]), yshift(new float[MAXNUMFRAMES]),  previouspagewidth(new double[MAXNUMFRAMES]),\
  previouspageheight(new double[MAXNUMFRAMES])
{
  double adjust;  
  if(HSCREENPIX/1024.0 <= WSCREENPIX/1280)   
      adjust = ((double)HSCREENPIX)/1024.0; 
  else
      adjust = ((double)WSCREENPIX)/1280.0;
  //char debugmsg[300];
  //sprintf(debugmsg, "adjust = %g and MAXNUMFRAMES = %d", adjust, MAXNUMFRAMES);
  //writetolog(debugmsg);


  //writetolog("ready to start initialization for frame parameters");
  //int sz = sizeof(firsttime);
  //printf("sizeof firsttime = %d\n", sz);
  for(int i = 0; i < MAXNUMFRAMES; i++) {

    bkgrndwhite[i] = false;

    firsttime[i] =   true; 
     


    //origwinwidth[i]  = int(this->w()*adjust); // replaced by following two  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ out  11/6/07
    //origwinheight[i] = int(this->h()*adjust); // replaced by following two  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ out  11/6/07
    //printf("origwinwidth[%d] = %d  origwinheight[%d] =%d\n", i, origwinwidth[i], i, origwinheight[i]);
    origwinwidth[i]  = int(MAINWINDOWWIDTHSIZE*adjust); //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ in 11/6/07 
    origwinheight[i] = int(MAINWINDOWWIDTHSIZE*adjust); //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ in 11/6/07
    

  
    lastwinwidth[i]  = origwinwidth[i];
    lastwinheight[i] = origwinheight[i];


    //origmainwinwidth[i]  =  parent()->parent()->parent()->w();
    //origmainwinheight[i] =  parent()->parent()->parent()->h(); 
    //origmainwinwidth[i]  =  (int)((parent()->parent()->parent()->w())*adjust); //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ out  11/6/07
    //origmainwinheight[i] =  (int)((parent()->parent()->parent()->h())*adjust); //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ out  11/6/07

    origmainwinwidth[i]  =  origwinwidth[i];                   //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ in 11/6/07 
    origmainwinheight[i] =  origwinheight[i] + int(22*adjust); //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ in 11/6/07
    //note the 25 above is the height of the controls group which should NOT be hardcoded !!!//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 11/6/07

       
    pt0x[i] = 0.0;
    pt0y[i] = 0.0;

    zoomfactor[atframe] = oldzoomfactor[atframe] = 1.0; //++++++++++++++++++++++++++++++++++++++++++++++++++++++++ in test: added 3/7/08 

    size[i]   = oldsize[i] = 1.0;

    xshift[i] = 0.0;       
    yshift[i] = 0.0;   
          
  }
  //writetolog("finished initialization for frame parameters");

  if(runmode == 1) {  
   
    cnsstartup();  //Reads the science model's drawing commands from stdin (in tcp/ip socket mode) which is provided via xinetd

    LISTOFCOMMANDS::iterator frit;
    frit = frametracker[atframe]; //frame pointed to by the iterators in frametracker[n]
    left = ((Newframe*)(*frit))->left;
    right = ((Newframe*)(*frit))->right;
    bottom = ((Newframe*)(*frit))->bottom;
    top = ((Newframe*)(*frit))->top;

  }

}//end of FrameView::FrameView


#if 0
//see next version of home which is implemented to "best fit" current frame to window rather than reset to original frame view
void FrameView::home(void) {
    //if(firsttime[atframe] == true)
    //    return; //cannot reset to original settings if frame has not been visited yet

    bool skipsize;
    int deltax = abs( lastwinwidth[atframe] - origwinwidth[atframe]);
    int deltay = abs( lastwinheight[atframe] - origwinheight[atframe]);
    //cout<<"in home: deltax = "<<deltax<<" deltay = "<<deltay<<endl;
    if(deltax < 9 && deltay < 9) //9 pixel tolerance abitrary - that is we choose to redraw but not resize if resize is very small
      skipsize = true;
    else
      skipsize = false; 
      
    lastwinwidth[atframe]  = origwinwidth[atframe];
    lastwinheight[atframe] = origwinheight[atframe];
    //pt0x[atframe] = 0.0;
    //pt0y[atframe] = 0.0;
    pt0x[atframe] = (right - left)/2.0;
    pt0y[atframe] = (top - bottom)/2.0;
    size[atframe] = oldsize[atframe] = 1.0;
    xshift[atframe]=0.0;
    yshift[atframe]=0.0;
    zoomfactor[atframe] = oldzoomfactor[atframe] = 1.0;
    ptrzoom->value(1.0); //when initializing the zoom widget should reflect the initial size which is 1.0

    pfvui->zoomadjuster->value(size[atframe]);     //support for adjuster widget
    pfvui->zoomadjuster->do_callback();            //support for adjuster widget
  
    xshift[atframe]=0.0;
    yshift[atframe]=0.0;
    bkgrndwhite[atframe] = false;

    setorigin = false;
    ptrorigin->value(0);
  
    ptrnetdialog->hide();
    fl_up_count    =  0;
    fl_down_count  =  0;
    current_line_thickness = 0;
    cursor(FL_CURSOR_DEFAULT);

    
    if(skipsize == false) {
      int mainlesswinht =  parent()->parent()->parent()->h() - this->h();
      parent()->parent()->parent()->size(lastwinwidth[atframe], lastwinheight[atframe] + mainlesswinht); //let the desktop manager position the main window
      //parent()->parent()->parent()->size(lastwinwidth[atframe], lastwinheight[atframe]); //let the desktop manager position the main window
    }

    redraw();
}
#endif

void FrameView::home(void) {

    //if(firsttime[atframe] == true)
    //    return; //cannot reset to original settings if frame has not been visited yet

    bool skipsize;
    int deltax = abs( lastwinwidth[atframe] - origwinwidth[atframe]);
    int deltay = abs( lastwinheight[atframe] - origwinheight[atframe]);
    //cout<<"in home: deltax = "<<deltax<<" deltay = "<<deltay<<endl;

    if(deltax < 9 && deltay < 9) //9 pixel tolerance abitrary - that is we choose to redraw but not resize if resize is very small
      skipsize = true;
    else
      skipsize = false; 
         

    //lastwinwidth[atframe]  = origwinwidth[atframe];  //+++++++++++++++++++++++++++++++++++++++ commented out for best fit
    //lastwinheight[atframe] = origwinheight[atframe]; //+++++++++++++++++++++++++++++++++++++++ commented out for best fit


    //pt0x[atframe] = 0.0;
    //pt0y[atframe] = 0.0;
    pt0x[atframe] = (right - left)/2.0;
    pt0y[atframe] = (top - bottom)/2.0;


    //size[atframe] = oldsize[atframe] = 1.0;         //+++++++++++++++++++++++++++++++++++++++ commented out for best fit
    size[atframe] = oldsize[atframe] = zoomfactor[atframe] = oldzoomfactor[atframe]; //new for fit

    xshift[atframe]=0.0;
    yshift[atframe]=0.0;

    //zoomfactor[atframe] = oldzoomfactor[atframe] = 1.0;  //+++++++++++++++++++++++++++++++++++++++ commented out for best fit

    ptrzoom->value(1.0); //when initializing the zoom widget should reflect the initial size which is 1.0 //+++++++++++++++++++++++++++++++++++++++ commented out for best fit
    ptrzoom->value(size[atframe]);

    pfvui->zoomadjuster->value(size[atframe]);     //support for adjuster widget
    pfvui->zoomadjuster->do_callback();            //support for adjuster widget
  
    xshift[atframe]=0.0;
    yshift[atframe]=0.0;
    // bkgrndwhite[atframe] = false;  //++++++++++++++++++++++++++++++++++++++ don't change background on fit

    setorigin = false;
    ptrorigin->value(0);
  
    ptrnetdialog->hide();
    fl_up_count    =  0;
    fl_down_count  =  0;
    current_line_thickness = 0;
    cursor(FL_CURSOR_DEFAULT);

    
    if(skipsize == false) {
      int mainlesswinht =  parent()->parent()->parent()->h() - this->h();
      parent()->parent()->parent()->size(lastwinwidth[atframe], lastwinheight[atframe] + mainlesswinht); //let the desktop manager position the main window
      //parent()->parent()->parent()->size(lastwinwidth[atframe], lastwinheight[atframe]); //let the desktop manager position the main window
    }

    redraw();

}

void FrameView::init(void) {

  //init() sets frame viewing parameters
  //init() leaves the user in the frame currently being viewed

  //static double previouspagewidth, previouspageheight; //replaced by array

  //if we are NOT at the first frame at the start of the program then
  //propagate the visual settings to the atframe frame from the previous frame
  //if the previous frame has the same page dimensions as the atframe frame
  //sprintf(msglog, "atframe = %d pagewidth = %g pageheight = %g and previousframe = %d", atframe, (right-left), (top - bottom), previousframe);
  //printf("in init 200: %s\n", msglog);
  //writetolog(msglog);

  //sprintf(msglog, "**** upon entry to init() line %d: atframe = %d pagewidth = %g pageheight = %g and previousframe = %d with previouspagewidth = %g  previouspageheight = %g",__LINE__, atframe, (right-left), (top - bottom), previousframe, previouspagewidth[previousframe], previouspageheight[previousframe]  ); //++++++ debug
  // printf("%s\n", msglog);  //++++++++++++++++++++ debug 


  if(!firsttime[1]) {    

    //get (from stored ?) left/right and top/bottom for previous and at frames
    //and deftermine if we should propogate or continue with the rest of init
 
    //sprintf(msglog, "init(): atframe = %d pagewidth = %g pageheight = %g and previousframe = %d\n", atframe, (right-left), (top - bottom), previousframe);
    // printf("%s\n", msglog);  
   
    if( (previouspagewidth[previousframe]  == (right - left))  &&  (previouspageheight[previousframe] == (top - bottom)) ) {
      //copy all of the previous page's view settings into current page
      //sprintf(msglog, "INHERITING: atframe = %d pagewidth = %g pageheight = %g and previousframe = %d\n", atframe, (right-left), (top - bottom), previousframe);
      //writetolog(msglog);
      //printf("not first time and inheritance is required\n");
      //printf("%s\n", msglog);                       

      xshift[atframe]           = xshift[previousframe]; 
      yshift[atframe]           = yshift[previousframe];
      pt0x[atframe]             = pt0x[previousframe]; 
      pt0y[atframe]             = pt0y[previousframe];

      origwinwidth[atframe]     = origwinwidth[previousframe];
      origwinheight[atframe]    = origwinheight[previousframe];
      origmainwinwidth[atframe] = origmainwinwidth[previousframe];

      //printf("%s line %d: lastwinwidth[%d] = %d and lastwinwidth[%d] = %d\n",__FILE__,__LINE__, atframe, lastwinwidth[atframe], previousframe, lastwinwidth[previousframe]);
      lastwinwidth[atframe]     = lastwinwidth[previousframe];
      //printf("%s line %d: lastwinwidth[%d] = %d and lastwinwidth[%d] = %d\n",__FILE__,__LINE__, atframe, lastwinwidth[atframe], previousframe, lastwinwidth[previousframe]);

      lastwinheight[atframe]    = lastwinheight[previousframe];
      size[atframe]             = size[previousframe];
      oldsize[atframe]          = oldsize[previousframe];
      zoomfactor[atframe]       = zoomfactor[previousframe];
      oldzoomfactor[atframe]    = oldzoomfactor[previousframe];
    
      sizew[atframe] = sizew[previousframe];
      sizeh[atframe] = sizeh[previousframe];

      //sprintf(msglog, "in init() at line %d inheritance atframe %d with sizew = %g and sizeh = %g\n",  __LINE__, atframe, sizew[atframe], sizeh[atframe]);
      //sprintf(msglog, "in init() at line %d inheritance atframe %d previousframe %d with size = %g\n", __LINE__, atframe, previousframe,  size[atframe]);
      //writetolog(msglog);

      //ptrzoom->value(size[atframe]); //needed for exponential slider ++++++++++++++++++++++++++++++++++++++++++ experiment for new
      //printf("mfdraw @ line %d: atframe = %d  size = %f  with sizestep = %g\n", __LINE__, atframe, size[atframe],  sizestep);

       
      //printf("%s %d end of init before size call: lastwinwidth[%d] = %d\n", __FILE__ , __LINE__ , atframe, lastwinwidth[atframe]);
      int mainlesswinht =  parent()->parent()->parent()->h() - this->h(); //we need to add the controls group's height to opengl window height for main window height

      parent()->parent()->parent()->size(lastwinwidth[atframe], lastwinheight[atframe] + mainlesswinht);
      //parent()->parent()->parent()->size(lastwinwidth[atframe], lastwinheight[atframe]);
      //printf("%s %d end of init after size call: lastwinwidth[%d] = %d\n", __FILE__ , __LINE__ , atframe, lastwinwidth[atframe]);
     
      previouspagewidth[atframe]  = right - left; 
      previouspageheight[atframe] = top - bottom; 

      firsttime[atframe] = false;
      return;
    } //end of page size same as previous page   
  }//end of not firsttime


   previouspagewidth[atframe]  = right - left;
   previouspageheight[atframe] = top - bottom;


  if(firsttime[atframe] == true) {

    if( (right - left) > (top - bottom)) {
      origwinwidth[atframe] = int( origwinwidth[atframe] * (right-left)/(top-bottom) );
    }

    if( (right - left) < (top - bottom)) {
      origwinheight[atframe] = int( origwinheight[atframe] * (top-bottom)/(right - left) );
    }

    //sprintf(msglog,"first time atframe %d origwinheight = %d", atframe,  origwinheight[atframe]);
    //writetolog(msglog);
 
    origmainwinwidth[atframe] =  origwinwidth[atframe];

    int mainlesswinht =  parent()->parent()->parent()->h() - this->h();      
    //printf("atframe = %d with mainlesswinht = %d\n", atframe, mainlesswinht);
    origmainwinheight[atframe] = origwinheight[atframe] + mainlesswinht;                  
  
    lastwinwidth[atframe]  = origwinwidth[atframe];
    lastwinheight[atframe] = origwinheight[atframe];
    
    size[atframe] = oldsize[atframe] = 1.0;
    zoomfactor[atframe] = oldzoomfactor[atframe] = 1.0;

    ptrzoom->value(1.0); //when initializing the zoom widget should reflect the initial size which is 1.0
    pfvui->zoomadjuster->value(1.0);      //support for adjuster widget

    //pfvui->zoomadjuster->do_callback();
    pfvui->zoomdisplay->label("1.0");  

    xshift[atframe]=0.0;
    yshift[atframe]=0.0;
    pt0x[atframe] = (right - left)/2.0; //reset to center of the window
    pt0y[atframe] = (top - bottom)/2.0; //reset to center of the window

    //sprintf(msglog,"first time atframe %d size = %g", atframe, size[atframe]);
    //writetolog(msglog);


  } //end of first time 


  //#if 0
  if( atframe <= 1 && firsttime[atframe]) {
    //printf("kick the video card at frame %d\n", atframe);
    Fl::damage(1);
    Fl::flush();
  }
  //#endif


  bkgrndwhite[atframe] = false;
  setorigin = false;

  //linear slider
  ptrzoom->value(size[atframe]);
  sizestep = 0.02;
  sizemin =  0.5; 
  sizemax =  100;


  //new exponential zoom parameters follow:
  //sizestep = 0.02;
  //sizemin =  0.5; 
  //sizemax =  5.0;

  sizew[atframe] = size[atframe]*(float(origwinwidth[atframe])/this->w()); 
  sizeh[atframe] = size[atframe]*(float(origwinheight[atframe])/this->h());
  //sprintf(msglog, "in init()at line %d atframe = %d and sizew = %g sizeh = %g",__LINE__,  atframe, sizew[atframe], sizeh[atframe]); //++++++++++++++++++++++++++=11/07/07
  //writetolog(msglog);                                                                                                               //++++++++++++++++++++++++++=11/07/07

  ptrorigin->value(0);  
  ptrnetdialog->hide();
  fl_up_count    =  0;
  fl_down_count  =  0;
  current_line_thickness = 0;
  cursor(FL_CURSOR_DEFAULT);
  /*
  if(firsttime[0]) { //there is no 0 frame so we can use it to flag start of program
    //this unnecessary + and - sizing is too 'prevent' some bug that is preventing redraw()
    origmainwinheight[atframe] += 1;
    parent()->parent()->parent()->size(origmainwinwidth[atframe], origmainwinheight[atframe]); //let the desktop manager position the main window
    origmainwinheight[atframe] -= 1;
    parent()->parent()->parent()->size(origmainwinwidth[atframe], origmainwinheight[atframe]); //let the desktop manager position the main window
    firsttime[0] = false;
  }
  else {
  */

  int mainlesswinht =  parent()->parent()->parent()->h() - this->h();
  parent()->parent()->parent()->size(lastwinwidth[atframe], lastwinheight[atframe] + mainlesswinht);
  //parent()->parent()->parent()->size(lastwinwidth[atframe], lastwinheight[atframe]);
  //printf("atframe = %d with mainlesswinht = %d\n", atframe, mainlesswinht); 
   
  //}

  previouspagewidth[atframe]  = right - left;
  previouspageheight[atframe] = top - bottom;    

  firsttime[atframe] = false; 
   
}//end of init()

//opengl drawing commands follow

/*
void FrameView::drawCircle(double x1, double y1, double r) {
  const double PI= 3.1415926535897;
  double angle;  
  GLint circle_points = 180; 
  glBegin(GL_LINE_LOOP); 
  for (int i = 0; i < circle_points; i++) {    
    angle = 2*PI*i/circle_points; 
    glVertex2f(r*cos(angle)+x1, r*sin(angle)+y1); 
  } 
  glEnd();
}

void FrameView::drawFilledcircle(double x1, double y1, double r) {
  const double PI= 3.1415926535897;
  double angle;  
  GLint circle_points = 180; 
  glBegin(GL_POLYGON); 
  for (int i = 0; i < circle_points; i++) {    
    angle = 2*PI*i/circle_points; 
    glVertex2f(r*cos(angle)+x1, r*sin(angle)+y1); 
  } 
  glEnd();
}
*/

void FrameView::drawCircle(double x1, double y1, double r) {
  int numpix = int(6.28318*r*(this->h()/(top-bottom))*(ptrzoom->value())); //(length of circumference)*(pixels per inch)*zoom ==> num pixels needed to draw circle
  if(numpix < 3)
    numpix = 3;
  else if(numpix > 360)
    numpix = 360;
  int increment = 360/numpix; 
  //cout<<"cricle increment = "<<increment<<" with r = "<<r<<endl;  
  glBegin(GL_LINE_LOOP); 
  for (int i = 0; i < circledata.getcirclepoints(); i += increment) {    
    glVertex2f(r*circledata.getmycos(i)+x1, r*circledata.getmysin(i)+y1);
  }
  glEnd();
}

void FrameView::drawFilledcircle(double x1, double y1, double r) { 
  int numpix = int(6.28318*r*(this->h()/(top-bottom))*(ptrzoom->value())); //(length of circumference)*(pixels per inch)*zoom ==> num pixels needed to draw circle
  if(numpix < 3)
    numpix = 3;
  else if(numpix > 360)
    numpix = 360; 
  int increment = 360/numpix; 
  //cout<<"filled cricle increment = "<<increment<<" with r = "<<r<<endl;  
  glBegin(GL_POLYGON); 
  for (int i = 0; i < circledata.getcirclepoints(); i += increment) {    
    glVertex2f(r*circledata.getmycos(i)+x1, r*circledata.getmysin(i)+y1);
  } 
  glEnd();
}

void FrameView::drawOutlineellipse(double x, double y, double w, double h, double inclineangle) {
  //incline angle not yet implemented
  double r;
  (w >= h) ? r=w : r=h; //approx. ellipse with a circle instead of using first terms of an infinite series that approximates perimenter of ellipse
  int numpix = int(6.28318*r*(this->h()/(top-bottom))*(ptrzoom->value())); //(length of circumference)*(pixels per inch)*zoom ==> num pixels needed to draw circle
  if(numpix < 3)
    numpix = 3;
  else if(numpix > 360)
    numpix = 360; 
  int increment = 360/numpix; 
  //cout<<"outlined ellipse increment = "<<increment<<" with r = "<<r<<endl;
  glBegin(GL_LINE_LOOP); 
    for (int i = 0; i < circledata.getcirclepoints(); i += increment) { 
      glVertex2f(x + w*circledata.getmycos(i), y + h*circledata.getmysin(i)); 
  } 
  glEnd();
}

void FrameView::drawFilledellipse(double x, double y, double w, double h, double inclineangle) {
  //incline angle not yet implemented
  double r;
  (w >= h) ? r=w : r=h; //approx. ellipse with a circle instead of using first terms of an infinite series that approximates perimenter of ellipse
  int numpix = int(6.28318*r*(this->h()/(top-bottom))*(ptrzoom->value())); //(length of circumference)*(pixels per inch)*zoom ==> num pixels needed to draw circle
  if(numpix < 3)
    numpix = 3;
  else if(numpix > 360)
    numpix = 360; 
  int increment = 360/numpix; 
  //cout<<"filled ellipse increment = "<<increment<<" with r = "<<r<<endl;
  glBegin(GL_POLYGON); 
  for (int i = 0; i < circledata.getcirclepoints(); i += increment) {
     glVertex2f(x + w*circledata.getmycos(i), y + h*circledata.getmysin(i));
  } 
  glEnd();
}

void FrameView::drawLine(double x1, double y1, double x2, double y2){
  glLineWidth(1.0 + current_line_thickness);
  glBegin(GL_LINES);
  glVertex2f(x1,y1);
  glVertex2f(x2,y2);
  glEnd();
}

//=============================================== start tesselation work area =====================================
/*  the callback routines registered by gluTessCallback() */
void vertexCallback(GLdouble *v)
{
  glVertex2dv(v);
}
void beginCallback(GLenum which)
{
   glBegin(which);
}
void endCallback(void)
{
   glEnd();
}
void errorCallback(GLenum errorCode)
{
   const GLubyte *estring;
   estring = gluErrorString(errorCode);
   fprintf (stderr, "Tessellation Error: %s\n", estring);
   exit (0);
}
void FrameView::drawFillclosepolyline(int np, double *c ) {
#if 0
  //code that supports convex hulls ONLY. replaced by tessellation approach which permits
  //concave as well as convex hulls.
  writetolog("In mfdraw.cxx in drawFillclosepolyline() drawing convex polygons"); 
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_DOUBLE, 0, c);
  glDrawArrays(GL_POLYGON, 0, np);
#endif

  //#if 0
  //writetolog("**In mfdraw.cxx in drawFillclosepolyline() drawing convex or concave polygons using polygon tesellation ");
  
 
  GLUtesselator * tobj;
  tobj = gluNewTess();
  //gluTessCallback(tobj, GLU_TESS_VERTEX,(GLvoid (*) ()) &vertexCallback); //Registering Tessellation Callbacks
  gluTessCallback(tobj, GLU_TESS_VERTEX,(GLvoid (*) ()) &glVertex3dv); //Registering Tessellation Callbacks
  gluTessCallback(tobj, GLU_TESS_BEGIN, (GLvoid (*) ()) &beginCallback);
  gluTessCallback(tobj, GLU_TESS_END,   (GLvoid (*) ()) &endCallback);
  gluTessCallback(tobj, GLU_TESS_ERROR, (GLvoid (*) ()) &errorCallback);
  //gluTessProperty(tobj, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE );
  //gluTessProperty(tobj, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NEGATIVE );
  gluTessProperty(tobj,GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD );
  //gluTessProperty(tobj,GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO );

  //Begins and ends the specification of a polygon to be tessellated and associates a tessellation object, tobj, with it.
  //c points to a user-defined data structure, which is passed along all the GLU_TESS_*_DATA callback functions that have been bound.
  gluTessBeginPolygon (tobj, c);
  gluTessBeginContour(tobj);

  for(int i = 0; i <np; i++) { //clockwise
    gluTessVertex(tobj, c+3*i, c+3*i); //clockwise
    //cout<<"vertex number "<<i+1<<" = ("<<*(c+3*i)<<","<<*(c+3*i+1)<<")"<<endl;
  }

  gluTessEndContour(tobj);
  gluTessEndPolygon (tobj);

  //#endif
  
}
//===================================================end tesselation work area ====================================

void FrameView::drawOutlineclosepolyline(int np, double *c ) {
  //writetolog("in mfdraw.cxx: in drawOutlineclosepolyline()");
  glBegin(GL_LINE_LOOP);
  for(int i = 0; i < 2*np; i += 2) {
    glVertex2d(c[i], c[i+1]);    
  }
  glEnd();
}
void FrameView::drawOpenpolyline(int np, double *c ) {
  //writetolog("in drawOpenpolyline()");
  glBegin(GL_LINE_STRIP);
  for(int i = 0; i < 2*np; i += 2) {
    glVertex2d(c[i], c[i+1]);
   }
  glEnd();
}

void FrameView::drawOutlinesquare(double x, double y, double e) {
  //x,y coordinates of lower left-hand corner of square
  //e = edge of square
  glBegin(GL_LINE_LOOP);
  glVertex2f(x,y);
  glVertex2f(x+e,y);
  glVertex2f(x+e,y+e);
  glVertex2f(x,y+e);
  glEnd();
}

void FrameView::drawFilledsquare(double x, double y, double e) {
  //x,y coordinates of lower left-hand corner of square
  //e = edge of square
  glRectd(x,y,x+e,y+e); 
}

void FrameView::drawOutlinerectangle(double x, double y, double e1, double e2) {
  //x,y coordinates of lower left-hand corner of square
  //e1 = edge of x side of rectangle
  //e2 = edge of y side of rectangle
  glBegin(GL_LINE_LOOP);
  glVertex2f(x,y);
  glVertex2f(x+e1,y);
  glVertex2f(x+e1,y+e2);
  glVertex2f(x,y+e2);
  glEnd();
}

void FrameView::drawFilledrectangle(double x, double y, double e1, double e2) {
  //x,y coordinates of lower left-hand corner of square
  //e1 = edge of x side of rectangle
  //e2 = edge of y side of rectangle
  glRectd(x,y,x+e1,y+e2); 
}

void FrameView::colorname(string s) {
  if(s == "RED") {
    glColor3f(1.0, 0.0, 0.0);
    return;
  }
  if(s == "YELLOW") {
    glColor3f(1.0, 1.0, 0.0);
    return;
  }
  if(s == "MAGENTA") {
    glColor3f(1.0, 0.0, 1.0);
    return;
  }
  if(s == "CYAN") {
    glColor3f(0.0, 1.0, 1.0);
    return;
  }
  if(s == "ORANGE") {
    glColor3f(1.0, .65, 0.0);
    return;
  }
  if(s == "BLACK") {
    if(!bkgrndwhite[atframe]) {
      glColor3f(1.0, 1.0, 1.0);            //if background is not white (i.e. it is black) then do not color in black but color in white
    } else {
      glColor3f(0.0, 0.0, 0.0);
    }
    return;
  }
  if(s == "BLUE") {
    glColor3f(0.0, 0.0, 1.0);
    return;
  }
  if(s == "GREEN") {
    glColor3f(0.0, 1.0, 0.0);
    return;
  }
  if(s == "WHITE") {
    if(bkgrndwhite[atframe]) {            //if background is white then do not color in white but color in black
      glColor3f(0.0, 0.0, 0.0);
    } else
    glColor3f(1.0, 1.0, 1.0);
    return;
  }
  if(s == "VIOLET") {
    glColor3f(.50, 0.0, .75);
    return;
  }
  //cout<<"Error: color "<<s<<" in colorname not defined"<<endl;
  writetolog("Error: colorname not defined");
  exit(1); 
}

void FrameView::colorbgr(int b, int g, int r) {
  //printf("colorbgr: b = %d g = %d r = %d\n", b, g, r);
  //fflush(stdout);

  if(b == 0 && g == 0 && r == 0 &&  !bkgrndwhite[atframe]) {   //if drawing color is black and background black make drawing color white
    glColor3f(1.0, 1.0, 1.0);
    //printf("in file %s line %d command colorbgr\n", __FILE__, __LINE__);
    return;
  }
  if(b == 15 && g == 15 && r == 15 && bkgrndwhite[atframe]) {   //if drawing color is black and background black make drawing color white
    glColor3f(0.0, 0.0, 0.0);
    //printf("in file %s line %d command colorbgr\n", __FILE__, __LINE__);
    return;
  }
  float blue  = b/15.0;
  float green = g/15.0;
  float red   = r/15.0;
  glColor3f(red, green, blue);
}

void FrameView::colorbbggrr(int b, int g, int r) {

  //printf("colorbbggrr:  b = %d g = %d r = %d\n", b, g, r);
  //fflush(stdout);

  if(b == 0 && g == 0 && r == 0 &&  !bkgrndwhite[atframe]) {   //if drawing color is black and background black make drawing color white
    glColor3f(1.0, 1.0, 1.0);
    //printf("in file %s line %d command colorbgr\n", __FILE__, __LINE__);
    return;
  }
  if(b == 255 && g == 255 && r == 255 && bkgrndwhite[atframe]) {   //if drawing color is black and background black make drawing color white
    glColor3f(0.0, 0.0, 0.0);
    //printf("in file %s line %d command colorbgr\n", __FILE__, __LINE__);
    return;
  }
  float blue  = b/255.0;
  float green = g/255.0;
  float red   = r/255.0;

  //printf("blue = %g green = %g red = %g\n", blue, green, red);
  //fflush(stdout);

  glColor3f(red, green, blue);
}
//added colorbbggrr() on 6/20/2008



void FrameView::drawText(double x, double y, double ht, double angle, int len, string text) {
#if 0
  //start obsolete only supports horizontal text ---------------------------------------------
  //double numpixhigh = (ht/(double)(top-bottom))*(this->h())*size[atframe];
  double numpixhigh = (ht/(double)(top-bottom))*(this->h())*sizeh;
  if(numpixhigh < 10)
    numpixhigh = 10;
  if(numpixhigh > 14)
    numpixhigh = 14;
  //cout<<"numpixhigh = "<<numpixhigh<<endl;
  gl_font(FL_COURIER, (int)numpixhigh); 
  gl_draw(text.c_str(), (float)x, (float)y);
  //end obsolete -----------------------------------------------------------------------------
#endif

#if 0
  //start glut bitmap approach - works but vertical characters not rotated -------------------
  if(angle != 0.0 && angle != 90.0) {
    writetolog("error writing text whose angle is not 0 or 90");
    exit(1);
  }
  if(angle == 90) {
    glPushMatrix();
    double bitmapspaceheight = 12.0/72.0; //for 12 point helvetica   
    reverse(text.begin(), text.end());//reverse string for vertical display 
    const char *c = text.c_str();
    for (int i = 0; *c != '\0'; i++, c++) {
      glRasterPos2f(x, y + (bitmapspaceheight*i) );
      //glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
      glutBitmapCharacter( GLUT_BITMAP_HELVETICA_12, *c);
    }
    glPopMatrix();
  } 
  if(angle == 0) {
    double bitmapWidth = (12.0/72.0)*.75; //for 12 point helvetica 
    glPushMatrix(); 
    const char *c = text.c_str();
    for (int i = 0; *c != '\0'; i++, c++) {
      glRasterPos2f(x + bitmapWidth*i, y);
      //glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
    glPopMatrix();
  } 
  //end glut bitmap approach - works but vertical characters not rotated -------------------
 #endif 

  //glut stroke approach - requires libglut in make 
  if(angle == 0) { 
    glPushMatrix();  
    glTranslatef((float)x,(float)y,(float)0);
    glScalef(0.001, 0.001, 0.0);
    for (const char *c = text.c_str(); *c; c++) {
      //glutStrokeCharacter(GLUT_STROKE_ROMAN, *c); 
      glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *c);
    }
    glPopMatrix();
  }
  else { //angle != 0 
    glPushMatrix();
    glTranslatef((float)x,(float)y,(float)0);
    glRotatef(angle,0,0,1); 
    glScalef(0.001, 0.001, 0.0); 
    for (const char *c = text.c_str(); *c; c++) {
      //glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
      glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *c);
    }
    glTranslatef(-(float)x,-(float)y,(float)0);
    glPopMatrix();
  }  
     
} //end FrameView::drawText

void FrameView::drawBitmap(int type, int mode, float xc, float yc, int rowlen, int colht, int xofs, int yofs, int iwd, int iht, int length, char* buf) {

  //cout<<"drawing non-scalable bit map at frame "<<atframe<<endl; 

  //writetolog("in FrameView::drawBitmap()");
  //test code for opengl bit map
  // cout<<"FrameView::drawBitmap():  type = "<<type<<" mode = "<<mode<<" xc = "<<xc<<" yc = "<<yc<<" rowlen = "<<rowlen<<" colht = "<<colht<<" xofs = "
  //               <<xofs<<" yofs = "<<yofs<<" iwd = "<<iwd<<" iht = "<<iht<<" length = "<<length<<endl; //++++++++++++++++++++++++++ debug
  //end test code for opengl bit map


  glPushMatrix();

  if(bkgrndwhite[atframe])
    glColor3f(0.0, 0.0, 0.0);
  else
    glColor3f(1.0, 1.0, 1.0);

  //adjust for bitmap tile positioning when zooming
  double dxofs = xofs/sizew[atframe];
  double dyofs = yofs/sizeh[atframe];

  float xpixtoinches = (right-left)/(float)this->w();
  float ypixtoinches = (top - bottom)/(float)this->h();

  glRasterPos2f(xc+dxofs*xpixtoinches, yc+dyofs*ypixtoinches);

  if(mode == 0) { //set frame data to buf data
    glBitmap(iwd,iht,0,0,0,0,(const GLubyte*)buf);
  }

  if(mode == 1) { // xor buf data with existing frame data
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_XOR);
    glBitmap(iwd,iht,0,0,0,0,(const GLubyte*)buf); 
    glDisable(GL_COLOR_LOGIC_OP);   
  }

  if(mode == 2) { //and buf data with existing frame data
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_AND);
    glBitmap(iwd,iht,0,0,0,0,(const GLubyte*)buf); 
    glDisable(GL_COLOR_LOGIC_OP);   
  }

  if(mode == 3) { //if buf bit == 1 clear frame bit else leave frame bit as is
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_AND_INVERTED);
    glBitmap(iwd,iht,0,0,0,0,(const GLubyte*)buf); 
    glDisable(GL_COLOR_LOGIC_OP);   
  }

  if(mode > 3 || mode < 0) {
    writetolog("Error, mode not recognized");
    exit(1);
  }
  glPopMatrix();
}

//support for bit map gray scale  that is scalable and non-scalable
void FrameView::drawBitmapgrayscalescalable(int type, int mode, float xc, float yc, float bwd, float bht, int rowlen, int colht, int xofs, int yofs, int iwd, int iht, int length, char* buf) {

  //writetolog("in FrameView::drawBitmapgrayscalescalable()");
  //debug code for opengl bit map
  //cout<<" FrameView::drawBitmapgrayscalescalable  type = "<<type<<" mode = "<<mode<<" xc = "<<xc<<" yc = "<<yc<<" bwd = "<<bwd<<" bht = "<<bht
  //       <<" rowlen = "<<rowlen<<" colht = "<<colht<<" xofs = "
  //      <<xofs<<" yofs = "<<yofs<<" iwd = "<<iwd<<" iht = "<<iht<<" length = "<<length<<endl;
  //cout<<"in drawBitmapgrayscalescalable: right = "<<right<<" left = "<<left<<"  top = "<<top<<" bottom = "<<bottom<<endl;
  //end debug code for opengl bit map
  
  glPushMatrix();
  if(bkgrndwhite[atframe])
    glColor3f(0.0, 0.0, 0.0);
  else
    glColor3f(1.0, 1.0, 1.0);


 
  if( bwd != 0 && bht != 0) { 
  //SCALEABLE GREY SCALE BIT MAPS



#if 0    
    // experiment only does not take size in inches of the bit map into account 
    glPixelZoom(size[atframe],size[atframe]);  //glPixelZoom removed because NON SCALEABLE implies NON ZOOMABLE
    float xpixtoinches = (right - left)/((float)origwinwidth[atframe]);  //should be moved to initialization
    float ypixtoinches = (top - bottom)/((float)origwinheight[atframe]); //should be moved to initialization
    glRasterPos2f(xc + xofs*xpixtoinches, yc + yofs*ypixtoinches);
#endif



    // cout<<"draw scaleable: at frame = "<<atframe<<endl;
//     cout<<" FrameView::drawBitmapgrayscalescalable  type = "<<type<<" mode = "<<mode<<" xc = "<<xc<<" yc = "<<yc<<" bwd = "<<bwd<<" bht = "<<bht
//          <<" rowlen = "<<rowlen<<" colht = "<<colht<<" xofs = "
//          <<xofs<<" yofs = "<<yofs<<" iwd = "<<iwd<<" iht = "<<iht<<" length = "<<length<<endl;
//     cout<<"this frame has width in pixels = "<<(this->w())<<" and height in pixels = "<<(this->h())<<endl;
//     cout<<"this frame has sizew[atframe] = "<<sizew[atframe]<<"  sizeh[atframe] = "<<sizeh[atframe]<<endl;
//     cout<<"((float)origwinheight[atframe]) = "<<((float)origwinheight[atframe])<<"((float)origwinwidth[atframe]) = "<<((float)origwinwidth[atframe])<<endl;



//     float percentpageinchX = bwd/(right- left);
//     float percentpageinchY = bht/(top - bottom); 
//     float percentpagepixX  = (float)rowlen/(float)(this->w());
//     float percentpagepixY  = (float)colht/(float)(this->h()); 
//     float sizewide   = percentpageinchX/percentpagepixX;
//     float sizeheight = percentpageinchY/percentpagepixY;
//     glPixelZoom(sizewide*sizew[atframe], sizeheight*sizeh[atframe]);

    //alternative order of calculation to improve precision
    float sizewide   = (bwd*(float)(this->w())*sizew[atframe])/((right-left)*((float)rowlen));
    float sizeheight = (bht*(float)(this->h())*sizeh[atframe])/((top-bottom)*((float)colht));

   
    glPixelZoom(sizewide, sizeheight);



    //float xinchesperpixel = .970*(right - left)/((float)origwinwidth[atframe]); //the .970 correction factor is required ti counteract a BUG
    //cout<<"xinchesperpixel = "<<xinchesperpixel<<"  and (bwd*.5)/(float)iwd = "<<(bwd*.5)/(float)iwd<<" and bwd/(float)rowlen = "<<(bwd/(float)rowlen)<<endl;
    //float yinchesperpixel =  (top - bottom)/((float)origwinheight[atframe]);

    float xinchesperpixel = bwd/(float)rowlen;
    float yinchesperpixel = bht/(float)colht;

    glRasterPos2f(xc + xofs*xinchesperpixel, yc + yofs*yinchesperpixel);  //note no correction for zooming when computing offset. glPixelZoom has already been called.
  }
  else {
    // ________________________________ NON SCALEABLE GREY SCALE BIT MAPS _________________________________________________________________________________

//      cout<<"FrameView::drawBitmapgrayscalescalable line 763: draw non-scaleable"<<endl;
//      cout<<" FrameView::drawBitmapgrayscale:  type = "<<type<<" mode = "<<mode<<" xc = "<<xc<<" yc = "<<yc<<" bwd = "<<bwd<<" bht = "<<bht
//                <<" rowlen = "<<rowlen<<" colht = "<<colht<<" xofs = "
//      	<<xofs<<" yofs = "<<yofs<<" iwd = "<<iwd<<" iht = "<<iht<<" length = "<<length<<endl<<"-------- end of non-scalable gray scale bit map ------"<<endl;
    //glPixelZoom(size[atframe],size[atframe]);  //glPixelZoom removed because NON SCALEABLE implies NON ZOOMABLE
    float xpixtoinches = (right - left)/((float)origwinwidth[atframe]);  //should be moved to initialization
    float ypixtoinches = (top - bottom)/((float)origwinheight[atframe]); //should be moved to initialization


    //cout<<"xpixtoinches = "<<xpixtoinches<<"  ypixtoinches = "<<ypixtoinches<<"  sizew[atframe] = "<<sizew[atframe]<<"  sizeh[atframe] = "<<sizeh[atframe]<<endl;
    //cout<<"left = "<<left<<" right = "<<right<<" top = "<<top<<" bottom = "<<bottom<<endl;


    //glRasterPos2f(xc + xofs*xpixtoinches, yc + yofs*ypixtoinches);
    glRasterPos2f(xc +  xofs*xpixtoinches/size[atframe], yc + yofs*ypixtoinches/size[atframe]); 
  } 
  //cout<<"size = "<<size[atframe]<<" sizew = "<<sizew[atframe]<<" sizeh = "<<sizeh[atframe]<<"this->w() = "<<this->w()<<" this->h() = "<<this->h()<<endl;  
  //cout<<"origwinwidth[atframe] = "<<origwinwidth[atframe]<<"  origwinheight[atframe] = "<<origwinheight[atframe]<<endl;



  if (type == BM_C24) { 
    //cout<<"ready to draw pixels for type = "<<type<<" color bit map with width = "<<iwd<<" and height = "<<iht<<endl;  
 
    //glClearColor(0.0, 0.0, 0.0, 0.0);
    //above line replaced by:
    if(bkgrndwhite[atframe]) {
      glClearColor(1.0,1.0,1.0,1.0); //make bkgrnd white
    }
    else
      glClearColor(0.0,0.0,0.0,1.0); //make bkgrnd black


    glShadeModel(GL_FLAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


    //glClear(GL_COLOR_BUFFER_BIT); //commented out because if there is more than one bitmap per frame it will erase existing bit maps 


    glDrawPixels(iwd, iht, GL_RGB, GL_UNSIGNED_BYTE, (const GLubyte*)buf ); //see opengl redbook, version 1.2, third edition, page 296 & 301
    //  glDrawPixels(iwd, iht, GL_RGB, GL_UNSIGNED_BYTE, (const GLvoid *)buf ); //see opengl redbook, version 1.2, third edition, page 296 & 301
    glFlush();

  } else {
    glDrawPixels(iwd, iht, GL_LUMINANCE, GL_UNSIGNED_BYTE, (const GLubyte*)buf ); //see opengl redbook, version 1.2, third edition, page 296 & 301 
    glFlush();
  }

  glPixelZoom(1.0,1.0);
  glPopMatrix();   
}

void FrameView::linethick(int t) {
  current_line_thickness = t;
}


void FrameView::resize(int xp, int yp, int wd, int ht) {
  //linear version of resize
  //drawing is resized,if possible, to fill new window size

  //printf("in %s line %d size[1] = %g and wd = %d and origwinwidth = %d and ht = %d, origwinheight = %d and oldzoomfactor[1] = %g\n",__FILE__,  __LINE__, size[1], wd, origwinwidth[1], ht, origwinheight[1], oldzoomfactor[1]  ); //+++++++++++++++++++++++ test  3/7/08 
  
  double zoomfactor_w = (double(wd)/origwinwidth[atframe]);
  double zoomfactor_h = (double(ht)/origwinheight[atframe]); 
  zoomfactor[atframe] = (zoomfactor_w <= zoomfactor_h) ?  zoomfactor_w : zoomfactor_h;
  size[atframe] += (zoomfactor[atframe] - oldzoomfactor[atframe]);

  //printf("in %s line %d size[1] = %g \n",__FILE__,  __LINE__, size[1] ); //+++++++++++++++++++++++++++++++ test  3/7/08 
  

  if(size[atframe] != oldsize[atframe]) {
    ptrzoom->value(size[atframe]);
    zoomfunc();
    
    pfvui->zoomadjuster->value(size[atframe]);  //support for adjuster widget
    pfvui->zoomadjuster->do_callback();         //support for adjuster widget

  }

  double sw = size[atframe]*double(origwinwidth[atframe])/double(wd);
  double sh = size[atframe]*double(origwinheight[atframe])/double(ht);

  xshift[atframe] += sw*(pt0x[atframe])*((double(wd - lastwinwidth[atframe]))/(double)lastwinwidth[atframe]);
  yshift[atframe] += sh*(pt0y[atframe])*((double(ht - lastwinheight[atframe]))/(double)lastwinheight[atframe]);

  //#if 0 
  //filter out unnecessary resizes //+++++++++++++++++++++++++++++++++++++++++++++++++++ ifdefed out for adjuster
  int deltax =  abs(w()-wd);
  int deltay =  abs(h()-ht);
  int deltaxp = abs(x()-xp);
  int deltayp = abs(y()-yp);
  if( deltax == 0 && deltay == 0 && deltaxp == 0 && deltayp == 0 )
    return;  
  //end of filter
  //#endif
 
  //cout<<"mfdraw FrameView::resize(): atframe = "<<atframe<<" resize called with wd = "<<wd<<" and  ht = "<<ht<<" xp = "<<xp<<" yp = "<<yp<<endl;
  //cout<<"mfdraw FrameView::resize(): oldzoomfactor[atframe]="<<oldzoomfactor[atframe]<<"  zoomfactor[atframe]= "<<zoomfactor[atframe]<<endl;
  Fl_Gl_Window::resize(xp, yp, wd, ht);

  oldzoomfactor[atframe] = zoomfactor[atframe];

  lastwinwidth[atframe]  = w();
  lastwinheight[atframe] = h(); 
}


#if 0
void FrameView::resize(int xp, int yp, int wd, int ht) {
  //drawing is resized,if possible, to fill new window size
  double zoomfactor_w = (double(wd)/origwinwidth[atframe]);
  double zoomfactor_h = (double(ht)/origwinheight[atframe]); 



  //zoomfactor[atframe] = (zoomfactor_w <= zoomfactor_h) ?  zoomfactor_w : zoomfactor_h;
  double tmpzoomf = ((zoomfactor_w <= zoomfactor_h) ?  zoomfactor_w : zoomfactor_h); 

  
#if 0
  if (tmpzoomf > oldzoomfactor[atframe]) { //user made window larger
    if( size[atframe] < pow(sizemax,2) ) { 
      ptrzoom->value( ptrzoom->value() + (tmpzoomf - oldzoomfactor[atframe]) ); //this updates widget appearance but does not invoke callback
      zoomfactor[atframe] = tmpzoomf;
      size[atframe] = pow(ptrzoom->value(),2);  
      zoomfunc();
    } else {
      return;
    }
  } else { //user made window smaller
    if( size[atframe] > pow(sizemin,2) )  {
      ptrzoom->value( ptrzoom->value() + (tmpzoomf - oldzoomfactor[atframe]) ); //this updates widget appearance but does not invoke callback
      zoomfactor[atframe] = tmpzoomf;
      size[atframe] = pow(ptrzoom->value(),2);
      zoomfunc();
    }
    else {
      return;
    }
  }
#endif

 
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ start window size resize
  if (tmpzoomf > oldzoomfactor[atframe]) { //user made window larger
    if( size[atframe] < pow(sizemax,2) ) { 
      zoomfactor[atframe] = tmpzoomf;
      size[atframe] *= (1.0 + zoomfactor[atframe] - oldzoomfactor[atframe]);  
      zoomfunc();
    } else {
      return;
    }
  } else { //user made window smaller
    if( size[atframe] > pow(sizemin,2) )  {      
      zoomfactor[atframe] = tmpzoomf;
      size[atframe] *= (1.0 + zoomfactor[atframe] - oldzoomfactor[atframe]); 
      zoomfunc();
    }
    else {
      return;
    }
  }
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ end window size resize




  double sw = size[atframe]*double(origwinwidth[atframe])/double(wd);
  double sh = size[atframe]*double(origwinheight[atframe])/double(ht);
  xshift[atframe] += sw*(pt0x[atframe])*((double(wd - lastwinwidth[atframe]))/(double)lastwinwidth[atframe]);
  yshift[atframe] += sh*(pt0y[atframe])*((double(ht - lastwinheight[atframe]))/(double)lastwinheight[atframe]);

  //#if 0 
  //filter out unnecessary resizes 
  int deltax =  abs(w()-wd);
  int deltay =  abs(h()-ht);
  int deltaxp = abs(x()-xp);
  int deltayp = abs(y()-yp);
  if( deltax == 0 && deltay == 0 && deltaxp == 0 && deltayp == 0 )
    return;  
  //end of filter
  //#endif
 
  //cout<<"mfdraw FrameView::resize(): atframe = "<<atframe<<" resize called with wd = "<<wd<<" and  ht = "<<ht<<" xp = "<<xp<<" yp = "<<yp<<endl;
  //cout<<"mfdraw FrameView::resize(): oldzoomfactor[atframe]="<<oldzoomfactor[atframe]<<"  zoomfactor[atframe]= "<<zoomfactor[atframe]<<endl;
  Fl_Gl_Window::resize(xp, yp, wd, ht);

  oldzoomfactor[atframe] = zoomfactor[atframe];
  lastwinwidth[atframe]  = w();
  lastwinheight[atframe] = h(); 
}

#endif






//static int rc = 0; //for debug purposes only
void FrameView::draw() {

  if (!valid()) {
    glLoadIdentity();

    //bug: image cliping on zoom at origin
    //glViewport(0,0,w(),h());
    //printf("Debug: !valid() and window size is %d width by %d h()\n", w(), h());  
    //glOrtho(left,right,bottom,top,-1,1);
    //printf("Debug: current left = %g right = %g bottom = %g top = %g\n", left, right, bottom, top);  

    //second try on the above
    //     glViewport( -100, -100, w()+100, h()+100 );
    //     glOrtho(left,right,bottom,top,-1,1);
    //     glTranslatef((right-left)*100/w(), (top-bottom)*100/h(), 0.0);
    //     xshift[atframe] += (right-left)*100/w();
    //     yshift[atframe] += (top-bottom)*100/h();


    //resolves bug - no more image clipping on zoom at origin
     glViewport(-w(),-h(),2*w(),2*h());
     glOrtho(left,right,bottom,top,-1,1);
     glScalef(.5,.5, 1.0);
     glTranslatef( (right-left), (top-bottom), 0.0);

//     //resolves bug - no more image clipping on zoom at origin (bigger view port)
//      glViewport(-3*w(),-3*h(),4*w(),4*h());
//      glOrtho(left,right,bottom,top,-1,1);
//      glScalef(.25,.25, 1.0);
//      glTranslatef( 3*(right-left), 3*(top-bottom), 0.0);

    
  }
 

  if(bkgrndwhite[atframe]) {
    glClearColor(1.0,1.0,1.0,1.0); //make bkgrnd white
  }
  else
    glClearColor(0.0,0.0,0.0,1.0); //make bkgrnd black
    
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);   //initializes the screen


  //if(sListofCommands.size() == 0) { 
  if(sListofCommands.empty()) {
    return; 
  }
  


   
  glPushMatrix();

  glTranslatef(xshift[atframe], yshift[atframe], 0);
  //--------- maintain original gl window aspect ratio ---------

  //sprintf(msglog, "in draw() line 617 atframe = %d and sizeh = %g and origwinheight = % g", atframe, sizeh[atframe],(float(origwinheight[atframe])) );
  //printf("%s\n",msglog); //+++++++++++++++++ debug only
  //writetolog(msglog);

  sizew[atframe] = size[atframe]*(float(origwinwidth[atframe])/w());
  sizeh[atframe] = size[atframe]*(float(origwinheight[atframe])/h());

  //sprintf(msglog, "in draw() line 665 atframe = %d and sizeh = %g and origwinheight = % g", atframe, sizeh[atframe],(float(origwinheight[atframe])) );
  //writetolog(msglog);

  glScalef(sizew[atframe], sizeh[atframe], 0); //we have collapsed the z axis!

  //it appears that we should use -DMESA when under os x (MAC) even though it appears that we are not using MESA opengl
#ifndef MESA
  glDrawBuffer(GL_FRONT_AND_BACK);
  //cout<<"don't have mesa front and back"<<endl;
#endif // !MESA


  drawFrame();


  //it appears that we should use -DMESA when under os x (MAC) even though it appears that we are not using MESA opengl
#ifndef MESA
  glDrawBuffer(GL_BACK);
  //cout<<"don't have mesa back"<<endl;
#endif // !MESA

  glFlush();
  glPopMatrix();

}

void FrameView::drawFrame(void) {

  static LISTOFCOMMANDS::iterator listit;
  if(listit == sListofCommands.begin()) { 
    listit = sListofCommands.end(); //ListofCommands is fifo queue 
  }
  else {
    listit = frametracker[atframe];    
  }
  while(listit-- != sListofCommands.begin()) {
    //commandcount++; for debug only
    //char msglog[300];
    //sprintf(msglog, "Debug: inframe is  %d drawing frame %d and current drawing command is %d\n",inframe, atframe, (*listit)->cmd);
    //writetolog(msglog);
    if(advanceframe)
      listit++; //we want the NEWFRAME record if we just advanced the frame
    switch ( (*listit)->cmd ) {  

    case ENDOFFILE : {
      //printf("Debug: you hit an end of file and in frame %d\n", atframe);
      listit = frametracker[atframe];     //reprocess this last frame next time we draw, advance frame only through gui.
      inframe = true;
      return;
    }
    case NEWFRAME : {
      if(advanceframe) {
	//writetolog("Debug: you advanced the frame via the gui.");
	LISTOFCOMMANDS::iterator frit;
	frit = frametracker[atframe]; //first frame is is pointed to by the iterator frametracker[1]
	left = ((Newframe*)(*frit))->left;
	right = ((Newframe*)(*frit))->right;
	bottom = ((Newframe*)(*frit))->bottom;
	top = ((Newframe*)(*frit))->top;
	advanceframe = false;
        inframe = true;
	break; //in effect continue processing draw commands for this frame
      }
      if(!inframe) {
	inframe = true;
	break; //in effect continue processing draw commands for this frame
      }
      if(inframe) {
        //printf("you got a newframe command while in frame %d\n",atframe); 
	listit = frametracker[atframe];     //reprocess current frame next time we draw and do not advance
	return;
      }
    }
    case DRWLINE : {
      drawLine( ( (Line*)(*listit) )->x1,( (Line*)(*listit) )->y1, ( (Line*)(*listit) )->x2, ( (Line*)(*listit) )->y2 );
      break;
    }
    case DRWCIRCLE : {
      drawCircle( ( (Circle*)(*listit) )->x,( (Circle*)(*listit) )->y, (( Circle*)(*listit) )->r);
      break;
    }
    case DRWFILLEDCIRCLE : {
      drawFilledcircle( ( (Filledcircle*)(*listit) )->x,( (Filledcircle*)(*listit) )->y, (( Filledcircle*)(*listit) )->r);
      break;
    }   
    case DRWOUTLINEELLIPSE : {
      drawOutlineellipse( ( (Outlineellipse*)(*listit) )->x,( (Outlineellipse*)(*listit) )->y, ( (Outlineellipse*)(*listit) )->w,( (Outlineellipse*)(*listit) )->h, 
			   (( Outlineellipse*)(*listit) )->angle);
      break;
    }
    case DRWFILLEDELLIPSE : {
      drawFilledellipse( ( (Filledellipse*)(*listit) )->x,( (Filledellipse*)(*listit) )->y,  ( (Filledellipse*)(*listit) )->w,( (Filledellipse*)(*listit) )->h,
			 (( Filledellipse*)(*listit) )->angle);
      break;
    } 
    case DRWFILLCLOSEPOLYLINE : {
      drawFillclosepolyline(  ( (Fillclosepolyline*)(*listit) )->np, ( (Fillclosepolyline*)(*listit) )->c );
      break;
    }
    case DRWOUTLINECLOSEPOLYLINE : {
      drawOutlineclosepolyline(  ( (Outlineclosepolyline*)(*listit) )->np, ( (Outlineclosepolyline*)(*listit) )->c );
      break;
    }
 
    case DRWOPENPOLYLINE : {
      drawOpenpolyline( ( (Openpolyline*)(*listit) )->np,( (Openpolyline*)(*listit) )->c );
      break;
    }

    case DRWOUTLINESQUARE : {
      drawOutlinesquare( ( (Outlinesquare*)(*listit) )->x,( (Outlinesquare*)(*listit) )->y, ( (Outlinesquare*)(*listit) )->e);
      break;
    }
    case DRWFILLEDSQUARE : {
      drawFilledsquare( ( (Filledsquare*)(*listit) )->x,( (Filledsquare*)(*listit) )->y, ( (Filledsquare*)(*listit) )->e);
      break;
    }

    case DRWOUTLINERECTANGLE : {
      drawOutlinerectangle( ( (Outlinerectangle*)(*listit) )->x,( (Outlinerectangle*)(*listit) )->y, ( (Outlinerectangle*)(*listit) )->e1,
                            ( (Outlinerectangle*)(*listit) )->e2);
      break;
    }

    case DRWFILLEDRECTANGLE : {
      drawFilledrectangle( ( (Filledrectangle*)(*listit) )->x,( (Filledrectangle*)(*listit) )->y, ( (Filledrectangle*)(*listit) )->e1,
                           ( (Filledrectangle*)(*listit) )->e2);
      break;
    }

    case DRWBITMAP : {
      drawBitmap(  ( (Bitmap*)(*listit) )->type,  ( (Bitmap*)(*listit) )->mode,  ( (Bitmap*)(*listit) )->xc,     ( (Bitmap*)(*listit) )->yc,  
	           ( (Bitmap*)(*listit) )->rolen, ( (Bitmap*)(*listit) )->colht, ( (Bitmap*)(*listit) )->xofs,   ( (Bitmap*)(*listit) )->yofs, 
		   ( (Bitmap*)(*listit) )->iwd,   ( (Bitmap*)(*listit) )->iht,   ( (Bitmap*)(*listit) )->length, ( (Bitmap*)(*listit) )->bitdata );
      break;
    }
    case DRWBITMAPGRAYSCALESCALABLE : {
      drawBitmapgrayscalescalable(  ( (Bitmapgrayscalescalable*)(*listit) )->type,  ( (Bitmapgrayscalescalable*)(*listit) )->mode,  ( (Bitmapgrayscalescalable*)(*listit) )->xc,( (Bitmapgrayscalescalable*)(*listit) )->yc, ( (Bitmapgrayscalescalable*)(*listit) )->bwd, ( (Bitmapgrayscalescalable*)(*listit) )->bht,  ( (Bitmapgrayscalescalable*)(*listit) )->rolen, ( (Bitmapgrayscalescalable*)(*listit) )->colht, ( (Bitmapgrayscalescalable*)(*listit) )->xofs,   ( (Bitmapgrayscalescalable*)(*listit) )->yofs, ( (Bitmapgrayscalescalable*)(*listit) )->iwd,   ( (Bitmapgrayscalescalable*)(*listit) )->iht,   ( (Bitmapgrayscalescalable*)(*listit) )->length, ( (Bitmapgrayscalescalable*)(*listit) )->bitdata );

      //printf("id for bit map gray scale scalable = %d\n", ( (Bitmapgrayscalescalable*)(*listit) )->xcmdidnum );  // support for unique command id numbers experiments
      //fflush(stdout);                                                                                            // support for unique command id numbers experiments
      //printf("case DRWBITMAPGRAYSCALESCALABLE\n");
      //fflush(stdout);

      break;
    }


    case COLORNAME : {
      colorname( ( (Colorname*)(*listit) )->colorname);
      break;
    }
    case COLORBGR : {
      colorbgr( ((Colorbgr*)(*listit) )->blue, ((Colorbgr*)(*listit) )->green,((Colorbgr*)(*listit) )->red );
      break;
    }
    case COLORBBGGRR : {
      //printf("in case COLORBBGGRR line 1417 mfdraw.cxx\n");
      colorbbggrr( ((Colorbbggrr*)(*listit) )->blue, ((Colorbbggrr*)(*listit) )->green,((Colorbbggrr*)(*listit) )->red );
      break;
    }
    //added class Colorbbggrr on 6/20/2008
    


    case DRWTEXT : {
      drawText( ((Text*)(*listit))->x, ((Text*)(*listit))->y, ((Text*)(*listit))->ht, ((Text*)(*listit))->angle,
		((Text*)(*listit))->length, ((Text*)(*listit))->text );
      break;
    }
    case LINETHICK : {
      linethick( ((Linethick*)(*listit))->thickness );
      //cout<<"got linethick command with line thickness = "<<((Linethick*)(*listit))->thickness<<endl;
      break;
    }
    default :
      writetolog("in drawframe in mfdraw.cxx  don't know this command, terminating.");
      exit(EXIT_FAILURE);
    }//end of switch     
   
  }//end of while

}

int FrameView::handle(int event) {

  Fl_Gl_Window::handle(event); // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  let base class handle event first added 8/31/2007


  //  static bool d = true;
  static int startx, endx, starty, endy;
  //static int px,py,pw,ph;

  //char debugmsg[300];
  //sprintf(debugmsg, "in handle atframe = %d got event %d got Fl::event %d and Fl::event_key %d ", atframe, event, Fl::event(),  Fl::event_key());
  //writetolog(debugmsg);
  

  if( (Fl::event_key(FL_Control_L) || Fl::event_key(FL_Control_R)) &&  (Fl::event_key('q') || Fl::event_key('Q') ) ) {
    exit(0);
  }

  if( (Fl::event_key(FL_Alt_L) || Fl::event_key(FL_Alt_R)) && Fl::event_key(4+FL_F) ) {
    exit(0);
  }




#if 0
  //==================== User has switched to view mode - Corntrl key and left and right key that is -> and <- =======================
  if( Fl::event_state() ==  FL_CTRL  &&  ( Fl::event_key() == FL_Right ||  Fl::event_key() == FL_Left )  ) {
    //printf("you clicked control key and pressed -> key or <- key \n");
    //fflush(stdout);
    //fl_beep(); //doesnt work on arion - causes window frame to flash but due to no speaker there is no beep
    frameorder_view = true;
    return 1;
  }
  //==================== Corntrl key and left key > ==================================================
#endif





  if( !(Fl::event_key(FL_Left) || Fl::event_key(FL_Right)) )
     ptrzoom->activate();
   else
     ptrzoom->deactivate();       

  //==================== keyboard up and down arrow control for zoom ==================================

  if(Fl::event_key(FL_Up) && fl_up_count > 0) {
    if(++fl_up_count == 2) {
      fl_up_count = 0;
      return 1;
    }
  }
  if(Fl::event_key(FL_Down) && fl_down_count > 0) {
    if(++fl_down_count == 2) {
      fl_down_count = 0;
      return 1;
    }
  }

  //------------------------------------------------------------------------------------------------------
#if 0
  //linear slider
  if(Fl::event_key(FL_Up) && fl_up_count == 0) {
    fl_up_count++;   
    if( size[atframe] <= (sizemax - sizestep) ) {   
      ptrzoom->value(size[atframe] + sizestep); //this updates widget appearance but does not invoke callback
      size[atframe] += sizestep;         
      zoomfunc();     
    }
    return 1;
  }  
  if(Fl::event_key(FL_Down) && fl_down_count == 0) {
    fl_down_count++;   
    if( size[atframe] >= ( sizemin + sizestep) ) {
      ptrzoom->value(size[atframe] - sizestep); //this updates widget appearance but does not invoke callback
      size[atframe] -= sizestep;         
      zoomfunc();   
    }
    return 1;
  } 
#endif



  //------------------------------------------------------------------------------------------------------
  //linear slider with 'exponential step size'
  if(Fl::event_key(FL_Up) && fl_up_count == 0) {
    fl_up_count++;   
    if( size[atframe] <= (sizemax  - sizestep*size[atframe])) {   
      ptrzoom->value(size[atframe] + sizestep*size[atframe]); //this updates widget appearance but does not invoke callback
      size[atframe] += sizestep*size[atframe];         
      zoomfunc();     
    }
    return 1;
  }  
  if(Fl::event_key(FL_Down) && fl_down_count == 0) {
    fl_down_count++;   
    if( size[atframe] >= ( sizemin + sizestep*size[atframe]) ) {
      ptrzoom->value(size[atframe] - sizestep*size[atframe]); //this updates widget appearance but does not invoke callback
      size[atframe] -= sizestep*size[atframe];         
      zoomfunc();   
    }
    return 1;
  } 



  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ exponential zoom if 0 out
#if 0
  //above block replaced by this exponential zoom version 
  if(Fl::event_key(FL_Up) && fl_up_count == 0) {
    fl_up_count++;   
    if( size[atframe] < pow(sizemax,2) ) {  
      ptrzoom->value(ptrzoom->value() + sizestep); //this updates widget appearance but does not invoke callback
      size[atframe] = pow(ptrzoom->value(),2);        
      zoomfunc();     
    }
    return 1;
  }  
  if(Fl::event_key(FL_Down) && fl_down_count == 0) {
    fl_down_count++;   
    if( size[atframe] > pow(sizemin,2) ) {
      ptrzoom->value(ptrzoom->value() - sizestep); //this updates widget appearance but does not invoke callback
      size[atframe] = pow(ptrzoom->value(),2);          
      zoomfunc();   
    }
    return 1;
  } 
#endif


 
  if(  Fl::event_key(FL_Down) || Fl::event_key(FL_Up) ) { 

    return 1; //do nothing it's either a release or a unneeded typematic
  }    
  //================end keyboard up and down arrow control for zoom ===================================

  //======================== keyboard left and right arrow control for frame movement ================

  //static int keyupcnt = 1;     //================= //typermatic support
  //static int keydowncnt = 1;   //================= //typermatic support
  if(Fl::event() == FL_KEYUP) {

#if 0
    //================= //after typermatic ignore key release as indicator advance frame ===================
    if( keyupcnt == 2 || keydowncnt == 2 ) {
      keyupcnt = 1;
      keydowncnt = 1;
      return 1;
    }
#endif

    if(Fl::event_key() == FL_Right) {
      //if(sListofCommands.size() == 0)
      if(sListofCommands.empty())
	return 1;
      //cout<<"about to call ptrnxtfrm->do_callback()"<<endl;
      ptrnxtfrm->do_callback();
      return 1;

    } //end FL_Right

    if(Fl::event_key() == FL_Left) {
      //if(sListofCommands.size() == 0)
      if(sListofCommands.empty())
	return 1;
      ptrprevfrm->do_callback();
      return 1;
    }//end FL_Left 
    
    //if(Fl::event() == FL_KEYBOARD && ((Fl::event_key() == FL_Left) || (Fl::event_key() == FL_Right)) )
    //return 1; //get rid of typematic for now

  }//end of FL_KEYUP

#if 0
  else {     
    if(Fl::event() == FL_KEYBOARD && ((Fl::event_key() == FL_Left) || (Fl::event_key() == FL_Right)) ) { //typematic support for fast frame movement 
      if(Fl::event_key() == FL_Right &&  Fl::event_key(FL_Right)) {
	keyupcnt = 2;
	//if(sListofCommands.size() == 0)
        if(sListofCommands.empty())
	  return 1;
	ptrnxtfrm->do_callback();
	return 1;
      }//end FL_Right
      if(Fl::event_key() == FL_Left  &&  Fl::event_key(FL_Left)) {
	keydowncnt = 2;
	//if(sListofCommands.size() == 0)
        if(sListofCommands.empty()) 
	  return 1;
	ptrprevfrm->do_callback();
	return 1;
      }//end FL_Left
    }
  }//end of else for typematic support for fast frame movement
#endif


  //=========================== end keyboard left and right arrow control for frame movement ==========================
   
  if(event == FL_SHORTCUT) {
    if(Fl::event_key() == 's' || Fl::event_key() == 'S') {
       //printf("issued S command\n");
       skey = true;
       parent()->parent()->parent()->do_callback();
       skey = false;
       return 1;
    }
    
    if (Fl::event_key() == 'm' ||  Fl::event_key() == 'M') {
      if(runmode == 0) {
        //writetolog("the mode toggle pertains only to socket data input"); 
	return 1;
      }
      //writetolog("toggle mode still<->movie because we got letter m or M key\n");
      if( mmode == MM_MOVIE ) {
	//writetolog("(1) toggle from movie mode to still mode");
	mmode = MM_STILL;

	fastforward = false; // +++++++++++++++++ 1/9/08

        return 1;
      } else if (mmode == MM_STILL) {

#if 0
	//deleted as part of debug of m/M toggle 10/31/2007
	//start copied from dialog button callback for movie mode
	if(mmode == MM_STILL && previousframe < atframe ) { //we are transitioning from still to movie - need to handle previousframe when atframe is == 1 ???????????????? 
	  previousframe = atframe;
	  atframe++;
	}
        //end copied from dialog button callback for movie mode
#endif

        if(moreframes == false) //if there is no more connection to the science model then there can be no movie
	  return 1;
        //sprintf(msglog, "toggle from stil mode to movie mode where atframe = %d and previousframe = %d", atframe, previousframe); 
       	//writetolog(msglog); 
	fastforward = true;
        mmode = MM_MOVIE;
	//writetolog("(2) toggle from still mode to movie mode");
        ptrnxtfrm->do_callback();
        return 1;
      }
      return 1;
    }

    if (Fl::event_key() == 'n' || Fl::event_key() == 'N') {
      if(runmode == 0) {
	//fl_alert("you are in file mode not cns socket mode. use the file menu or exit.");

	// int choice = fl_choice(" ", '\0', "Continue","Quit");
        //if(choice == 2)
	//  exit(0);
	//return 1;

       ptrexitcontinue->show(); //+++++++++++++++++++++++ replacement for fl_choice
       return 1;
      }
      ptrnetdialog->show();
      //ptrcontrols->deactivate();
      //ptrzoom->take_focus();
      return 1;
    }
    if (Fl::event_key() == 'b' || Fl::event_key() == 'B') {
      //printf("toggle background black/white because we got letter b or B\n");
      setorigin = true;
      if(bkgrndwhite[atframe])
	bkgrndwhite[atframe] = false;
      else
	bkgrndwhite[atframe] = true;
      redraw();
      //ptrzoom->take_focus();
      return 1;
    }
    if (Fl::event_key() == 'o' || Fl::event_key() == 'O') {
      //printf("set the mode to set origin because we got letter o or O for set origin\n");
      setorigin = true;
      ptrorigin->value(1);
      ptrorigin->do_callback();
      return 1;
    }
    if (Fl::event_key() == 'h' ||  Fl::event_key() == 'H') {
      //printf("called home() because we got letter h or H key\n");
      home();
      return 1;
    }
    if (Fl::event_key() == 'f' ||  Fl::event_key() == 'F') {
      if(runmode == 1) {
	//fl_alert("you are in cns socket mode. use the Network dialog (right mouse click or n/N shortcut keys, not the File dialog.");
        
        //int choice = fl_choice(" ", '\0', "Continue","Quit");
        //if(choice == 2)
       //  exit(0);
       //return 1;

       ptrexitcontinue->show(); //+++++++++++++++++++++++ replacement for fl_choice
       return 1;
      }

#if 0
      static char filename[100];
      sprintf(filename, "%s", fl_file_chooser("pick a mfdraw data file", "mfdraw command files (*.mf)", "", 0));
      string s(filename);
      if(s == "(null)")
	return 1;
      deletedata(); //clear out any data and initialize prior to loading new data
      createdata(filename);
      buildframetracker();
      LISTOFCOMMANDS::iterator frit;
      frit = frametracker[atframe]; //frame is is pointed to by the iterators in frametracker[n]
      left = ((Newframe*)(*frit))->left;
      right = ((Newframe*)(*frit))->right;
      bottom = ((Newframe*)(*frit))->bottom;
      top = ((Newframe*)(*frit))->top;
      invalidate();
      redraw();

      static char versionandfile[120];
      parent()->parent()->parent()->label("");
      sprintf(versionandfile, "%s using file %s", parent()->parent()->parent()->label(), filename);
      parent()->parent()->parent()->label(versionandfile);

      ptrzoom->value(1);
      this->init();
      return 1;
#endif

    }

    if (Fl::event_key() == 'a' ||  Fl::event_key() == 'A') {
      fl_message("%s\n%s\n%s",header1.c_str(), header2.c_str(), header3.c_str());
      return 1;
    }
    else {
      return Fl_Gl_Window::handle(event);
    }
  }

  //if(!Fl::event_inside(this)) { //this didn't work don't know why ? using bounding box instead.
  if(!Fl::event_inside(0,0,w(),h())) {
    //printf("not inside this: x orig screen = %d y orig screen = %d w() = %d h() = %d mouse xpos = %d mouse ypos = %d\n",
    //	   x(), y(), w(), h(), Fl::event_x(), Fl::event_y());
    return Fl_Gl_Window::handle(event);
  }

  switch(event) {
  case FL_DRAG:
    endx = Fl::event_x();
    endy = Fl::event_y();
    //printf("you shift x by %d and shift y by %d\n", endx - startx, endy - starty);
    xshift[atframe] += (right - left)*((double)(endx - startx))/w();
    yshift[atframe] -= (top - bottom)*((double)(endy - starty))/h();
    startx = endx;
    starty = endy;
    //cout<<"calling modelviewsetorigin from handle drag:  with xpos = "<<(this->w())/2<<" and ypos = "<<(this->h())/2<<endl;
    //when we translate display we move pt0x[atframe] and pt0y[atframe] back to center of the screen
    //this will provide zooming which originates at screen center

    //if(setorigin == true) {   
    //  int xpos =  Fl::event_x();
    // int ypos =  Fl::event_y();
    //cout<<"setorigin = "<<setorigin<<"called mdoelviewsetorigin on release with xpos = "<<xpos<<" and ypos = "<<ypos<<endl;
      //modelviewsetorigin(xpos,ypos);     
    //}

    redraw();
    return 1;
  case FL_PUSH:

    //printf("PUSH: x orig screen = %d y orig screen = %d w() = %d h() = %d mouse xpos = %d mouse ypos = %d\n",
    //	   x(), y(), w(), h(), Fl::event_x(), Fl::event_y());

    
  
    startx  = Fl::event_x();
    starty  = Fl::event_y();
    return 1;
   
     
  case FL_RELEASE:
    //----------------get user desired location of the new origin --------------------

    int xpos; // addded as work around for ubuntu 11/7/2004
    int ypos; // addded as work around for ubuntu 11/7/2004
    
    if(Fl::event_button() == FL_RIGHT_MOUSE) {

      if(runmode == 1) {
	ptrnetdialog->position(Fl::event_x(), Fl::event_y());
	ptrnetdialog->show();
	return 1;
      }
      else {
	//fl_alert("you are in file mode not cns socket mode. do not use right mouse button in file mode.");

        //int choice = fl_choice(" ", '\0', "Continue","Quit");
        //if(choice == 2)
	//  exit(0);
	//return 1;

        ptrexitcontinue->show(); //replacement for fl_choice
        return 1;


      }
    }

    //we set origin upon mouse release 
    //if(setorigin == true) { 

  
    //int xpos =  Fl::event_x(); // removed as work around for ubuntu 11/7/2004
    //int ypos =  Fl::event_y(); // removed as work around for ubuntu 11/7/2004
      xpos =  Fl::event_x();     // addded as work around for ubuntu 11/7/2004
      ypos =  Fl::event_y();     // addded as work around for ubuntu 11/7/2004

      //cout<<"setorigin = "<<setorigin<<"called mdoelviewsetorigin on release with xpos = "<<xpos<<" and ypos = "<<ypos<<endl;
      modelviewsetorigin(xpos,ypos);
      //setorigin = false;
      return 1;
   // }

    //--------------------------------------------------------------------------------
  default:
    //writetolog("in   FrameView::handle and letting Fl_Gl_Window::handle(event) process the event as the default");
    return Fl_Gl_Window::handle(event);
  }
}

void FrameView::modelviewsetorigin(int xpos, int ypos) {
  //this function is called by case FL_RELEASE or case FL_DRAG in FrameView::handle()
  //printf("in modelviewsetorigin(): setorigin = %d and  mouse released at x = %d y = %d\n",setorigin, xpos, ypos);
  //printf("left = %g right = %g bottom = %g top = %g\n", left, right, bottom, top);

  pt0x[atframe] = -xshift[atframe] + ((double)xpos/w())*(right-left);
  pt0y[atframe] = -yshift[atframe] + (top - bottom) - ((double)ypos/h())*(top - bottom);

  pt0x[atframe] = pt0x[atframe]/sizew[atframe];
  pt0y[atframe] = pt0y[atframe]/sizeh[atframe];

  setorigin = false;
  ptrorigin->value(0);

  //printf("setorigin is %d new origin location in inches relative to the current origin: x = %g y = %g\n",setorigin, pt0x[atframe], pt0y[atframe]);
  //fflush(stdout);

  cursor(FL_CURSOR_DEFAULT);

  return;
}

void FrameView::zoomfunc() {
  xshift[atframe] -= pt0x[atframe]*(size[atframe] - oldsize[atframe])*(float(origwinwidth[atframe])/this->w());
  yshift[atframe] -= pt0y[atframe]*(size[atframe] - oldsize[atframe])*(float(origwinheight[atframe])/this->h());
  oldsize[atframe] = size[atframe];

  //printf("in zoomfunc with size = %g\n",size[atframe]); //support for adjuster widget
  pfvui->zoomadjuster->value(size[atframe]);              //support for adjuster widget
  pfvui->zoomdisplay->do_callback();                      //support for adjuster widget

  redraw(); 
}

void createtestdata(void) {
  //createtestdata() is for debugging purposes only
  Command *ptrendoffile;
  Newframe *ptrnewframe;
  Line *ptrline;
  Circle *ptrcircle;
  Filledcircle *ptrfilledcircle;
  Colorname *ptrcolorname;
  Outlinesquare *ptroutlinesquare;
  
  //record #1
  ptrnewframe = new Newframe(1,0,11,0,11,"CLIP");
  sListofCommands.push_front(ptrnewframe);
  //record #2
  ptroutlinesquare = new Outlinesquare(3,3,1);
  sListofCommands.push_front(ptroutlinesquare);
  //record #3
  ptrline = new Line(0,0,.5,.5);
  sListofCommands.push_front(ptrline);
  //record #4
  ptrcircle = new Circle(0,0,.25);
  sListofCommands.push_front(ptrcircle);
  //record #5
  ptrcircle = new Circle(0,0,1.0);
  sListofCommands.push_front(ptrcircle);

  //record #6
  ptrnewframe = new Newframe(2,-10,10,-10 ,10,"CLIP");
  sListofCommands.push_front(ptrnewframe);
  //record #7
  ptrcircle = new Circle(2,2,1);
  sListofCommands.push_front(ptrcircle);
  //record #8
  ptrcolorname = new Colorname("RED");
  sListofCommands.push_front(ptrcolorname);

  //record #9
  ptrfilledcircle = new Filledcircle(4,4,1);
  sListofCommands.push_front(ptrfilledcircle);

  //record #10
  ptrendoffile = new Command(ENDOFFILE);
  sListofCommands.push_front(ptrendoffile);
}



void buildframetracker(void) {
  //maxframe control version but assumes that meta file is read one frame at a time as in socket mode
  //printf("using maxframe version of buildframetracker");
  //fflush(stdout);

  //if(sListofCommands.size() == 0)
  if(sListofCommands.empty())
    return;

  int deletedsofar = deleteframe(); //deletes frames so that only the last MAXNUMFRAMES -1 are present in run mode 0 (meta file) amd run mode 1 (sockets) 
  if (deletedsofar < 0) {
    writetolog("Error: terminate deletedsofar return value < 0");
    exit(1);
  }



  //used by sListofCommands implemented as a list container - note that the list's iterators are not invalidated by addition of frames
  if(deletedsofar == 0) {
    //can only be used when we do not have to delete frames
    //following if empty not needed it is performed above
    //if(sListofCommands.empty())
    //return;

    static LISTOFCOMMANDS::iterator itX = sListofCommands.end();

    while(itX-- != sListofCommands.begin()) {
      if( (*itX)->cmd == NEWFRAME ) {
	frametracker[((Newframe*)(*itX))->frameindex] = itX;  //frametracker values are iterator which points to the frame's NEWFRAME record
	lastframe =  ((Newframe*)(*itX))->frameindex;
        //sprintf(msglog,"buildframetracker() newframes index is %d and lastframe = %d\n",((Newframe*)(*itX))->frameindex, lastframe);
	//printf(msglog);      //do not uncomment when in any of the socket modes                                                      
        //writetolog(msglog);  //safe to use when in any of the socket modes
	break;  //comment out for deque version 4/4/2008
      }
      //writetolog("drawing command but not frame command");
    } 
    return;
  }


#if 0
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  4/14/2008 start experiment replaces above for deque instead of list  
  if(deletedsofar == 0) {
    //can only be used when we do not have to delete frames
  
    //sprintf(msglog, "entered buildframetracker: and sListofCommands has size %d with max size %x", sListofCommands.size(), sListofCommands.max_size());
    //writetolog(msglog);

    //static LISTOFCOMMANDS::iterator itX = sListofCommands.end();  //<------------ automatic resizing container invalidates iterators !!!!
    LISTOFCOMMANDS::iterator itX =  sListofCommands.end();          //<------------ this works but takes linear time


    while(itX != sListofCommands.begin()) {
      itX--;
      if( (*itX)->cmd == NEWFRAME ) {
	frametracker[((Newframe*)(*itX))->frameindex] = itX;  //frametracker values are iterator which points to the frame's NEWFRAME record
	lastframe =  ((Newframe*)(*itX))->frameindex;
        //sprintf(msglog,"buildframetracker() newframes index is %d and lastframe = %d\n",((Newframe*)(*itX))->frameindex, lastframe);
	//printf(msglog);      //do not uncomment when in any of the socket modes                                                      
	//writetolog(msglog);  //safe to use when in any of the socket modes
      } else {
	//writetolog("drawing command not Newframe");	   
      }
    }
    return;
  }
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  4/14/2008 end experiment replaces above for deque instead of list
#endif


  LISTOFCOMMANDS::iterator it;
  for(int fr = 2; fr < MAXNUMFRAMES; fr++) {
    frametracker[fr-1] = frametracker[fr];
  }  

  //===================== support for synchronizing delete with frame properties ========================
  pfvui->glView->leftshift_frame_props(); //align frame properties with buildframetracker after frame delete

  it = sListofCommands.begin();
  while(1) {
    it++;
    if( (*it)->cmd == NEWFRAME ) {
      frametracker[MAXNUMFRAMES -1] = it;
      lastframe = MAXNUMFRAMES - 1;
      //sprintf(msglog, "in file %s at line %d atframe = %d got newframe with frame index = %d  and loaded an iterator to it in frametracker[%d]\n", __FILE__, __LINE__, atframe,  ((Newframe*)(*it))->frameindex,  MAXNUMFRAMES -1);
      //printf(msglog);
      //writetolog(msglog);
      break;
    }
  }

}//end of buildframetracker

void FrameView::leftshift_frame_props() {
  //align frame properties with buildframetracker after frame deletes

  //char debugmsg[300];
  //sprintf(debugmsg,"in leftshift_frame_props: mode = %d fastforward = %d atframe = %d lastframe = %d moreframes = %d",
  //       mmode, fastforward, atframe, lastframe, moreframes);
  //writetolog(debugmsg);

  for(int fr = 2; fr < MAXNUMFRAMES; fr++) {
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

#if 0
  //set lastframe, that is #  MAXNUMFRAMES -1, to initial values
  firsttime[MAXNUMFRAMES - 1]         = true; 
  origwinwidth[MAXNUMFRAMES - 1]      = origwinwidth[0];
  origwinheight[MAXNUMFRAMES - 1]     = origwinheight[0];
  origmainwinwidth[MAXNUMFRAMES - 1]  = origmainwinwidth[0];
  origmainwinheight[MAXNUMFRAMES - 1] = origmainwinheight[0];
#endif

  //set lastframe, that is #  MAXNUMFRAMES -1, to initial values +++++++++++++++++++++++++++++++++++++++++++++++++ test in 12/28/07
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

}


#if 0
void buildframetracker(void) {
  //This buildframetracker() is for design experiments ONLY. It assumes there are no deletes that is MAXNUMFRAMES - 1  > actual number of frames in the meta file
  //printf("using pre delete versiion of buildframetracker");
  //fflush(stdout);

  //if(sListofCommands.size() == 0)
  if(sListofCommands.empty())
    return;

  //can only be used when we do not have to delete frames
  //following if empty not needed it is performed above
  //if(sListofCommands.empty())
  //return;

  LISTOFCOMMANDS::iterator itX = sListofCommands.end();
  while(itX-- != sListofCommands.begin()) {
    if( (*itX)->cmd == NEWFRAME ) {
      //sprintf(msglog,"the newframes index is %d\n",((Newframe*)(*itX))->frameindex);
      //printf(msglog);      //do not uncomment when in any of the socket modes
      //writetolog(msglog);  //safe to use when in any of the socket modes
      frametracker[((Newframe*)(*itX))->frameindex] = itX;  //frametracker values are iterator which points to the frame's NEWFRAME record 
      lastframe =  ((Newframe*)(*itX))->frameindex;
    }
  }
}
#endif

string header1, header2, header3;

#if 0
//this createdata loads entire meta file and then it's buildframetrackers job to strip out all but the last MAXNUMFRAMES - 1
void createdata(const char *inputdatafile) {
  char buffer[MAXCOMMANDSIZE];
  ifstream inputStream(inputdatafile);  
  if( !inputStream ) {
    cerr << "Error opening input stream" << endl;
    exit(1);
  }
  while (1) {
    inputStream.getline( buffer, MAXCOMMANDSIZE  );
    int num  = inputStream.gcount();
    if(num <= 0) {
      break;
    }
    buffer[num] = '\n';
    //buffer[num + 1] = '\0';
    //cout<<"the buffer has "<<num<<" characters not including the null terminator"<<buffer<<endl; 
    //cout<<"the buffer has "<<num<<" characters not including the null terminator"<<endl;
    parsebuffer(buffer,num);
  }
  moreframes = false;
}
#endif

//start new createdata 10/23/2007
void createdata(const char *inputdatafile) {
  char buffer[MAXCOMMANDSIZE];         
  static int frmcnt = 0;  
  static int firsttime = true;

  static ifstream inputStream(inputdatafile);
  if( !inputStream ) {
    writetolog("Error opening input stream in createdata()");
    cerr << "Error opening input stream" << endl;
    exit(1);
  }


  if(firsttime == true) {
    firsttime = false;
    while (1) {
      inputStream.getline( buffer, MAXCOMMANDSIZE  );
      int num  = inputStream.gcount();
      if(num <= 0) {
	//break;
        //cout<<"marker that we are at the end of input meta file - no more frames"<<endl;
        inputStream.close();
        return;
      }
      buffer[num] = '\n';
      //buffer[num + 1] = '\0';
      //cout<<"the buffer has "<<num<<" characters not including the null terminator"<<buffer<<endl; 
      //cout<<"the buffer has "<<num<<" characters not including the null terminator"<<endl;
      parsebuffer(buffer,num);
      int next = inputStream.peek();
      if (next == '[') {
	frmcnt++;

	//if(frmcnt == MAXNUMFRAMES) { 
        if(frmcnt == 2) { 
	  //cout<<"about to process frame # "<<frmcnt<<" we will not load this frame"<<endl; 
	  frmcnt--;
	  return;
	}
	//cout<<"* at createdata() in meta file mode will load frame # "<<frmcnt<<endl;
	continue;
      }
      if (next == ']') {
        //cout<<"next is ]"<<endl;
	moreframes = false;
	continue;
      }   
    }//end while
    return;
  }//end of firsttime is true

  //add one frame and its drawing commands to sListofCommands


  while (1) {
      inputStream.getline( buffer, MAXCOMMANDSIZE  );
      int num  = inputStream.gcount();
      if(num <= 0) {
        inputStream.close();
        //cout<<"file "<<__FILE__<<" at line "<<__LINE__<<" : marker that we are at the end of input meta file - no more frames"<<endl<<fflush(stdout); //+++++++++ 1/10/08
	moreframes = false; //+++++++++ 1/10/08
	return;;
      }
      buffer[num] = '\n';
      //buffer[num + 1] = '\0';
      //cout<<"the buffer has "<<num<<" characters not including the null terminator"<<buffer<<endl; 
      //cout<<"the buffer has "<<num<<" characters not including the null terminator"<<endl;

      parsebuffer(buffer,num);

      int next = inputStream.peek();
      if (next == '[') {
	frmcnt++;        
	//cout<<"at createdata() came to but will not load frame # = "<<frmcnt<<endl; 
 
 #if 0   
        //moved to open call back in gui     1/25/08    
        //if(runmode == 0) {  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++replaced by next line for bug 1/14/08
	if(runmode == 0 && !fastforward) {  //used to load frames 1 through N automaticlly while displaying frame 1 (metafile only)    
	  Fl::check();
	}
#endif
          	  
	break;
      }
     
      if (next == ']') {
        //cout<<"next is ]"<<endl;
	moreframes = false;
	continue;
      }   
    }//end while

} //end new createdata 10/23/2007

void deletedata(void) {
  if(sListofCommands.empty())
    return;
  writetolog("warning: mfdraw.cxx deletedata() called and there were commands on sListofCommands");
  sListofCommands.clear();
  atframe = 1;
  inframe = false;
}

void nextframe_cb(FrameView* glView) {

  //sprintf(msglog, "in nextframe_cb: upon entry to nextframe_cb: lastframe = %d  atframe = %d mmode = %d\n", lastframe, atframe, mmode); 
  //printf(msglog);
  //writetolog(msglog);
  //cout<<"just entered nextframe_cb with atframe = "<<atframe<<endl;
  //sprintf(msglog, " in nextframe_cb: upon entry runmode = %d and fastforward = %d\n", runmode, fastforward); 
  //writetolog(msglog);

  if(runmode == 0) { //meta file data source
    //writetolog("in nxtfrm_cb: runmode == 0 in nextframe_cb");
    if(atframe == lastframe && moreframes == false) { //do not cycle frames with > or -> 
 
      //sprintf(msglog, "(1) in file %s line %d: atframe = %d lastframe = %d moreframes = %d", __FILE__, __LINE__, atframe, lastframe, moreframes);
      //cout<<msglog<<endl; 
      //writetolog(msglog);

      return; 
    }

    //    if(atframe == lastframe && moreframes == false) {
    //  return; //no cycle of frames from end to start or start to end
    // }

    //if(atframe == (MAXNUMFRAMES - 1) && moreframes == true) { // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 10/25/2007
    if( (atframe == lastframe) && moreframes == true) {         // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 10/29/2007

      //sprintf(msglog, "(2) in file %s line %d: atframe = %d lastframe = %d moreframes = %d", __FILE__, __LINE__, atframe, lastframe, moreframes);
      //cout<<msglog<<endl; 
  
      createdata(mf_filename.c_str());                          // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 10/25/2007
      buildframetracker();                                      // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 10/25/2007

      if(atframe < (MAXNUMFRAMES - 1)) { // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 10/29/2007
	previousframe = atframe;         // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 10/29/2007
	atframe++;                       // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 10/29/2007

        //sprintf(msglog, "(3) in file %s line %d: atframe = %d lastframe = %d moreframes = %d", __FILE__, __LINE__, atframe, lastframe, moreframes);
        //cout<<msglog<<endl; 
 
      }
 
      advanceframe = true;                             // ???????????????????????????????????????????????????????????????????? 10/25/2007

      //} else { // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 10/25/2007
    } else if(atframe < lastframe) {  //++++++++++++++++++++++++++++++++++++++++++++++++ 1/25/08

      previousframe = atframe;   //orig
      atframe++;                 //orig
      advanceframe = true;       //orig

      //sprintf(msglog, "(4) in file %s line %d: atframe = %d lastframe = %d moreframes = %d", __FILE__, __LINE__, atframe, lastframe, moreframes);
      //cout<<msglog<<endl; 
    }                                               
                                            
  }
  else { //socket data source
    switch (mmode) {
    case MM_MOVIE: 

      //sprintf(msglog, "(0) nextframe_cb: in nextframe at last frame with fastforward: lastframe = %d  atframe = %d mmode = %d\n", lastframe, atframe, mmode); 
      //writetolog(msglog);
      
      //#if 0
      //if(moreframes == true) { //replaced by next line
      if(moreframes == true && fastforward == true) {

        //sprintf(msglog, "(1) nextframe_cb: in nextframe at last frame with fastforward: lastframe = %d  atframe = %d mmode = %d\n", lastframe, atframe, mmode); 
        //writetolog(msglog);

	cnsstartup();

      }    
   
      if(moreframes == true && atframe == lastframe && fastforward == true) {

        //sprintf(msglog, "(2) nextframe_cb: in nextframe at last frame with fastforward: lastframe = %d  atframe = %d mmode = %d\n", lastframe, atframe, mmode); 
        //writetolog(msglog);

	return;
      }

      //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ test in 12/28/2007
      if(moreframes == true && atframe < lastframe) {
        advanceframe = 	true;
        previousframe = atframe++;


        //sprintf(msglog,"(3) nextframe_cb: moreframes = %d mmode = %d atframe = %d lastframe = %d",moreframes, mmode, atframe, lastframe); 
      	//writetolog(msglog);

        break;    //+++++++++++++++++++++++++++++++++++++++ 2/13/2008
      	//return; //+++++++++++++++++++++++++++++++++++++++ 2/13/2008
      }
      //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ test in 12/28/2007
      if(moreframes == false && atframe == lastframe) { //stop at last frame of the movie added atframe == lastframe test to insure that we display frames that we traverse
	advanceframe = false;
	mmode = MM_STILL;  
   
        //sprintf(msglog,"(4) nextframe_cb: moreframes = %d mmode = %d atframe = %d lastframe = %d",moreframes, mmode, atframe, lastframe); 
	//writetolog(msglog);

        return;
      }
      else {
	advanceframe = true;

	//sprintf(msglog,"(5) nextframe_cb: moreframes = %d mmode = %d atframe = %d lastframe = %d",moreframes, mmode, atframe, lastframe);
	//writetolog(msglog);

      }
      break;
      //#endif

#if 0
      // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  test MM_MOVIE case
      if(moreframes == false) { //stop at last frame of the movie added atframe == lastframe test to insure that we display frames that we traverse
	advanceframe = false;
	mmode = MM_STILL;     
        sprintf(msglog,"(4) nextframe_cb: moreframes = %d mmode = %d atframe = %d lastframe = %d",moreframes, mmode, atframe, lastframe); 
	writetolog(msglog);
        return;
      }

      if(atframe == lastframe) {
	//previousframe = atframe++;
        sprintf(msglog,"(5) nextframe_cb: moreframes = %d mmode = %d atframe = %d lastframe = %d",moreframes, mmode, atframe, lastframe); 
	writetolog(msglog);
        cnsstartup();
      };
      break;
      // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  test MM_MOVIE case
#endif

    default: //at this time the default == still mode (the only non-movie mode using sockets)
      //sprintf(msglog, "still mode is mmode default in nxtfrm_cb: lastframe = %d  atframe = %d previousframe = %d mmode = %d", lastframe, atframe, previousframe,  mmode); 
      //writetolog(msglog);


#if 0
      //this #if 0 block replaced by code immediately after it
      // do not ask for more data if we are moving to a frame, in still mode,  that already has its data !!!!!
      if(moreframes)  {
        if (atframe  >= (lastframe - 1)) { //do not ask for more data if we are moving to a frame, in still mode,  that already has its data !!!!!
          //sprintf(msglog,"*** get more data: atframe %d is >= lastframe %d", atframe, lastframe);
	  //writetolog(msglog);
	  cnsstartup();
	}
      } 
      previousframe = atframe;                         
      (atframe == lastframe) ? atframe = 1 : atframe++; 
#endif
   
      //replaces bloc below for frame add/delete support  ++++++++++++++++++++++++++++++++++++++++++++++++ 1/10/08
      if(moreframes && (atframe == lastframe))  {
	cnsstartup(); 
      } 
      if(atframe < lastframe || atframe < (MAXNUMFRAMES-1)) {
        advanceframe = true;
	previousframe = atframe++;
	//sprintf(msglog, "in file %s at line %d:  previousframe = %d atframe =%d lastframe = %d", __FILE__, __LINE__, previousframe, atframe, lastframe);
	//writetolog(msglog);
      }
      
#if 0
      //++++++++++++++++++++++++++++++++++++++++++++++++ replaced by bloc above 1/10/08      
      //do not cycle through frames  
      if(atframe < lastframe || atframe < (MAXNUMFRAMES-1)) {
        advanceframe = true;
	previousframe = atframe++;
	//sprintf(msglog, "in file %s at line %d:  previousframe = %d atframe =%d lastframe = %d", __FILE__, __LINE__, previousframe, atframe, lastframe);
	//writetolog(msglog);
      }
      else if( atframe == (MAXNUMFRAMES - 1)) {
        advanceframe = true;
  
	if(previousframe == atframe - 2) //once atframe = MAXNUMFRAMES-1 then subsequent atframes equal  previousframe
	  previousframe = atframe - 1;
	else
	  previousframe = atframe;

        //sprintf(msglog, "in file %s at line %d:  previousframe = %d atframe =%d lastframe = %d", __FILE__, __LINE__, previousframe, atframe, lastframe);
        //writetolog(msglog);
      } else {	
        advanceframe = true;
        //sprintf(msglog, "in file %s at line %d:  previousframe = %d atframe =%d lastframe = %d", __FILE__, __LINE__, previousframe, atframe, lastframe);
        //writetolog(msglog);
      }
      //replaces above for frame add/delete support
      //+++++++++++++++++++++++++++++++++++++++++++++++++1/10/08
#endif
        
      
    }//end of switch
  }//end of else that is socket not metafile data source
 

  //sprintf(msglog, "before inherit and display in nextframe_cb: last frame # is %d atframe is %d previous frame is %d mode is %d", lastframe, atframe, previousframe, mmode);
  //writetolog(msglog);



  if( moreframes == false && atframe > lastframe) { //+++++++++++++++++++++++++++ 1/10/08
    atframe--;  //+++++++++++++++++++++++++++ 1/10/08
    //sprintf(msglog, "(5) in file %s line %d: atframe = %d lastframe = %d moreframes = %d", __FILE__, __LINE__, atframe, lastframe, moreframes);
    //cout<<msglog<<endl; 
  }

  //initialize orthographic projection from the framenumber frame data record
  LISTOFCOMMANDS::iterator frit;
  frit = frametracker[atframe]; //frame is pointed to by the iterator in frametracker[n]

 
  //char debugmsg[300];    
  //sprintf(debugmsg,"in file %s at line %d in nextframe_cb just before drawing atframe = %d with mode = %d",__FILE__, __LINE__,  atframe, mmode);
  //writetolog(debugmsg);

  glView->left = ((Newframe*)(*frit))->left;
  glView->right = ((Newframe*)(*frit))->right;
  glView->bottom = ((Newframe*)(*frit))->bottom;
  glView->top = ((Newframe*)(*frit))->top;
  glView->invalidate();
  glView->init();
  glView->redraw();
  glView->show();

  //sprintf(debugmsg,"in file %s at line %d in nextframe_cb just after drawing atframe = %d with mode = %d",__FILE__, __LINE__,  atframe, mmode);
  //writetolog(debugmsg);
  
  static char lbl[120];
  if( (runmode == 1) &&  (mmode == MM_MOVIE) ) {

    //char debugmsg[300];
    // sprintf(debugmsg, "in nxtfrm_cb line 2379: runmode = 1 and mode = movie"); 
    // writetolog(debugmsg);
    // sleep(5);       //diagnostic aid to slow down the movie so each frame is viewable +++++++++++++++++++++++++++++++++++++++++++++++ remove only for test 1/10/08

    //sprintf(lbl, "%s  frame = %d", mf_filename.c_str(), atframe);
    sprintf(lbl, "%s", header2.c_str());                              //display title on main window  

    pfvui->mainWindow->label(lbl);
    pfvui->mainWindow->redraw();
    Fl::check();

    /*
    //comment out firsttimeflag logic for test
    static bool firsttimeflag = true;
    if(firsttimeflag == true) {
    writetolog("in mfdraw.cxx in firsttimeflag block in nextframe_cb");
    firsttimeflag = false;
    atframe = lastframe;
    glView->init();
    Fl::check();
    fastforward = true;
    ptrnxtfrm->do_callback(); //resume the automatic advance and display of frames
    Fl::check();
    }
    */


    //char debugmsg[300]; 
    //sprintf(debugmsg,"in nxtfrm callback: atframe = %d, lastframe = %d, fastforward = %d",atframe,lastframe,fastforward);
    //writetolog(debugmsg);

    //char debugmsg[300]; 
    //sprintf(debugmsg, "in nxtfrm callback: atframe = %d and lastframe = %d and fastforward = %d", atframe, lastframe, fastforward);
    //writetolog(debugmsg);
	
    if( (moreframes == true && atframe == lastframe) && (fastforward) ) {  
      if( mmode == MM_MOVIE ) {
        //char debugmsg[300]; 
        //sprintf(debugmsg, "in nxtfrm callback: atframe = %d and lastframe = %d and fastforward = %d", atframe, lastframe, fastforward);
        //writetolog(debugmsg);

	ptrnxtfrm->do_callback(); //automatic frame display advance

      }     
    }
  } 


  //sprintf(lbl, "%s  frame = %d", mf_filename.c_str(), atframe);
  sprintf(lbl, "%s", header2.c_str());                             //display title on main window

  pfvui->mainWindow->label(lbl);
  pfvui->mainWindow->redraw();
  Fl::check();

}//end nextframe_cb

void fastforward_cb(FrameView* glView) { //added 1/17/08 

  if(sListofCommands.empty())
    return;
  static char lbl[120];
  //speed adjusted when fast forward clicked
  static double speedup = 0;  
  //static bool speedclick = false; //only used for speedclick version of speedup logic
  const double speedincrement = MINFRAMEVIEWTIMENEXT/10.0;

  /*
  //detect rapid key press versus single - will not use this approach for now
  if(Fl::event_clicks() > 0) {
    speedclick = true;        
    Fl::event_clicks(0);	
  } else {                       
    speedclick = false;
    Fl::event_clicks(0); 
  } 
  if(speedclick == true) {
    speedup += speedincrement;		
  } else {
    speedup = 0;
  }
  */
  //cout<<"(1): atframe = "<<atframe<<" fastforward = "<<fastforward<<endl;


  static int tustart;   //in microseconds ++++++++ 2/18/08  
  int tuend;            //in microseconds ++++++++ 2/18/08  
  static int tstart;    //in seconds  ++++++++ 2/18/08  
  int tend;             //in seconds ++++++++ 2/18/08
  struct timeval  tv;   //++++++++ 2/18/08
  struct timezone tz;   //++++++++ 2/18/08



  if(!fastforward) {
    gettimeofday(&tv, &tz); //++++++++ 2/18/08
    tstart  = tv.tv_sec;    //in seconds //++++++++ 2/18/08
    tustart = tv.tv_usec;   //in microseconds //++++++++ 2/18/08
    speedup = 0;

    // Draw the first frame immediately
    if(atframe < lastframe) {
      previousframe = atframe;
      atframe++;
      LISTOFCOMMANDS::iterator frit;
      frit = frametracker[atframe]; //frame is pointed to by the iterator in frametracker[n]
      glView->left   = ((Newframe*)(*frit))->left;
      glView->right  = ((Newframe*)(*frit))->right;
      glView->bottom = ((Newframe*)(*frit))->bottom;
      glView->top    = ((Newframe*)(*frit))->top;
      glView->invalidate();
      glView->init();
      glView->redraw();
      glView->show();
      Fl::check();
      //sprintf(lbl, "%s   frame = %d", mf_filename.c_str(), atframe);
      sprintf(lbl, "%s", header2.c_str());                             //display title on main window
      pfvui->mainWindow->label(lbl);
      pfvui->mainWindow->redraw();
      Fl::check();
    }
    //else {
    //  
    //}

  }


  speedup += speedincrement;
  if(speedup > MINFRAMEVIEWTIMENEXT) {
    speedup = MINFRAMEVIEWTIMENEXT;        	  	 	                               
  }  
  //cout<<"atframe = "<<atframe<<" speedclick = "<<speedclick<<" and speedup = "<<speedup<<" speed increment = "<<speedincrement<<endl;                            
  fastforward  = true;
  fastprevious = false;     //for implementation of time controlled fastprevious
 
  //  if(runmode == 0) { // ++++++++++++++++++++++++++ WE NOW HANDLE ALL RUNMOCES  2/18/08

    //static char lbl[120];
 
    while( moreframes || (atframe <= (lastframe - 1) )  )  {   //++++++++++++++++++++++++++++++++++++++++++++++++  1/29/08
    //while( moreframes || (atframe < (lastframe - 1) )  ) {  //++++++++++++++++++++++++++++++++++++++++++++++++  1/18/08
    //while( moreframes && (atframe < lastframe) ) {          //+++++++++++++++++++++++++++++++++++++++++++++++++++++++  1/16/08 

      
     //#if 0
      //------------- delay of fastforward ------------------------
      if(fastforward) {
	//time_t t0 = time(NULL);  
       
	//int tustart = 0;   //in microseconds //++++++++ 2/18/08
	//int tuend  =  0;   //in microseconds //++++++++ 2/18/08
	//int tstart =  0;   //in seconds     //++++++++ 2/18/08
	//int tend   =  0;   //in seconds  //++++++++ 2/18/08


	//struct timeval  tv;//++++++++ 2/18/08
	//struct timezone tz; //++++++++ 2/18/08
	//gettimeofday(&tv, &tz);//++++++++ 2/18/08
	//tstart  = tv.tv_sec;    //++++++++ 2/18/08
	//tustart = tv.tv_usec;//++++++++ 2/18/08
                       
	while(1) {
 	  //time_t t1 = time(NULL);   
	  //double elapsed = difftime(t1,t0);       
	  gettimeofday(&tv, &tz); 
	  tend  = tv.tv_sec;     
	  tuend = tv.tv_usec;
	  double tuelapsed = (tend - tstart)*10e6 + (tuend - tustart);
	  double elapsed = tuelapsed/10e6; //elapsed time in seconds 

                   	
	  if(elapsed >= (MINFRAMEVIEWTIMENEXT - speedup)) {
            //cout<<"break out of view delay loop with elaspsed time = "<<elapsed<<" (MINFRAMEVIEWTIMENEXT - speedup) = "<<(MINFRAMEVIEWTIMENEXT - speedup)<<endl; //for debug
	    tstart  = tv.tv_sec;    //in seconds //++++++++ 2/18/08
	    tustart = tv.tv_usec;   //in microseconds //++++++++ 2/18/08
	    break;
	  } 
          Fl::check(); 
     
	  fd_set rfds;
	  struct timeval tv;
	  int retval; 
	  FD_ZERO(&rfds);
	  FD_SET(0, &rfds);
			   
	  tv.tv_sec =  0;
	  tv.tv_usec = 100000;
          
	  retval = select(0, NULL, NULL, NULL, &tv); 
        
	  Fl::check();
         
	  if (retval == -1) {
	    perror("select()");
	  }
	  //else if (retval) {
          //  cout<<"break out of view delay loop via select where retval = "<<retval<<endl;
	  //  break;
	  // }

	}//end while (for delay)
      }//------------- end delay of fastforward ------------------------
      //#endif


      if(fastforward == false) //for implementation of time controlled fastprevious & halt fast forward by < > || etc.
        return;
      if(atframe < lastframe) {
        previousframe = atframe;
        atframe++;
        LISTOFCOMMANDS::iterator frit;
        frit = frametracker[atframe]; //frame is pointed to by the iterator in frametracker[n]
        glView->left   = ((Newframe*)(*frit))->left;
        glView->right  = ((Newframe*)(*frit))->right;
        glView->bottom = ((Newframe*)(*frit))->bottom;
        glView->top    = ((Newframe*)(*frit))->top;
        glView->invalidate();
        glView->init();
        glView->redraw();
        glView->show();
        Fl::check();
        //sprintf(lbl, "%s   frame = %d", mf_filename.c_str(), atframe);
        sprintf(lbl, "%s", header2.c_str());                             //display title on main window
        pfvui->mainWindow->label(lbl);
        pfvui->mainWindow->redraw();
        Fl::check();
      }
      else {
        nextframe_cb(glView);
      }

   

#if 0
      //------------- original delay of fastforward ------------------------
      if(fastforward) {
	//time_t t0 = time(NULL);         
	int tustart = 0;   //in microseconds
	int tuend  =  0;   //in microseconds
	int tstart =  0;   //in seconds
	int tend   =  0;   //in seconds
	struct timeval  tv;
	struct timezone tz; 
	gettimeofday(&tv, &tz);
	tstart  = tv.tv_sec;
	tustart = tv.tv_usec;                  
	while(1) {
 	  //time_t t1 = time(NULL);   
	  //double elapsed = difftime(t1,t0);       
	  gettimeofday(&tv, &tz); 
	  tend  = tv.tv_sec;
	  tuend = tv.tv_usec; 
	  double tuelapsed = (tend - tstart)*10e6 + (tuend - tustart);
	  double elapsed = tuelapsed/10e6; //elapsed time in seconds                    	
	  if(elapsed >= (MINFRAMEVIEWTIMENEXT - speedup)) {
            //cout<<"break out of view delay loop with elaspsed time = "<<elapsed<<" (MINFRAMEVIEWTIMENEXT - speedup) = "<<(MINFRAMEVIEWTIMENEXT - speedup)<<endl; //for debug
	    break;
	  } 
          Fl::check(); 
     
	  fd_set rfds;
	  struct timeval tv;
	  int retval; 
	  FD_ZERO(&rfds);
	  FD_SET(0, &rfds);
			   
	  tv.tv_sec =  0;
	  tv.tv_usec = 100000;
          
	  retval = select(0, NULL, NULL, NULL, &tv); 
        
	  Fl::check();
         
	  if (retval == -1) {
	    perror("select()");
	  }
	  //else if (retval) {
          //  cout<<"break out of view delay loop via select where retval = "<<retval<<endl;
	  //  break;
	  // }

	}//end while (for delay)
      }//------------- end delay of fastforward ------------------------
#endif

      //following is for fast forward being ended by backframe or other gui action
      if(fastforward == false)
        return;  	  	 	  	
    }//end while
    return;

  // } //end all runmodes +++++++++++++++++++++ 2/12/08
  //====================  end fast forward for all runmodes ===========================

#if 0
  if(mmode == MM_MOVIE || mmode == MM_STILL) {  
 
    while(atframe < lastframe) {
      if(fastforward == false) {
 	return;
      }
      previousframe = atframe;
      atframe++;
   
      //char debugmsg[300];
      //sprintf(debugmsg,"in >>: mode = %d fastforward = %d atframe = %d lastframe = %d moreframes = %d MINFRAMEVIEWTIMENEXT = %d ",
      //          mmode, fastforward, atframe, lastframe, moreframes, MINFRAMEVIEWTIMENEXT);
      //writetolog(debugmsg);
  
      LISTOFCOMMANDS::iterator frit;
      frit = frametracker[atframe]; //frame is pointed to by the iterator in frametracker[n]
      glView->left = ((Newframe*)(*frit))->left;
      glView->right = ((Newframe*)(*frit))->right;
      glView->bottom = ((Newframe*)(*frit))->bottom;
      glView->top = ((Newframe*)(*frit))->top;
      glView->invalidate();
      glView->init();
      glView->redraw();
      glView->show();
      Fl::check();
      //sprintf(lbl, "%s   frame = %d", mf_filename.c_str(), atframe);
      sprintf(lbl, "%s", header2.c_str());                             //display title on main window

      pfvui->mainWindow->label(lbl);
      pfvui->mainWindow->redraw();

      Fl::check();  
      //time_t t0 = time(NULL);
    
      int tustart = 0;   //in microseconds
      int tuend   = 0;   //in microseconds
      int tstart  = 0;   //in seconds
      int tend    = 0;   //in seconds
      struct timeval  tv;
      struct timezone tz; 
      gettimeofday(&tv, &tz);
      tstart  = tv.tv_sec;
      tustart = tv.tv_usec;
      
    
      while(1) {
	//time_t t1 = time(NULL);
	//double elasped = difftime(t1,t0);
      
	gettimeofday(&tv, &tz); //speed adust fast forward when clicked  replaces 2 lines above
	tend  = tv.tv_sec;
	tuend = tv.tv_usec; 
	double tuelapsed = (tend - tstart)*10e6 + (tuend - tustart);
	double elapsed = tuelapsed/10e6; //elapsed time in seconds 
      
           
	if(elapsed >= (MINFRAMEVIEWTIMENEXT - speedup) ) //speed adjust fast forward when clicked  replaces 2 lines above
	  break;     
        
        
	Fl::check();
	fd_set rfds;
	struct timeval tv;
	int retval; 
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);			   
	tv.tv_sec =  0; 
	tv.tv_usec = 100000;  
 
	retval = select(0, NULL, NULL, NULL, &tv);


	Fl::check();
	if (retval == -1)
	  perror("select()");
	else if (retval) {
	  break;
	}
      }
    }

    if(mmode == MM_STILL)
      fastforward = false;


    if(atframe == lastframe) {
      speedup = 0;
    }

    //following if supports resumption of fastforward in movie mode once lastframe is reached
    if(mmode == MM_MOVIE && atframe == (lastframe) ) { //++++++++++++++++++ 1/28/08                             
      nextframe_cb(glView);                            //++++++++++++++++++ 1/28/08
    }
    
    return;   
  } //end of mmode == MM_MOVIE || mmode == MM_STILL
  writetolog("Error: in fastforward_cb and mmode not recognized, terminating");
  exit(1);
#endif

}//end of fastforward_cb

void fastprevious_cb(FrameView* glView) { //added 2/13/2008
  if(sListofCommands.empty())
    return;
  static char lbl[120];
  static double speedup = 0; 
  const double speedincrement = MINFRAMEVIEWTIMEPREV/10.0; 
  static int tustart;   //in microseconds
  int tuend;            //in microseconds  
  static int tstart;    //in seconds 
  int tend;             //in seconds
  struct timeval  tv;   
  struct timezone tz; 

  if(!fastprevious) {
    gettimeofday(&tv, &tz); //++++++++ 2/18/08
    tstart  = tv.tv_sec;    //in seconds //++++++++ 2/18/08
    tustart = tv.tv_usec;   //in microseconds //++++++++ 2/18/08
    speedup = 0;
    if(atframe > 1) {
      previousframe = atframe;
      atframe--;
      LISTOFCOMMANDS::iterator frit;
      frit = frametracker[atframe]; //frame is pointed to by the iterator in frametracker[n]
      glView->left   = ((Newframe*)(*frit))->left;
      glView->right  = ((Newframe*)(*frit))->right;
      glView->bottom = ((Newframe*)(*frit))->bottom;
      glView->top    = ((Newframe*)(*frit))->top;
      glView->invalidate();
      glView->init();
      glView->redraw();
      glView->show();
      Fl::check();
      //sprintf(lbl, "%s   frame = %d", mf_filename.c_str(), atframe);
      sprintf(lbl, "%s", header2.c_str());                             //display title on main window
      pfvui->mainWindow->label(lbl);
      pfvui->mainWindow->redraw();
      Fl::check();
    }
    //else {
    //
    //}
  }
  speedup += speedincrement;
  if(speedup > MINFRAMEVIEWTIMEPREV) {
    speedup = MINFRAMEVIEWTIMEPREV;        	  	 	                               
  }  
  fastforward  = false;
  fastprevious = true; 
  
  while( atframe > 1)  {
    //------------- delay fastprevious ------------------------
    if(fastprevious) {
      while(1) {
	//time_t t1 = time(NULL);   
	//double elapsed = difftime(t1,t0);       
	gettimeofday(&tv, &tz); 
	tend  = tv.tv_sec;     
	tuend = tv.tv_usec;
	double tuelapsed = (tend - tstart)*10e6 + (tuend - tustart);
	double elapsed = tuelapsed/10e6; //elapsed time in seconds 

                   	
	if(elapsed >= (MINFRAMEVIEWTIMEPREV - speedup)) {
	  //cout<<"break out of view delay loop with elaspsed time = "<<elapsed<<" (MINFRAMEVIEWTIMENEXT - speedup) = "<<(MINFRAMEVIEWTIMENEXT - speedup)<<endl; //for debug
	  tstart  = tv.tv_sec;    //in seconds //++++++++ 2/18/08
	  tustart = tv.tv_usec;   //in microseconds //++++++++ 2/18/08
	  break;
	} 
	Fl::check(); 
     
	fd_set rfds;
	struct timeval tv;
	int retval; 
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
			   
	tv.tv_sec =  0;
	tv.tv_usec = 100000;
          
	retval = select(0, NULL, NULL, NULL, &tv); 
        
	Fl::check();
         
	if (retval == -1) {
	  perror("select()");
	}
	//else if (retval) {
	//  cout<<"break out of view delay loop via select where retval = "<<retval<<endl;
	//  break;
	// }

      }//end while (for delay)
    }//------------- end delay of fastforward ------------------------

    if(fastprevious == false) //for implementation of time controlled fastprevious & halt fast forward by < > || etc.
      return;

    if(atframe > 1) {
      previousframe = atframe;
      atframe--;
      LISTOFCOMMANDS::iterator frit;
      frit = frametracker[atframe]; //frame is pointed to by the iterator in frametracker[n]
      glView->left   = ((Newframe*)(*frit))->left;
      glView->right  = ((Newframe*)(*frit))->right;
      glView->bottom = ((Newframe*)(*frit))->bottom;
      glView->top    = ((Newframe*)(*frit))->top;
      glView->invalidate();
      glView->init();
      glView->redraw();
      glView->show();
      Fl::check();
      //sprintf(lbl, "%s   frame = %d", mf_filename.c_str(), atframe);
      sprintf(lbl, "%s", header2.c_str());                             //display title on main window
      pfvui->mainWindow->label(lbl);
      pfvui->mainWindow->redraw();
      Fl::check();
    }
    else {
      break;
    }
  }
} //end fastprevious_cb added 2/13/2008


int deleteframe(void) { 
  //deleteframe will insure that number of frames on sListofCommands is < MAXNUMFRAMES;

  //int listsize = sListofCommands.size(); //Note that sListofCommands.size() is O(N)
  //printf("listsize pre-delete = %d\n", listsize);

  //note that this is an alternate method (alternate to buildframetracker approach)  of finding the index of the last frame in constant time
  static LISTOFCOMMANDS::iterator listit;
  static int deletedsofar = 0;
  listit = sListofCommands.begin();  
  while(listit++ != sListofCommands.end()) {
    if( (*listit)->cmd == NEWFRAME ) {
      //sprintf(msglog, "last frame on list has index = %d",  ((Newframe*)(*listit))->frameindex);
      //writetolog(msglog);
      break;
    }
  }  
  //end of find last frame
    
  int numdeleteframe    = (((Newframe*)(*listit))->frameindex) - MAXNUMFRAMES + 1 - deletedsofar;// generalized to inlude runmode = 1 (sockets)
  //sprintf(msglog,"number of frames to delete: %d",numdeleteframe);                                
   //cout<<"in file "<<__FILE__<<" at line "<<__LINE__<<" delete "<<numdeleteframe<<" frames"<<endl; 
   //writetolog(msglog);

  if (numdeleteframe < 1) {
    //memfile();                                //for  testing
    //sprintf(msglog,"atframe %d - did not delete frame", atframe);
    //writetolog(msglog);
    //printf(" did not delete frame\n");      //for  testing 
    return 0;
  }
  else {
    //printf("will now delete %d frames\n", numdeleteframe); //for  testing 
    deletedsofar += numdeleteframe;
  } // end find the index of the last frame  (meta file mode)


  listit = sListofCommands.end(); 
  while (1) {
    --listit;
    switch ( (*listit)->cmd ) {
    case NEWFRAME : {
      if( numdeleteframe--  > 0 ) {
	//sprintf(msglog, "in mfdraw.cxx / deleteframe(): runmode = %d and delete frame. deleted so fare %d", runmode, deletedsofar); //++++++++++++++++++++++++ debug 11/2/07
        //writetolog(msglog);                                                              //++++++++++++++++++++++++++++++++++++++++++++++++++++ debug 11/2/07
      }
      else {
	 //listsize = sListofCommands.size();  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++ debug
         //printf("listsize post-delete just before return from deleteframe() = %d\n", listsize);
         //writetolog("no more frames to delete return from deleteframe\n");  
	 //memfile(); //for memfile testing     
	//printf("listsize post-delete just before return from deleteframe() = %d\n", listsize);    
	return deletedsofar;
      }

      //printf("about to delete frame of index = %d in %s at line %d: runmode = %d atframe = %d previousframe = %d, lastframe = %d\n", ((Newframe*)(*listit))->frameindex, __FILE__ , __LINE__ , runmode, atframe, previousframe, lastframe);
      //fflush(stdout);


      delete (Newframe*)(*listit);
      sListofCommands.erase(listit);   

      break;
    }
    case DRWLINE : {
      delete (Line*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Line\n");
      break;
    }
    case DRWCIRCLE : {
      delete (Circle*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Circle\n");
      break;
    }
    case DRWFILLEDCIRCLE : {
      delete (Filledcircle*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Filledcircle\n");
      break;
    }
    case DRWOUTLINEELLIPSE : {
      delete (Outlineellipse*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Outlineellipse\n");
      break;
    }
    case DRWFILLEDELLIPSE : {
      delete (Filledellipse*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Filledellipse\n");
      break;
    } 
    case DRWFILLCLOSEPOLYLINE : {
      delete (Fillclosepolyline*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Fillclosepolyline\n");
      break;
    } 
    case DRWOUTLINECLOSEPOLYLINE : {
      delete (Outlineclosepolyline*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Outlineclosepolyline\n");
      break;
    }
    case DRWOPENPOLYLINE : {
      delete (Openpolyline*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Openpolyline\n");
      break;
    }
    case DRWOUTLINESQUARE : {
      delete (Outlinesquare*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Outlinesquare\n");
      break;
    }
    case DRWFILLEDSQUARE : {
      delete (Filledsquare*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Filledsquare\n");
      break;
    }
    case DRWOUTLINERECTANGLE : {
      delete (Outlinerectangle*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Outlinerectangle\n");      
      break;
    }
    case DRWFILLEDRECTANGLE : {
      delete (Filledrectangle*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Filledrectangle\n");
      break;
    }
    case DRWBITMAP : {
      delete (Bitmap*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Bitmap\n");
      break;
    }
    case DRWBITMAPGRAYSCALESCALABLE : {
      delete (Bitmapgrayscalescalable*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Bitmapgrayscalescalable\n");
      break;
    }
    case COLORNAME : {
      delete (Colorname*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Colorname\n");
      break;
    }
    case COLORBGR : {
      delete (Colorbgr*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Colorbgr\n");
      break;
    }


    case COLORBBGGRR : {
      delete (Colorbbggrr*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Colorbbggrr\n");
      break;
    }
    //added class Colorbbggrr on 6/20/2008


    case DRWTEXT : {
      delete (Text*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Text\n");
      break;
    }
    case LINETHICK : {
      delete (Linethick*)(*listit);
      sListofCommands.erase(listit);
      //printf("Deleted command = Linethick\n");
      break;
    }
   
    default :
      sprintf(msglog, "Error in %d in %s:  Did not delete drawing command  %d\n", __LINE__ , __FILE__ , (*listit)->getcmd() );
      //printf(msglog);
      writetolog(msglog);
      exit(EXIT_FAILURE);

    }//end of switch 
  }//end of while

  return deletedsofar;
}//end deletframe()



void memfile() {

#if 0
  //start stat() file size (proxy for memory) test
  struct stat bufstat;
  const char *fn = "/home/smarx/mfdraw/data/bmtest072007.mf";
  //const char *fn = "/home/smarx/mfdraw/data/bmtest.mf";
  int err = stat(fn, &bufstat);
  if(err != 0) {
    writetolog("error accessing file with stat()");
    exit(1);
  }
  sprintf(msglog, "\nfile size for %s = %ld", fn, bufstat.st_size);
  writetolog(msglog);
#endif

  //obtain memory usage statistics using proc file statm
  char buf[30];
  snprintf(buf, 30, "/proc/%u/statm", (unsigned)getpid());
  //snprintf(buf, 30, "/proc/self/statm"); //alternative to the preceeding line
  FILE* pf = fopen(buf, "r");
  if (pf) {
    unsigned size;       //       total program size
    unsigned resident;   //       resident set size
    unsigned share;      //       shared pages
    unsigned text;       //       text (code)
    unsigned lib;        //       library
    unsigned data;       //       data/stack
    //unsigned dt;         //       dirty pages (unused in Linux 2.6)
    //fscanf(pf, "%u" /* %u %u %u %u %u"*/, &size/*, &resident, &share, &text, &lib, &data*/);
    fscanf(pf, "%u %u %u %u %u %u", &size, &resident, &share, &text, &lib, &data);
    

    //DOMSGCAT(MSTATS, std::setprecision(4) << size / (1024.0) << "MB mem used");
   
   //sprintf(msglog,"\npagessize = %d total program size = %u and resident set size = %u",getpagesize(), size, resident);
   //sprintf(msglog,"\n units = pages: program = %u resident = %u shared = %u code = %u library = %u data/stack = %u", size, resident, share, text, lib, data);
   //writetolog(msglog);

  }

  fclose(pf);
}





