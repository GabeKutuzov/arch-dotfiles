/* (c) Copyright 1992,1993, Neurosciences Research Foundation, Inc. */
/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    mfprint [filename]                                              */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*    The mfprint program is a tool for displaying and printing NSI   */
/*    graphics metafiles (abbreviated NGM).  The NGM file contains    */
/*    graphics commands that are ordered into frames and each one of  */
/*    the frames is indexed with a unique number.  Mfprint first      */
/*    scans the NGM file and displays the first frame on the screen.  */ 
/*    Then the user, by using the proper menu commands, can display,  */
/*    print, or save as a postscript file any of the frames.          */
/*    Currently mfprint supports only postscript printers and in      */
/*    particular TEKTRONIX PHASER II.  However, when saved as a       */
/*    postscript file, the NGM frame can be then processed by any     */
/*    device or platform (including IBM PC) that supports postscript. */
/*                                                                    */
/*                                                                    */
/*  NOTE:                                                             */
/*    For more information see ~nsi/docs/mfprint.memo.                */
/*                                                                    */
/*  Version 1, 07/16/92, ROZ                                          */
/*  Rev,   04/26/93, ROZ - Add support for NGM format version 1.3     */
/*                             and for Laserjet 4 on network.         */
/*  Rev.,  07/14/94   LC  Flashes "Done" after file save.             */
/*  Rev.,  07/21/94   LC  Added retrace option.                       */
/**********************************************************************/

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <xview/notify.h>
#include <xview/frame.h>
#include <xview/notice.h>
#include <xview/rect.h>
#include <xview/svrimage.h>

#define  MAIN
#include "xglut.h"
#include "pltxgl.h"
#include "pltps.h"
#include "mfcv.h"
#include "mfprn.h"

#define  RATIO       0.79  /* Ratio between the width and height (11/14) */
#define  CONTINUE    1     /* Notice control window with continue button */
#define  YES_NO      2     /* Notice control window with yes/no buttons */
#define  TYPE_FILE   0     /* File name is a valid file */
#define  TYPE_DIR    1     /* File name is a directory */

/* Allocate memory for target string and copy source into target. */
#define allo_cpy(target,source) { \
   if((target = (char *)malloc(strlen(source))) == NULL) { \
      printf("can not allocate memory \n"); \
      exit(1); \
      } \
   strcpy(target,source); \
   } \

/* Functions prototype */
int refresh_list(char *str);
int get_dir_files(char *path,char **dfarray,int *start_files,int *total);
static int user_notice(int type,char *str);
static Notify_value quit_proc(Frame fr,Destroy_status status);
void repaint_screen();
void event_proc (Xv_Window window,Event *event,Notify_arg arg);
void show_info();
void frame_quit_proc (Panel_item item, int value, Event event);
int window_loop (Frame l_frame);
int window_return();
void open_menu_proc ();
int refresh_list(char *str);
void f_file_proc();
void p_OK__proc();
void p_Cancel_proc();
void f_list_proc(Panel_item item,char *str,Xv_opaque client_data,
   Panel_list_op op, Event *event, int row);
void p_list_proc(Panel_item item,char *str,Xv_opaque client_data,
   Panel_list_op op, Event *event, int row);
void f_load_proc (Panel_item item, int value, Event event);
void save_menu_proc ();
void Page_Layout_proc ();
void Printer_Setup_proc ();
void Set_Speed_proc ();
void adjust_speed_proc (Panel_item item, int value);
/*void draw_screen(Notify_client client, int which);*/
void Save_prop_proc ();
void fn_save_proc ();
void prop_color_proc (Panel_item item, int value, Event event);
void prop_border_proc (Panel_item item, int value, Event event);
void prop_header_proc (Panel_item item, int value, Event event);
void prop_retrace_proc (Panel_item item, int value, Event event);
void print_menu_proc ();
void next_menu_proc ();
void previous_menu_proc ();
void goto_menu_proc ();
void repaint_screen();
void set_defaults();
int get_prn_files(char *path);
int get_usrflds();
int get_prnflds();
Notify_value prnt_done();
Notify_value draw_screen();

/* This image is used to draw directories in the file menu.
   The image is a bitmap type and measures 16X16 bits */
#define dir_width 16
#define dir_height 16
static unsigned char dir_bits[] = {
   0x00, 0x3c, 0x00, 0x42, 0x00, 0xc1, 0x00, 0x81, 0x7f, 0x01, 0x80, 0x01,
   0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
   0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0xff, 0xff};

/* This image is used to draw files in the file menu.
   The image is a bitmap type and measures 16X16 bits */
#define file_width 16
#define file_height 16
static unsigned char file_bits[] = {
   0x1f, 0xfc, 0x10, 0x1c, 0x10, 0x1e, 0x10, 0x1f, 0x10, 0x1f, 0x10, 0x01,
   0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01,
   0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x1f, 0xff};

/* This image is used to draw an arrow in the file menu.
   The image is a bitmap type and measures 16X16 bits */
#define arrow_width 16
#define arrow_height 16
static char unsigned arrow_bits[] = {
   0x01, 0x80, 0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x1f, 0xf8, 0x3f, 0xfc,
   0x7f, 0xfe, 0xff, 0xff, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0,
   0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0};

/* This image is the icon associate with mfprint.
   The image is a bitmap type and measures 64X64 bits.  
   It is stored in a file "mfp.icon" which is inculded in compile time. */
#define mfp_width 64
#define mfp_height 64
static unsigned short mfp_bits[] = {
#include "mfp.icon"
};

static Frame frame,                     /* Main frame */
   frm_get_fn,                          /* File name frame */
   frm_sel_prn,                         /* Select printer frame */
   frm_get_frame,                       /* Goto frame */
   frm_prop_lout,                       /* Properties layout frame */
   frm_save_fn;                         /* Save  frame */

static Panel_slider_item frm_prop_speed;

/* Panels used in the various windows */
static Panel panel_get_fn,   
   panel_sel_prn,                     
   panel_get_frame,                     
   panel_prop_lout,                   
   panel_save_fn,                     
   panel_main;               

/* Menus used by panels */        
static Menu  properties_menu,
   file_menu;

/* Panel items */
static Panel_item fn_get_item,
   fn_save_item,
   frame_item,
   prop_color_item,
   show_info_item1,
   show_info_item2,
   show_info_item3,
   show_info_item4,
   Next,Goto,Previous,
   prop_border_item,
   f_file_item,
   f_dir_item,
   f_list_item,
   p_OK_item,
   p_Cancel_item,
   p_msg_item,
   p_list_item,
   f_load_item,
   prop_header_item,
   prop_retrace_item;

/* Menu items */
static Menu_item Open, 
   Save ,
   Print,
   Page_Layout,
   Save_prop,
   Printer_Setup;

/* Images used in the scrolling list of the file menu */
Server_image  dir_image, 
   file_image,
   arrow_image,
   mfp_icon_image;

/* Program icon */
Icon   mfp_icon;

struct itimerval timer;
int adjust;

/* Static array that holds the path name */
static char currpath[MAXPATHLEN];

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    int get_dir_files(char *path,char **dfarray,                    */
/*       int *start_files,int *total)                                 */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*    load the names of data files for the directory and              */
/*    return the number loaded.                                       */
/*    Arguments are:                                                  */
/*       path - Search path.                                          */
/*       dfarray - Array where the file names are being stored.       */
/*       start_files - Store the location of the first file name      */
/*       total - Get the number of files in dfarray.                  */
/* RETURN VALUES:                                                     */
/*    Retuns the number of files loaded into dfarray.                 */
/**********************************************************************/
int 
get_dir_files(char *path,char **dfarray,int *start_files,int *total) {

   DIR  *dirp;
   struct dirent  *dp;
   int  i,dircount=0,filecount=0;
   struct stat sbuff;
   char *filearr[512],*dirarr[512];
   
   if ((dirp = opendir(path)) != NULL) {
      for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
         stat(dp->d_name,&sbuff);
         if ((sbuff.st_mode & S_IFMT) == S_IFDIR && (dp->d_name[0] != '.')) {
            allo_cpy(dirarr[dircount],dp->d_name);
            dircount++;
            }
         else if (dp->d_name[0] != '.') {
            filearr[filecount] = (char *)malloc(3+strlen(dp->d_name));
            strcpy(filearr[filecount],dp->d_name);
            filecount++;
            }
         }
      closedir(dirp);

      *total = dircount+filecount;
      *start_files = dircount+1;
      for (i=1;i<=dircount;i++) {
         allo_cpy(dfarray[i],dirarr[i-1]);
         }
      for (i=(dircount+1);i<(*total);i++) {
         allo_cpy(dfarray[i],filearr[i-(dircount+1)]);
         }

      dfarray[0] = (char *)malloc(3);
      strcpy(dfarray[0],"..");
      if (!(*total)) *total = 1;
      return (*total);
      }
   else {
     return (-1);
     }
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    int get_prn_files(char *path)                                   */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*    load the names of data files for the directory and              */
/*    return the number loaded.                                       */
/*    Arguments are:                                                  */
/*       path - Search path.                                          */
/*       farray - Array where the file names are being stored.        */
/* RETURN VALUES:                                                     */
/*    Retuns the number of files loaded into farray.                  */
/**********************************************************************/
int 
get_prn_files(char *path) {

   DIR  *dirp;
   struct dirent  *dp;
   int  filecount=1,nlen;
   struct stat sbuff;
   
   SLP.index = 1;
   if ((dirp = opendir(path)) != NULL) {
      for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
         stat(dp->d_name,&sbuff);
         nlen = strlen(dp->d_name);
         if (((sbuff.st_mode & S_IFMT) != S_IFDIR) && 
            (dp->d_name[0] != '.') && (nlen > 4)) {
            if ((strcmp(dp->d_name+(nlen-4),".pdf")) == 0) {
               strcpy(MFP.prnfn,"/home/nsi/src/mfprint/");
               strcat(MFP.prnfn,dp->d_name);
               /* Open printer's data file */
               if ((MFP.prnfd = fopen(MFP.prnfn,"r")) != NULL) {
                  get_prnflds();
                  SLP.index++;
                  }
               }
            }
         }
      closedir(dirp);
      return (SLP.index);
      }
   else {
     return (-1);
     }
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    int user_notice(int type,char *str)                             */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*    Popup a notice window.  The notice window can have two forms    */
/*    depends on the value of the argument type.  It can either be    */
/*    YES,NO" notice window or "Continue".  Continue notice window    */
/*    has one button labeled "Continue", and "YES,NO" window has two  */
/*    buttons which are labeled "YES" and "NO".                       */
/*    Arguments are:                                                  */
/*        type - Can be one of two defined constants ->CONTINUE,      */
/*               YES_NO.                                              */
/*        str - Notice text.                                          */
/* RETURN VALUES:                                                     */
/*    Returns the value of the selected button.                       */
/**********************************************************************/
static
int user_notice(int type,char *str) {

   Xv_Notice notice0,notice1;
   int result0,result1;

   switch(type) {
      case CONTINUE:
         notice0 = xv_create(frame,NOTICE,
                     NOTICE_STATUS,&result0,
                     NOTICE_MESSAGE_STRINGS," ",NULL,
                     NOTICE_BUTTON,"Continue",1,
                     NULL);
         xv_set(notice0,NOTICE_MESSAGE_STRING,str,NULL);
         xv_set(notice0,XV_SHOW,TRUE,NULL);
         xv_destroy_safe(notice0);
         return result0;
      case YES_NO:
         notice1 = xv_create(frame,NOTICE,
                     NOTICE_STATUS,&result1,
                     NOTICE_MESSAGE_STRINGS," ",NULL,
                     NOTICE_BUTTON_YES,"Yes",
                     NOTICE_BUTTON_NO,"No",
                     NULL);
         xv_set(notice1,NOTICE_MESSAGE_STRING,str,NULL);
         xv_set(notice1,XV_SHOW,TRUE,NULL);
         xv_destroy_safe(notice1);
         return result1;
      }
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    static Notify_value quit_proc(Frame fr,Destroy_status status)   */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*    The function closes opened file and frees momries before exit.  */
/* RETURN VALUES:                                                     */
/*    None                                                            */
/**********************************************************************/
static Notify_value
quit_proc(Frame fr,Destroy_status status) {
   
   if (user_notice(YES_NO,"Do you really want to quit?") == NOTICE_YES) {
      plot_close();
      if (MFP.infd) {
         fclose(MFP.infd);
         }
      if (MFP.outfd) {
         fclose(MFP.outfd);
         }
      if (MFP.usrfd) {
         fclose(MFP.usrfd);
         }
      if (MFP.prnfd) {
         fclose(MFP.prnfd);
         }
      exit(0);
      }
   else {
      return;
      }
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    void event_proc (Xv_Window window,Event *event,Notify_arg arg)  */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*    Process the panding event, check for resize event and incase    */
/*    there is call resize() and repaint the screen.                  */
/* RETURN VALUES:                                                     */
/*    None.                                                           */
/**********************************************************************/
static void
event_proc (Xv_Window window,Event *event,Notify_arg arg) {

   if (event->ie_code == WIN_RESIZE) {
      resize_proc ( ((XConfigureEvent *) (event->ie_xevent))->width,
                    ((XConfigureEvent *) (event->ie_xevent))->height);
      repaint_screen();
      }
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    void show_info()                                                */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*    Show information on the processed metafile.  The information    */
/*    include the current displayd frame number, file name and total  */
/*    frames in file.                                                 */
/* RETURN VALUES:                                                     */
/* None.                                                              */
/**********************************************************************/
void
show_info() {
   
   char	buf1[80],buf2[80],buf3[80];
   sprintf( buf1, "File name: %s",MFP.infn);
   sprintf( buf2, "Current Frame: %d",(MFP.currframe+1));
   sprintf( buf3, "Total Frames: %d",MFP.maxfrms);
   panel_set( show_info_item1, PANEL_LABEL_STRING, buf1, 0);
   panel_set( show_info_item2, PANEL_LABEL_STRING, buf2, 0);
   panel_set( show_info_item3, PANEL_LABEL_STRING, buf3, 0);   
}

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*   void frame_quit_proc (Panel_item item, int value, Event event)   */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   This function reads the value returned from the goto window and  */
/*   display the chosen frame.                                        */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
frame_quit_proc (Panel_item item, int value, Event event) {
   char str[81];
   int currf;
   sscanf ((char *)xv_get (frame_item, PANEL_VALUE), "%d", &currf);
      currf -=1;
      xv_set (frm_get_frame, WIN_SHOW, FALSE, 0);
      /*window_return();*/
   if (currf >= 0 && (currf < (MFP.maxfrms))) {
      MFP.currframe = currf; 
      show_info();
      if (MFP.infd) mfconv(MFP.infd, MFP.currframe,MFC_SCREEN,MFC_XGL);
      }
   else {
      sprintf(str,"Invalid frame number {%d}!!!\n",(currf+1));
      user_notice(CONTINUE,str);
      }
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*   window_loop (Frame l_frame)                                      */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Dommy function.                                                  */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
window_loop (Frame l_frame) {
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    window_return ()                                                */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Dommy function.                                                  */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
/*window_return () {
   }*/

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*   void open_menu_proc ()                                           */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   This function displays the file open window which displays a     */
/*   selection of file in scrolling list where the can select and     */
/*   open one of the files.                                           */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
open_menu_proc () {

   getwd(currpath);
   refresh_list(currpath);
   if (!(int) xv_get (frm_get_fn, WIN_SHOW)) {
      xv_set (frm_get_fn, WIN_ROWS, 16, 0);
      xv_set (frm_get_fn, WIN_SHOW, TRUE, 0);
      window_loop (frm_get_fn);
      }
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    int refresh_list(char *str)                                     */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*    Display a list of files in a scrolling window when called.  The */
/*    function gets a file nane from either a scroll list or text     */
/*    item and does the following:                                    */
/*    In case str is a valid file name, it returns defined constant   */
/*    TYPE_FILE.                                                      */
/*    If str is a valid dirctory name then a new list of files which  */
/*    reside in this directory is diplayed.                           */
/* RETURN VALUES:                                                     */
/*    If str is a valid dirctory the function returns defined costant */
/*    TYPE_DIR.  Otherwise, it returns defined constant TYPE_FILE.    */
/**********************************************************************/
int
refresh_list(char *str) {

   int total,start_files,i;
   char *dfarray[1024];
   char path[MAXPATHLEN];
   struct stat sbuff;
   static int lastcount;

   stat(str,&sbuff);
   if ((sbuff.st_mode & S_IFMT) != S_IFDIR) {
      return TYPE_FILE;
      }
   chdir(str);
   getwd(path);
   if (lastcount>0) { 
      xv_set(f_list_item, PANEL_LIST_DELETE_ROWS,0,lastcount,NULL);
      }
   if ((lastcount = get_dir_files(path,dfarray,&start_files,&total))==-1) {
      return (-1);
      }
   xv_set(f_list_item,XV_SHOW,FALSE,NULL);
   for (i=0;i<total;i++) {
      if (i<start_files) {
         if (i==0) {
            xv_set(f_list_item, PANEL_LIST_GLYPH,i,arrow_image,NULL);
            }
         else {
            xv_set(f_list_item, PANEL_LIST_GLYPH,i,dir_image,NULL);
            }
         }
      else {
         xv_set(f_list_item, PANEL_LIST_GLYPH,i,file_image,NULL);
         }
      xv_set(f_list_item, PANEL_LIST_STRING,i,dfarray[i],NULL);
      }
   xv_set(f_list_item, PANEL_LIST_DISPLAY_ROWS,8,NULL);
   xv_set(f_list_item,PANEL_LIST_SELECT,0,FALSE,NULL);
   xv_set(f_list_item,XV_SHOW,TRUE,NULL);
   strcpy(currpath,path);
   xv_set(f_file_item,PANEL_VALUE,path,NULL);
   return TYPE_DIR;
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    void f_file_proc()                                              */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*    Gets the file name from the text item in the file menu window.  */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
f_file_proc() {
   char str[81];

   strcpy(str,(char *)xv_get (f_file_item, PANEL_VALUE));
   refresh_list(str);
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    void p_OK_proc()                                               */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*    Gets the selected printer name from the "Select Printer" menu   */
/*    window.                                                         */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
void
p_OK_proc() {

   SLP.sel_prn = SLP.curr_prn;
   mfc_prn(MPD[SLP.sel_prn].alias,
          MPD[SLP.sel_prn].tmargin,
          MPD[SLP.sel_prn].lmargin,
          MPD[SLP.sel_prn].width,
          MPD[SLP.sel_prn].height,
          MPD[SLP.sel_prn].visual,
          MPD[SLP.sel_prn].depth,
          MPD[SLP.sel_prn].lwidth);
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    void p_Cancel_proc()                                            */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*    Exit the "Select printer" menu window without selection.        */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
p_Cancel_proc() {
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    void f_list_proc(Panel_item item,char *str,                     */
/*                    Xv_opaque client_data,                          */
/*                    Panel_list_op op, Event *event, int row)        */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Gets the file name from the scolling list in the file menu       */
/*   window.                                                          */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
void
f_list_proc(Panel_item item,char *str,Xv_opaque client_data,
            Panel_list_op op, Event *event, int row) {
   int i;
   static char path[MAXPATHLEN];

   if (op == PANEL_LIST_OP_SELECT) {
      strcpy(path,currpath);
      strcat(path,"/");
      strcat(path,str);
      xv_set(f_file_item,PANEL_VALUE,path,NULL);
      refresh_list(str);
      }
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    void p_list_proc(Panel_item item,char *str,                     */
/*                    Xv_opaque client_data,                          */
/*                    Panel_list_op op, Event *event, int row)        */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Gets the file name from the scrolling list in the file menu      */
/*   window.                                                          */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
void
p_list_proc(Panel_item item,char *str,Xv_opaque client_data,
            Panel_list_op op, Event *event, int row) {
   int i;
   static char path[MAXPATHLEN];

   if (op == PANEL_LIST_OP_SELECT) {
      SLP.curr_prn = row;
      }
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*   void f_load_proc (Panel_item item, int value, Event event)       */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Load the selected NGM file and initialize some global variables  */
/*   which set the defaults for printing.  The information is         */
/*   supplied by the user through the "load file menu".               */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
f_load_proc (Panel_item item, int value, Event event) {
   char str[81],fn[MAXPATHLEN];
   FILE *fd;
   int currframe,maxfrms;

   sscanf((char *)xv_get (f_file_item, PANEL_VALUE), "%s", fn);
   if ((fd = fopen(fn,"r")) == NULL) {
      sprintf(str,"Can not open %s !!!\nFile does not exist.",fn);
      user_notice(CONTINUE,str);
      }
   else {
      if ((maxfrms = MFmaxfrms(fd))) {
         currframe = 0;
         xv_set (Save, MENU_INACTIVE, FALSE, 0);
         xv_set (Print, MENU_INACTIVE, FALSE, 0);
         xv_set (frm_get_fn, WIN_SHOW, FALSE, 0);
         if (mfconv(fd, currframe,MFC_SCREEN,MFC_XGL) == 2) {
            sprintf(str,
            "Error reading %s\n This is not an NSI graphic metafile!!!\n",
             fn);
            user_notice(CONTINUE,str);
            }
         else {
            MFP.maxfrms = maxfrms;
            MFP.currframe = currframe;
            if (MFP.infd) {
               fclose(MFP.infd);
               }
            MFP.infd = fd;
            strcpy(MFP.infn,fn);
            show_info();
            }
         }
      else {
            sprintf(str,
            "Error reading %s\n This is not an NSI graphic metafile!!!\n",
             fn);
            user_notice(CONTINUE,str);
         }
      }
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*   void save_menu_proc ()                                           */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Displays the save window.                                        */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
save_menu_proc () {
   if (!(int) xv_get (frm_save_fn, WIN_SHOW)) {
      xv_set (frm_save_fn, WIN_SHOW, TRUE, 0);
      window_loop (frm_save_fn);
      }
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*   void Page_Layout_proc ()                                         */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Displays the layout window.                                      */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
Page_Layout_proc () {
   if (!(int) xv_get (frm_prop_lout, WIN_SHOW)) {
      xv_set (frm_prop_lout, WIN_SHOW, TRUE, 0);
      window_loop (frm_prop_lout);
      }
   }

/**********************************************************************/
/* SYNOPSIS:                                                          */
/*   void Set_Speed_proc ()                                           */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Sets speed that frames are drawn.                                */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
Set_Speed_proc () {
   if (!(int) xv_get (frm_prop_speed, WIN_SHOW)) {
      xv_set (frm_prop_speed, WIN_SHOW, TRUE, 0);
      window_loop (frm_prop_speed);
      }
    }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*     void Printer_Setup_proc()                                      */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Set printer paramaters. Show a menu of possible parameters.      */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
Printer_Setup_proc() {
   
   int i;
   for (i=0;i<SLP.num_prn;i++) {
      xv_set(p_list_item, PANEL_LIST_STRING,i,SLP.name[i],NULL);
      }
   xv_set(p_list_item,PANEL_LIST_SELECT,SLP.sel_prn,TRUE,NULL);
   if (!(int) xv_get (frm_sel_prn, WIN_SHOW)) {
      xv_set (frm_sel_prn, WIN_ROWS, 16, 0);
      xv_set (frm_sel_prn, WIN_SHOW, TRUE, 0);
      window_loop (frm_sel_prn);
      }
   }

/**********************************************************************/
/*                                                                    */
/*                                                                    */
/**********************************************************************/
Notify_value 
draw_screen() {

   if ((MFP.currframe+1) < (MFP.maxfrms)) 
      next_menu_proc();
      
      return NOTIFY_DONE; 
  }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*     void adjust_speed_proc()                                       */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*                                                                    */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
adjust_speed_proc(item, value) {

    if (value > 0) {
       if (1&value) {
          timer.it_value.tv_usec = 500000;
          timer.it_interval.tv_usec = 500000;
          }
       else {
          timer.it_value.tv_usec = 1000;
          timer.it_interval.tv_usec = 1000;
          }

       timer.it_interval.tv_sec = value>>1;

       notify_set_itimer_func( frm_prop_speed, draw_screen, ITIMER_REAL,
                              &timer, NULL);
       }
    else
       notify_set_itimer_func( frm_prop_speed, NOTIFY_FUNC_NULL, ITIMER_REAL,
                              NULL, NULL);
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*     void Save_prop_proc()                                          */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Save user selections for the next time the user                  */
/*   uses the program.                                                */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
Save_prop_proc() {
   
   struct passwd *ud;
   int pn;

   if (MFP.usrfd) {
      fclose(MFP.usrfd);
      /* Open user defaults file */
      if ((MFP.usrfd = fopen(MFP.usrfn,"w+")) == NULL) {
         fprintf(stderr,"Can not open %s \n",MFP.usrfn);
         }
      }
   else {
      /* Construct user default`s file name */
      ud = getpwuid(geteuid());
      strcpy(MFP.usrfn,"/home/");
      strcat(MFP.usrfn,ud->pw_name);
      strcat(MFP.usrfn,"/.mfprint.dfa");

      /* Open user defaults file */
      if ((MFP.usrfd = fopen(MFP.usrfn,"w+")) == NULL) {
         fprintf(stderr,"Can not open %s \n",MFP.usrfn);
         }
      }

   pn = fprintf(MFP.usrfd,"#%s\n",MFP.usrfn);
   fprintf(MFP.usrfd,"#\n");
   fprintf(MFP.usrfd,"# Printer name\n");
   fprintf(MFP.usrfd,"printer: %s\n",MPD[SLP.sel_prn].alias);

   fprintf(MFP.usrfd,"# Page layout\n");
   if (MFP.flgbdr) {
      fprintf(MFP.usrfd,"border: on\n");
      }
   else {
      fprintf(MFP.usrfd,"border: off\n");
      }
   if (MFP.pgdes) {
      fprintf(MFP.usrfd,"header: on\n");
      }
   else {
      fprintf(MFP.usrfd,"header: off\n");
      }
   if (MFP.retrace) {
      fprintf(MFP.usrfd,"retrace: on\n");
      }
   else {
      fprintf(MFP.usrfd,"retrace: off\n");
      }

   fprintf(MFP.usrfd,"# Visual\n");
   if (MFP.flgcol == PPS_COLOR) {
      fprintf(MFP.usrfd,"visual: color\n");
      }
   else if (MFP.flgcol == PPS_GRAY) {
      fprintf(MFP.usrfd,"visual: gray\n");
      }
   else {
      fprintf(MFP.usrfd,"visual: bw\n");
      }
   fclose(MFP.usrfd);
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*     void fn_save_proc ()                                           */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Gets file name from text item in the save window and save frame  */
/*   to a postscript file. Call mfc_attribute to update the page      */
/*   layout and colors before printing.                               */
/*                                                                    */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/*                                                                    */
/* Rev.,  07/14/94  LC  Flashes "Done" after file save.               */
/*                                                                    */
/**********************************************************************/
static void
fn_save_proc (Panel_item item, int value, Event event) {
   sscanf((char *)xv_get (fn_save_item, PANEL_VALUE), "%s", MFP.outfn);
   show_info();

   if (MFP.infd) {
      if (MFP.pgdes) {
         mfc_pgdes(MFP.infn,MFP.currframe);
         }
      mfc_attribute(MFP.outfn,MFP.flgbdr,MFP.flgcol);
      mfconv(MFP.infd, MFP.currframe,MFC_SAVE,MFC_EPS);
      }
   xv_set (frm_save_fn, WIN_SHOW, FALSE, 0);
   timer.it_value.tv_usec = 500000;
   timer.it_interval.tv_usec = 500000;
   panel_set( show_info_item4, PANEL_LABEL_STRING, "Done", 0);
   notify_set_itimer_func( frame, prnt_done, ITIMER_REAL, &timer, NULL);


   /*window_return();*/
   }

/**********************************************************************/
/*                                                                    */
/*                                                                    */
/**********************************************************************/

Notify_value
prnt_done( ) {

   panel_set( show_info_item4, PANEL_LABEL_STRING, "", 0);
   notify_set_itimer_func( frame, prnt_done, ITIMER_REAL, NULL, NULL);

   return NOTIFY_DONE;
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*   void prop_color_proc (Panel_item item, int value, Event event)   */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Set the foreground color attribute for printing.                 */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
prop_color_proc (Panel_item item, int value, Event event) {
   int col;
   if ((col = (int)xv_get (prop_color_item, PANEL_VALUE)) == 2) {
      MFP.flgcol = PPS_GRAY;
      }
   else if (col == 1) {
      MFP.flgcol = PPS_COLOR;
      }
   else {
      MFP.flgcol = PPS_MONO;
      }
   xv_set (frm_prop_lout, WIN_SHOW, FALSE, 0);
   /*window_return();*/
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    void prop_border_proc (Panel_item item, int value, Event event) */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Set frame border on/off for printing.                            */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
prop_border_proc (Panel_item item, int value, Event event) {
   if ((int)xv_get (prop_border_item, PANEL_VALUE) == 0) {
      MFP.flgbdr = PPS_OFF;
      }
   else {
      MFP.flgbdr = PPS_ON;
      }
   xv_set (frm_prop_lout, WIN_SHOW, FALSE, 0);
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    void prop_header_proc (Panel_item item, int value, Event event) */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Set frame header on/off for printing. This includes the folowing */
/*   information: title(as appeared in the cns inputfile)             */
/*                file name, frame number, date created, date printed */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
prop_header_proc (Panel_item item, int value, Event event) {
   if ((int)xv_get (prop_header_item, PANEL_VALUE) == 0) {
      MFP.pgdes = 0;
      }
   else {
      MFP.pgdes = 1;
      }
   xv_set (frm_prop_lout, WIN_SHOW, FALSE, 0);
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*   void prop_retrace_proc (Panel_item item, int value, Event event) */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Retrace the graphics.                                            */
/*                                                                    */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
prop_retrace_proc (Panel_item item, int value, Event event) {
   if ((int)xv_get (prop_retrace_item, PANEL_VALUE) == 0) {
      MFP.retrace = 0;
      adjust = 0;
      }
   else {
      MFP.retrace = 1;
      adjust = 1;
      }
   xv_set (frm_prop_lout, WIN_SHOW, FALSE, 0);
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*   void print_menu_proc ()                                          */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Send frame to printer.  Before doing so, a notice window pops up */
/*   to varify the user's request.                                    */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
print_menu_proc () {
   if (MFP.infd) {
      if (user_notice(YES_NO,"Print frame ?") == NOTICE_YES) {
         show_info();
         mfc_attribute("",MFP.flgbdr,MFP.flgcol);
         if (MFP.pgdes) {
            mfc_pgdes(MFP.infn,MFP.currframe);
            }
         mfconv(MFP.infd, MFP.currframe,MFC_PRINT,MFC_EPS);
         }
      else {
         return;
         }
      }
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    void next_menu_proc ()                                          */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Displays the next frame in the sequence, which is the current    */
/*   frame number plus one.                                           */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
next_menu_proc () {
   if ((MFP.currframe+1) < (MFP.maxfrms)) {
      ++MFP.currframe;
      show_info();
      if (MFP.infd) {
         mfconv(MFP.infd, MFP.currframe,MFC_SCREEN,MFC_XGL);
         }
      }
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*    void previous_menu_proc ()                                      */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Displays the previous frame, which means the current frame minus */
/*   one.                                                             */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
previous_menu_proc () {
   if ((MFP.currframe-1) >= 0) {
      --MFP.currframe;
      show_info();
      if (MFP.infd) mfconv(MFP.infd, MFP.currframe,MFC_SCREEN,MFC_XGL);
      }
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*   void goto_menu_proc ()                                           */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Displays the frame which its number was selected by the user in  */
/*   in the goto window text item.                                    */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
static void
goto_menu_proc () {
   if (!(int) xv_get (frm_get_frame, WIN_SHOW)) {
      xv_set (frm_get_frame, WIN_SHOW, TRUE, 0);
      window_loop (frm_get_frame);
      }
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*   void repaint_screen()                                            */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Repaint the screen by calling mfconv to parse the current frame. */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
void
repaint_screen() {
   
   show_info();
   if (MFP.infd) mfconv(MFP.infd, MFP.currframe,MFC_SCREEN,MFC_XGL);
   }

/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*   int get_usrflds()                                                */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Get the next field from user defaults file. The file is already  */
/*   open and the information is read line by line.  The format of    */
/*   file is documented in ~nsi/docs/mfprint.memo.                    */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
int
get_usrflds() {
 
   char line[NAMEMAX];
   char ukw[NAMEMAX];
   char uvl[NAMEMAX];
   int i;

   fgets(line,80,MFP.usrfd);
   while (!feof(MFP.usrfd)) {
      if (isalpha(line[0])) {
         sscanf(line,"%s %s",ukw,uvl);
         for (i=0;i<KWLEN;i++) {
            if (strcmp(usrkw[i],ukw)==0) {
               switch(i) {
                  case PRN:
                     strcpy(MFP.dfprn,uvl);
                     break;
                  case BDR:
                     if (strcmp(uvl,"on")==0) {
                        MFP.flgbdr = PPS_ON;
                        }
                     else if (strcmp(uvl,"off")==0) {
                        MFP.flgbdr = PPS_OFF;
                        }
                     break;
                  case HDR:
                     if (strcmp(uvl,"on")==0) {
                        MFP.pgdes = PPS_ON;
                        }
                     else if (strcmp(uvl,"off")==0) {
                        MFP.pgdes = PPS_OFF;
                        }
                     break;
                  case RTR:
                     if (strcmp(uvl,"on")==0) {
                        MFP.retrace = PPS_ON;
                        adjust = 1;
                        }
                     else if (strcmp(uvl,"off")==0) {
                        MFP.retrace = PPS_OFF;
                        adjust = 0;
                        }
                     break;

                  case VSL:
                     if (strcmp(uvl,"color")==0) {
                        MFP.flgcol = PPS_COLOR;
                        }
                     else if (strcmp(uvl,"gray")==0) {
                        MFP.flgcol = PPS_GRAY;
                        }
                     else if (strcmp(uvl,"bw")==0) {
                        MFP.flgcol = PPS_MONO;
                        }
                     break;
                  }
               }
            }
         }
      fgets(line,80,MFP.usrfd);
      }
   }
   
/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*   int get_prnflds()                                                */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Get all the necessary data from the ptinter data file.  Look     */
/*   for any file in the current directory with the exetention ".pdf" */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
int
get_prnflds() {
 
   char line[NAMEMAX];
   char pkw[NAMEMAX];
   char pvl[NAMEMAX];
   int i,slen;

   fgets(line,80,MFP.prnfd);
   while (!feof(MFP.prnfd)) {
      if (isalpha(line[0])) {
         sscanf(line,"%s %s",pkw,pvl);
         for (i=0;i<KWPRN;i++) {
            if (strcmp(prnkw[i],pkw)==0) {
               switch(i) {
                  case PNAM:
                     {int i=5;
                     while (line[i] == ' ') {i++;}
                     allo_cpy(SLP.name[SLP.index],line+i);
                     }
                     if (SLP.index == NUMPRN) {
                        SLP.name = (char**)realloc(SLP.name,
                          (SLP.index+NUMPRN)*sizeof(char *));
                        }
                     slen = strlen(SLP.name[SLP.index]);
                     SLP.name[SLP.index][slen-1] = '\0';
                     break;
                  case PALI:
                     {int i=6;
                     while (line[i] == ' ') {i++;}
                     allo_cpy(MPD[SLP.index].alias,line+i);
                     }
                     slen = strlen(MPD[SLP.index].alias);
                     MPD[SLP.index].alias[slen-1] = '\0';
                     break;
                  case PTMA:
                     MPD[SLP.index].tmargin = atof(pvl);
                     break;
                  case PLMA:
                     MPD[SLP.index].lmargin = atof(pvl);
                     break;
                  case PWID:
                     MPD[SLP.index].width = atof(pvl);
                     break;
                  case PHEI:
                     MPD[SLP.index].height = atof(pvl);
                     break;
                  case PCLR:
                     allo_cpy(MPD[SLP.index].visual,"color");
                     MPD[SLP.index].depth = atoi(pvl);
                     break;
                  case PGRY:
                     allo_cpy(MPD[SLP.index].visual,"gray");
                     MPD[SLP.index].depth = atoi(pvl);
                     break;
                  case PBW:
                     allo_cpy(MPD[SLP.index].visual,"bw");
                     MPD[SLP.index].depth = atoi(pvl);
                     break;
                  case PLWI:
                     MPD[SLP.index].lwidth = atoi(pvl);
                     break;
                  }
               }
            }
         }
      fgets(line,80,MFP.prnfd);
      }
   }
   
/**********************************************************************/
/*                                                                    */
/* SYNOPSIS:                                                          */
/*   void set_defaults()                                              */
/*                                                                    */
/* DESCRIPTION:                                                       */
/*   Set the default values for the various global variable and       */
/*   structures used in this program.                                 */
/* RETURN VALUES:                                                     */
/*   None.                                                            */
/**********************************************************************/
void
set_defaults() {

   struct passwd *ud;
   int usrfld,i=0;

   /* Construct user default`s file name */
   ud = getpwuid(geteuid());
   strcpy(MFP.usrfn,"/home/");
   strcat(MFP.usrfn,ud->pw_name);
   strcat(MFP.usrfn,"/.mfprint.dfa");
   
   /* Initialize the paramaters for page layout */
   MFP.pgdes = 1;
   MFP.flgcol = PPS_COLOR;
   MFP.flgbdr = PPS_OFF;

   /* Open user defaults file */
   if ((MFP.usrfd = fopen(MFP.usrfn,"r")) != NULL) {
      get_usrflds();
      }

   /* Initialize the parameters for printer setup */
   MPD = (struct PDF*)malloc(NUMPRN * sizeof(struct PDF));
   SLP.name = (char **)malloc(NUMPRN * sizeof(char *));
   MPD[0].alias = (char *)malloc(strlen("phaser"));
   allo_cpy(SLP.name[0],"Generic Postscript Color Laser Printer");
   allo_cpy(MPD[0].alias,"phaser");    /* Alias as appears in printcap */
   MPD[0].tmargin = 1.09;              /* Top margin of printed area */
   MPD[0].lmargin = 0.20;              /* Left margin of printed area */
   MPD[0].width = 8.10;                /* Max width of printed area */
   MPD[0].height = 8.825;              /* Max height of printed area */
   allo_cpy(MPD[0].visual,"color");    /* Visual properties */
   MPD[0].depth = 24;                  /* Depth of color */
   MPD[0].lwidth = 1;                  /* Line width */

   SLP.num_prn = get_prn_files("/home/roz/src/mfprint");
   SLP.sel_prn = 0;
   while(i < SLP.index) {
      if (strcmp(MPD[i].alias,MFP.dfprn) == 0) {
         SLP.sel_prn = i;
         mfc_prn(MPD[i].alias,MPD[i].tmargin,MPD[i].lmargin,
            MPD[i].width,MPD[i].height,MPD[i].visual,
            MPD[i].depth,MPD[i].lwidth);
         return;
         }
      i++;
      }
   }

/**********************************************************************/
/*                                MAIN                                */
/**********************************************************************/
main(int argc,char *argv[],char *envp[]) {

   Canvas  canvas;                /* Canvas object */
   Xv_Window canvas_win;          /* Canvas window */
   int     Width,                 /* Initial width of the main window */
           Height;                /* Initial height of the main window */

   /* Information about the attributes of the display. Make use 
     of the width and height of screen to initialize the width and 
     height of main window */
   struct {
      Display   *display;         /* Pointer to frame's display */
      XWindowAttributes winatt;   /* Window attributes information */
      int width;                  /* Width of the display */
      int height;                 /* Height of the display */
      int scn_num;                /* Screen number */
      } inf;

   inf.scn_num=0;                 /* Initialize screen ID */

   (void)xv_init(XV_INIT_ARGS, argc, argv, 0);

   /* Open file from command line argument */
   if (argv[1]) {
      strcpy(MFP.infn,argv[1]);
      if ((MFP.infd = fopen(MFP.infn,"r")) == NULL) {
         char str[81];
         fprintf(stderr,"Can not open %s, file does not exist.\n",MFP.infn);
         exit(1);
         }
      }

   /* get test options - canvas depth & raster color type */
   xglut_argprocess(argc, argv);
   if (xglut_color_type == XGL_COLOR_RGB) {
      /*printf("RGB raster color type\n");*/
      }
   else {
      /*printf("INDEX raster color type\n");*/
      }

   if (xglut_depth == 24) {
      /*printf("24bit visual case\n");*/
      }
   else if (xglut_depth == 8) {
      /*printf("8bit visual case\n");*/
      }
   else if (xglut_depth == 24) {
      printf("Depth is not 8bit visual\n");
      exit(1);
      }
   set_defaults();
   if ((frame = xv_create (NULL, FRAME,
                      FRAME_LABEL, "MF Print",
                      WIN_X, 0,
                      WIN_Y, 0,
                      0)) == NULL) {
      printf ("Cannot make frame\n");
      exit (0);
      }

   /* Create the control panels  */
   panel_main = xv_create (frame, PANEL, WIN_ROWS, 2, 0);

   /* Create menu items for "file" menu */
   Open = (Menu_item)xv_create(XV_NULL, MENUITEM,
                      MENU_STRING, "Open",
                      MENU_NOTIFY_PROC, open_menu_proc,
                      0);
   Save = (Menu_item)xv_create(XV_NULL, MENUITEM,
                      MENU_STRING, "Save",
                      MENU_NOTIFY_PROC, save_menu_proc,
                      0);
   Print = (Menu_item)xv_create(XV_NULL, MENUITEM,
                      MENU_STRING, "Print",
                      MENU_NOTIFY_PROC, print_menu_proc,
                      0);


   file_menu = (Menu)xv_create(XV_NULL, MENU, NULL);
   xv_set(file_menu,
                      MENU_APPEND_ITEM, Open,
                      MENU_APPEND_ITEM, Save,
                      MENU_APPEND_ITEM, Print,
                      0);

   /* Create menu items for "Properties" menu */
   Page_Layout = (Menu_item)xv_create(XV_NULL, MENUITEM,
                      MENU_STRING, "Page Layout",
                      MENU_NOTIFY_PROC, Page_Layout_proc,
                      0);
   Printer_Setup = (Menu_item)xv_create(XV_NULL, MENUITEM,
                      MENU_STRING, "Select Printer",
                      MENU_NOTIFY_PROC, Printer_Setup_proc,
                      0);

   Save_prop = (Menu_item)xv_create(XV_NULL, MENUITEM,
                      MENU_STRING, "Save Properties",
                      MENU_NOTIFY_PROC, Save_prop_proc,
                      0);

   properties_menu = (Menu)xv_create(XV_NULL, MENU, NULL);
   xv_set(properties_menu,
                      MENU_APPEND_ITEM, Page_Layout,
                      MENU_APPEND_ITEM, Printer_Setup,
                      MENU_APPEND_ITEM, Save_prop,
                      0);


   xv_set (Save, MENU_INACTIVE, TRUE, 0);
   xv_set (Print, MENU_INACTIVE, TRUE, 0);


   /* Create the "Properties" menu */
   (void) panel_create_item (panel_main, PANEL_BUTTON,
                      XV_Y, xv_row (panel_main, 0),
                      XV_X, xv_col (panel_main, 10),
                      PANEL_LABEL_STRING, "Properties",
                      PANEL_ITEM_MENU, properties_menu,
                      0);

   /* Create the "File" menu */
   (void) panel_create_item (panel_main, PANEL_BUTTON,
                      XV_Y, xv_row (panel_main, 0),
                      XV_X, xv_col (panel_main, 1),
                      PANEL_LABEL_STRING, "File",
                      PANEL_ITEM_MENU, file_menu,
                      0);

   /* Create the "Quit" button */
   (void) panel_create_item (panel_main, PANEL_BUTTON,
                      XV_Y, xv_row (panel_main, 0),
                      XV_X, xv_col (panel_main, 26),
                      PANEL_NOTIFY_PROC, quit_proc,
                      PANEL_LABEL_STRING, "Quit",
                      0);

   /* Create the "Goto" button */
   (void) panel_create_item (panel_main, PANEL_BUTTON,
                      XV_Y, xv_row (panel_main, 1),
                      XV_X, xv_col (panel_main, 1),
                      PANEL_LABEL_STRING, "Goto",
                      PANEL_NOTIFY_PROC, goto_menu_proc,
                      0);

   /* Create the "Next" button */
   (void) panel_create_item (panel_main, PANEL_BUTTON,
                      XV_Y, xv_row (panel_main, 1),
                      XV_X, xv_col (panel_main, 10),
                      PANEL_LABEL_STRING, "Next",
                      PANEL_NOTIFY_PROC, next_menu_proc,
                      0);


   /* Create the "Previous" button */
   (void) panel_create_item (panel_main, PANEL_BUTTON,
                      XV_Y, xv_row (panel_main, 1),
                      XV_X, xv_col (panel_main, 18),
                      PANEL_LABEL_STRING, "Previous",
                      PANEL_NOTIFY_PROC, previous_menu_proc,
                      0);

   /* Create the "Print" button */
   (void) panel_create_item (panel_main, PANEL_BUTTON,
                      XV_Y, xv_row (panel_main, 1),
                      XV_X, xv_col (panel_main, 31),
                      PANEL_NOTIFY_PROC, print_menu_proc,
                      PANEL_LABEL_STRING, "Print",
                      0);

   /* Create the slider */
   frm_prop_speed = (Panel_slider_item) 
                      xv_create(panel_main, PANEL_SLIDER,
                      XV_Y, xv_row (panel_main, 0),
                      XV_X, xv_col (panel_main, 40),
                      PANEL_LABEL_STRING, "Speed",
                      PANEL_VALUE, 0,
                      PANEL_MIN_VALUE, 0,
                      PANEL_MAX_VALUE, 10,
                      PANEL_TICKS, 4,
                      PANEL_NOTIFY_PROC, adjust_speed_proc,
                      NULL);


   /* Create panel messages */
   show_info_item1 = panel_create_item( panel_main, PANEL_MESSAGE,
                      XV_Y, xv_row (panel_main, 1),
                      XV_X, xv_col (panel_main, 40),
                      PANEL_LABEL_STRING, "File name:",
                      0);

   show_info_item2 = panel_create_item( panel_main, PANEL_MESSAGE,
                      XV_Y, xv_row (panel_main, 0),
                      XV_X, xv_col (panel_main, 80),
                      PANEL_LABEL_STRING, "Current Frame:",
                      0);

   show_info_item3 = panel_create_item( panel_main, PANEL_MESSAGE,
                      XV_Y, xv_row (panel_main, 1),
                      XV_X, xv_col (panel_main, 80),
                      PANEL_LABEL_STRING, "Total Frames:",
                      0);

   show_info_item4 = panel_create_item( panel_main, PANEL_MESSAGE,
                      XV_Y, xv_row (panel_main, 0),
                      XV_X, xv_col (panel_main, 100),
                      0);

  /* Create dialog windows */
  frm_get_fn = xv_create (frame, FRAME_CMD,
                      FRAME_LABEL, "Load File",
                      FRAME_SHOW_LABEL, TRUE,
                      FRAME_NO_CONFIRM, TRUE,
                      0);

  frm_sel_prn = xv_create (frame, FRAME_CMD,
                      FRAME_LABEL, "Select Printer",
                      FRAME_SHOW_LABEL, TRUE,
                      FRAME_NO_CONFIRM, TRUE,
                      0);

  /* Create image servers for icons */
  dir_image = xv_create (NULL, SERVER_IMAGE,
                      XV_WIDTH, dir_width,
                      XV_HEIGHT, dir_height,
                      SERVER_IMAGE_BITS,dir_bits, 
                      0);

  file_image = xv_create (NULL, SERVER_IMAGE,
                      XV_WIDTH, file_width,
                      XV_HEIGHT, file_height,
                      SERVER_IMAGE_BITS,file_bits, 
                      0);

  arrow_image = xv_create (NULL, SERVER_IMAGE,
                      XV_WIDTH, arrow_width,
                      XV_HEIGHT, arrow_height,
                      SERVER_IMAGE_BITS,arrow_bits, 
                      0);

  panel_sel_prn = (Panel)xv_get (frm_sel_prn, FRAME_CMD_PANEL, 0);
  panel_get_fn = (Panel)xv_get (frm_get_fn, FRAME_CMD_PANEL, 0);
  xv_set(panel_get_fn, WIN_ROWS,50,NULL);

   /* Create the file menu scrolling list. */
  f_load_item = panel_create_item (panel_get_fn, PANEL_BUTTON,
                      XV_Y, xv_row (panel_get_fn, 1),
                      XV_X, xv_col (panel_get_fn, 40),
                      PANEL_LABEL_STRING, "  Load  ",
                      PANEL_NOTIFY_PROC, f_load_proc,
                      0);

  f_file_item = panel_create_item (panel_get_fn, PANEL_TEXT,
                      XV_Y, xv_row (panel_get_fn, 0),
                      XV_X, xv_col (panel_get_fn, 2),
                      PANEL_LABEL_STRING, "File:",
                      PANEL_VALUE_DISPLAY_LENGTH, 30,
                      PANEL_NOTIFY_PROC, f_file_proc,
                      0);

  f_list_item = panel_create_item (panel_get_fn, PANEL_LIST,
                      XV_Y, xv_row (panel_get_fn, 1),
                      XV_X, xv_col (panel_get_fn, 2),
                      PANEL_LIST_STRING," ", NULL,
                      PANEL_LIST_ROW_HEIGHT,20,
                      PANEL_LIST_WIDTH,250,
                      PANEL_NOTIFY_PROC, f_list_proc,
                      0);

  window_fit(panel_get_fn);
  window_fit(frm_get_fn);

  p_OK_item = panel_create_item (panel_sel_prn, PANEL_BUTTON,
                      XV_Y, xv_row (panel_sel_prn, 1),
                      XV_X, xv_col (panel_sel_prn, 48),
                      PANEL_LABEL_STRING, "  OK  ",
                      PANEL_NOTIFY_PROC, p_OK_proc,
                      0);

  p_Cancel_item = panel_create_item (panel_sel_prn, PANEL_BUTTON,
                      XV_Y, xv_row (panel_sel_prn, 2),
                      XV_X, xv_col (panel_sel_prn, 48),
                      PANEL_LABEL_STRING, "Cancel",
                      PANEL_NOTIFY_PROC, p_Cancel_proc,
                      0);

  p_msg_item = panel_create_item (panel_sel_prn, PANEL_MESSAGE,
                      XV_Y, xv_row (panel_sel_prn, 0),
                      XV_X, xv_col (panel_sel_prn, 2),
                      PANEL_LABEL_STRING, "Installed Printers:",
                      0);

  p_list_item = panel_create_item (panel_sel_prn, PANEL_LIST,
                      XV_Y, xv_row (panel_sel_prn, 1),
                      XV_X, xv_col (panel_sel_prn, 2),
                      PANEL_LIST_STRING," ",NULL,
                      PANEL_LIST_DISPLAY_ROWS,9,
                      PANEL_LIST_ROW_HEIGHT,20,
                      PANEL_LIST_WIDTH,300,
                      PANEL_NOTIFY_PROC, p_list_proc,
                      0);

   /* Setup the icon of the program before the window_main_loof call */
   {
   Rect *r_l,r_f;

   r_l = (Rect *)xv_get(p_list_item,XV_RECT);

   r_f.r_top = r_f.r_left = 20;
   r_f.r_width = 400;
   r_f.r_height = rect_bottom(r_l) + 50;

   frame_set_rect(frm_sel_prn,&r_f);
   }

   window_fit(panel_sel_prn);
   window_fit(frm_sel_prn);

/* lisa */
   frm_prop_lout = xv_create (frame, FRAME_CMD,
                      FRAME_LABEL, "Page Layout",
                      FRAME_SHOW_LABEL, TRUE,
                      FRAME_NO_CONFIRM, TRUE,
                      0);

   panel_prop_lout = (Panel)xv_get (frm_prop_lout, FRAME_CMD_PANEL, 0);

   prop_color_item = panel_create_item (panel_prop_lout, PANEL_CHOICE,
                      PANEL_LABEL_STRING, "Foreground Color:",
                      PANEL_CHOICE_STRINGS, "B&W","Colors","Gray scale",NULL,
                      PANEL_NOTIFY_PROC, prop_color_proc,
                      PANEL_VALUE,MFP.flgcol ,
                      0);

   prop_border_item = panel_create_item (panel_prop_lout, PANEL_CHOICE,
                      PANEL_LABEL_STRING, "Page Borders:",
                      PANEL_CHOICE_STRINGS, "Off","On",NULL,
                      PANEL_NOTIFY_PROC, prop_border_proc,
                      PANEL_VALUE, MFP.flgbdr,
                      0);

   prop_header_item = panel_create_item (panel_prop_lout, PANEL_CHOICE,
                      PANEL_LABEL_STRING, "Print Header:",
                      PANEL_CHOICE_STRINGS, "Off","On",NULL,
                      PANEL_NOTIFY_PROC, prop_header_proc,
                      PANEL_VALUE, MFP.pgdes,
                      0);

   prop_retrace_item = panel_create_item (panel_prop_lout, PANEL_CHOICE,
                      PANEL_LABEL_STRING, "Retrace Graphics:",
                      PANEL_CHOICE_STRINGS, "Off","On",NULL,
                      PANEL_NOTIFY_PROC, prop_retrace_proc,
                      PANEL_VALUE, MFP.retrace,
                      0);


   window_fit(panel_prop_lout);
   window_fit(frm_prop_lout);

   frm_save_fn = xv_create (frame, FRAME_CMD,
                      FRAME_LABEL, "Save File",
                      FRAME_SHOW_LABEL, TRUE,
                      FRAME_NO_CONFIRM, TRUE,
                      0);

   panel_save_fn = (Panel)xv_get (frm_save_fn, FRAME_CMD_PANEL, 0);

   fn_save_item = panel_create_item (panel_save_fn, PANEL_TEXT,
                      PANEL_LABEL_STRING, "File Name:",
                      PANEL_VALUE_DISPLAY_LENGTH, 30,
                      PANEL_NOTIFY_PROC, fn_save_proc,
                      0);

   window_fit(panel_save_fn);
   window_fit(frm_save_fn);

   frm_get_frame = xv_create (frame, FRAME_CMD,
                      FRAME_LABEL, "Get Frame Number",
                      FRAME_SHOW_LABEL, TRUE,
                      FRAME_NO_CONFIRM, TRUE,
                      0);

   panel_get_frame = (Panel)xv_get(frm_get_frame, FRAME_CMD_PANEL, 0);


   frame_item = panel_create_item (panel_get_frame, PANEL_TEXT,
                      PANEL_LABEL_STRING, "Frame Number:",
                      PANEL_VALUE_DISPLAY_LENGTH, 30,
                      PANEL_NOTIFY_PROC, frame_quit_proc,
                      0);

   window_fit (panel_get_frame);
   window_fit (frm_get_frame);

   if ((canvas = xv_create (frame, CANVAS,
                      WIN_DEPTH, xglut_depth,
                      XV_VISUAL_CLASS, xglut_visual_class,
                      WIN_SHOW, TRUE,
                      CANVAS_REPAINT_PROC,   repaint_screen,
                      WIN_BELOW, panel_main,
			 0)) == NULL) {
      printf ("Cannot make canvas\n");
      exit (0);
      }

   canvas_win = (Xv_window) canvas_paint_window(canvas);

   mfp_icon_image = xv_create (NULL, SERVER_IMAGE,
                      XV_WIDTH, mfp_width,
                      XV_HEIGHT, mfp_height,
                      SERVER_IMAGE_BITS,mfp_bits, 
                      0);

   mfp_icon = xv_create(frame, ICON,
                      ICON_IMAGE, mfp_icon_image, 
                      0);

   xv_set(frame,FRAME_ICON,mfp_icon,NULL);


   xv_set(canvas_win,
                      WIN_EVENT_PROC, event_proc,
                      WIN_NO_EVENTS,
                      WIN_CONSUME_EVENTS,
                      WIN_RESIZE, WIN_REPAINT, 
                      WIN_MOUSE_BUTTONS,NULL,
                      WIN_CONSUME_X_EVENT_MASK,
                      StructureNotifyMask, NULL,
                      NULL);

   xglut_wm_install(frame, canvas);

   /* Get information about the width and height of the screen */
   inf.display = (Display *)xv_get(frame, XV_DISPLAY);
   if (XGetWindowAttributes(inf.display,RootWindow(inf.display,inf.scn_num),
      &inf.winatt) == 0) {
      fprintf(stderr,"(mfdraw) failed to get window attributes.\n");
      fflush(stderr);
      exit(-1);
      }
   else {
       Width = inf.winatt.width-90; 
       Height = (int)((float)(Width)*(float)RATIO);
       xv_set (frame, WIN_WIDTH, Width, 0);
       xv_set (frame, WIN_HEIGHT, Height, 0);
      }

   plot_init((Display *)xv_get(frame, XV_DISPLAY),
          (Xgl_usgn32)xv_get(canvas_win,XV_XID),
          (int)DefaultScreen((Display *)xv_get(frame, XV_DISPLAY)),
          Width,Height);

   (void) notify_interpose_destroy_func(frame, quit_proc);

   /* If user specified the file name as a command line argument
      then process the file */
   if (argv[1]) {
         MFP.currframe = 0;
         MFP.maxfrms = MFmaxfrms(MFP.infd);
         xv_set (Save, MENU_INACTIVE, FALSE, 0);
         xv_set (Print, MENU_INACTIVE, FALSE, 0);
         xv_set (frm_get_fn, WIN_SHOW, FALSE, 0);
         show_info();
         }

   window_main_loop (frame);
   exit(0);

   }

