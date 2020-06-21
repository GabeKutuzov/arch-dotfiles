// (c) Copyright 2012, The Rockefeller University *21500*
#ifndef MFDRAWSOCKSERVER_H
#define MFDRAWSOCKSERVER_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <byteswap.h>

#include <FL/Fl.H>

const int NAMEMAX = 200, TITMAX = 200, DATMAX = 200;
struct MFCONV{
  char infn[NAMEMAX];     /* Input file name */
  char outfn[NAMEMAX];    /* Output file name */
  char devfn[NAMEMAX];    /* Printer device name */
  char tmpfn[NAMEMAX];    /* Temporary storage file name */
  FILE *infd;             /* Input output file descriptore */
  FILE *outfd;            /* Input output file descriptore */
  FILE *tmpfd;            /* Temporary file descriptore */
  int devfd;              /* Device's file descriptore */
  int currop;             /* Current operation flag (SAVE,LOAD) */
  int flgcol;             /* Current color flag (COLORE,MONOCH) */
  int flgbkg;             /* Current backgroud flag (BLACK,WHITE) */
  int flgbdr;             /* Current boarder flag (BDRON,BDROFF) */
  int device;             /* Device's type (MFC_PRINT,MFC_SAVE,MFC_SCREEN) */
  int type;               /* Implementaion (XGL,EPS) */
  int pgdes;              /* Page descripor */
  char title[TITMAX];     /* Meatafile title */
  char date[DATMAX];      /* Metafile-date created */
  char curdat[DATMAX];    /* Metafile-date printed */
};

//const int PORT    = 10090;       //default port on server for socket connection. not specified within mfdraw, it is read from etc/services by xinetd
const int Sockin  = 0;
const int Sockout = 1;
const int ackfd   = -1;

#define  MFDVERS "MFD8"          /* Must match header received from client */
#define  MFPREFS ".mfdrawrc"     /* Name of preferences file (corrected) */
#define  MFB_COUNT  0x0fffffff   /* Mask to extract count from header */
#define  MFB_CLOSE 0x80000000    //Close the connection after this buffer.

#define MFB_COMPLT  0x40000000
//This is the last buffer for this frame.

#define MFB_INIT 0x20000000
//This is the start-up buffer.

extern int runmode;
extern int mmode;

void error(char *msg);
void writetolog(const char* buf);   //since stdout is occupied by socket
                                    // traffic we use a disk file to hold
                                    // error and other logout data
void cnsstartup(void);        //builds input commands from data obtained from cns via sockets. including frame tracker.
int messagehdr(void);         //reads message header and returns current buffer length
void parsebuffer(char*, int); //breaks buffer into three headers and drawing commands
void mftodraw(char *buf);     //convert mf format to drawing commands
void closeconnection(void);   //closes cns and the connection
void interruptcns(void);      //permits cns operator to input new input parameters e.g cycle 1, 5, 1

void stdin_cb(int, void*);    //monitor standard input to support movie mode decouple of sockets from gui
void cmdline_metafile(void);  //build drawing commands when meta file is specified on the command line

/* commands sent by mfdraw to Cns (based on mfdraw) */
struct butt {
  char movie;             /* See MM_xxx in plotdefs.h */
  char next;              /* 1 for next frame */
  char intxeq;            /* 1 See MM_xxx in plotdefs.h */
  char snapshot;          /* 1 for snapshot--obsolete */
};

//from plotdefs.h
#define MM_STILL  1
#define MM_MOVIE  2
#define MM_BATCH  3

#endif
