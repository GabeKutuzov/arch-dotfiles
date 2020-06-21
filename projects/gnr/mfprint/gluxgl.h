/* (c) 1992 Neurosciences Research Foundation */
/* $Id: gluxgl.h 1 2008-12-19 16:56:22Z  $ */
/**********************************************************************
 *                                                                    *
 *                              GLUXGL.H                              *
 *                                                                    *
 * Version 1, 06/23/92, ROZ                                           *
 * ==>, 07/01/02, GNR - Last date before committing to svn repository *
 **********************************************************************/


#define ON  1
#define OFF 0
#define NSI_CMAP_SIZE 128

#define PENDOWN   2
#define PENUP     3
#define RETRACE   1
#define NORETRACE 0
#define FILLED   -1
#define OPEN      0
#define THICK     1
#define THIN      0
#define MM_NOCHG  0
#define MM_STILL  1
#define MM_MOVIE  2
#define MD_MATRIX 0
#define MD_BEEPER 1

#define PPI INTOPIX       /* Pixels per inch (768/11.0) */
#define MAXSUFFIX 16      /* Maximum suffix length */
#define MAXSTRING 32      /* Maximum label size */
#define MINPPCH 6         /* Minimum size of label characters */
#define MAXPPCH 134       /* Maximum size of label characters */

#define EXPTIME  2.0      /* Minimum time for one movie exposure */
#define TORADS 0.01745329 /* Angle to radians converter       */
#define COSBS 0.866025    /* cosine pi/6 */
#define SINBS 0.5         /* sin pi/6    */

#define SCALEHT  0.15     /* Standard height for axis scale ticks  */
#define VALUEHT  0.105    /* Standard height for axis value labels */
#define VALUEOFF 0.20     /* Standard offset of axis value labels  */
#define LABELHT  0.14     /* Height of axis labels                 */
#define LABELOFF 0.36     /* Offset of plot labels from axis       */

#define JUST_LT 0         /* Left justify text at current plot pointer*/
#define JUST_RT 2         /* Right justify text at current plot pointer */
#define CENTERED 3        /* Center text around current plot pointer  */

#define LINEAR 0          /* Label horizontally left-to-right */
#define UPWARDS 8         /* Label vertically bottom-to-top   */
#define UPSIDE_DOWN 16    /* Label horizontally upside-down   */
#define DOWNWARDS 24      /* Label vertically top-to-bottom   */

#define METF      0       /* Metafile only mode */
#define SGX       1       /* Sun graphics only mode */
#define MFSG      2       /* Sun graphics + metafile mode */
#define NO_MFSG   3       /* No Sun graphics and no metafile mode */

#define ON  1
#define OFF 0

#define WHITE   0  /* "BLACK" */
#define BLACK   1  /* "WHITE" */
#define BLUE    2
#define CYAN    3
#define MAGENTA 4
#define VIOLET  5
#define ORANGE  6
#define GREEN   7
#define YELLOW  8
#define RED     9


#define hexch_to_int(c) ( ((c) <= '9') ? (c) - '0' : (c) - 'A' + 10 )
#define scalex(c) ((int)((c)*SXGL.factx *SXGL.base_fact))
#define scaley(c) ((int)((c)*SXGL.facty *SXGL.base_fact))
#define a_scaley(c) ((int)((c)*SXGL.facty * \
                   SXGL.base_fact) + SXGL.trans_fact)

#define STDCOL 10             /* Number of standard defined colors */

struct SXGLdef {
       float xcurr, ycurr;    /* Current (x,y) */
       float factx;           /* Current factor value */
       float facty;           /* Current factor value */
       int vpx;               /* User coordinate x */
       int vpy;               /* User coordinate y */
       float base_fact;       /* Initial factor value */
       int trans_fact;        /* Translation value based on new width and
                                 height when a window is resized */
       int trans_x;           /* Translation value for x */
       int trans_y;           /* Translation value for y */
       int currcol;           /* Current plot color */
       int init_width;        /* Iinitial width of the main window */
       int init_height;       /* Iinitial height of the main window */
       int curretrac;         /* Current line retrace (krt) */
       int ngcallno;          /* 0 before ngraph called, 1 after first
                                 call, then 2 */
       char moviemode;        /* 1 if in movie mode, 0 otherwise.
                                 (Note: This option refers to whether
                                 each new display appears at once or
                                 waits for a button--it has nothing
                                 to do with photography.) */
       };



#ifndef PLOTMAIN
extern struct SXGLdef SXGL;
#else
struct SXGLdef SXGL= {
   0.0,0.0,  /* xcurr, ycurr */
   1.0,      /* factx */
   1.0,      /* facty */
   2800,     /* vpx */
   2200,     /* vpy */
   1.0,      /* base_fact */
   0,        /* trans_fact */
   0,        /* trans_x */
   0,        /* trans_y */
   1,        /* currcol */
   1000,     /* init_width */
   785,      /* init_height */
   1,        /* curretrac */
   0,        /* ngcallno */
   1,        /* moviemode */
   } ;
#endif
