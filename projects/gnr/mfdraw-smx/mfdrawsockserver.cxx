/*##################################################################
  #               File mfsockserver.cxx               		   #
  #								   #
  # Version                     3/7/2006             Steven Marx   # 
  ##################################################################
    
  Functions to support the socket connectivity between mfdraw
  and cns. 

  Note: mfdraw performs all of it's socket communication, 
  relying on xinetd, via standard in and standard out.

  The programmer can control the communication between cns on 
  grane and the mfdraw on say arion by doing the following:
  0) xhost +localhost on arion
  1) telnet grane
  2) cd mfdraw
  3) ./run/Cns
  4) continue with mfdraw gui commands
  This will startup mfdraw in socket mode. If it's desired to
  perform an interrupt, via mfdraw to permit the entry of 
  a type 3 control card, then follow the following sequence at
  startup: 
  0) xhost +localhost on arion
  1) telnet grane
  2) cd mfdraw
  3) /home/cdr/src/d3v8b/sparc/Cns
  4) execute file='/home/smarx/mfdraw/Cns.smarx.input'
  5) press interrupt button at the time of your choice and
  enter the type 3 control card at the cns prompt, e.g.
  cycle 1 4 1
  6) continue with mfdraw gui commands
  7) you can use ctl-u to clear error from input at cns prompt!        
  ###################################################################*/      

#include "mfdrawsockserver.h"
#include "mfdraw.h"

#include "mfdrawUI.h"
//mfdrawUI can be used to provide access to FrameViewUI *pfvui for example pfvui->glView

static char *buffer;
static int ba; //advance buffer amount


bool partial = false;                           //support for buffer straddling commands
//char* partialcmd = new char[MAXCOMMANDSIZE];  //support for buffer straddling commands
string partialcmd;                              //replace heap maanagement with string class 


char debugmsg[300]; //for debugging  writetolog traces

void error(char *msg)
{
  perror(msg);
  exit(1);
}

void writetolog(char* buf) {
  //create log file
  static int fd = -1;
  if(fd == -1) { //create file if does not exist or open if closed
    //if((fd = open("/home/smarx/mfdraw/server.log", O_CREAT| O_WRONLY | O_APPEND, 0666)) < 0) { //not portable
    //if((fd = open("/var/tmp/cnsgraphic.server.log", O_CREAT| O_WRONLY | O_APPEND, 0666)) < 0) {//portable but not friendly
    if((fd = open("/var/tmp/mfdraw.server.log", O_CREAT| O_WRONLY | O_APPEND, 0666)) < 0) {      //portable but not friendly
      //printf("Error: Could not open or create mfdraw.server.log");
      exit(1);
    }
  }

  //comment out the #if and #endif if you want to timestamp log entries
  //#if 0
  //create and log time stamp
  time_t timebuf; 
  time(&timebuf);
  int tlen  = strlen(ctime(&timebuf));
  char msg[tlen+1];
  strncpy(msg, ctime(&timebuf), tlen-1); //The string returned by ctime() is in the form: Sun Sep 16 01:03:52 1973\n\0
  msg[tlen] = '\0';
  strcat(msg, " ");
  write(fd, msg, tlen-1);
  write(fd, " ", 1); //new
  //#endif

  int blen = strlen(buf);
  //write(fd," ",1);    //provide a space after time stamp and before start of buf (first line only)
  write(fd, buf, blen); //log caller supplied buf

  write(fd,"\n",1);

  //close log upon reciept of "close" string
  if(!strcmp(buf, "close")) {
    write(fd,"------------------------------\n",31);
    close(fd);
    fd = -1;
    return;
  }
}

static int lcmdbuf;
static int msghdr  = 0;
butt msgbutt;
int mmode; //Movie mode made this global

bool batchmode = false;
//movie mode and batchmode are identical except upon reciept of MFB_CLOSE. upon reciept of MFB_CLOSE in batchmode == 1 we close Cns and close mfdraw

void cnsstartup(void) {   //Read the data from the Cns/xinetd stdin connection  

  //for debug ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 2/26/08
  //sprintf(debugmsg, "in file %s at line %d: entered cnsstartup atframe = %d previousframe = %d lastframe = %d mmode = %d", __FILE__, __LINE__, atframe,previousframe, lastframe, mmode);
  //writetolog(debugmsg); 



  try{

    if(msghdr & MFB_CLOSE) {
      char debugmsg[300];  //++++++++++++++++++ 4/17/2008
      sprintf(debugmsg, "at line 120: connection to cns closed. cnsstartup returns to caller with no additional drawing commands   atframe = %d lastframe = %d", atframe, lastframe);//++++++++++++++++++ 4/17/2008
      writetolog(debugmsg);  //++++++++++++++++++ 4/17/2008
      moreframes = false;   	
      closeconnection();
      if(batchmode == true) {
	exit(0);
      }  
      return;
    }


#if 0 
    if( (msghdr & MFB_CLOSE) && batchmode == true) {
      //writetolog("on entry to  cnsstartup batchmode is true and closed connection to cns");
      closeconnection();
      moreframes = false;
      return;
    }
#endif


    Fl::remove_fd(0); //in case we call cnsstartup after having add_fd(0) note: a useless remove can do no harm!

    /* Format of the startup message received via socket interface.
     *  (Variable-length names follow last item in this struct).  */
    struct {
      char vers[4];              /* Version of interface */
      char l_disp_name[4];       /* Length of display name */
      char l_home_name[4];       /* Length of home directory name */
      char l_win_title[4];       /* Length of window title */
      char cmd_buff_len[6];      /* Length of command buffer */
      char i_movie_mode[2];      /* Movie mode */
      char f_aspect_ratio[10];   /* Screen aspect ratio */
      char z_debug[9];           /* Debug flags */
      char m_pad[1];
    } start_msg;

    //FILE        *fWinPrefs;
    char        *disp_name;
    char        *win_title;
    //char        *xvargs[4];
    float       ratio;
    int         ackfd = -1;
    //int         Height, Width;    /* Height & width of main window */
    int         ldispnm;          /* Length of display name */
    int         lhomedir;         /* Length of home directory name */
    int         lwintitl;         /* Length of window title */
    //int         mmode;            /* Movie mode made this global */
    int         xdebug;           /* Debug bits */

    static int
      msglength = 0,          /* Length of current command buffer */
      msgrecvd  = 0;          /* Length of message already received */
    //int nr;                   /* Number of bytes read this time */
    //int nrem;                 /* Number of bytes to complete segment */

    static int skipstart = 0;
    if(skipstart == 0) {
      skipstart = 1;
      /* Read the startup data from the new connection */
      if (read(Sockin, &start_msg, sizeof(start_msg)) !=
	  sizeof(start_msg)) {
	writetolog("fatal error in mfsockserver.cxx reading startup parameters");
	exit(5);
      }

      if (7 != sscanf((char *)&start_msg,
		      MFDVERS "%4d%4d%4d%6d%2d%10f%9x", &ldispnm, &lhomedir,
		      &lwintitl, &lcmdbuf, &mmode, &ratio, &xdebug)) {
	writetolog("fatal error in mfsockserver.cxx wrong number of startup parameters");
	exit(6);
      }

      buffer = new char[lcmdbuf];

      //sprintf(msglog, "length of home directory = %d and length of command buffer %d", lhomedir, lcmdbuf);  //+++++++++++++++++++++++++++++++++++++++ debug 4/3/2008
      //writetolog(msglog);                                                                                   //+++++++++++++++++++++++++++++++++++++++ debug 4/3/2008

      //sprintf(msglog, "movie mode = %d",mmode); //+++++++++++++++++++++++++++++++++++++++ debug 4/3/2008
      //writetolog(msglog);                       //+++++++++++++++++++++++++++++++++++++++ debug 4/3/2008

      //if(mmode == MM_BATCH) 
      //  mmode = MM_MOVIE; //we equivalence BATCH mode and MOVIE mode in mfdraw 

      //we revise the above to differentiate batch from movie mode at termination (upon receipt of MFB_CLOSE)
      if(mmode == MM_BATCH) { 
        //writetolog("in mfdrawsockserver.cxx line 192:  we are in batch mode and we set batchmode to true"); //+++++++++++++++++++++++++++++++++++++++ debug 4/3/2008
	mmode = MM_MOVIE;  
        batchmode = true;
      }

      msgbutt.movie = mmode;	  

      /* Get name of display server */
      if (ldispnm > 0) {
	if ((disp_name = (char*)malloc(ldispnm+1)) == NULL) {
	  writetolog("fatal error in mfsockserver.cxx Unable to allocate storage for display name");
	  exit(7);
	}
	if (read(Sockin, disp_name, ldispnm) != ldispnm) {
	  writetolog("fatal error in mfsockserver.cxx reading mfdraw startup parameter disp_name");
	  exit(8);
	}
	disp_name[ldispnm] = '\0';
      }
      else {
	writetolog("fatal error in mfsockserver.cxx client did not specify display");
	exit(9);
      }

      /* Get name of directory where prefs file can be found */
      static volatile char plot_sav = 0;
      static char *WPfname;
      plot_sav = (lhomedir > 0);
      if (plot_sav) {
	/* Get name of home directory and use it
	 *  to construct name of prefs file */
	if (WPfname) free(WPfname);   /* Prevent unauthorized leakage */
	if ((WPfname = (char*)malloc(lhomedir+sizeof(MFPREFS)+1)) == NULL) {
	  writetolog("fatal error in mfsockserver.cxx  unable to allocate storage for prefs file name");
	  exit(10);
	}
	if (read(Sockin, WPfname, lhomedir) != lhomedir) {
	  writetolog("fatal error in mfsockserver.cxx  unable to read startup preferences");
	  exit(11);
	}
	WPfname[lhomedir] = '\0';
	strcat(WPfname, MFPREFS);
      }

      /* Get window title, if any */
      if (lwintitl > 0) {
	if ((win_title = (char*)malloc(lwintitl+1)) == NULL) {
	  writetolog("fatal error mfsockserver: unable to allocate storage for window title");
	  exit(12);
	}
	if (read(Sockin, win_title, lwintitl) != lwintitl) {
	  writetolog("fatal error mfsockserver: reading window title");
	  exit(13);
	}
	win_title[lwintitl] = '\0';
      }
      else
	win_title = "";

      //sprintf(msglog, "length of display name = %d movie mode = %d and length of home dir = %d", ldispnm,mmode,lhomedir); //+++++++++++++++++++++++++++++++ debug 4/3/2008
      //writetolog(msglog); //+++++++++++++++++++++++++++++++++++++++ debug 4/3/2008

      sprintf(msglog, "display name is: %s  window title is: %s and WPfname is: %s",disp_name, win_title, WPfname);
      writetolog(msglog);

      /* Tell CNS client "I am alive" */
      if (write(Sockout, &ackfd, 1) < 1) {
	sprintf(msglog, "fatal error %d sending startup ack to client\n", errno);
	writetolog(msglog);
	exit(25);
      }

      //sprintf(msglog,"line 264: mfsockserver cnsstartup() acked CNS upon reciept of startup header with value %d",ackfd); //+++++++++++++++++++++++++++++++ debug 4/3/2008
      //writetolog(msglog); //+++++++++++++++++++++++++++++++++++++++ debug 4/3/2008
      //stopwatch();
      //stopwatch(); 

    }//end of skipstart

    //=============  Cns startup data processed, now read Cns headers and drawing commands  ========================

#if 0
   // process 1st frame that is acquired in constructor experiment since fastforward is false during construction this code is never executed
   if( (mmode == MM_MOVIE) && atframe == 1 && fastforward == true) {   	
        sprintf(debugmsg, "process 1st frame acquired in constructor pre catch up cnsstartup: atframe = %d previous frame = %d lastframe = %d moreframes = %d mmode = %d ffwd = %d", atframe, previousframe, lastframe, moreframes, mmode, fastforward);           
	writetolog(debugmsg);
	LISTOFCOMMANDS::iterator frit;
	frit = frametracker[atframe]; //frame is pointed to by the iterators in frametracker[n] 
	pfvui->glView->left   = ((Newframe*)(*frit))->left;
	pfvui->glView->right  = ((Newframe*)(*frit))->right;  
	pfvui->glView->bottom = ((Newframe*)(*frit))->bottom; 
	pfvui->glView->top    = ((Newframe*)(*frit))->top;
        pfvui->glView->init();                                
	pfvui->glView->invalidate();
	pfvui->glView->redraw();
	pfvui->glView->show();
	static char lbl[120];
	//sprintf(lbl, "%s   frame = %d", mf_filename.c_str(), atframe);
        sprintf(lbl, "%s", header2.c_str());                             //display title on main window
	pfvui->mainWindow->label(lbl);
	pfvui->mainWindow->redraw();
	Fl::check();
        previousframe = atframe;
        atframe++;

	Fl::add_fd(0, FL_READ, stdin_cb);
      }
      //end process 1st frame that is acquired in constructor +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 2/22/08
#endif

  dialogloop: while(1) {

    //writetolog("In start dialogloop: Cns startup data processed, now read Cns headers and drawing commands");
    //stopwatch();
    //stopwatch();

    msglength = messagehdr(); //assumption: msghdr not included in length;

    //sprintf(msglog, "sockserver line %d: start dialogloop cnsstartup main read/write loop. we are at atframe = %d previousframe = %d lastframe = %d  msglength = %d",__LINE__, atframe, previousframe, lastframe, msglength);
    //writetolog(msglog);
    //stopwatch();
    //stopwatch();
                                 

    if(msglength > lcmdbuf) {
      lcmdbuf = msglength + 1;
      delete [] buffer;
      buffer = new char[lcmdbuf];
      //writetolog("expanded buffer in dialogloop");
    }
    msgrecvd = 0;   
    do {
      int nr = read(Sockin, buffer + msgrecvd, msglength);
      if( nr == 0 && msglength == 0) {
	//sprintf(msglog, "sockserver at line %d atframe = %d got from Cns: read returned 0 and msglength = 0",__LINE__,  atframe);
	//writetolog(msglog);
	//we continue with the outermost dialog loop when cns responds with a 0 message length which appears to be a
	//response to our interrupt command             
	goto dialogloop;
      }
      if( nr < 0 ) {
	writetolog("fatal error reading socket in CnsSockserver");
	exit(1);
      }
      msglength -= nr;
      msgrecvd  += nr;

#if 0
      sprintf(msglog, "in file %s at line %d: got %d chars from stdin with cum amt. for buffer of %d with %d remaining", __FILE__, __LINE__, nr, msgrecvd, msglength);
      writetolog(msglog);
#endif

    }while(msglength > 0);
    if(msglength < 0) {
      writetolog("fatal error: msglength after read in CnsGockServer is < 0");
      exit(1);
    }

    parsebuffer(buffer, msgrecvd);
  
    if(msghdr & MFB_COMPLT) {

      Fl::remove_fd(0);
                                   
      if(msglength == 0) {
        msgbutt.movie    = (char)mmode;
	msgbutt.next     = (char)1;
	msgbutt.intxeq   = (char)0;
	msgbutt.snapshot = (char)0;



        //sprintf(msglog,"line %d in MFB_COMPLT just BEFORE Cns request for next buffer atframe = %d with mode = %d fastforward = %d",__LINE__, atframe, mmode, fastforward); 
	//writetolog(msglog); //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 4/3/2008
        //stopwatch();
        //stopwatch();


	if (write(Sockout, &msgbutt, sizeof(msgbutt)) < (int)sizeof(msgbutt)) {
	  sprintf(msglog, "mfsockserver error %d writing button status to client.\n", errno);
	  writetolog(msglog);
	  exit(62);
	}

        //sprintf(msglog,"line %d in MFB_COMPLT just AFTER Cns request for next buffer atframe = %d with mode = %d fastforward = %d",__LINE__, atframe, mmode, fastforward); 
        //writetolog(msglog); //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 4/3/2008

        msgbutt.movie    = (char)mmode;
	msgbutt.next     = (char)0;
	msgbutt.intxeq   = (char)0;
	msgbutt.snapshot = (char)0; 

      }
      else {
	writetolog("fatal error: mfsockserver has messaglength != 0 after reading buffer");
	exit(1);
      }

      buildframetracker();

      //start catch up for MM_MOVIE
      //since acquisition of buffers continues even though the display might fall behind we must, when fast forward is requested, catch up


      while((mmode == MM_MOVIE)  && (atframe <= lastframe) && fastforward) {       
	//sprintf(debugmsg, "pre catch up cnsstartup: atframe = %d previous frame = %d lastframe = %d moreframes = %d mmode = %d ffwd = %d", atframe, previousframe, lastframe, moreframes, mmode, fastforward);           
	//writetolog(debugmsg);

	//inheriting user changes to the gui             
	advanceframe = true;

	if(atframe < lastframe) //++++++++++++++++++++++++++++++++++++ advance frame if it is not lastframe
	  atframe++;

        previousframe = atframe - 1;
	
	//display backlogged frames
	LISTOFCOMMANDS::iterator frit;
	frit = frametracker[atframe]; //frame is pointed to by the iterators in frametracker[n] 
	pfvui->glView->left   = ((Newframe*)(*frit))->left;
	pfvui->glView->right  = ((Newframe*)(*frit))->right;  
	pfvui->glView->bottom = ((Newframe*)(*frit))->bottom; 
	pfvui->glView->top    = ((Newframe*)(*frit))->top;
        pfvui->glView->init();                                
	pfvui->glView->invalidate();
	pfvui->glView->redraw();
	pfvui->glView->show();
	static char lbl[120];
	//sprintf(lbl, "%s   frame = %d", mf_filename.c_str(), atframe);
        sprintf(lbl, "%s", header2.c_str());                             //display title on main window
	pfvui->mainWindow->label(lbl);
	pfvui->mainWindow->redraw();
	Fl::check();
       	     
	//sprintf(debugmsg, "post catch up and inheritance cnsstartup: atframe = %d previous frame = %d lastframe = %d moreframes = %d mmode = %d ffwd = %d size = %g\n", atframe, previousframe, lastframe, moreframes, mmode, fastforward, pfvui->glView->size[atframe] );
	//writetolog(debugmsg);


	if(fastforward == false)
	  break;


	if(mmode != MM_MOVIE) {
	  //this could happen because we are 'multithreading' catch-up, gui actions, and socket handling
	  break;
	}

	
	if(atframe < lastframe) {
	  previousframe++;
	  atframe++; 
	}
	else {
	  break;
	}


      }//end catch up for MM_MOVIE

      //if(moreframes == true && mmode == MM_MOVIE ) {

      if(moreframes == true && mmode == MM_MOVIE  && atframe == lastframe) {
        
        //sprintf(msglog, "In file %s at line %d: add stdin_cb upon completing buffer at frame %d with lastframe %d",__FILE__ ,__LINE__, atframe, lastframe);
        //writetolog(msglog);

	Fl::add_fd(0, FL_READ, stdin_cb);

      }


      //sprintf(msglog, "end of MFB_COMPLT in file %s at line %d: last frame received is %d and display is atframe  %d mode = %d and fastforwad = %d"
      //	      ,__FILE__ ,__LINE__, lastframe, atframe, mmode, fastforward);  //++++++++++++++++++++++++++ 1/10/2008
      //writetolog(msglog);                                                          //++++++++++++++++++++++++++ 1/10/2008

      return; 
    }//end of MFB_COMPLT


    if(msghdr & MFB_CLOSE) {


     //+++++++++++++++++++++++++++++++++++++++++ following replaces the above #if 0  out block 4/18/2008
     closeconnection();
     connectionclosed = true;
     moreframes = false;
     mmode = MM_STILL;
     fastforward = false;

     sprintf(msglog,"quitpressed = %d and batchmode = %d",quitpressed, batchmode);
     writetolog(msglog);

     if(batchmode || quitpressed) {
       exit(0);
     }
    //+++++++++++++++++++++++++++++++++++++++++ end replaces the above #if 0 block 4/18/2008
     

      while((mmode == MM_MOVIE)  && (atframe <= lastframe) && fastforward ) { //catchup with back log of frames

        //char msgdebug[300];

        //sprintf(msgdebug, "top of  mfb close catchup loop cnsstartup: atframe %d mmode %d lastframe %d fastforard %d", atframe, mmode, lastframe, fastforward);
        //writetolog(msgdebug);

	//previousframe = atframe;
        previousframe = atframe -1;     
	advanceframe = true;                                 
	//display backlogged frames
	LISTOFCOMMANDS::iterator frit;
	frit = frametracker[atframe]; //frame is pointed to by the iterators in frametracker[n]
	pfvui->glView->left = ((Newframe*)(*frit))->left;
	pfvui->glView->right = ((Newframe*)(*frit))->right;
	pfvui->glView->bottom = ((Newframe*)(*frit))->bottom;
	pfvui->glView->top = ((Newframe*)(*frit))->top;
	pfvui->glView->init(); 
	pfvui->glView->invalidate();
	pfvui->glView->redraw();
	pfvui->glView->show();

	static char lbl[120];
	//sprintf(lbl, "%s   frame = %d", mf_filename.c_str(), atframe);
        sprintf(lbl, "%s", header2.c_str());                             //display title on main window

	pfvui->mainWindow->label(lbl);
	pfvui->mainWindow->redraw();
	Fl::check();
        atframe++;

        //sprintf(msgdebug, "bottom of  mfb close catchup loop cnsstartup: atframe %d mmode %d lastframe %d fastforard %d", atframe, mmode, lastframe, fastforward);
        //writetolog(msgdebug);

      } 



      fastforward = false;
      mmode = MM_STILL;
      if(atframe > lastframe)
	atframe = lastframe;
      moreframes = false;

      
      //sprintf(msglog, "sockserver:line 494: moreframes %d  atframe %d mmode %d lastframe %d fastforward %d",  moreframes, atframe, mmode, lastframe, fastforward);
      //writetolog(msglog);                                                                                                                      


      break;
    }//end of MFB_CLOSE

  }//end while 
    delete [] buffer;

  }//end of try block
  catch(bad_alloc) {
    writetolog("fatal mfsockserver.cxx cnsstartup() memory allocation for buffer");
    exit(1);
  }
}//end cnsstartup()

  
int messagehdr(void) {
  //try to read data until message header has been received and return message length
  int nrem = 0, msglength = 0, nr = 0, msgrecvd = 0;

//following replaced with select version +++++++++++++++++++++++++ 2/8/08
#if 0

  while ((nrem = sizeof(msghdr) - msgrecvd) > 0)  {
    if (ioctl(Sockin, FIONREAD, &nr) < 0) {
      sprintf(msglog,"mfsockserver in cnsstartup: Errno %d doing ioctl on msg socket.\n", errno);
      writetolog(msglog);
      exit(40);
    }
    if(nr == 0) {  //we prevent blocking

      Fl::check();


#if 0
      //obsolete now that acquisition of frame data via sockets does not depend on frame traversal for display
      if(locknextframe == 0) { //we do not want the next button to "stack" up nexts
	locknextframe = atframe; //we do not want the next button to "stack" up nexts
	//sprintf(msglog, "mfdrawsockserver.cxx -  messagehdr() : atframe = %d locknextframe = %d", atframe, locknextframe);
	//writetolog(msglog);
      }
#endif

      //sleep(1); //relenquish cpu due to polling loop
      continue;
    }

#if 0
    //obsolete now that acquisition of frame data via sockets does not depend on frame traversal for display
    else {
      locknextframe = 0;
    }
#endif

    nr = read(Sockin, ((char *)&msghdr) + msgrecvd, nrem);
    if (nr < 1) {
      sprintf(msglog,"Errno %d reading msg header.\n", errno);
      writetolog(msglog);
      exit(41);
    }
    msgrecvd += nr;

 
  }//end of while

#endif
// end of following replaced with select version +++++++++++++++++++++++++ 2/8/08

  fd_set rfds;
  struct timeval tv;
  int retval; 
  while ((nrem = sizeof(msghdr) - msgrecvd) > 0)  {
    //writetolog("in messagehdr select loop");
    //fd_set rfds;
    //struct timeval tv;
    //int retval; 
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);			   
    tv.tv_sec  = 0;            
    tv.tv_usec = 100000;  
    //Fl::check();
    retval = select(1, &rfds, NULL, NULL, &tv);
    //Fl::check();         
    if (retval == -1) {
      writetolog("Error: in messagehdr() in select()");
      perror("select()");
    }
    else if (retval) {  
      nr = read(Sockin, ((char *)&msghdr) + msgrecvd, nrem);
      if (nr < 1) {
	sprintf(msglog,"Errno %d reading msg header.\n", errno);
	writetolog(msglog);
	exit(41);
      }
      msgrecvd += nr;
    }


  }//end of while


  unsigned short int x = 0x3148;
  char *p = (char *)&x;
  if (*p == 0x31) {
    //writetolog("This machine is big endian, no byte swap since Cns data input is big endian");
    msglength = msghdr & MFB_COUNT;
  }
  else {
    //writetolog("This machine is little endian");
    msghdr = bswap_32(msghdr);                                  
    msglength = msghdr & MFB_COUNT;
  }

  //sprintf(msglog, "in messagehdr(): cns sent us msghdr with message of length = %d and hex representation (little endian) %x", msglength, msghdr);  
  //writetolog(msglog);

  
#if 0
  if(msghdr & MFB_INIT) {
    sprintf(debugmsg, "in messagehdr(): start-up buffer received atframe = %d mode = %d runmode = %d moreframes = %d",atframe, mmode, runmode, moreframes);
    writetolog(debugmsg);
  }
  if(msghdr & MFB_COMPLT) {
    sprintf(debugmsg, "in messagehdr(): last buffer for this frame received atframe = %d mode = %d runmode = %d moreframes = %d",atframe, mmode, runmode, moreframes);
    writetolog(debugmsg);
  }
  if(msghdr & MFB_CLOSE) {
    sprintf(debugmsg, "in messagehdr(): close connection after this buffer  atframe = %d mode = %d runmode = %d moreframes = %d",atframe, mmode, runmode, moreframes);
    writetolog(debugmsg);
  }
#endif
 
  //sprintf(msglog, "msghdr holds length of new buffer = %d",msglength);
  //writetolog(msglog);

  if(msgrecvd != 4) {
    writetolog("msghdr not 4 bytes long. terminating");
    exit(1);
  }

  return msglength;
}

void parsebuffer(char* buf, int len) {
  //char debugmsg[300]; //debug use only
    
  if( (len == 0) || (buf[0] == '\0') ) {
    writetolog("fatal error in mfsockserver: parsebuffer ignoring buffer len == 0 or first member == '\0'");
    exit(1);
  }
  //breaks down buffer into three headers and drawing commands 

  int nr = 0; //characters parsed
  int rc = 0; //cumulative characters parsed
  
  static int hdr = 0;
  char* pch;
  pch = strtok (buf,"\n");
  if(hdr < 3) {  //parse headers
    MFCONV MFC; 
    while (pch != NULL) {
      switch (hdr) {
      case 0: //header 1
	sprintf(msglog,"%s",pch);
	if (strncmp(pch,"PLOTDATA V1C",12) !=0) {
	  writetolog("first header has incorrect version number");
	  exit(1);
	} 
	header1 = msglog;
	break;
      case 1: //header 2
	sprintf(msglog,"%s",pch);
	header2 = msglog;
	break;
      case 2: //header 3
	sprintf(MFC.date,"%s",pch);
	sprintf(msglog,"generated on %c%c/%c%c/%c%c at hr = %c%c min = %c%c sec = %c%c" \
		,MFC.date[2],MFC.date[3],MFC.date[4],MFC.date[5],MFC.date[0],MFC.date[1] \
		,MFC.date[6],MFC.date[7],MFC.date[8],MFC.date[9],MFC.date[10],MFC.date[11]);
	header3 = msglog;
	break;
      }
      writetolog(msglog);
      nr = strlen(pch) + 1; //+1 because strtok overwrites newline delimiter with '\0'
      rc += nr;
      pch = strtok (NULL, "\n");
      if(++hdr == 3)
	break;	
    }//end of while in headers
  }//end in headers

  while( (pch != NULL)) {
    sprintf(msglog,"%s",pch); 
#ifdef MFLOG
    writetolog(msglog);
#endif
    nr = strlen(pch) + 1; //we add 1 because a strtok replaces newline with \0 and yet must be counted as received!

#if 0
    //begin partial command handling - using now obsolete pre-allocated array partialcmd

    if(rc + nr > len)  {//must be buffer straddling.  commmand doesn't have newline within buffer
      //sprintf(debugmsg, "in buffer straddling  processing frame at %d with nr = %d rc = %d and len = %d", lastframe + 1, nr, rc, len);
      //writetolog(debugmsg); 
      nr = len - rc;
      rc = len;
      if(partial == false) {
        partialcmd[0] = '\0';
        partial = true;
        //strcat(partialcmd, pch);
        strncat(partialcmd, pch, nr);
        //must be at buffer end so do not call strtok
        partialcmd[nr] = '\0';
	//sprintf(debugmsg, "partial cmd processing frame at %d with nr = %d rc = %d and len = %d", lastframe + 1, nr, rc, len);
	//writetolog(debugmsg);                        
        break;
      }

      //strcat(partialcmd, pch);
      strncat(partialcmd, pch, nr);
      pch = strtok(NULL, "\n");

      break; //we do not attempt to parse the command yet and we must be at the end of the buffer
    }//end of straddling command processing   
    if( partial == true ) {
      //strcat(partialcmd,pch);
      strncat(partialcmd, pch, nr);
      partial = false;
      rc += nr;
      mftodraw( partialcmd);
      pch = strtok(NULL, "\n");
      //sprintf(debugmsg, "end of partial cmd processing frame at %d with nr = %d rc = %d and len = %d", lastframe + 1, nr, rc, len);
      //writetolog(debugmsg);                              
      continue;
    }
    //end partial command handling
#endif


    //partial command handling using c++ string class instead of of c string.h  and heap management - this replaces above block of code 
    if(rc + nr > len)  {//must be buffer straddling.  commmand doesn't have newline within buffer
      //sprintf(debugmsg, "in buffer straddling  processing frame at %d with nr = %d rc = %d and len = %d", lastframe + 1, nr, rc, len);
      //writetolog(debugmsg);
      nr = len - rc;
      rc = len;
      if(partial == false) {

        //partialcmd[0] = '\0';
        partialcmd.clear();

        partial = true;

        //strncat(partialcmd, pch, nr);
        string tmpstr = pch;
        partialcmd = partialcmd + tmpstr.substr(0,nr);

        //must be at buffer end so do not call strtok

        //partialcmd[nr] = '\0';
        //no need to replace this
        

	//sprintf(debugmsg, "partial cmd processing frame at %d with nr = %d rc = %d and len = %d", lastframe + 1, nr, rc, len);
	//writetolog(debugmsg);                        
        break;
      }

      //strncat(partialcmd, pch, nr);
      string tmpstr = pch;
      partialcmd = partialcmd + tmpstr.substr(0,nr);

      pch = strtok(NULL, "\n");

      break; //we do not attempt to parse the command yet and we must be at the end of the buffer
    }//end of straddling command processing   
    
    if( partial == true ) {

      //strncat(partialcmd, pch, nr);
      string tmpstr = pch;
      partialcmd = partialcmd + tmpstr.substr(0,nr);

      partial = false;
      rc += nr;

      //mftodraw( partialcmd);
      mftodraw( (char*)partialcmd.c_str());
      pch = strtok(NULL, "\n");
      //sprintf(debugmsg, "end of partial cmd processing frame at %d with nr = %d rc = %d and len = %d", lastframe + 1, nr, rc, len);
      //writetolog(debugmsg);       
      continue; 
    }
    //end partial command handling - c++ string class version

    rc += nr;

    mftodraw(msglog); //we convert mf format to drawing commands. one command at a time

    pch = strtok(NULL, "\n");
    if(rc == len) {
      break;
    }
  }

  if((rc != len) && (partial == false) ) {
    writetolog(msglog); 
    writetolog("Error in mfdrawsockserver parsebuffer() parsed diff num of chars than function arg. check that parsed file ends with a newline."); 
    //printf("Error in mfdrawsockserver parsebuffer() parsed diff num of chars than function arg. check that parsed file ends with a newline.");
    exit(1);
  }
} //end parsebuffer()

//==============================support for meta file to drawing command format conversion follows ===============================

float tab[] = {1.0,10.0,100.0,1000.0,10000.0,100000.0};


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++= bit map work area starts here test

/*---------------------------------------------------------------------*
 *                                 a2f                                  *
 *         Metafile-coded ASCII floating-point number to float.         *
 *         Taken from mfxpar.c in /src/mfdraw by Marx,                  *
 *                                                                      *
 *---------------------------------------------------------------------*/

float a2fx(int scale, char *buf) {
  int cc;
  float sign =  1.0F;
  float value = 0.0F;
  ba = 0;
  if (*buf == '-') {
    buf++;
    ba++;
    sign = -1.0F;
  }
  for (;;) {
    cc = tolower(*buf++); 
    ba++;
    if (cc >= 'a') {
      value = sign * (value*10.0F + (float)(cc - 'a'))/tab[scale];
      break;
    }
    value = value*10.0F + (float)(cc - '0');
  }   
  return value;
} /* End a2fx() */

/*---------------------------------------------------------------------*
 *                                 a2i                                  *
 *                 Metafile-coded ASCII integer to int                  *
 *                 Taken from mfxpar.c in /src/mfdraw by Marx,          *
 *---------------------------------------------------------------------*/

int a2ix(char *buf) {
  int cc;
  int sign =  1;
  int value = 0;
  ba = 0;
  if (*buf == '-') {
    buf++;
    ba++;
    sign = -1;
  }
  for (;;) {
    cc = tolower(*buf++);
    ba++;
    if (cc >= 'a') {
      value = sign * (value*10 + (cc - 'a'));
      break;
    }
    value = value*10 + (cc - '0');
  }
  return value;
} /* End a2ix() */




//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++= bit map work area ends  here test


#define a2f(value,field,size,scale,type) { \
   char cc;\
   float sign=1.0; \
   value = 0.0; \
   if (*field == '-') { \
      field++; \
      sign = -1.0; \
      } \
   do { \
      cc = *field++; \
      if (cc >= 'a') { \
         value = sign * (float)(value*10 + (cc - 'a'))/tab[scale]; \
         break; \
         } \
      value = value*10 + (cc - '0'); \
      } while (1); \
   } 

#if 0
#define a2i(value,field,size,type) { \
   char cc;\
   int sign = 1; \
   value = 0; \
   if (*field == '-') { \
      field++; \
      sign = -1; \
      } \
   do { \
      cc = *field++; \
      if (cc >= 'a') { \
         value = sign * (float)(value*10 + (cc - 'a')); \
         break; \
         } \
      value = value*10 + (cc - '0'); \
      } while (1); \
   } 
#endif

//marx: replaced the a2i above with int cast to float and reassigned to an int with just no float cast
#define a2i(value,field,size,type) { \
   char cc;\
   int sign = 1; \
   value = 0; \
   if (*field == '-') { \
      field++; \
      sign = -1; \
      } \
   do { \
      cc = *field++; \
      if (cc >= 'a') { \
         value = sign * (value*10 + (cc - 'a')); \
         break; \
         } \
      value = value*10 + (cc - '0'); \
      } while (1); \
   } 

void mftodraw(char *buf) {

#if 0
  //for debug and trace purposes only - strcmp requires the exact mf command in mf format
  if( strcmp(buf,"CF222e315j2d") == 0) {
    char debugmsg[300];
    sprintf(debugmsg,"found CF222e315j2d lastframe = %d in %s at line %d", lastframe, __FILE__ , __LINE__);
    writetolog(debugmsg);
  }
#endif


  //Command *ptrendoffile;
  Newframe *ptrnewframe;
  Line *ptrline;
  Circle *ptrcircle;
  Filledcircle *ptrfilledcircle;

  Outlineellipse *ptroutlineellipse;
  Filledellipse  *ptrfilledellipse;

  Fillclosepolyline *ptrfillclosepolyline;
  Outlineclosepolyline *ptroutlineclosepolyline;
  Openpolyline *ptropenpolyline; 

  Outlinesquare *ptroutlinesquare;
  Filledsquare *ptrfilledsquare;

  Outlinerectangle *ptroutlinerectangle;
  Filledrectangle  *ptrfilledrectangle;

  Bitmap *ptrbitmap;
  Bitmapgrayscalescalable *ptrbitmapgrayscalescalable;

  Colorname *ptrcolorname;
  Colorbgr  *ptrcolorbgr;
  Colorbbggrr *ptrcolorbbggrr;
  //added Colorbbggrr *ptrcolorbbggrr on 6/20/2008


  Linethick *ptrlinethick;
  Text *ptrtext;
  //convert mf format socket buffer to drawing commmands

  //char c1 = toupper(buf[0]);           
  //char c2 = toupper(buf[1]); 
  char c1 = buf[0]; //no case restriction imposed by mfdraw on first two characters of the command
  char c2 = buf[1];

  //sprintf(debugmsg, "first command char = %c second char = %c", c1, c2);
  //writetolog(debugmsg);

  switch (c1) {
  case '[':
    {

      //stopwatch(); //for debug - will time frame to frame processing time within mftodraw()
                   //single stop watch here times inter frame timing while using a stopwatch 
                   //at the end of this frame case will measure time to proecss frame command

      int n;
      int index;
      double width;
      double height;
      double xorig;
      double yorig;
      string type;
      buf += 1;
      a2i(index,buf,4,DECENCD);
      a2f(width,buf,5,3,DECENCD);
      a2f(height,buf,5,3,DECENCD);
      a2f(xorig ,buf,5,3,DECENCD);
      a2f(yorig ,buf,5,3,DECENCD);
      a2i(n,buf,1,DECENCD);

      //sprintf(debugmsg, "new frame # %d  width = %f  height = %f xorig = %f yorig = %f type = %s", index + 1,width,height,xorig,yorig,buf);
      //writetolog(debugmsg);


      //Newframe(int frameindex, GLdouble lft, GLdouble rt, GLdouble bot, GLdouble tp, string frmtype = "")
      ptrnewframe = new Newframe(index +1, xorig, xorig + width, yorig, yorig + height, type.c_str());
      sListofCommands.push_front(ptrnewframe);

      //stopwatch(); //for debug - will time to process a frame command
  
      break;       
    }
  case ']':
    {
      //writetolog("processed end of meta file via ] command in mftodraw()");
      break;
    }

  case 'B': /* NON SCALABLE Bitmap */

    /*            DESCRIPTION AND CODE TAKEN FROM  bitmap.c              *
     *   Plots an image on the screen in the specified location and size. *
     *   Arguments:                                                       *
     *      'buf' - is a pointer to a byte array containing the data to *
     *      be plotted.  Data for each horizontal row are packed in left- *
     *      to-right order, with one bit per pixel if the 'type' is       *
     *      BM_BW, otherwise 'lci' bits per pixel (i.e. each value is     *
     *      a color index).  Data for each new row begin on a byte        *
     *      boundary and are spaced (rowlen*lci+7)/8 bytes from the start *
     *      of the previous row.  Rows are arranged in memory from top to * 
     *      bottom if 'iht' > 0, and from bottom to top if 'iht' < 0.     * 
     *                                                                    *
     *      'rowlen' and 'colht' give the size of the complete bitmap (not*
     *      just the portion being plotted by this call) in pixels. The   *
     *      length of each row in bytes implied by rowlen may be greater  *
     *      than or equal to the row length implied by the 'type', 'iwd'  *
     *      and 'lci' parameters.                                         *
     *                                                                    *
     *      'xc' and 'yc' - are the coordinates in inches of the lower    *
     *      left-hand corner of the complete bitmap relative to the       *
     *      current plot origin. These coordinates are adjusted according *
     *      to the values of 'xoffset' and 'yoffset' for this bitmap      *
     *      fragment.                                                     *
     *                                                                    *
     *      'iwd' and 'iht' - are the width and height in pixels of the   *
     *      bitmap or portion of a bitmap to be plotted by this call.     *
     *      If 'iht' > 0, the data in the 'array' are arranged in         *
     *      top-to-bottom order;  if 'iht' < 0, the data are arranged     *
     *      in bottom-to-top order. (Data are always stored in the        *
     *      metafile in top-to-bottom order.                              *
     *                                                                    *
     *      'xoffset' and 'yoffset' are positive x,y offsets (in pixels)  *
     *      of this bitmap fragment relative to the full bitmap contained *
     *      in the data array. These parameters allow bitmaps to be       *
     *      plotted in sub-tiles from different computational nodes in a  *
     *      parallel computer, and accordingly they affect both display   *
     *      coordinates and selection of data from the 'array' argument.  *
     *      The first 'xoffset' pixels of each row and the first          *
     *      'yoffset' full rows are omitted from the plot. (These rows    *
     *      are at the top of the screen if iht>0 and at the bottom if    *
     *      iht<0.)                                                       *
     *                                                                    *
     *      type - is BM_BW (0) if the bitmap contains black and white    *
     *      data, and it is BM_COLOR (4) if the bitmap contains color     *
     *      data.                                                         *
     *      mode - is GM_SET (0) to set the color index at each point     *
     *      of the bitmap unconditionally from the array data, GM_XOR     *
     *      (1) to xor the array data with any existing data at each      *
     *      point, GM_AND (2) to "and" the array data with any existing   *
     *      data at each point, and GM_CLR (3) to clear any existing      *
     *      data at points of the bitmap at which the data array is       *
     *      nonzero.                                                      *
     *                                                                    *
     * NOTE:                                                              *
     *   At the moment, 'lci' is not supported.  The data array must      *
     *   contain bit values for BM_BW mode and byte values for BM_COLOR   *
     *   mode.  The NCUBE code only supports BM_COLOR and GM_SET modes,   *
     *   although 'dmod' can probably be used to support other GM modes.  *
     *   dwtx does not use standard gcmd buffering, therefore no          *
     *   reset_grafix test and call are required.
     */
    {
      //defines moved to mfdrawsockserver.h
      //#define BM_BW     0       /* Bitmap contains black and white */
      //#define BM_GS     1       /* Bitmap is grey scale type per G. Reeke as of 5/1/2006 */ 
      //#define BM_C24    7       /* Added on 6/6/07 for RGB Bitmap 24 bit scalable and non-scalable support */ 
      //#define GM_PKD    8       /* obtained from: cns_newdoc/PLOTTING.txt:*/

      float xc,yc;

      //int iwd,iht,xofs,yofs,type,mode,length,i,j,ls,ms;
      int i,j,ls,ms;
      int xofs,yofs,iwd,iht,type,mode,length;

      int rowlen, colht;
      int c3 = toupper(buf[2]);

      type = (c2 < 'A') ? (c2 - '0') : (c2 - ('A'-10));
      mode = (c3 < 'A') ? (c3 - '0') : (c3 - ('A'-10));

      //cout<<"case 'B': /* NON SCALABLE Bitmap */ with type = "<<type<<endl;
      

      /* Set flag indicating this record already packed       */
      /* Note:  The hex data are packed in place to give the  *
       *  binary data needed by the call to img_shw().  If the *
       *  image is redrawn, e.g. due to a resize, it must not  *
       *  be repacked.  The GM_PKD bit is set in the mode byte *
       *  to indicate that packing has already been done.      */

      c3 = mode | GM_PKD;
      buf[2] = (c3 < 10) ? (c3 + '0') : (c3 + ('A'-10));

      buf += 3;             //marx appended x to the following a2f and a2i to distinguish them from previously defined a2f and a2i
      xc     = a2fx(3,buf);
      buf += ba;
      yc     = a2fx(3,buf);
      buf += ba;
      rowlen = a2ix(buf);
      buf += ba;
      colht  = a2ix(buf);
      buf += ba;
      xofs   = a2ix(buf);
      buf += ba;
      yofs   = a2ix(buf);
      buf += ba;
      iwd    = a2ix(buf);
      buf += ba;
      iht    = a2ix(buf);
      buf += ba;


      if (type == BM_BW) {
	length = iht * ((iwd+7+(xofs&7))>>3);
      }
      else {
	/* Default 'lci' is 8 bits, and for now it is the only
	 *  supported */
	length = iht * iwd;
      }

      //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ new test support for color bit map
      if (type == BM_C24) {
	length = length*3;
      }
      //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ new test support for color bit map



      //we only make one pass against input data  (marx commment)
      //if (mode & GM_PKD) {
      /* Already packed in an earlier scan */
      //   mode &= ~GM_PKD;
      //   }
      //else {

      for (i=0,j=0; j<length; i+=2,j++) {
	c3 = toupper(buf[i]);
	ms = c3 < 'A' ? c3 - '0' : c3 - ('A'-10); //if its less than A must be digit if greater or equal to A must be hex A-F (marx comment)
	c3 = toupper(buf[i+1]);
	ls = c3 < 'A' ? c3 - '0' : c3 - ('A'-10); //now ms and ls have the two hex digits   (marx comment)
	buf[j] = ls | (ms << 4);                  //pack both hex digits into a single char (marx comment)
      }
      //} //end of scope for else which has been blocked out
      /* Draw bitmap */
      //img_shw((unsigned char *)buf, xc, yc, xofs, yofs,iwd, iht, type, mode); //from bitmap.c from /src/plot


      //cout<<"while parsing at line "<<__LINE__<<" in mfdrawsockserver  non scalable bitmap:\n  type = "<<type<<" mode = "<<mode<<" xc = "<<xc<<" yc = "<<yc<<" rowlen = "<<rowlen<<" colht = "<<colht<<" xofs = "<<xofs<<" yofs = "<<yofs<<" iwd = "<<iwd<<" iht = "<<iht<<" length = "<<length<<endl<<flush;

      //char *bmp = new char[length+length];
      char *bmp = new char[length*3]; //accomodates color rgb bit map with three bytes per pixel

  
      if(iht == 0) {
	writetolog("Error bitmap height == 0");
	exit(1);
      }
      
      if(iht  < 0) {
        sprintf(msglog, "warning: in command B parsing if iht < 0 we have not yet implemented (omitted) logic for alternate row ordering in mfdrawsockserver.cxx");
	writetolog(msglog);
	for(int i = 0; i < length+length; i++)
	  bmp[i] = buf[i];
      }
      else {
	int index = 0;
	int rows = iht;
        int cols;

        if(type == BM_BW)
	  cols = ((iwd +7)/8);
        if(type == BM_GS) 
	  cols = iwd;
	
        //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ new testing for color bit map
        if(type == BM_C24) {
          //cout<<"while parsing at line "<<__LINE__<<" in mfdrawsockserver  non scalable bitmap:\n  type = "<<type<<" mode = "<<mode<<" xc = "<<xc<<" yc = "<<yc<<" rowlen = "<<rowlen<<" colht = "<<colht<<" xofs = "<<xofs<<" yofs = "<<yofs<<" iwd = "<<iwd<<" iht = "<<iht<<" length = "<<length<<endl<<flush;
	  cols = iwd;
 
          index = 0;

	  //  	   for(int i = 0; i < length; i++) {
	  //              for(int rgb = 1; rgb <=3; rgb++) { 
	  // 	      bmp[index++] = buf[3*length - index -1];
	  // 	    }
	  // 	  }

	  for(int irow = rows - 1; irow >= 0; irow--) {
	    for(int icol = 0; icol < cols; icol++) {
	      for(int rgb = 0; rgb < 3; rgb++) {
		bmp[index++] = buf[irow*cols*3 + icol*3 + rgb];
	      }
	    }
	  }  



	  float bwd, bht;
	  bwd = bht = 0; //convention for not scalable!
	  ptrbitmapgrayscalescalable = new Bitmapgrayscalescalable(type, mode, xc, yc, bwd, bht, rowlen, colht, xofs, yofs, iwd, iht, length, bmp);
	  sListofCommands.push_front(ptrbitmapgrayscalescalable);
          
	  //buf += (length+length+length); //+++++++++++++++++++ comment out test 
	}
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ new testing for color bit map

        if(type == BM_BW || type == BM_GS) {

	  for(int irow = rows - 1; irow >= 0; irow--) {
	    for(int icol = 0; icol < cols; icol++) {
	      bmp[index++] = buf[irow*cols + icol];
	    }
	  }          	    
	  if(type == BM_BW) {
	    ptrbitmap = new Bitmap(type, mode, xc, yc, rowlen, colht, xofs, yofs, iwd, iht, length, bmp);
	    sListofCommands.push_front(ptrbitmap);
	  }                    
	  if(type == BM_GS) {
	    float bwd, bht;
	    bwd = bht = 0; //convention for not scalable!
	    ptrbitmapgrayscalescalable = new Bitmapgrayscalescalable(type, mode, xc, yc, bwd, bht, rowlen, colht, xofs, yofs, iwd, iht, length, bmp);
	    sListofCommands.push_front(ptrbitmapgrayscalescalable);
	  }

	  //buf += (length+length);
	}

      }//end of else i.e. iht > 0 which implies top row goes first bottom row goes last
	   
      buf += (length+length); //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ new needs testing
 
      delete [] bmp;

      break;
    }//end case bitmap



    //start bit map grayscale and scalable
    //Bitmap record (scaled):
    //ASCII format:  "b",2Z1,4F5.d,6I4,(wd*ht)Zc
    //Binary format:  "b",2k4,x',y',w,h,6k14,(wd*ht)j
    //Data:  type,mode,x1,y1,bwd,bht,rowlen,colht,xoff,yoff,wd,ht,[skip],
    //       p11,p12,..., p21,p22,...,p(wd,ht)
    //Notes: All data items same as for unscaled bitmap except 'bwd', 'bht'.
    //       bwd,bht = width and height of full bitmap (of rowlen columns and
    //       colht rows) in inches.  The data are scaled to fit in the
    //       specified rectangular area.  (The scales in the x and y
    //       directions are not necessarily equal.)

    //as compared to:

    //Bitmap record (unscaled):
    //Decimal format:  "B",2Z1,2F5.d,6I4,(wd*ht)Zc
    //Binary format:   "B",2k4,x,y,6k14,(wd*ht)j
    //Data:  type,mode,x1,y1,rowlen,colht,xoff,yoff,wd,ht,p11,p12,...,
    //       p21,p22,...,p(wd,ht)


  case 'b' : //scaled bit map
    {

      float xc,yc;
      float bwd, bht;
      int i,j,ls,ms;
      int xofs,yofs,iwd,iht,type,mode,length;
      int rowlen, colht;

      int c3 = toupper(buf[2]);

      type = (c2 < 'A') ? (c2 - '0') : (c2 - ('A'-10));
      mode = (c3 < 'A') ? (c3 - '0') : (c3 - ('A'-10));

      //cout<<"case 'b': /* SCALABLE Bitmap */ with type = "<<type<<endl; //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++ debug only

      /* Set flag indicating this record already packed       */
      /* Note:  The hex data are packed in place to give the  *
       *  binary data needed by the call to img_shw().  If the *
       *  image is redrawn, e.g. due to a resize, it must not  *
       *  be repacked.  The GM_PKD bit is set in the mode byte *
       *  to indicate that packing has already been done.      */

      c3 = mode | GM_PKD;
      buf[2] = (c3 < 10) ? (c3 + '0') : (c3 + ('A'-10));

      buf += 3;             //marx appended x to the following a2f and a2i to distinguish them from previously defined a2f and a2i
      xc     = a2fx(3,buf);
      buf += ba;
      yc     = a2fx(3,buf);
      buf += ba;
           
      bwd     = a2fx(3,buf);
      buf += ba;
      bht     = a2fx(3,buf);
      buf += ba;   
     
      rowlen = a2ix(buf);
      buf += ba;
      colht  = a2ix(buf);
      buf += ba;
      xofs   = a2ix(buf);
      buf += ba;
      yofs   = a2ix(buf);
      buf += ba;
      iwd    = a2ix(buf);
      buf += ba;
      iht    = a2ix(buf);
      buf += ba;

      if (type == BM_BW) {
        //cout<<"type == BM_BW"<<" where BM_BW = "<<BM_BW<<endl;  //for metafile mode debug
	length = iht * ((iwd+7+(xofs&7))>>3);
      }
      else {
	/* Default 'lci' is 8 bits, and for now it is the only
	 *  supported */
	length = iht * iwd;
      }


      //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ new test support for color bit map
      if (type == BM_C24) {
	length = length*3;
      }
      //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ new test support for color bit map

     
      //sprintf(debugmsg, "in mfdrawtodraw file = %s line = %d -- with iwd = %d and iht = %d and length = %d", __FILE__, __LINE__, iwd, iht, length);
      //writetolog(debugmsg);

      for (i=0,j=0; j<length; i+=2,j++) {
	c3 = toupper(buf[i]);
	ms = c3 < 'A' ? c3 - '0' : c3 - ('A'-10); //if its less than A must be digit if greater or equal to A must be hex A-F (marx comment)
	c3 = toupper(buf[i+1]);
	ls = c3 < 'A' ? c3 - '0' : c3 - ('A'-10); //now ms and ls have the two hex digits   (marx comment)
	buf[j] = ls | (ms << 4);                  //pack both hex digits into a single char (marx comment)
      }

      /* Draw bitmap */
      //img_shw((unsigned char *)buf, xc, yc, xofs, yofs,iwd, iht, type, mode); //from bitmap.c from /src/plot
      //cout<<"while parsing at line "<<__LINE__<<" in mfdrawsockserver gray scale scalable bitmap:\n  type = "<<type<<" mode = "<<mode<<" xc = "<<xc<<" yc = "<<yc
      //   <<" bwd = "<<bwd<<" bht = "<<bht<<" rowlen = "<<rowlen<<" colht = "<<colht
      //   <<" xofs = "<<xofs<<" yofs = "<<yofs<<" iwd = "<<iwd<<" iht = "<<iht<<" length = "<<length<<endl<<endl<<flush;

      //char *bmp = new char[length+length];
      char *bmp = new char[length*3]; //accomodates color rgb bit map with three bytes per pixel

   
      if(iht == 0) {
	writetolog("Error grayscale bitmap height == 0");
	exit(1);
      }

      if(iht  < 0) { 
        sprintf(msglog, "warning: in command b parsing if iht < 0 we have omitted logic in mfdrawsockserver.cxx");
	writetolog(msglog);
	for(int i = 0; i < length+length; i++)
	  bmp[i] = buf[i];
      }
      else {
	// 	int index = 0;
	// 	int rows = iht;
	// 	//int cols = ((iwd +7)/8);                             //removed this line. gray scale works with bytes not bits
	//         int cols = iwd;
	//                                         //added to replace remove above
	// 	for(int irow = rows - 1; irow >= 0; irow--) {
	// 	  for(int icol = 0; icol < cols; icol++) {
	// 	    bmp[index++] = buf[irow*cols + icol];
	// 	  }
	// 	}  

        int index = 0;
	int rows = iht;
        int cols;

        if(type == BM_BW)
	  cols = ((iwd +7)/8);
        if(type == BM_GS) 
	  cols = iwd;
	
        if(type == BM_C24) {
          //cout<<"while parsing at line "<<__LINE__<<" in mfdrawsockserver  non scalable bitmap:\n  type = "<<type<<" mode = "<<mode<<" xc = "<<xc<<" yc = "<<yc<<" rowlen = "<<rowlen<<" colht = "<<colht<<" xofs = "<<xofs<<" yofs = "<<yofs<<" iwd = "<<iwd<<" iht = "<<iht<<" length = "<<length<<endl<<flush;
	  cols = iwd;
 
          index = 0;

	  //  	   for(int i = 0; i < length; i++) {
	  //              for(int rgb = 1; rgb <=3; rgb++) { 
	  // 	      bmp[index++] = buf[3*length - index -1];
	  // 	    }
	  // 	  }

	  for(int irow = rows - 1; irow >= 0; irow--) {
	    for(int icol = 0; icol < cols; icol++) {
	      for(int rgb = 0; rgb < 3; rgb++) {
		bmp[index++] = buf[irow*cols*3 + icol*3 + rgb];
	      }
	    }
	  } 

	  ptrbitmapgrayscalescalable = new Bitmapgrayscalescalable(type, mode, xc, yc, bwd, bht, rowlen, colht, xofs, yofs, iwd, iht, length, bmp);
	  sListofCommands.push_front(ptrbitmapgrayscalescalable);

	}

	if(type == BM_BW || type == BM_GS) {

	  for(int irow = rows - 1; irow >= 0; irow--) {
	    for(int icol = 0; icol < cols; icol++) {
	      bmp[index++] = buf[irow*cols + icol];
	    }
	  }          	    
	  if(type == BM_BW) {
	    ptrbitmap = new Bitmap(type, mode, xc, yc, rowlen, colht, xofs, yofs, iwd, iht, length, bmp);
	    sListofCommands.push_front(ptrbitmap);
	  }                    
	  if(type == BM_GS) {
	    ptrbitmapgrayscalescalable = new Bitmapgrayscalescalable(type, mode, xc, yc, bwd, bht, rowlen, colht, xofs, yofs, iwd, iht, length, bmp);
	    sListofCommands.push_front(ptrbitmapgrayscalescalable);
	  }

	  //buf += (length+length);
       
	}

      }//end of else i.e. iht > 0 which implies top row goes first bottom row goes last

      buf += (length+length);
      delete [] bmp;

      break;    
    }//end bit map grayscale and scalable

  case 'C':
    {
      double x, y, r;
      buf += 2;
      /* Get float arguments */
      a2f(x,buf,5,3,DECENCD);
      a2f(y,buf,5,3,DECENCD);
      a2f(r,buf,5,3,DECENCD);
      if (c2 == 'F') {
	/* draw filled circle */
	//sprintf(msglog, "filled circle: x center = %f  y center = %f radius = %f", x, y, r);
	//writetolog(msglog);
        ptrfilledcircle = new Filledcircle(x,y,r);
        sListofCommands.push_front(ptrfilledcircle);
      }
      else if (c2 == 'O') {
	/* draw outlined circle */
	//sprintf(msglog, "outlined circle: x center = %f  y center = %f radius = %f", x, y, r);
	//writetolog(msglog);
        ptrcircle = new Circle(x,y,r);
        sListofCommands.push_front(ptrcircle);       
      }  
      break;
    }
  case 'E':
    {
      double x,y,w,h,angle;
      buf += 2;
      /* Get float arguments */
      a2f(x,buf,5,3,DECENCD);
      a2f(y,buf,5,3,DECENCD);
      a2f(w,buf,5,3,DECENCD);
      a2f(h,buf,5,3,DECENCD);
      a2f(angle,buf,7,3,DECENCD);
      if (c2 == 'F') {
	/*draw filled ellipse */
	//sprintf(msglog, "filled ellipse: x center = %f  y center = %f width = %f height = %f angle = %f", x, y, w, h, angle);
	//writetolog(msglog);
        ptrfilledellipse = new Filledellipse(x,y,w,h,angle);
        sListofCommands.push_front(ptrfilledellipse);
      }
      else if (c2 == 'O') {
	/*draw outlined ellipse*/
	//sprintf(msglog, "outlined ellipse: x center = %f  y center = %f width = %f height = %f angle = %f", x, y, w, h, angle);
	//writetolog(msglog);
        ptroutlineellipse = new Outlineellipse(x,y,w,h,angle);
        sListofCommands.push_front(ptroutlineellipse);       
      }            
      break;
    }
  case 'H': 
    {
      int krt;
      buf += 1;
      a2i(krt,buf,1,DECENCD);               
      /*Heaviness of lines)*/  
      //sprintf(msglog, "retrace command or heaviness = %d",krt);
      //writetolog(msglog);
      ptrlinethick = new Linethick(krt);
      sListofCommands.push_front(ptrlinethick);  
      break;
    }
  case 'K':
    {
      /*hex color bgr and named color parse ============= */
      //each hex digit converted to int

#define hexch_to_int(c) ((((c))<='9')?(c)-'0':(c)-'A'+10)
      if(buf[2] == 'X') {
	int BB = (hexch_to_int(buf[3]));
	int GG = (hexch_to_int(buf[4]));
	int RR = (hexch_to_int(buf[5]));
	//sprintf(msglog, "color bgr: %d %d %d ",BB, GG, RR);
        //printf(msglog);
	//writetolog(msglog);
        ptrcolorbgr = new Colorbgr(BB,GG,RR);
        sListofCommands.push_front(ptrcolorbgr);
      } else if( buf[2] == 'Z') {

        //printf("at %s line %d hex characters %c %c %c %c %c %c\n", __FILE__ ,  __LINE__ , buf[3], buf[4], buf[5], buf[6], buf[7],buf[8]);
         
	int BB1 = (hexch_to_int(buf[3]));
        int BB2 = (hexch_to_int(buf[4]));
        int GG1 = (hexch_to_int(buf[5]));
        int GG2 = (hexch_to_int(buf[6]));
        int RR1 = (hexch_to_int(buf[7]));
        int RR2 = (hexch_to_int(buf[8]));

        int BB = BB1<<4;
	BB = BB | BB2;
        int GG = GG1<<4;
	GG = GG | GG2;
        int RR = RR1<<4;
	RR = RR | RR2;

	//printf("case K subcommand Z put on list\n");
 
        ptrcolorbbggrr = new Colorbbggrr(BB,GG,RR);
        sListofCommands.push_front(ptrcolorbbggrr);
      }
      //added subcommand Z on 6/20/2008
      else {
	//sprintf(msglog, "color %s", buf);
	//writetolog(msglog);
        ptrcolorname = new Colorname(buf+2);
        sListofCommands.push_front(ptrcolorname);	      
      }
      break;
    }
  case 'L': 
    {
      double x1, y1, x2, y2;          
      buf += 1;
      /* Get float arguments */
      a2f(x1,buf,5,3,DECENCD);
      a2f(y1,buf,5,3,DECENCD);
      a2f(x2,buf,5,3,DECENCD);
      a2f(y2,buf,5,3,DECENCD);
      //sprintf(msglog,"line (x1,y1) to (x2,y2) %g %g %g %g ", x1,y1,x2,y2);
      //writetolog(msglog);
      ptrline = new Line(x1,y1,x2,y2);
      sListofCommands.push_front(ptrline);
      break;
    }
  case 'P':
    {
      double *vertex;
      int np;
      buf += 2;
      /* Get float arguments */
      a2i(np,buf,3,DECENCD);
      if( (np*2*(int)sizeof(double)+ 3*(int)sizeof(int))  > MAXCOMMANDSIZE ) {  
        writetolog("Error in processing P command: need to enlarge MAXCOMMANDSIZE");
        exit(1);
      }

      //sprintf(debugmsg, "did a polyline and np = %d\n", np);
      //writetolog(debugmsg);

      if(c2 == 'F') {
	try {
	  vertex = new double[3*np];
	}
	catch(const bad_alloc &x) {
	  writetolog((char*)x.what());
	  abort();
	}
 
#if 0
	//+++++++++++++++++++++++ test only
	static bool fillpoly = false; 
	if(fillpoly == false) {    
	  fillpoly = true;            
	}                            
	else                        
	  return;                  
	printf("number of vertices %d\n", np);  
	//++++++++++++++++++++++++ test only
#endif

	for(int i = 0; i < 3*np; i += 3) {

	  //FOR TESTING:
	  //use command PXd in smarx_ellipse.mf meta file where X is F or O or L and the d is one more (starting at letter a) than the number of points (np)
	
	  //ELSE uncomment the following three lines
	  a2f(vertex[i],buf,5,3,DECENCD);
	  a2f(vertex[i+1],buf,5,3,DECENCD);

	  vertex[i+2] = 0.0;
#if 0
	  printf("vertex %d has x = %g and y = %g  and z = %g\n", i/2, vertex[i], vertex[i+1],  vertex[i+2]); //++++++++++++++++++++++++++++++++++= test only
#endif

	}



#if 0
	//use command PXi and np = 8 for a concave polygon
	//vertex[2i] and vertex[2i+1] and vertex[2i+2] are (x,y,z) for vertex number i
	//vertex[2i+2] is required for Fillclosepolyline due to undocumented
	//limitation that tessellated polygons have 3 dimensional vertices even if polygon is two dimensional.
	vertex[0] = 1.0; //0
	vertex[1] = 4.0;

	vertex[3] = 2.0; //1
	vertex[4] = 4.0;

	vertex[6] = 3.0; //2
	vertex[7] = 5.0;

	vertex[9] =  4.0; //3
	vertex[10] = 4.0;

	vertex[12] = 5.0; //4
	vertex[13] = 4.0;

	vertex[15] = 4.0; //5
	vertex[16] = 1.0;

	vertex[18] = 3.0; //6
	vertex[19] = 4.0;

	vertex[21] = 2.0; //7
	vertex[22] = 1.0;
#endif

        //sprintf(msglog, "in mfsockserver.cxx line 697: draw closed polyline-filled");
        //writetolog(msglog);
        ptrfillclosepolyline = new Fillclosepolyline(np, vertex);
        sListofCommands.push_front(ptrfillclosepolyline);
      }
      else if(c2 == 'O') {
        //sprintf(debugmsg, "draw closed polyline-outlined with np = %d points", np);
        //writetolog(debugmsg);

	try {
	  vertex = new double[2*np];
	}
	catch(const bad_alloc &x) {
	  writetolog((char*)x.what());
	  abort();
	}
	for(int i = 0; i < 2*np; i += 2) {

	  //FOR TESTING:
	  //use command PXd in smarx_ellipse.mf meta file where X is F or O or L and the d is one more (starting at letter a) than the number of points (np)
	
	  //ELSE uncomment the following two lines
	  a2f(vertex[i],  buf,5,3,DECENCD);
	  a2f(vertex[i+1],buf,5,3,DECENCD);

	}

#if 0
	//use command PXi and np = 8 for a concave polygon
	//vertex[2i] and vertex[2i+1] and vertex[2i+2] are (x,y,z) for vertex number i
	//vertex[2i+2] is required for Fillclosepolyline due to undocumented
	//limitation that tessellated polygons have 3 dimensional vertices even if polygon is two dimensional.
	vertex[0] = 1.0; //0
	vertex[1] = 4.0;

	vertex[2] = 2.0; //1
	vertex[3] = 4.0;

	vertex[4] = 3.0; //2
	vertex[5] = 5.0;

	vertex[6] = 4.0; //3
	vertex[7] = 4.0;

	vertex[8] = 5.0; //4
	vertex[9] = 4.0;

	vertex[10] = 4.0; //5
	vertex[11] = 1.0;

	vertex[12] = 3.0; //6
	vertex[13] = 4.0;

	vertex[14] = 2.0; //7
	vertex[15] = 1.0;
#endif

        ptroutlineclosepolyline = new Outlineclosepolyline(np, vertex);
        sListofCommands.push_front(ptroutlineclosepolyline);
      }

      else if(c2 == 'L') {
        //sprintf(debugmsg, "draw open polyline with np = %d points", np);
        //writetolog(debugmsg);
	try {
	  vertex = new double[2*np];
	}
	catch(const bad_alloc &x) {
	  writetolog((char*)x.what());
	  abort();
	}
        for(int i = 0; i < 2*np; i += 2) {

	  //FOR TESTING:
	  //use command PXd in smarx_ellipse.mf meta file where X is F or O or L and the d is one more (starting at letter a) than the number of points (np)
	
	  //ELSE uncomment the following two lines
	  a2f(vertex[i],buf,5,3,DECENCD);
	  a2f(vertex[i+1],buf,5,3,DECENCD);
        
#if 0
	  if( ( (lastframe+1) == 43 ) ) {//++++++++++++++++++++++++++++++++++++++++ test
	    char debugmsg[300];
	    sprintf(debugmsg, "open polyline vertex[%d] = %g vertex[%d] = %g ", i, vertex[i], i+1, vertex[i+1]);
	    writetolog(debugmsg); 
	  } //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ end test 
#endif                                                                         
                                                                 

	}

#if 0
	//use command PXi and np = 8 for a concave polygon
	//vertex[2i] and vertex[2i+1] and vertex[2i+2] are (x,y,z) for vertex number i
	//vertex[2i+2] is required for Fillclosepolyline due to undocumented
	//limitation that tessellated polygons have 3 dimensional vertices even if polygon is two dimensional.
	vertex[0] = 1.0; //0
	vertex[1] = 4.0;

	vertex[2] = 2.0; //1
	vertex[3] = 4.0;

	vertex[4] = 3.0; //2
	vertex[5] = 5.0;

	vertex[6] = 4.0; //3
	vertex[7] = 4.0;

	vertex[8] = 5.0; //4
	vertex[9] = 4.0;

	vertex[10] = 4.0; //5
	vertex[11] = 1.0;

	vertex[12] = 3.0; //6
	vertex[13] = 4.0;

	vertex[14] = 2.0; //7
	vertex[15] = 1.0;
#endif

        ptropenpolyline = new Openpolyline(np, vertex);
        sListofCommands.push_front(ptropenpolyline);      
      }
      //delete [] vertex; //error: cannot delete vertex array !
      break;
    }
  case 'Q':
    {
      double x,y,e;
      buf += 2;
      /* Get float arguments */
      a2f(x,buf,5,3,DECENCD);
      a2f(y,buf,5,3,DECENCD);
      a2f(e,buf,5,3,DECENCD);
      if (c2 == 'F') {
	/* Draw filled box */
	ptrfilledsquare = new Filledsquare(x,y,e);
	sListofCommands.push_front(ptrfilledsquare);
      }
      else if (c2 == 'O') {
	/* Draw outline box  */
	ptroutlinesquare = new Outlinesquare(x,y,e);
	sListofCommands.push_front(ptroutlinesquare);
      }	   
      break;
    }

  case 'R':
    {
      double x1,y1,x2,y2;
      buf += 2;
      /* Get float arguments */
      a2f(x1,buf,5,3,DECENCD);
      a2f(y1,buf,5,3,DECENCD);
      a2f(x2,buf,5,3,DECENCD);
      a2f(y2,buf,5,3,DECENCD);
      if (c2 == 'F') {
	/* Draw filled box */
	ptrfilledrectangle = new Filledrectangle(x1,y1,x2,y2);
	sListofCommands.push_front(ptrfilledrectangle);
      }
      else if (c2 == 'O') {
	/* Draw outline box  */
	ptroutlinerectangle = new Outlinerectangle(x1,y1,x2,y2);
	sListofCommands.push_front(ptroutlinerectangle);
      }	   
      break;
    }

  case 'S':
    {
      double x, y, ht, angle;
      string txt;
      int length;
      buf += 1;
      a2f(x,buf,5,3,DECENCD);
      a2f(y,buf,5,3,DECENCD);
      a2f(ht,buf,5,3,DECENCD);
      a2f(angle,buf,7,3,DECENCD);
      a2i(length ,buf,3,DECENCD);
      txt = buf;

      //sprintf(debugmsg,"%s at angle = %g",txt.c_str(), angle);
      //writetolog(debugmsg);

      ptrtext = new Text(x, y, ht, angle, length, txt);
      sListofCommands.push_front(ptrtext);
      txt.clear();          
      break;
    }
  default:
    { 
      if( (c1 == 'I') || (c1 == 'M') ) //we do not support color index 'I' or pen 'M'
	break;
      sprintf(msglog, "fatal error: mfsockserver mftodraw found unsupport command %c \n ", c1);
      writetolog(msglog);
      exit(1);
    }
  }//end of switch

}//end of mftodraw

#if 0
void closeconnection(void) {
  //sprintf(debugmsg, "in start of close connection(): msghdr = %d", msghdr);
  //writetolog(debugmsg);

  //cnsstartup(); //need to get next buffer if we are at the last frame but not yet received MFB_CLOSE and no harm if we weren't - OBSOLETE 
  
  if(msghdr & MFB_CLOSE) { 
    moreframes = false;
    //writetolog("in closeconnection:  science model sent an MFB_CLOSE. We generate close connection to science model");
    //return;           
  }
  else {
    //writetolog("in closeconnection: science model did NOT send a MFB_CLOSE. mfdraw will generate close connection to science model");
  }
  
  int numsent = 0;
  //writetolog("closeconnection(): generate close connection message to science model");

  msgbutt.movie    = (char)mmode;
  msgbutt.next     = (char)0;
  msgbutt.intxeq   = (char)2;  //terminate
  msgbutt.snapshot = (char)(0); // added this 3/7/08

  //sprintf(debugmsg, " before end of closeconnection() atframe = %d: sent movie = %d next = %d intxeq = %d snapshot = %d", atframe,
  //	        msgbutt.movie, msgbutt.next, msgbutt.intxeq,  msgbutt.snapshot);
  //writetolog(debugmsg); 

  do {
    numsent += write(Sockout, &msgbutt, sizeof(msgbutt));
    if(numsent < 0)   {
      writetolog("error in writing close connection to Cns(1)");
      sprintf(msglog, "mfsockserver error %d when attempting to write msgbutt struct to client.\n", errno);
      writetolog(msglog);
      //exit(62); replaced by 
      break; //replaces above - we do not try to notify cns of closed connection
    }
  }while(numsent < (int)sizeof(msgbutt));

  moreframes = false; 
  //sleep(1); // provides time for Cns to make an orderly shutdown.  added 3/7/08
  fastforward = false;
  mmode = MM_STILL;
  if(atframe > lastframe)
    atframe = lastframe;
  //delete [] buffer; //cannot remove buffer here this is a bug
 
  sprintf(debugmsg, "* at end of closeconnection() atframe = %d: sent movie = %d next = %d intxeq = %d snapshot = %d", atframe, \
	  msgbutt.movie, msgbutt.next, msgbutt.intxeq,  msgbutt.snapshot);
  writetolog(debugmsg);                                                           
}
#endif


void closeconnection(void) {  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ new as 4/18/2008
      int numsent = 0;
  //writetolog("closeconnection(): generate close connection message to science model");

  msgbutt.movie    = (char)mmode;
  msgbutt.next     = (char)0;
  msgbutt.intxeq   = (char)2;  //terminate
  msgbutt.snapshot = (char)(0); // added this 3/7/08

  //sprintf(debugmsg, " before end of closeconnection() atframe = %d: sent movie = %d next = %d intxeq = %d snapshot = %d", atframe,
  //	        msgbutt.movie, msgbutt.next, msgbutt.intxeq,  msgbutt.snapshot);
  //writetolog(debugmsg); 

  do {
    numsent += write(Sockout, &msgbutt, sizeof(msgbutt));
    if(numsent < 0)   {
      writetolog("error in writing close connection to Cns(1)");
      sprintf(msglog, "mfsockserver error %d when attempting to write msgbutt struct to client.\n", errno);
      writetolog(msglog);
      //exit(62); replaced by 
      break; //replaces above - we do not try to notify cns of closed connection
    }
  }while(numsent < (int)sizeof(msgbutt));
}


void interruptcns(void) {
  if( !(msghdr & MFB_COMPLT) ) {
    fl_alert("we interrupted cns within an incomplete frame? this is appears to be an error we are terminating");
    exit(1);
  }
  //char msglog[2000];  
  int numsent = 0;
  //writetolog("send interrupt to science model");
  msgbutt.movie    = (char)mmode;
  msgbutt.next     = (char)0;    
  msgbutt.intxeq   = (char)1;    //interrupt
  msgbutt.snapshot = (char)0; 
 
  do {
    numsent += write(Sockout, &msgbutt, sizeof(msgbutt));
 
    //writetolog("sent interrupt intxeq = 1 via msgbutt to science model"); //++++++++++++++++++++++++++++++++ 4/3/2008

    if(numsent < 0)   {
      writetolog("error in writing interrupt to science model");
      sprintf(msglog, "mfsockserver error %d writing button status to client during interrupt command.\n", errno);
      writetolog(msglog);
      exit(62);
    }
  }while(numsent < (int)sizeof(msgbutt));
}

void stdin_cb(int, void*) {

  Fl::remove_fd(0);

  //if( mmode == MM_MOVIE &&  ( fastforward == false || atframe < lastframe || moreframes == false) ) { // ++++++++++++++++++++++++++++++++=+++ 01/09/08
  //  return;
  //}

  //char debugmsg[300];
  //sprintf(debugmsg,"start stdin_cb: atframe = %d, lastframe = %d, fastforward = %d",atframe, lastframe, fastforward);
  //writetolog(debugmsg);
  cnsstartup();
  //sprintf(debugmsg,"end stdin_cb: atframe = %d, lastframe = %d, fastforward = %d",atframe, lastframe, fastforward);
  //writetolog(debugmsg);
}

void cmdline_metafile(void) {
  if(runmode == 1) {
    fl_alert("you are in cns socket mode. you cannot use the File control to input data.");
    //o->deactivate();
    mf_filename = "cns socket mode ";
    return;
  }
  static char filename[120];
  static bool metaopened = false;

  //sprintf(filename, "%s", fl_file_chooser("Pick a meta data file.", "Meta Command Files (*.mf)", "", 0)); //orig
  strcpy(filename, mf_filename.c_str()); //replaces line above
  string s(filename); //orig


  if(s == "(null)")
    return;
  if(metaopened == false)
    metaopened = true;
  else {
    fl_alert("cannot open metafile if metafile is already open");
    return;
  }

  deletedata(); //clear out any data and initialize prior to loading new data

  //createdata(filename);
  createdata(mf_filename.c_str());

  buildframetracker();
  //cout<<"at file "<<__FILE__<<" line "<<__LINE__<<" lastframe = "<<lastframe<<endl;

  LISTOFCOMMANDS::iterator frit;
  frit = frametracker[atframe]; //frame is is pointed to by the iterators in frametracker[n]
  pfvui->glView->left = ((Newframe*)(*frit))->left;
  pfvui->glView->right = ((Newframe*)(*frit))->right;
  pfvui->glView->bottom = ((Newframe*)(*frit))->bottom;
  pfvui->glView->top = ((Newframe*)(*frit))->top;
  pfvui->glView->invalidate();
  pfvui->glView->redraw();
  glFlush();


  static char versionandfile[120];
  pfvui->mainWindow->label("");
  sprintf(versionandfile, "%s using file %s", pfvui->mainWindow->label(), filename);
  mf_filename = versionandfile;


  static char firstlabel[120];
  //sprintf(firstlabel,"using file %s at frame %d", filename, atframe);
  sprintf(firstlabel, "%s", header2.c_str());                             //display title on main window
  pfvui->mainWindow->label(firstlabel);


  //char logmsg[300];
  //sprintf(logmsg, "runmode = %d data from file not from a socket to Cns", runmode);
  //writetolog(logmsg);

  //zoom->take_focus();
  pfvui->zoom->value(1);
  pfvui->glView->init();

  runmode = 0; 
}
