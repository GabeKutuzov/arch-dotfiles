/* (c) Copyright 1989-2005, The Rockefeller University *11115* */
/* $Id: plbdef.h 70 2017-01-16 19:27:55Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              PLBDEF.H                                *
*                                                                      *
*  Definition of records for plot label (PLAB) and line (PLINE).       *
*  Note: All (x,y) coordinates are measured in inches.                 *
*                                                                      *
************************************************************************
*  Rev, 03/01/89, Initial version                                      *
*  Rev, 05/01/91, GNR - Make color an array rather than a pointer      *
*  V8B, 08/25/01, GNR - Label text to text cache, two structure types  *
*  Rev, 02/16/05, GNR - Add line thickness parameter                   *
*  ==>, 02/17/05, GNR - Last mod before committing to svn repository   *
***********************************************************************/

   struct PLBDEF {            /* Plot label record */
      struct PLBDEF *pnxplb;     /* Ptr to next PLBDEF record */
      float lbx;                 /* X coord of label (from left edge) */
      float lby;                 /* Y coord of label (from bottom) */
                                 /* Note: (lbx,lby) is relative to lower
                                 *  left corner of 1st letter in plot */
      float lht;                 /* Lettering height (inches) */
      float ang;                 /* Lettering angle
                                 *  (ccw deg. from horiz.) */
      txtid hplabel;             /* Text locator for plot label */
      short lplabel;             /* Length of label text */
      int   kplbenb;             /* TRUE if this block is enabled */
      char  lbcolor[COLMAXCH];   /* Color of label */
      };

   struct PLNDEF {            /* Plot line record */
      struct PLNDEF *pnxpln;     /* Ptr to next PLNDEF record */
      float x1;                  /* X starting coord (from left edge) */
      float y1;                  /* Y starting coord (from bottom) */
      float x2;                  /* X ending coord */
      float y2;                  /* Y ending coord */
      char  lncolor[COLMAXCH];   /* Color of line */
      char  kplnenb;             /* TRUE if this block is enabled */
      schr  kthick;              /* Line thickness (-1 = default) */
      };
