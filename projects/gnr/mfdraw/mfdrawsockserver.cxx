// (c) Copyright 2008-2017, The Rockefeller University *21500*
/*##################################################################
  #               File mfsockserver.cxx                            #
  #                                                                #
  # Version                     3/7/2006             Steven Marx   #
  # Rev, 10/02/08, GNR - Obey display name from socket file header #
  # Rev, 01/14/09, GNR - Handle MM_BATCH mode                      #
  # Rev, 04/15/09, GNR - Further fixes to make MM_BATCH mode work  #
  # Rev, 06/17/09, GNR - Change log open from O_WRONLY to O_RDWR   #
  # Rev, 08/16/16, GNR - Startup ratio field-->l_icon_name,version #
  # Rev, 09/02/16, GNR - Remove O_APPEND flag from log file open   #
  # Rev, 05/01/17, GNR - Add some temp code to implement 'Draw'    #
  # Rev, 05/17/17, GNR - Remove BM_C16, add BM_GS16, BM_C48        #
  # Rev, 05/26/17, GNR - Construct colorname,Text w/string length  #
  # Rev, 12/31/17, GNR - Retry on error EAGAIN reading header      #
  ##################################################################

  Functions to support the socket connectivity between mfdraw
  and cns.

  Note: mfdraw performs all of it's socket communication,
  relying on xinetd, via standard in and standard out.

  Note (11/03/08, GNR) - I took out the part of smarx's cnsstartup
    function that is used on every buffer and made it into a new
    cnsckinput function that can be called from the fltk add_fd.
    This keeps the current setup as nearly unmodified as possible.
    Now cnsstartup really only does startup and is called only
    from main before anything else happens.
  Note (05/01/17, GNR) - smarx never implemented the concept of
    "current plotting position", so the basic plot() call cannot
    be implemented as defined without a lot of work.  Just to have
    something for simple tests, I made the line() call save its
    current plotting position and the 'D' and 'M' codes work with
    that.
  ###################################################################*/

#include "mfdraw.h"
#include "mfdrawsockserver.h"
#include "mfdrawUI.h"
//mfdrawUI can be used to provide access to FrameViewUI *pfvui for example pfvui->glView

static char *buffer;
static double lastx, lasty;
static int ba; //advance buffer amount

bool partial = false;      //support for buffer straddling commands
string partialcmd;         //replace heap maanagement with string class
static int
   msglength = 0,          /* Length of current command buffer */
   msgrecvd  = 0;          /* Length of message already received */

extern char msglog[];      //for debugging  writetolog traces

/*=====================================================================*
*                                error                                 *
*=====================================================================*/

void error(char *msg) {
   perror(msg);
   exit(1);
   }

/*=====================================================================*
*                             writetolog                               *
*=====================================================================*/

void writetolog(const char* buf) {
   static int fd = -1;
   if (fd == -1) { //create file if does not exist or open if closed
      //portable but not friendly
      if ((fd = open("/var/tmp/mfdraw.server.log",
         O_CREAT| O_RDWR, 0666)) < 0) exit(1);
      }

  //comment out the #if and #endif if you want to timestamp log entries
  //#if 0
  //create and log time stamp
  time_t timebuf;
  time(&timebuf);
  int tlen  = strlen(ctime(&timebuf));
  char msg[tlen+1];
  strncpy(msg, ctime(&timebuf), tlen-1);
  //The string returned by ctime() is in the form: Sun Sep 16 01:03:52 1973\n\0
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
  if (!strcmp(buf, "close")) {
    write(fd,"------------------------------\n",31);
    close(fd);
    fd = -1;
    return;
    }
}

/* I guess these are globals  --GNR */
static int lcmdbuf;
static int msghdr  = 0;
butt msgbutt;
int mmode,initmmode;

/*=====================================================================*
*                             cnsckinput                               *
*=====================================================================*/

//GNR new name for non-startup part of old cnsstartup
void cnsckinput(void) {

#if DEBUG & DBGCKINPUT
  sprintf(msglog, "in file %s at line %d: entered cnsckinput at "
     "frame = %d previousframe = %d lastframe = %d mmode = %d",
    __FILE__, __LINE__, atframe,previousframe, lastframe, mmode);
  writetolog(msglog);
#endif

  try {

    if (msghdr & MFB_CLOSE) {
#if DEBUG & DBGCKINPUT
      sprintf(msglog, "at line %d: connection to cns closed. cnsckinput "
         "returns to caller with no additional drawing commands, "
         "atframe = %d, lastframe = %d", __LINE__,
         atframe, lastframe);
      writetolog(msglog);
#endif
      moreframes = false;
      closeconnection();
      if (initmmode == MM_BATCH) {
        exit(0);
      }
      return;
    }

    //in case we call cnsckinput after having add_fd(0)
    // Note: a useless remove can do no harm!
    Fl::remove_fd(0);

    dialogloop: while(1) {

    msglength = messagehdr(); //assumption: msghdr not included in length;

#if DEBUG & DBGCKINPUT
    sprintf(msglog, "sockserver line %d: start dialogloop cnsckinput "
      "main read/write loop. we are at atframe = %d previousframe = %d "
      "lastframe = %d  msglength = %d",
      __LINE__, atframe, previousframe, lastframe, msglength);
    writetolog(msglog);
#endif

    if (msglength > lcmdbuf) {
      lcmdbuf = msglength + 1;
      delete [] buffer;
      buffer = new char[lcmdbuf];
#if DEBUG & DBGCKINPUT
      writetolog("expanded buffer in dialogloop");
#endif
    }
    msgrecvd = 0;
    do {
      int nr = read(Sockin, buffer + msgrecvd, msglength);
      if ( nr == 0 && msglength == 0) {
        //sprintf(msglog, "sockserver at line %d atframe = %d got from Cns: read returned 0 and msglength = 0",__LINE__,  atframe);
        //writetolog(msglog);
        //we continue with the outermost dialog loop when cns responds
        // with a 0 message length which appears to be a response to
        // our interrupt command
        goto dialogloop;
      }
      if ( nr < 0 ) {
        writetolog("fatal error reading socket in CnsSockserver");
        exit(1);
      }
      msglength -= nr;
      msgrecvd  += nr;

#if DEBUG & DBGCKINPUT
      sprintf(msglog, "in file %s at line %d: got %d chars from stdin "
         "with cum amt. for buffer of %d with %d remaining",
         __FILE__, __LINE__, nr, msgrecvd, msglength);
      writetolog(msglog);
#endif

    } while (msglength > 0);

    if (msglength < 0) {
      writetolog("fatal error: msglength after read in CnsGockServer is < 0");
      exit(1);
    }

    parsebuffer(buffer, msgrecvd);

    if (msghdr & MFB_COMPLT) {

      Fl::remove_fd(0);

#if 0
// A little bit of crazy code to debug sending error msgs to CNS */
     char fakemsg[] = "This is a test of emsg code.";
     write(Sockout, fakemsg, sizeof(fakemsg));
     close(Sockout);
     exit(63);
#endif

      if (msglength == 0) {
        msgbutt.movie    = (char)mmode;
        msgbutt.next     = (char)1;
        msgbutt.intxeq   = (char)0;
        msgbutt.snapshot = (char)0;

#if DEBUG & DBGCKINPUT
        sprintf(msglog, "line %d in MFB_COMPLT just BEFORE Cns request "
         "for next buffer atframe = %d with mode = %d fastforward = %d",
         __LINE__, atframe, mmode, fastforward);
        writetolog(msglog);
        //stopwatch();
        //stopwatch();
#endif

#if DEBUG & DBGSOCKW
         sprintf(msglog, "line %d in cnsckinput, writing msgbutt to socket.",
            __LINE__);
         writetolog(msglog);
         sprintf(msglog,"   movie = %d, next = %d, intxeq = %d, snap = %d",
            (int)msgbutt.movie, (int)msgbutt.next, (int)msgbutt.intxeq,
            (int)msgbutt.snapshot);
         writetolog(msglog);
#endif
        if (write(Sockout, &msgbutt, sizeof(msgbutt)) < (int)sizeof(msgbutt)) {
          sprintf(msglog, "mfsockserver error %d writing button status to client.\n", errno);
          writetolog(msglog);
          exit(62);
        }

#if DEBUG & DBGCKINPUT
        sprintf(msglog,"line %d in MFB_COMPLT just AFTER Cns request for "
         "next buffer atframe = %d with mode = %d fastforward = %d",
         __LINE__, atframe, mmode, fastforward);
        writetolog(msglog);
#endif

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

      while ((mmode >= MM_MOVIE)  && (atframe <= lastframe) && fastforward) {
        //sprintf(msglog, "pre catch up cnsckinput: atframe = %d previous frame = %d lastframe = %d moreframes = %d mmode = %d ffwd = %d", atframe, previousframe, lastframe, moreframes, mmode, fastforward);
        //writetolog(msglog);

        //inheriting user changes to the gui
        advanceframe = true;

        if (atframe < lastframe)
          atframe++;

        previousframe = atframe - 1;

        // display backlogged frames
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
        // sprintf(lbl, "%s   frame = %d", mf_filename.c_str(), atframe);
        sprintf(lbl, "%s", header2.c_str());                             //display title on main window
        pfvui->mainWindow->label(lbl);
        pfvui->mainWindow->redraw();
        Fl::check();

#if DEBUG & DBGCKINPUT
        sprintf(msglog, "post catch up and inheritance cnsckinput: "
         "atframe = %d previous frame = %d lastframe = %d "
         "moreframes = %d mmode = %d ffwd = %d size = %g\n",
         atframe, previousframe, lastframe, moreframes, mmode,
         fastforward, pfvui->glView->size[atframe] );
        writetolog(msglog);
#endif

        if (fastforward == false)
          break;

        if (mmode == MM_STILL) {
          //this could happen because we are 'multithreading' catch-up, gui actions, and socket handling
          break;
        }

        if (atframe < lastframe) {
          previousframe++;
          atframe++;
        }
        else {
          break;
        }

      } //end catch up for MM_MOVIE

      if (moreframes == true && mmode >= MM_MOVIE  && atframe == lastframe) {

        //sprintf(msglog, "In file %s at line %d: add stdin_cb upon completing buffer at frame %d with lastframe %d",__FILE__ ,__LINE__, atframe, lastframe);
        //writetolog(msglog);

        Fl::add_fd(0, FL_READ, stdin_cb);
      }

      //sprintf(msglog, "end of MFB_COMPLT in file %s at line %d: last frame received is %d and display is atframe  %d mode = %d and fastforwad = %d"
      //              ,__FILE__ ,__LINE__, lastframe, atframe, mmode, fastforward);  //++++++++++++++++++++++++++ 1/10/2008
      //writetolog(msglog);                                                          //++++++++++++++++++++++++++ 1/10/2008

      return;
    } //end of MFB_COMPLT

    if (msghdr & MFB_CLOSE) {
     closeconnection();
     if (mmode == MM_BATCH || quitpressed)
       exit(0);

     connectionclosed = true;
     moreframes = false;
     fastforward = false;
     // Unnecessary code delete here so MM_BATCH mode can work, 04/15/09, GNR
     if (atframe > lastframe)
        atframe = lastframe;
     mmode = MM_STILL;

     //sprintf(msglog, "sockserver:line 494: moreframes %d  atframe %d mmode %d lastframe %d fastforward %d",  moreframes, atframe, mmode, lastframe, fastforward);
     //writetolog(msglog);

     break;
     } //end of MFB_CLOSE

  } //end while
    delete [] buffer;

  } //end of try block
  catch(bad_alloc) {
    writetolog("fatal mfsockserver.cxx cnsckinput() memory allocation for buffer");
    exit(1);
  }
} //end cnsckinput()

/*=====================================================================*
*                             cnsstartup                               *
*=====================================================================*/

void cnsstartup(void) {    //Read the data from the Cns/xinetd stdin connection

   extern char *disp_name;
   extern char *win_title;
   extern char *prefs_fname;

    /* Format of the startup message received via socket interface.
     *  (Variable-length names follow last item in this struct).  */
    struct {
      char vers[4];              /* Version of interface */
      char l_disp_name[4];       /* Length of display name */
      char l_home_name[4];       /* Length of home directory name */
      char l_win_title[4];       /* Length of window title */
      char cmd_buff_len[6];      /* Length of command buffer */
      char i_movie_mode[2];      /* Movie mode */
      char l_icon_name[4];       /* Length of icon name */
      char m_version[6];         /* Version identifier (6H) */
      char z_debug[9];           /* Debug flags */
      char m_pad[1];
      } start_msg;

    int         ackfd = -1;
    int         ldispnm;          /* Length of display name */
    int         lhomedir;         /* Length of home directory name */
    int         lwintitl;         /* Length of window title */
    int         xdebug;           /* Debug bits */

// Emergency debug stop
#if 0
   if (1) {
      volatile int cwt = 1;
      while (cwt) {
         sleep(1);
         }
      }
#endif

   /* Read the startup data from the new connection */
   if (read(Sockin, &start_msg, sizeof(start_msg)) != sizeof(start_msg)) {
      writetolog("fatal error in mfsockserver.cxx reading startup parameters");
      exit(5);
      }

      /* Rev, 08/16/16, GNR - Read the old ratio field into a temp of
      *  10 chars as it was never used for anything.  When all clients
      *  have replaced this with the new icon_name and version, this
      *  input can be changed accordingly.  Also, the first field,
      *  vers, is now I4 instead of 4H, so I am reading it into 4
      *  chars and checking for an 8.  This should be compatible with
      *  old clients that output "MFD8" there.  It is very frustrating
      *  that sscanf has no way to skip over exactly n chars.  */
   if (5 != sscanf((char *)&start_msg.l_disp_name, "%4d%4d%4d%6d%2d",
         &ldispnm, &lhomedir, &lwintitl, &lcmdbuf, &mmode) ||
       1 != sscanf((char *)&start_msg.z_debug, "%9x", &xdebug)) {
      writetolog("fatal error in mfsockserver.cxx wrong number of startup parameters");
      exit(6);
      }
   if (!memchr(start_msg.vers, '8', 4)) {
      writetolog("fatal error in mfsockserver.cxx version string has no 8 in it");
      exit(7);
      }
   initmmode = mmode;

   buffer = new char[lcmdbuf];

#if DEBUG & DBGSTARTUP
   sprintf(msglog, "length of home directory = %d and length of "
      "command buffer %d", lhomedir, lcmdbuf);
   writetolog(msglog);
   sprintf(msglog, "movie mode = %d",mmode);
   writetolog(msglog);
#endif

   /* Get name of display server */
   if (ldispnm > 0) {
      if ((disp_name = (char*)malloc(ldispnm+1)) == NULL) {
         writetolog("fatal error in mfsockserver.cxx: Unable to allocate storage for display name");
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
   if (lhomedir > 0) {
      /* Get name of home directory and use it
      *  to construct name of prefs file */
      if ((prefs_fname = (char*)malloc(lhomedir+sizeof(MFPREFS)+2)) == NULL) {
         writetolog("fatal error in mfsockserver.cxx: Unable to allocate storage for prefs file name");
         exit(10);
         }
      if (read(Sockin, prefs_fname, lhomedir) != lhomedir) {
         writetolog("fatal error in mfsockserver.cxx  unable to read startup preferences");
         exit(11);
         }
      if (prefs_fname[lhomedir-1] != '/')
         prefs_fname[lhomedir++] = '/';
      prefs_fname[lhomedir] = '\0';
      strcat(prefs_fname, MFPREFS);
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
      win_title[0] = '\0';

#if DEBUG & DBGSTARTUP
   sprintf(msglog, "display name is: %s  window title is: %s and prefs_fname is: %s",disp_name, win_title, prefs_fname);
   writetolog(msglog);
#endif

   /* If dbgflags == 2 sent from CNS, wait for debugger now */
   if (xdebug & 2) {
      volatile int cwt = 1;
      while (cwt) {
         sleep(1);
         }
      }

   /* Tell CNS client "I am alive" */
#if DEBUG & DBGSOCKW
         sprintf(msglog, "line %d in cnsstartup, writing ackfd = %d to socket.",
            __LINE__, ackfd);
         writetolog(msglog);
#endif
   if (write(Sockout, &ackfd, 1) < 1) {
      sprintf(msglog, "fatal error %d sending startup ack to client\n", errno);
      writetolog(msglog);
      exit(25);
      }

   } /* End cnsstartup */

/*=====================================================================*
*                             messagehdr                               *
*=====================================================================*/

int messagehdr(void) {
  //try to read data until message header has been received and
  // return message length
  int nrem = 0, msglength = 0, nr = 0, msgrecvd = 0;

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
      sprintf(msglog,"Error %d in messagehdr() in select.\n", errno);
      writetolog(msglog);
      perror("messagehdr");
      exit(40);
      }
    else if (retval) {
      nr = read(Sockin, ((char *)&msghdr) + msgrecvd, nrem);
      if (nr < 0) {
        if (errno == EAGAIN) continue;
        sprintf(msglog,"Errno %d reading msg header.\n", errno);
        writetolog(msglog);
        perror("messagehdr");
        exit(41);
      }
      msgrecvd += nr;
    }
  } //end of while

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

#if DEBUG & DBGCNSMSG
  sprintf(msglog, "In messagehdr(): cns sent us msghdr with message "
   "of length = %d and hex representation (little endian) %x",
   msglength, msghdr);
  writetolog(msglog);

  if (msghdr & MFB_INIT) {
    sprintf(msglog, "In messagehdr(): start-up buffer received "
      "atframe = %d mode = %d runmode = %d moreframes = %d",
      atframe, mmode, runmode, moreframes);
    writetolog(msglog);
  }
  if (msghdr & MFB_COMPLT) {
    sprintf(msglog, "In messagehdr(): last buffer for this frame "
      "received atframe = %d mode = %d runmode = %d moreframes = %d",
      atframe, mmode, runmode, moreframes);
    writetolog(msglog);
  }
  if (msghdr & MFB_CLOSE) {
    sprintf(msglog, "In messagehdr(): close connection after this "
      "buffer at frame = %d mode = %d runmode = %d moreframes = %d",
      atframe, mmode, runmode, moreframes);
    writetolog(msglog);
  }
  sprintf(msglog, "msghdr holds length of new buffer = %d",msglength);
  writetolog(msglog);
#endif

  if (msgrecvd != 4) {
    writetolog("msghdr not 4 bytes long. terminating");
    exit(1);
  }

  return msglength;
} /* End messagehdr */

/*=====================================================================*
*                             parsebuffer                              *
*=====================================================================*/

void parsebuffer(char* buf, int len) {

#if DEBUG & DBGPARSE
  writetolog("Entered parsebuffer");
#endif

  if ( (len == 0) || (buf[0] == '\0') ) {
    writetolog("fatal error in mfsockserver: parsebuffer ignoring buffer len == 0 or first member == '\0'");
    exit(1);
  }
  //breaks down buffer into three headers and drawing commands

  int nr = 0; //characters parsed
  int rc = 0; //cumulative characters parsed
  static int hdr = 0;
  char* pch;
  pch = strtok (buf,"\n");

  if (hdr < 3) {  //parse headers
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
      if (++hdr == 3)
        break;
    }//end of while in headers
  }//end in headers

  while ( (pch != NULL)) {
#if DEBUG & DBGPARSE
    sprintf(msglog,"%.120s",pch);
    writetolog(msglog);
#endif
    //we add 1 because a strtok replaces newline
    //with \0 and yet must be counted as received!
    nr = strlen(pch) + 1;

    //partial command handling using c++ string class instead of
    //  c string.h  and heap management
    if (rc + nr > len)  {
     //must be buffer straddling.  commmand doesn't have newline
     //within buffer
#if DEBUG & DBGPARSE
      sprintf(msglog, "in buffer straddling processing frame %d "
         "with nr = %d rc = %d and len = %d", lastframe + 1, nr, rc, len);
      writetolog(msglog);
#endif
      nr = len - rc;
      rc = len;
      if (partial == false) {
#if DEBUG & DBGPARSE
        writetolog("Entered partial == false block");
#endif
        partialcmd.clear();
        partial = true;
#if DEBUG & DBGPARSE
        writetolog("Returned from partialcmd.clear, set partial = true");
#endif

        string tmpstr = pch;
        partialcmd = partialcmd + tmpstr.substr(0,nr);

        //must be at buffer end so do not call strtok
#if DEBUG & DBGPARSE
        sprintf(msglog, "partial cmd processing frame %d with nr = %d, "
         "rc = %d, and len = %d", lastframe + 1, nr, rc, len);
        writetolog(msglog);
#endif
        break;
      }
#if DEBUG & DBGPARSE
      writetolog("Partial and partial again--extending partialcmd");
#endif

      string tmpstr = pch;
      partialcmd = partialcmd + tmpstr.substr(0,nr);

      pch = strtok(NULL, "\n");

      break; //we do not attempt to parse the command yet and we must be at the end of the buffer
    }//end of straddling command processing

    if ( partial == true ) {

      string tmpstr = pch;
      partialcmd = partialcmd + tmpstr.substr(0,nr);

      partial = false;
      rc += nr;

      //mftodraw( partialcmd);
#if DEBUG & DBGPARSE
      sprintf(msglog, "calling mftodraw with %.120s", (char *)partialcmd.c_str());
      writetolog(msglog);
#endif
      mftodraw( (char*)partialcmd.c_str());
      pch = strtok(NULL, "\n");
#if DEBUG & DBGPARSE
      sprintf(msglog, "end of partial cmd processing frame %d "
         "with nr = %d rc = %d and len = %d", lastframe + 1, nr, rc, len);
      writetolog(msglog);
#endif
      continue;
    }
    //end partial command handling - c++ string class version

    rc += nr;

    //we convert mf format to drawing commands. one command at a time
    // Bug fix, 12/03/08, GNR - smx had msglog here instead of pch,
    // which failed when I added new debug outputs
    mftodraw(pch);

    pch = strtok(NULL, "\n");
    if (rc == len) {
      break;
    }
  } /* End while (pch != NULL) */

  if ((rc != len) && (partial == false) ) {
    writetolog("Error in mfdrawsockserver parsebuffer() parsed diff "
      "num of chars than function arg. check that parsed file ends "
      "with a newline.");
    exit(1);
  }
} //end parsebuffer()

//==============================support for meta file to drawing command format conversion follows ===============================

float tab[] = {1.0,10.0,100.0,1000.0,10000.0,100000.0};

/*=====================================================================*
*                                 a2f                                  *
*         Metafile-coded ASCII floating-point number to float.         *
*         Taken from mfxpar.c in /src/mfdraw by Marx,                  *
*=====================================================================*/

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

/*=====================================================================*
*                                 a2i                                  *
*                 Metafile-coded ASCII integer to int                  *
*                 Taken from mfxpar.c in /src/mfdraw by Marx,          *
*=====================================================================*/

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

/*=====================================================================*
*                              mftodraw                                *
*=====================================================================*/

void mftodraw(char *buf) {

#if 0
  //for debug and trace purposes only - strcmp requires the exact mf command in mf format
  if (strcmp(buf,"CF222e315j2d") == 0) {
    sprintf(msglog,"found CF222e315j2d lastframe = %d in %s at line %d",
      lastframe, __FILE__ , __LINE__);
    writetolog(msglog);
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

  char c1 = toupper(buf[0]);
  char c2 = toupper(buf[1]);

  //sprintf(msglog, "first command char = %c second char = %c", c1, c2);
  //writetolog(msglog);

  switch (buf[0]) {
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

      //sprintf(msglog, "new frame # %d  width = %f  height = %f xorig = %f yorig = %f type = %s", index + 1,width,height,xorig,yorig,buf);
      //writetolog(msglog);


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

  //  See parameter description in documentation.  We get here from
  //  any call in the user program to bitmap()

    {
      float xc,yc;
      int i,j,ls,ms;
      int xofs,yofs,iwd,iht,type,mode;
      int length;          /* Length in bytes, not pixels */
      int rowlen, colht;
      int c3 = toupper(buf[2]);

      type = (c2 < 'A') ? (c2 - '0') : (c2 - ('A'-10));
      type &= ~BM_NSME;    /* GNR Temp bug fix, 04/30/10 */
      mode = (c3 < 'A') ? (c3 - '0') : (c3 - ('A'-10));

      buf += 3;
      //smarx appended x to the following a2f and a2i to distinguish
      // them from previously defined a2f and a2i
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

      if (iht == 0) {
         writetolog("Error bitmap height == 0");
         exit(1);
         }

      /* My tentative analysis here is that 'rowlen' should be left as
      *  it is set by the caller (pixels in a row) but 'length' should
      *  be adjusted to be the number of bytes in the whole image.  */
      /* Default 'lci' is 8 bits, and for now it is the only
      *  value supported */
      switch (type) {
      case BM_BW:
         length = (iwd+7+(xofs&7)) >> 3; break;
      case BM_GS:
      case BM_C8:
         length = iwd; break;
      case BM_GS16:
      case BM_C16:
         length = 2*iwd; break;
      case BM_C24:
         length = 3*iwd; break;
      case BM_C48:
         length = 6*iwd; break;
         } /* End type switch */
      length *= iht;

      for (i=0,j=0; j<length; i+=2,j++) {
        c3 = toupper(buf[i]);
        ms = c3 < 'A' ? c3 - '0' : c3 - ('A'-10);
        c3 = toupper(buf[i+1]);
        ls = c3 < 'A' ? c3 - '0' : c3 - ('A'-10);
        buf[j] = ls | (ms << 4);
        }

      if (type == BM_BW) {
         ptrbitmap = new Bitmap(type, mode, xc, yc, rowlen, colht,
            xofs, yofs, iwd, iht, length, buf);
         sListofCommands.push_front(ptrbitmap);
         }
      else {            /* Anything not a binary bitmap */
         float bwd, bht;
         bwd = bht = 0; // Convention for not scalable!
         ptrbitmapgrayscalescalable = new Bitmapgrayscalescalable(
            type, mode, xc, yc, bwd, bht, rowlen, colht, xofs, yofs,
            iwd, iht, length, buf);
         sListofCommands.push_front(ptrbitmapgrayscalescalable);
         }

      buf += (length+length);

      break;
    } // End case bitmap


  case 'b' : // Call to bitmap() with non-binary map or any call to
             // bitmaps() for scaled map
    {

      float xc,yc;
      float bwd, bht;
      int i,j,ls,ms;
      int xofs,yofs,iwd,iht,type,mode,length;
      int rowlen, colht;
      int c3 = toupper(buf[2]);

      type = (c2 < 'A') ? (c2 - '0') : (c2 - ('A'-10));
      type &= ~BM_NSME;    /* GNR Temp bug fix, 04/30/10 */
      mode = (c3 < 'A') ? (c3 - '0') : (c3 - ('A'-10));

      buf += 3;
      //smarx appended x to the following a2f and a2i to distinguish
      // them from previously defined a2f and a2i
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

      if (iht == 0) {
         writetolog("Error bitmaps height == 0");
         exit(1);
         }

      /* See note at corresponding code in case 'B' above */
      switch (type) {
      case BM_BW:
         length = (iwd+7+(xofs&7)) >> 3; break;
      case BM_GS:
      case BM_C8:
         length = iwd; break;
      case BM_GS16:
      case BM_C16:
         length = 2*iwd; break;
      case BM_C24:
         length = 3*iwd; break;
      case BM_C48:
         length = 6*iwd; break;
         } /* End type switch */
      length *= iht;

      for (i=0,j=0; j<length; i+=2,j++) {
        c3 = toupper(buf[i]);
        ms = c3 < 'A' ? c3 - '0' : c3 - ('A'-10);
        c3 = toupper(buf[i+1]);
        ls = c3 < 'A' ? c3 - '0' : c3 - ('A'-10);
        buf[j] = ls | (ms << 4);
        }

      /* Rev, 08/26/16, GNR - Remove incorrect new Bitmap here
      *  if type = BM_BW as this is a scalable bitmap by def.  */
      ptrbitmapgrayscalescalable = new Bitmapgrayscalescalable(
         type, mode, xc, yc, bwd, bht, rowlen, colht, xofs, yofs,
         iwd, iht, length, buf);
      sListofCommands.push_front(ptrbitmapgrayscalescalable);

      buf += (length+length);

      break;
    } // End bit map grayscale and scalable

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

  case 'D':
    {
      double x,y;
      buf += 1;
      a2f(x,buf,5,3,DECENCD);
      a2f(y,buf,5,3,DECENCD);
      if (fabs(x - lastx) > 0.01 || fabs(y - lasty) > 0.01) {
         ptrline = new Line(lastx,lasty,x,y);
         sListofCommands.push_front(ptrline);
         }
      lastx = x, lasty = y;
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
        int nc;
        buf += 1;
        a2i(nc,buf,3,DECEND);
        //sprintf(msglog, "color %s", buf);
        //writetolog(msglog);
        ptrcolorname = new Colorname(buf,(size_t)nc);
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
      lastx = x2, lasty = y2;
      break;
    }

  case 'M':
    {
      double x,y;
      buf += 1;
      a2f(x,buf,5,3,DECENCD);
      a2f(y,buf,5,3,DECENCD);
      lastx = x, lasty = y;
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

      //sprintf(msglog, "did a polyline and np = %d\n", np);
      //writetolog(msglog);

      if(c2 == 'F') {
        try {
          vertex = new double[3*np];
        }
        catch(const bad_alloc &x) {
          writetolog((char*)x.what());
          abort();
        }

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

        //sprintf(msglog, "in mfsockserver.cxx line 697: draw closed polyline-filled");
        //writetolog(msglog);
        ptrfillclosepolyline = new Fillclosepolyline(np, vertex);
        sListofCommands.push_front(ptrfillclosepolyline);
      }
      else if(c2 == 'O') {
        //sprintf(msglog, "draw closed polyline-outlined with np = %d points", np);
        //writetolog(msglog);

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

        ptroutlineclosepolyline = new Outlineclosepolyline(np, vertex);
        sListofCommands.push_front(ptroutlineclosepolyline);
      }

      else if(c2 == 'L') {
        //sprintf(msglog, "draw open polyline with np = %d points", np);
        //writetolog(msglog);
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

        }

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
      int length;      buf += 1;
      a2f(x,buf,5,3,DECENCD);
      a2f(y,buf,5,3,DECENCD);
      a2f(ht,buf,5,3,DECENCD);
      a2f(angle,buf,7,3,DECENCD);
      a2i(length,buf,3,DECENCD);

      //sprintf(msglog,"%.*s at angle = %g", length, buf, angle);
      //writetolog(msglog);

      ptrtext = new Text(x, y, ht, angle, length, buf);
      sListofCommands.push_front(ptrtext);
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

/*=====================================================================*
*                           closeconnection                            *
*=====================================================================*/

void closeconnection(void) {  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ new as 4/18/2008
      int numsent = 0;

#if DEBUG & DBGCLOSE
  writetolog("closeconnection(): generate close connection message to "
   "science model");
#endif

  msgbutt.movie    = (char)mmode;
  msgbutt.next     = (char)0;
  msgbutt.intxeq   = (char)2;  //terminate
  msgbutt.snapshot = (char)(0); // added this 3/7/08

  do {
#if DEBUG & DBGSOCKW
         sprintf(msglog, "line %d in closeconnection, writing msgbutt to socket.",
            __LINE__);
         writetolog(msglog);
         sprintf(msglog,"   movie = %d, next = %d, intxeq = %d, snap = %d",
            (int)msgbutt.movie, (int)msgbutt.next, (int)msgbutt.intxeq,
            (int)msgbutt.snapshot);
         writetolog(msglog);
#endif
    numsent += write(Sockout, &msgbutt, sizeof(msgbutt));
    if (numsent < 0)   {
      writetolog("error in writing close connection to Cns(1)");
      sprintf(msglog, "mfsockserver error %d when attempting to write msgbutt struct to client.\n", errno);
      writetolog(msglog);
      //exit(62); replaced by
      break; //replaces above - we do not try to notify cns of closed connection
    }
  } while (numsent < (int)sizeof(msgbutt));
}

/*=====================================================================*
*                            interruptcns                              *
*=====================================================================*/

void interruptcns(void) {
  if ( !(msghdr & MFB_COMPLT) ) {
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
#if DEBUG & DBGSOCKW
         sprintf(msglog, "line %d in interruptcns, writing msgbutt to socket.",
            __LINE__);
         writetolog(msglog);
         sprintf(msglog,"   movie = %d, next = %d, intxeq = %d, snap = %d",
            (int)msgbutt.movie, (int)msgbutt.next, (int)msgbutt.intxeq,
            (int)msgbutt.snapshot);
         writetolog(msglog);
#endif
    numsent += write(Sockout, &msgbutt, sizeof(msgbutt));

    //writetolog("sent interrupt intxeq = 1 via msgbutt to science model"); //++++++++++++++++++++++++++++++++ 4/3/2008

    if (numsent < 0)   {
      writetolog("error in writing interrupt to science model");
      sprintf(msglog, "mfsockserver error %d writing button status to client during interrupt command.\n", errno);
      writetolog(msglog);
      exit(62);
    }
  } while (numsent < (int)sizeof(msgbutt));
}

/*=====================================================================*
*                              stdin_cb                                *
*=====================================================================*/

void stdin_cb(int, void*) {

  Fl::remove_fd(0);

  //sprintf(msglog,"start stdin_cb: atframe = %d, lastframe = %d, fastforward = %d",atframe, lastframe, fastforward);
  //writetolog(msglog);
  cnsckinput();
  //sprintf(msglog,"end stdin_cb: atframe = %d, lastframe = %d, fastforward = %d",atframe, lastframe, fastforward);
  //writetolog(msglog);
}

/*=====================================================================*
*                          cmdline_metafile                            *
*=====================================================================*/

void cmdline_metafile(void) {
  if (runmode == SOCKMODE) {
    fl_alert("you are in cns socket mode. you cannot use the File control to input data.");
    exit(224);
  }
  static char filename[120];
  static bool metaopened = false;

  strcpy(filename, mf_filename.c_str());
  string s(filename);

  if (s == "(null)")
    return;
  if (metaopened == false)
    metaopened = true;
  else {
    fl_alert("cannot open metafile if metafile is already open");
    exit(224);
  }

  deletedata(); //clear out any data and initialize prior to loading new data
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

  //zoom->take_focus();
  pfvui->zoom->value(1);
  pfvui->glView->init();

  runmode = FILEMODE;
}
