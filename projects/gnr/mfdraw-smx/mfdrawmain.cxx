/*##################################################################
  #               mfdrawmain                		           #
  #								   #
  # Version            3/2/2006                      Steven Marx   # 
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

#include "mfdrawUI.h"
#include "mfdraw.h"
#include "mfdrawsockserver.h"

const char *mfdrawversion =  "mfdraw version: 0.7   " __DATE__"  "__TIME__;   //0.1 commit to cvs on 2/8/2008

//for checklocalhostxaccess
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
//end for checklocalhostxaccess 

LISTOFCOMMANDS  sListofCommands; //stl double linked list
LISTOFCOMMANDS::iterator listit = sListofCommands.begin();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++ preference defaults
int MAXNUMFRAMES     = 81;      // runtime default
//int MAXNUMFRAMES   =  3;      // used for testing - permits max of 2 frames
int MINFRAMEVIEWTIMENEXT =  0;  // run time default for fast forward view time
int MINFRAMEVIEWTIMEPREV =  0;  // run time default for fast previous view time
int XPOSWINDOW = 100;           // mainWindow->position(XPOSWINDOW,YPOSWINDOW);
int YPOSWINDOW = 100;           // see fluid for show() function code
int MAINWINDOWWIDTHSIZE = 500;  //mainWindow width in pixels - height will be this + 25 pixels

int WSCREENPIX = 1280;          //developers screen resolution - used to adjust to users resolution via preferences
int HSCREENPIX = 1024;          //developers screen resolution - used to adjust to users resolution via preferences
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++ preference defaults
string preffileinuse;           // full path name for preference file in use

//LISTOFCOMMANDS::iterator frametracker[MAXNUMFRAMES];
//LISTOFCOMMANDS::iterator frametracker[MAXNUMFRAMES + 1]; //the iterator that points to ith frame is frametracker[i]
                                                           //we do not use frametracker[0]

//LISTOFCOMMANDS::iterator* frametracker = new LISTOFCOMMANDS::iterator [MAXNUMFRAMES+1]; //runtime maxnumframes moved to main
LISTOFCOMMANDS::iterator* frametracker = NULL; //frametracker allocated space in main

//int Command::cmdidnum = 0; //experiment with auto numbering commands (uniquely) see mfdraw.h class Command

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

int runmode = 1; //runmode = 1 means invoked by xinetd and runmode = 0 means data source will be .mf file

Fl_Button* ptrnxtfrm;
Fl_Button* ptrprevfrm;

FrameViewUI *pfvui;

char msglog[MSGLOGSIZE];


bool skey = false; //skey is a flag to inform the mainwindow call back NOT to close but rather to make a full screen display

//int commandcount = 0; //used for debug only

int main(int argc, char **argv) {

  writetolog((char*)mfdrawversion);



  //sprintf(msglog, "RUNMODE__MFDRAW = %s", getenv("RUNMODE__MFDRAW")); //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 5/23/2008
  //writetolog(msglog);                                                 //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 5/23/2008


#if 0
  //only for gdb debugging (in attach mode) 
  while(1) {
    int temp = 3;
    if(temp == 4)
      break;
  }
#endif

  //getscreensize(); //commented out using preferences supplied user display resolutions

  umask(0);

  static bool dataoncmdline = false;

  //char msglog[300];
  //sprintf(msglog, "argv[0] = *%s* argv[1] = *%s*", argv[0], argv[1]);
  //writetolog(msglog);
 
  if(argc == 1) {
    runmode = 0; //if there are no command line arguments (i.e. argc == 1) we run in batch file mode with user selecting meta file via gui
  }
  else {
      string s  = argv[1];
      int where = s.length() - s.rfind(".mf");
      if(where == 3) {
	runmode = 0;
	argv[1] = "\0";
	argc = 1; //prevent fltk from dipslaying command line built in options
        mf_filename = s;
        //cout<<"we are in meta file mode using meta file:  "<<mf_filename<<endl;
        dataoncmdline = true;
      }
      else if(strcmp(argv[1],"-display") != 0) {
        //writetolog("error: input meta file not found");
	printf("error: input meta file not found\n");
        exit(1);
      }	
  }


// support for environment variables in meta file mode only has been tested but is not activated - this block has been #if zeroed
#if 0
   if (runmode == 0) { //MAXNUMFRAMES as an envirornment variable / to be used only in meta file mode
    //obtain MAXNUMFRAMES if in tcsh do: setenv MAXNUMFRAMES 4
    char* value = getenv("MAXNUMFRAMES");
    if( value != NULL) {
      MAXNUMFRAMES = atoi(value);
      sprintf(msglog,"environment varialbe MAXNUMFRAMES = %d",MAXNUMFRAMES);
      writetolog(msglog);
    } else {
      sprintf(msglog,"environment varialbe MAXNUMFRAMES not defined using default MAXNUMFRAMES = %d",MAXNUMFRAMES);
      writetolog(msglog);
    }
    //above will obtain MAXNUMFRAMES if tcsh we first did for example:  setenv MAXNUMFRAMES 4
    } else {
      sprintf(msglog," in socket mode MAXNUMFRAMES defined by default MAXNUMFRAMES = %d",MAXNUMFRAMES);
      writetolog(msglog);
    }
#endif

   //load preference file
   /*sprintf(msglog,"preload preferences: MAXNUMFRAMES = %d  MINFRAMEVIEWTIMENEXT = %d  MINFRAMEVIEWTIMEPREV = %d XPOSWINDOW = %d YPOSWINDOW = %d MAINWINDOWWIDTHSIZE =%d",\
     MAXNUMFRAMES, MINFRAMEVIEWTIMENEXT, MINFRAMEVIEWTIMEPREV, XPOSWINDOW, YPOSWINDOW, MAINWINDOWWIDTHSIZE);
     writetolog(msglog); */

   loadpreferences();

   sprintf(msglog,"postload preferences: MAXNUMFRAMES = %d  MINFRAMEVIEWTIMENEXT = %d  MINFRAMEVIEWTIMEPREV = %d XPOSWINDOW = %d YPOSWINDOW = %d MAINWINDOWWIDTHSIZE = %d \
    WSCREENPIX = %d HSCREENPIX = %d", MAXNUMFRAMES, MINFRAMEVIEWTIMENEXT, MINFRAMEVIEWTIMEPREV, XPOSWINDOW, YPOSWINDOW, MAINWINDOWWIDTHSIZE, WSCREENPIX, HSCREENPIX);
   writetolog(msglog);

   frametracker = new LISTOFCOMMANDS::iterator [MAXNUMFRAMES+1]; //runtime maxnumframes allocation moved to main


#ifdef DEBUGTESTDATA    
  createtestdata();    //test driver to create a small test data set
  buildframetracker(); //loads frametacker array with iterators that point to frame records and sets the variable lastframe
  //printf("Debug: size of sListofCommands is %d\n", sListofCommands.size()); 
#endif

#if 0
  //experiment - do not use 
  fl_display = NULL;
  putenv("DISPLAY=localhost:0.0");              
  fl_open_display();                                                                   
  if(fl_display == NULL)  {                                                             
    writetolog("Error: fl_display is NULL");                                         
    //printf("Error: fl_display is NULL");
    exit(1);
  } 
#endif



#if 0 
  char debugmsg[300];
  if(runmode == 0) {       //cannot make this work in socket mode 
    WSCREENPIX = Fl::w();  //test
    HSCREENPIX = Fl::h();  //test
    sprintf(debugmsg, "current window resolution width = %d and  height = %d", WSCREENPIX, HSCREENPIX);  //test
    writetolog(debugmsg);                                                                                //test
  }
#endif

  //writetolog("in mfdrawmain just before constructing FrameViewUI");
  //stopwatch();                                                      

  pfvui = new FrameViewUI;
  //writetolog("in mfdrawmain just after  construction FrameViewUI");
  
  Fl::visual(FL_DOUBLE|FL_INDEX);

  //sprintf(msglog,"in mfdrawmain line ~220 call show(argc,argv) with argc = %d argv[0] = %s argv[1] = %s argv[2] = %s ",argc, argv[0], argv[1],argv[2]);
  //writetolog(msglog);


  //writetolog(" about to call checklocalhostxaccess(argc, argv)");
  if(runmode == 1)
    checklocalhostxaccess(argc, argv);
  //writetolog(" returned from call checklocalhostxaccess(argc, argv)");

     
#if 0
  //obsolete now that we have decoupled gui frame advance and socket handling
  if(runmode == 1 && mmode == MM_MOVIE) {
    Fl::add_timeout(1.0,moviemode_cb);
  }
#endif


  //above replaced by 
  if(runmode == 1 && mmode == MM_MOVIE) { //moved to before FrameViewUI construction
    fastforward = true;
    //writetolog("in main():  mode is movie and fastforward set to true");
  }



 if(dataoncmdline) {
    cmdline_metafile();
  } 
  
 pfvui->show(argc, argv);

 //writetolog("in mfdrawmain about to enter main message loop in mfdrawmain.cxx");
 return Fl::run();
}



#if 0
//obsolete
void moviemode_cb(void*) {
  ptrnxtfrm->do_callback();  
}
#endif


void checklocalhostxaccess(int argc, char **argv) {
  //writetolog("entered checkloaclahostxaccess");
  //char msglog[300];
  //check that xhost +localhost access permission has been granted
  pid_t child;
  int status;
  if((child = fork()) == -1) {
    writetolog("error on fork for localhost permission test");
    exit(1); //EXIT_FAILURE;
  } else if(child == 0) {
    //sprintf(msglog, "in child with pid = %d and ppid = %d and argc = %d and argv[0] = %s", getpid(), getppid(), argc, argv[0]);
    //writetolog(msglog);
    status = 1;
    pfvui->show(argc, argv);
    exit(0);   //EXIT_SUCCESS
  } else {
    waitpid(child, &status, 0);
    //sprintf(msglog, "in parent with pid = %d and ppid = %d and child exited with status %d", getpid(), getppid(), status);
    //writetolog(msglog);
    if(status == 0) { //x access permission includes INET:localhost
      //writetolog("x access permission includes INET:localhost");
    } else {
       writetolog("Error cannot display gui, probably x access permission does NOT include INET:localhost");
      //printf("Error cannot display gui, probably x access permission does NOT include INET:localhost\n"); 
      //cannot use xinetd stdout to communicate with cns for system (x win) generated messages since they
      //will happen BEFORE our printf can !
      fflush(stdout);
      char buffer[MAXCOMMANDSIZE];
      int bufsz = sizeof(buffer);        
      while( read(0, buffer, bufsz) > 0 );
      //writetolog("Error exiting checklocalhostxaccess");	  
    }    
  }    
}


void getscreensize(void) {
  //use discontinued due to plaform (mostly xwindows) specific issues - replaced by preference implementation
  //obtains screen width/height in pixels, used to dynamically adjust mfdraw mainwindow to accomodate monitor resolution

  WSCREENPIX = 1024; //ubuntu work around 11/07/07
  HSCREENPIX =  768; //ubuntu work around 11/07/07
  return;            //ubuntu work around 11/07/07

  putenv("DISPLAY=localhost:0.0"); 
  char debugmsg[300];
  Display *display;
  int screen; 
  char *display_name = NULL;
  if ((display=XOpenDisplay(display_name)) == NULL) {
    sprintf(debugmsg,"Error: cannot connect to X server %s\n", XDisplayName(display_name)); 
    writetolog(debugmsg);
    //printf("Error: cannot connect to X server");
    //fflush(stdout);
    exit(-1); 
  } 
  screen = DefaultScreen(display);
  WSCREENPIX = (int)DisplayWidth(display,screen); 
  HSCREENPIX = (int)DisplayHeight(display,screen);

  //sprintf(debugmsg,"display width = %d display height = %d", WSCREENPIX, HSCREENPIX);  
  //printf(debugmsg);
  //fflush(stdout);
  //writetolog(debugmsg);
}

void loadpreferences(void) {
  bool prefile = true;
  //determine user id so that we can figure user name so we can build home directory
  int userid = getuid();
  //sprintf(msglog,"user id = %d",userid);
  //writetolog(msglog);
  struct passwd* psw;
  psw = getpwuid(userid);
  //sprintf(msglog,"user name = %s",psw->pw_name);
  //writetolog(msglog);

  //cannot use any envrionment variable, eg HOME, because in socket mode we do not have shell hence no environment vars.
  //char *ev = getenv("HOME");
  //sprintf(msglog,"environment variable HOME = %s",ev);
  //writetolog(msglog);
  
  ifstream in;
 
  if(runmode > 0) {
    //socket mode - locate preference file
    string s1("/home/");
    string s2(psw->pw_name);
    string s3("/.mfdrawrc");
    string s = s1 + s2 + s3;

    preffileinuse = s;
    
    in.open(s.c_str(), ifstream::in); //try to open preference file in the home directory of the user
    if(in.good()) {
      //writetolog("socket mode: Opened .mfdrawrc in the home directory of the user."); 
    } else {
      //writetolog("socket mode: Could not open .mfdrawrc in the home directory of the user. Will use preference defaults");
      prefile = false;
    }
  }

  if(runmode == 0 && mf_filename.size() != 0) {
    //locate preference file in meta file mode with meta file specified on the command line
    unsigned pos = mf_filename.find_last_of("/");
    string path = mf_filename.substr(0,pos+1);
    string s = path + ".mfdrawrc";

    preffileinuse = s;
    
    in.clear();
    in.open(s.c_str(), ifstream::in); //try to open preference file in the directory containing the meta file specified in mfdraw command line 
    if(in.good()) {
      //sprintf(msglog,"Opened .mfdrawrc in the directory of the metafile %s.", path.c_str());
      //writetolog(msglog);

      //sprintf(msglog, "preference file in meta file mode with meta file specified on the command line = %s",preffileinuse.c_str());
      //writetolog(msglog);
    
    } else {
      string s1("/home/");
      string s2(psw->pw_name);
      string s3("/.mfdrawrc");
      string s4 = s1 + s2 + s3;

      in.clear();
      in.open(s4.c_str(), ifstream::in); //try to open preference file in the home directory of the user
      if(in.good()) {

        preffileinuse = s4;

        //writetolog("Opened .mfdrawrc in meta file mode in the home directory of the user.");
        
        //sprintf(msglog, "preference file = %s",preffileinuse.c_str());
        //writetolog(msglog);
   
      } else {
	//writetolog("Could not open .mfdrawrc in the  directory containing the metafile or in the user home directory. Will use preference defaults");
	
	//sprintf(msglog, "preference file for save (if used) will be = %s",preffileinuse.c_str());
        //writetolog(msglog);

	prefile = false;
      }
    }
  }

  if(runmode == 0 && mf_filename.size() == 0 ) {
    //locate preference file in meta file mode with meta file NOT specified on the command line
    string s1("/home/");
    string s2(psw->pw_name);
    string s3("/.mfdrawrc");
    string s = s1 + s2 + s3;

    preffileinuse = s;

    in.open(s.c_str(), ifstream::in); //try to open preference file in the home directory of the user
    if(in.good()) {
      //writetolog("Opened .mfdrawrc in the home directory of the user."); 

      //sprintf(msglog, "preference file, in meta file mode with meta file NOT specified on the command line, = %s",preffileinuse.c_str());
      //writetolog(msglog);
    
  
    } else {
      //writetolog("Could not open .mfdrawrc in the home directory of the user. Will use preference defaults");
      prefile = false;
      
      //sprintf(msglog, "preference file for save (if used) will be = %s",preffileinuse.c_str());
      // writetolog(msglog); 
      
    }
  }
  
  
  if(prefile) {
    //writetolog("parse preference file");
    string s;
    //string whitespaces(" \t");
    //string value;

    bool breakflag = false; //handle case where last line in preference file does not have a newline just an eof

    while(1) {

      if(breakflag)         //handle case where last line in preference file does not have a newline just an eof
         break;

      getline(in,s);

      //if(in.eof())      //replaced by following
      //break;
      if(in.eof())        //handle case where last line in preference file does not have a newline just an eof
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
	  MINFRAMEVIEWTIMENEXT = atoi(s.c_str());
	  continue;
	} 
	//------------------------------------        
        key = "MINFRAMEVIEWTIMEPREV";
        found = s.find(key);
	if (found!=string::npos) {
	  extractval(s);
	  MINFRAMEVIEWTIMEPREV = atoi(s.c_str());
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
       

	
	
      }//end search for key/value
    }//end while loop for processing lines
  }//end of parse preference file
  in.close();
}

void extractval(string & s) {
  //support for loadpreference()
  //get value contaiend in s then s is value upon return
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

