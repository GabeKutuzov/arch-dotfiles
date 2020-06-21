// (c) Copyright 2008-2014, The Rockefeller University *21500* 
/*##################################################################
  #               mfdrawmain                                       #
  #                                                                #
  # Version            3/2/2006                      Steven Marx   #
  # Rev, 10/02/08, GNR - Obey display name from socket file header #
  # Rev, 11/17/08, GNR - Ignore SIGPIPE signals to get err msgs    #
  # Rev, 01/06/09, GNR - Remove fl_open_display call--now works    #
  # Rev, 01/14/09, GNR - Handle MM_BATCH mode                      #
  # Rev, 10/01/14, GNR - Eliminate "checklocalhostaccess"          #
  ##################################################################

    mfdraw is the graphical user interface to cns. mfdraw
    obtains its data via a socket interface with cns or via a meta
    file which is provided by cns and manually placed on the server.

    From a sockets point of view cns is the client and mfdraw
    is a server.

    In socket mode, cns activates the server mfdraw via
    xinetd. in file mode, mfdraw is launched by the user via
    the command line with NO arguments.

    In order for mfdraw to be executed via xinted it is
    necessary for the user at the server to issue xhost +localhost
    command prior to cns making the socket connect to port on
    the server.

    xinetd resides on the server and has configuration information
    in /etc/services and in /etc/xinetd.d. for convenience use
    cat /etc/services | grep 10090 to view relevant entry for
    mfdraw and similarly use grep -r 10090 /etc/xinetd.d/<glob *>
    we then can modify /etc/xinetd.d/mfdraw using say
    emacs. It is then necessary to issue the command, as root,
    prompt>service xinetd restart to have changes take effect.
  ###################################################################*/


//#define DEBUGTESTDATA
//uncomment the above if you want to log out all of the drawing commands
//this is useful, for example, to diff with the corresponding .mf file

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include "mfdrawUI.h"
#include "mfdraw.h"
#include "mfdrawsockserver.h"

const char  *mfdrawversion = "mfdraw rev. " SVNVRS ": " __DATE__ "  " __TIME__;
char        *disp_name;
char        *win_title;
char        *prefs_fname;

/*---------------------------------------------------------------------*
*                  These seem to be Steve's globals                    *
*** NOTE THAT SOME OF THEM HAVE NAMES IN ALL UPPERCASE, CONTRARY TO  ***
***                      THE NORMAL CONVENTION                       ***
*---------------------------------------------------------------------*/

LISTOFCOMMANDS  sListofCommands; //stl double linked list
LISTOFCOMMANDS::iterator listit = sListofCommands.begin();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++ preference defaults
int MAXNUMFRAMES     = 81;      // runtime default
//int MAXNUMFRAMES   =  3;      // used for testing - permits max of 2 frames
// Establish default frame intervals for fast fwd/rev
double MINFRAMEVIEWTIMENEXT =  DfltSpeedIncr;
double MINFRAMEVIEWTIMEPREV =  DfltSpeedIncr;
double fwdspeedup = DfltSpeedIncr;
double bckspeedup = DfltSpeedIncr;
int XPOSWINDOW = 100;           // mainWindow->position(XPOSWINDOW,YPOSWINDOW);
int YPOSWINDOW = 100;           // see fluid for show() function code
int MAINWINDOWWIDTHSIZE = 500;  //mainWindow width in pixels - height will be this + 25 pixels
int WSCREENPIX = 1280;          //developers screen resolution - used to adjust to users resolution via preferences
int HSCREENPIX = 1024;          //developers screen resolution - used to adjust to users resolution via preferences
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++ preference defaults
string preffileinuse;           // full path name for preference file in use

LISTOFCOMMANDS::iterator* frametracker = NULL; //frametracker allocated space in main

int atframe = 1;           //first frame has index = 1
int previousframe = 1;     //previousframe has initial index = 1
bool inframe = false;      //your not in a frame at startup
bool advanceframe = false; //used to indicate frame has been advanced via gui
int lastframe = 0;         //index of the last frame
bool moreframes = true;    //indicates if Cns is still connected so that more frames is potentially possible
bool connectionclosed = false; //flag to indicate that the socket between mfdraw and the application have been closed  ++++++ 4/18/2008
bool quitpressed = false;      //indicates that the button that can be used to close the socket used to communicate with the application has been pressed +++++ 4/18/2008
bool fastforward  = false;  //used to display next frame in time controlled sequence
bool fastprevious = false;  //used to display previous frame in time controlled sequeunce

//bool frameorder_view = false; //flag if false frames are in load order (default) or true then frames are in viewing order

Fl_Value_Slider* ptrzoom;  //used to reset slider from glView
Fl_Light_Button* ptrorigin;//used to toggle light button from user mouse action when in glView

Fl_Double_Window *ptrnetdialog;
Fl_Group *ptrcontrols;
Fl_Double_Window *ptrsettings;
Fl_Double_Window *ptrexitcontinue;

//int portnumber =  PORT;   //not needed in mfdraw.  handled by xinetd to the default port # 10090 (see CnsSocketServer.h)
string mf_filename = "";    //file name user selects for the .mf file

// runmode = SOCKMODE (default) means invoked by xinetd and
// runmode = FILEMODE means data source will be .mf file
int runmode = SOCKMODE;

Fl_Button* ptrnxtfrm;
Fl_Button* ptrprevfrm;

Display *display;
FrameViewUI *pfvui;

char msglog[MSGLOGSIZE];

bool skey = false; //skey is a flag to inform the mainwindow call back NOT to close but rather to make a full screen display

//int commandcount = 0; //used for debug only

/***********************************************************************
*                          main starts here                            *
***********************************************************************/

int main(int argc, char **argv) {

  static bool dataoncmdline = false;

  writetolog((char*)mfdrawversion);

#if DEBUG & DBGWAIT
  // Wait for gdb to be attached for debugging
  // N.B. As of 12/05/08, this wait can also be performed by sending
  //    dbgflags=2 via sockets, so this is only needed for debugging
  //    very early problems.
  while (1) {
    int temp = 1;
    if (temp == 0)
      break;
    }
#endif

  umask(0);

  // Ignore SIGPIPE signals so routines that read the pipe will get an
  // error that can be reported, rather than just having the program die.
  signal(SIGPIPE, SIG_IGN);

  //if there are no command line arguments (i.e. argc == 1) we run in
  // file mode with user selecting meta file via gui
  if (argc == 1)
    runmode = FILEMODE;
  else {
      string s  = argv[1];
      int where = s.length() - s.rfind(".mf");
      if (where == 3) {
        runmode = FILEMODE;
        argv[1] = '\0';
        mf_filename = s;
#if DEBUG & DBGSTARTUP
        sprintf(msglog,"In meta file mode using meta file: %s",
         mf_filename.c_str());
        writetolog(msglog);
#endif
        dataoncmdline = true;
      }
   argc = 1; //prevent fltk from choking on command line options
   }

   // Check for environment variables
   //   These could be forwarded from xinetd.d startup file in socket
   //   mode, but this has not yet been tested.  Variables relating to
   //   location of the preferences file are read in getpreferences().

   {  char* value = getenv("MAXNUMFRAMES");
      if (value) {
         MAXNUMFRAMES = atoi(value);
         sprintf(msglog,"environment variable MAXNUMFRAMES = %d",MAXNUMFRAMES);
         writetolog(msglog);
         }
#if DEBUG & DBGSTARTUP
      else {
         sprintf(msglog,"environment variable MAXNUMFRAMES not defined, using default MAXNUMFRAMES = %d",MAXNUMFRAMES);
         writetolog(msglog);
         }
#endif
      } /* End local scope */

   {  char* value = getenv("DISPLAY");
      int lval;
      if (value && (lval = strlen(value)) > 0) {
         disp_name = (char *)malloc(lval+1);
         strcpy(disp_name, value);
#if DEBUG & DBGSTARTUP
         sprintf(msglog,"Environment variable DISPLAY = %s",disp_name);
         writetolog(msglog);
#endif
         }
       else {
         disp_name = (char *)malloc(5);
         strcpy(disp_name, ":0.0");
#if DEBUG & DBGSTARTUP
         sprintf(msglog,"Environment variable DISPLAY not defined, "
            "using default = %s",disp_name);
         writetolog(msglog);
#endif
         }
      } /* End value local scope */


   // Read socket file header--get disp_name, win_title, prefs_fname
   // (disp_name from client will override environment, as it should
   if (runmode == SOCKMODE)
      cnsstartup();

   //load preference file
#if DEBUG & DBGLPREFS
   sprintf(msglog,"preload preferences: MAXNUMFRAMES = %d, "
     "MINFRAMEVIEWTIMENEXT = %.3f, MINFRAMEVIEWTIMEPREV = %.3f, "
     "XPOSWINDOW = %d, YPOSWINDOW = %d, MAINWINDOWWIDTHSIZE = %d",
     MAXNUMFRAMES, MINFRAMEVIEWTIMENEXT, MINFRAMEVIEWTIMEPREV,
     XPOSWINDOW, YPOSWINDOW, MAINWINDOWWIDTHSIZE);
   writetolog(msglog);
#endif

   loadpreferences();

#if DEBUG & DBGLPREFS
   sprintf(msglog,"postload preferences: MAXNUMFRAMES = %d, "
      "MINFRAMEVIEWTIMENEXT = %.3f, MINFRAMEVIEWTIMEPREV = %.3f, "
      "XPOSWINDOW = %d, YPOSWINDOW = %d, MAINWINDOWWIDTHSIZE = %d, "
      "WSCREENPIX = %d HSCREENPIX = %d", MAXNUMFRAMES,
      MINFRAMEVIEWTIMENEXT, MINFRAMEVIEWTIMEPREV,
      XPOSWINDOW, YPOSWINDOW, MAINWINDOWWIDTHSIZE,
      WSCREENPIX, HSCREENPIX);
   writetolog(msglog);
#endif

   frametracker = new LISTOFCOMMANDS::iterator [MAXNUMFRAMES+1]; //runtime maxnumframes allocation moved to main

#ifdef DEBUGTESTDATA
  createtestdata();    //test driver to create a small test data set
  buildframetracker(); //loads frametacker array with iterators that point to frame records and sets the variable lastframe
  //printf("Debug: size of sListofCommands is %d\n", sListofCommands.size());
#endif

/* GNR attempt to set fl_display soon enough */
   if (disp_name) {
      char *pcolon = strchr(disp_name, ':');
      if (!pcolon) pcolon = disp_name;
#if DEBUG & DBGSTARTUP
      sprintf(msglog,"Main got disp_name %s, screen %s, "
         "calling Fl::display()\n", disp_name, pcolon);
      writetolog(msglog);
#endif
      Fl::display(pcolon);
      }

/* At one time, it seemed a good idea to call fl_open_display() here.
*  But with that call in, the program never worked, always giving
*  mysterious error exits deep in the X code.  So, it was removed.
*  This comment is just a warning not to put it back.  -GNR */
//   fl_open_display();

#if DEBUG & DBGSTARTUP
  writetolog("in mfdrawmain just before constructing FrameViewUI");
  //stopwatch();
#endif
  pfvui = new FrameViewUI;
#if DEBUG & DBGSTARTUP
  writetolog("in mfdrawmain just after constructing FrameViewUI");
#endif

  Fl::visual(FL_DOUBLE|FL_INDEX);
#if DEBUG & DBGSTARTUP
  writetolog("in mfdrawmain just after FL::visual");
#endif

// At this point smx had code intended to determine whether mfdraw has
// permission to write on the requested screen.  The implementation
// does a fork and makes X calls that lead to the mesage:
// "[xcb] Unknown sequence number while processing queue"
// which several web sites say confuses X by sending requests out
// of sequence.  I could not find an alternative method of doing
// this test, but of course the program will just die anyway if it
// does not have permission.  I removed the bad code.  -GNR, 10/01/14

  if(runmode == SOCKMODE && mmode >= MM_MOVIE) {
    fastforward = true;
#if DEBUG & DBGSTARTUP
    writetolog("in main():  mode is movie and fastforward set to true");
#endif
    }

  if(dataoncmdline) {
#if DEBUG & DBGSTARTUP
    writetolog("in main():  about to call cmdlin_metafile()");
#endif
    cmdline_metafile();
    }

  pfvui->show(argc, argv);

#if DEBUG & DBGSTARTUP
  writetolog("in mfdrawmain about to enter main message loop in mfdrawmain.cxx");
#endif
  return Fl::run();
  } /* End main */


/*---------------------------------------------------------------------*
*                           loadpreferences                            *
*                                                                      *
*  Rev, 11/04/08, GNR - Use the order of priority of locations for the *
*  preferences file specified in the GNRnotes document.  Specifically, *
*  if in socket mode, use the location specified in the socket file    *
*  header, thereby allowing the user to have different preference      *
*  files for different projects.  The smarx version always used the    *
*  home directory of the user running mfdraw.  Also modified to use    *
*  the file name (.mfdrawrc) given in the specification doc.           *
*                                                                      *
*  I cannot find any code where smarx version edited this file from    *
*  the File->Settings menu.  This feature still needs to be added.     *
*                                                                      *
*  The string operators "+" or "+=" require that you append to an      *
*  existing string, hence the multiple line assignments below.  -GNR   *
*---------------------------------------------------------------------*/

void loadpreferences(void) {

   ifstream in;
   char *eval;
   string s;

   if (runmode == SOCKMODE && prefs_fname) {
      //socket mode -- try to open specified preference file
      s = prefs_fname;
      in.open(s.c_str(), ifstream::in);
      if (in.good()) {
#if DEBUG & DBGLPREFS
         writetolog("socket mode: Opened .mfdrawrc specified in "
            "socket header.");
#endif
         goto ProcessPrefs;
         }
      else {
         // It is specified that this situation is an error, however, this
         // version of mfdraw has no mechanism to pass error codes back to
         // the socket client (at least, not this early).  Temporarily,
         // use defaults as per smarx.
         writetolog("Could not open .mfdrawrc specified in socket hdr. "
            "Will use preference defaults");
         return;
         }
      }

   else if ((eval = getenv("MFDRAW_HOME"))) {   /* Assign intended */
      // Directory for preferences file given in environment
      int i = strlen(eval);
      if (eval[i-1] == '/') {
         s = eval;
         s += MFPREFS; }
      else {
         s = eval;
         s += "/";
         s += MFPREFS; }
      in.open(s.c_str(), ifstream::in);
      if (in.good()) {
#if DEBUG & DBGLPREFS
         writetolog("Opened .mfdrawrc specified in MFDRAW_HOME.");
#endif
         goto ProcessPrefs;
         }
      else {
         // It is specified that this situation is an error, however, this
         // version of mfdraw has no mechanism to pass error codes back to
         // the socket client.  Temporarily, use defaults as per smarx.
         writetolog("Could not open .mfdrawrc specified by MFDRAW_HOME. "
            "Will use preference defaults");
         return;
         }
      }

    else if (runmode == FILEMODE && mf_filename.size() != 0) {
      // File mode and file name given on command line--try to open
      //  preferences file in the directory containing the metafile.
      unsigned pos = mf_filename.find_last_of("/");
      string path = mf_filename.substr(0,pos+1);

      s = path + MFPREFS;
      in.open(s.c_str(), ifstream::in);
      if (in.good()) {
#if DEBUG & DBGLPREFS
         sprintf(msglog,"Opened .mfdrawrc in the directory of the "
            "metafile %s.", path.c_str());
         writetolog(msglog);
#endif
         goto ProcessPrefs;
         }
      }

   // None of the above methods to find prefs file is successful.
   // Try home directory of current user
   {  int userid = getuid();
      struct passwd* psw;
      psw = getpwuid(userid);
#if DEBUG & DBGLPREFS
      sprintf(msglog,"User id = %d, user name = %s",userid, psw->pw_name);
      writetolog(msglog);
#endif
      s = "/home/";
      s += psw->pw_name;
      s += "/";
      s += MFPREFS;
      in.open(s.c_str(), ifstream::in);
      if (in.good()) {
#if DEBUG & DBGLPREFS
         writetolog("Opened .mfdrawrc in the home directory of the user.");
#endif
         goto ProcessPrefs;
         }
      else {
         writetolog("Could not open .mfdrawrc in the home directory of "
            "the user.  Will use preference defaults");
         return;
         }
      } /* End local scope */

ProcessPrefs:
   preffileinuse = s;
#if DEBUG & DBGLPREFS
   sprintf(msglog, "Parsing preference file %s",preffileinuse.c_str());
   writetolog(msglog);
#endif
   bool breakflag = false;

   while (1) {

      //handle case where last line in preference file does not have a newline just an eof
      if (breakflag)
         break;

      getline(in,s);
      if (in.eof())
        breakflag = true;

      //parse line
      if( s.empty() )  // ignore empty lines
        ;
      else {
        //note if a key is subset of longer key
        //place shorter key before the longer key
        //in this code's search sequence
        size_t found;
        //-----------------------------------
        string key("MAXNUMFRAMES");
        found = s.find(key);
        if (found!=string::npos) {
          extractval(s);
          MAXNUMFRAMES = atoi(s.c_str());
          continue;
        }
        //-----------------------------------
        key = "MINFRAMEVIEWTIMENEXT";
        found = s.find(key);
        if (found!=string::npos) {
          extractval(s);
          MINFRAMEVIEWTIMENEXT = atof(s.c_str());
          continue;
        }
        //------------------------------------
        key = "MINFRAMEVIEWTIMEPREV";
        found = s.find(key);
        if (found!=string::npos) {
          extractval(s);
          MINFRAMEVIEWTIMEPREV = atof(s.c_str());
          continue;
        }
        //------------------------------------
        key = "XPOSWINDOW";
        found = s.find(key);
        if (found!=string::npos) {
          extractval(s);
          XPOSWINDOW = atoi(s.c_str());
          continue;
        }
        //------------------------------------
        key = "YPOSWINDOW";
        found = s.find(key);
        if (found!=string::npos) {
          extractval(s);
          YPOSWINDOW = atoi(s.c_str());
          continue;
        }
        //------------------------------------
        key = "MAINWINDOWWIDTHSIZE";
        found = s.find(key);
        if (found!=string::npos) {
          extractval(s);
          MAINWINDOWWIDTHSIZE = atoi(s.c_str());
          continue;
        }
       //------------------------------------

        key = "WSCREENPIX";
        found = s.find(key);
        if (found!=string::npos) {
          extractval(s);
          WSCREENPIX = atoi(s.c_str());
          continue;
        }
       //------------------------------------

        key = "HSCREENPIX";
        found = s.find(key);
        if (found!=string::npos) {
          extractval(s);
          HSCREENPIX = atoi(s.c_str());
          continue;
        }
       //------------------------------------

      } //end search for key/value
    }//end while loop for processing lines
  in.close();
}

/*---------------------------------------------------------------------*
*                             extractval                               *
*---------------------------------------------------------------------*/

void extractval(string & s) {
  //support for loadpreference()
  //get value contained in s then s is value upon return
  //delete any trailing whitespace
  size_t found;
  string whitespaces(" \t");
  found =s.find_last_not_of(whitespaces);
  if (found != string::npos) {
    s.erase(found+1);
    //sprintf(msglog,"line with trailing whitespace erased is:%s<-",s.c_str());
    //writetolog(msglog);
    //now capture the value
    found = s.find_last_of(whitespaces);
    //value = s.substr(found + 1);
    s = s.substr(found + 1);
  }
}

/*---------------------------------------------------------------------*
*                              stopwatch                               *
*---------------------------------------------------------------------*/

void stopwatch(void) {
  //stopwatch measures elapsed time between 1st and 2nd call.
  static int tustart = 0;   //in microseconds
  static int tuend   = 0;   //in microseconds
  static int tend    = 0;   //in seconds
  static int tstart  = 0;   //in seconds
  static bool first  = true;
  struct timeval  tv;
  struct timezone tz;
  static double total_elapsed_seconds = 0;

  if(first) {
    gettimeofday(&tv, &tz);
    tstart  = tv.tv_sec;
    tustart = tv.tv_usec;
    //printf("the current time of day is %d seconds and %d microseconds\n", (int)tv.tv_sec, (int)tv.tv_usec);
    //fflush(stdout);
    first = false;
    return;
  } else {
    gettimeofday(&tv, &tz);
    tend  = tv.tv_sec;
    tuend = tv.tv_usec;
    //printf("the current time of day is %d seconds and %d microseconds\n", (int)tv.tv_sec, (int)tv.tv_usec);
    //fflush(stdout);
    double tuelapsed = (tend - tstart)*10e6 + (tuend - tustart);
    double elapsed_seconds = tuelapsed/10e6;
    total_elapsed_seconds +=  elapsed_seconds;
    //printf("atframe = %d elapsed time in %g microseconds and restated in seconds %g and total elapsed seconds \n",atframe tuelapsed, elapsed_seconds,total_elapsed_seconds);
    char output[100];
    sprintf(output, "atframe = %d  elapsed time in seconds %g and total elapsed seconds %g \n",atframe, elapsed_seconds,total_elapsed_seconds);
    writetolog(output);
    //printf(output);
    first = true;
    return;
  }
}
