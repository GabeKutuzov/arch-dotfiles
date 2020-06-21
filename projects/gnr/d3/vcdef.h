/* (c) Copyright 1991-2016, The Rockefeller University *21115* */
/* $Id: vcdef.h 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                          VCDEF Header File                           *
*                                                                      *
*  Define structures for virtual cell types.  These structures are     *
*     created only on serial or PAR0 nodes.  These cells are used      *
*     to implement sensory inputs to a CNS model.                      *
*                                                                      *
************************************************************************
*  Rev, 04/12/91, JMS - Initial version                                *
*  Rev, 04/21/91, GNR - Change name, add size, plot, type parameters   *
*  V8A, 04/08/95, GNR - Add pointers to source and modality            *
*  V8B, 12/27/00, GNR - Move to new memory management routines         *
*  V8D, 02/13/07, GNR - Support BBD senses, IMGTOUCH                   *
*  ==>, 05/22/07, GNR - Last mod before committing to svn repository   *
*  V8H, 11/07/10, GNR - Add ability to receive ign,isn from BBDs       *
*  R66, 03/11/16, GNR - Rename nvx->nsxl, nvy->nsya so != ix->nv[xy]   *
***********************************************************************/

/* Structure used to define a virtual cell type */

struct VCELL {
   struct VCELL *pnvc;        /* Ptr to next VCELL block.  Must be
                              *  first item in struct for sort() */
   struct MODALITY *pvcmdlt;  /* Ptr to modality of this sense--
                              *  NULL value means has no modality */
   void *pvcsrc;              /* Ptr to arm, window, or other source */
   struct JNTDEF *pjnt0;      /* Ptr to first joint (VT_SRC only) */
   size_t ovc;                /* Offset in pbcst to virtual cells */
   char  vcname[LSNAME];      /* Built-in or protocol name */
   float vcmin;               /* Minimum virtual cell activity */
   float vcmax;               /* Maximum virtual cell activity */
   float vhw;                 /* Half-width of units */
   float vcplx;               /* X coordinate for plotting */
   float vcply;               /* Y coordinate for plotting */
   float vcplw;               /* Width for plotting */
   float vcplh;               /* Height for plotting */
   ui32  nvcells;             /* nvcells = sum(nsxl * nsya) */
   si32  tll,tla;             /* Lateral,axial array size (FBxy) */
   struct EXTRCOLR vcxc;      /* Color extraction information */
   txtid hvname;              /* Long name locator or VJ,VW index */
   /* BBD reader code and setup in d3snsa assumes order is isn,ign */
   ui16  isn;                 /* Stimulus number returned by sense */
   ui16  ign;                 /* Group number */
   ui16  jntinVT;             /* Joints used (VT only) */
   short lvrn;                /* Length of long virtual rep name
                              *  or 0 if hvname is an index */
   short nsxl;                /* Number of units along x (lateral) */
   short nsya;                /* Number of units along y (axial)
                              *  (sum over jntinVT for type VT) */
   short vcpt;                /* Circle plot threshold (S14) */
   ui16  kvp;                 /* Plot options for virtual cells */
#define VKPPL    0x0001          /* 'B' Opt--Bubble plot  */
#define VKPPG    0x0002          /* 'G' Opt--Plot grids   */
#define VKPPR    0x0004          /* '2' Opt--Plot retrace */
#define VKPSS    0x0008          /* 'Q' Opt--Use square for circle */
#define VKPCO    0x0010          /* 'K' Opt--Plot color fills      */
#define VKPOM    0x0020          /* 'O' Opt--Omit titles  */
#define VKPISG   0x0040          /* 'I' Opt--Plot isn,ign */
#define VKPNA    0x0080          /* 'N' Opt--Use |s| in vcpt test */
   /* N.B.  We want to allow users to change plot options at
   *  Group 3 time, but not VKR4 option, so we allow VKTR4 to
   *  be entered at Group 1 time, but copied to vcflags VKR4
   *  where it is stable for remainder of run.  */
#define VKTR4    0x0100          /* Temp for VKR4 bit */
   byte  vcflags;             /* Internal flags */
#define VKUSED     0x01          /* Sense is used by some conntype */
#define VKR4       0x80          /* 'F' Opt--4-byte real BBD data */
   byte  vtype;               /* Type of units represented
                                 (values defined in d3global.h) */
   };

/* Define virtual image touchpad.  This includes a VCELL for the
*  information relating to the position of the touch sensor, plus
*  some additional information relating to the image.  pvcsrc in
*  the vc struct points to the position-orientation source VCELL.
*  Variables vc.vcmin,vc.vcmax are used to store the size of one
*  touchpad box in image units along x,y in that order.
*  The pointer to the camera stored here is only valid after
*  d3tchk has executed and the device is known to be used.  */

struct IMGTPAD {
   struct VCELL   vc;         /* All the stuff above */
   TVTYPE *pcam;              /* Ptr to camera info */
   float  offx,offy,offa;     /* X,Y,angle offsets */
   float  sclx,scly,scla;     /* X,Y,angle movement scales */
   float  usca;               /* scla * DEGS_TO_RADS */
   /* CAUTION:  For compatibility with old code, tla,tll here are
   *  floats, but tla,tll in vcdef are si32 (FBxy = S15) */
   float  tla,tll;            /* Axial,lateral pad size in image */
   txtid  hcamnm;             /* Handle to camera name */
   short  ovsx,ovsy,ovso;     /* Offsets in pvcsrc to x,y,o */
   byte   itchng;             /* CHANGE [0-3] option */
   byte   itop;               /* Pressure type and other options */
#define ITOP_INT     0x01        /* Integral touch */
#define ITOP_PRES    0x02        /* Pressure touch */
#define ITOP_USR     0x80        /* User-defined sense */
   } ;

