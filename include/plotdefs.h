/* (c) Copyright 1992-2017, The Rockefeller University *11115* */
/* $Id: plotdefs.h 62 2017-09-15 17:47:40Z  $ */
/**********************************************************************
*                       PLOTDEFS.H Header File                        *
*                 ROCKS plot parameter declarations                   *
*                                                                     *
***********************************************************************
*  V1A, 05/11/92, GNR - Initial version                               *
*  V2A, 12/12/98, GNR - Adhere to specification document              *
*  Rev, 06/02/07, GNR - Add codes for BM_C8, BM_16, BM_C24            *
*  Rev, 02/24/08, GNR - Add codes for SKP_META, SKP_XG                *
*  ==>, 03/08/08, GNR - Last date before committing to svn repository *
*  Rev, 08/26/11, GNR - COLOR typedef to unsigned int for 64 bit      *
*  Rev, 08/09/14, GNR - Add new codes for frames, etc.                *
*  Rev, 04/07/17, GNR - Add BUT_DEAD intxeq bit                       *
*  Rev, 05/17/17, GNR - Add BM_GS16, BM_C48, BM_NONSTD                *
**********************************************************************/
#ifndef _PXDEF_H_
#define _PXDEF_H_

#include "sysdef.h"

/* Type of a color index */
typedef unsigned int COLOR;

/* Use these macros to determine whether an image is colored
*  or grayscale based on its BM_xxx mode code */
#define qColored(mode) ((mode & BM_MASK) > BM_GS16)
#define qGray(mode)    ((mode & BM_MASK) <= BM_GS16)

/* Defined constants used in plot calls */
#define DFLTFRAME    0     /* Default plotting frame        */
#define FRM_BORD  0x01     /* Frame has border              */
#define FRM_CLIP  0x02     /* Clip contents at border       */
#define FRM_DRAG  0x04     /* Frame is draggable by user    */
#define FRM_ROTN  0x08     /* Rotation matrix is entered    */
#define FRM_STOR  0x10     /* Store commands for later use  */
#define FRM_BMDR  0x20     /* Bookmark whenframe is drawn   */
#define FRM_SYNC  0x40     /* frmdef requires outbound sync */
#define FRM_BDIS  0x80     /* frmdbm requires inbound sync  */
/* Following codes reserved for internal use */
#define FRM_NOOP  0x0100   /* Store info, no output to file */

#define PENDOWN      2     /* Move with pen down            */
#define PENUP        3     /* Move with pen up              */
#define RETRACE      1     /* Draw with retracing           */
#define NORETRACE    0     /* Draw without retracing        */
#define FILLED      -1     /* Draw filled object            */
#define THIN         0     /* Draw thin lines               */
#define THICK        1     /* Draw thick lines              */
#define CLOSED_THIN  2     /* Draw closed thin lines        */
#define CLOSED_THICK 3     /* Draw closed thick lines       */
#define CENTERED     3     /* Center text at current x,y    */
#define MAXLPCOL    16     /* Max length of pencol() arg    */
#define VXX          0     /* Index of vxx in frame matrix  */
#define VXY          1     /* Index of vxy in frame matrix  */
#define VXC          2     /* Index of vxc in frame matrix  */
#define VYX          3     /* Index of vyx in frame matrix  */
#define VYY          4     /* Index of vyy in frame matrix  */
#define VYC          5     /* Index of vyc in frame matrix  */
#define AX_TKCCW     1     /* Counterclockwise tick marks   */
#define AX_TKEFMT    2     /* E format tick values          */
#define AX_TKEXP     4     /* Log plot ticks as exponents   */
#define AX_LBLLS     8     /* Polar plot label long spokes  */
#define AX_LBLTOP   16     /* Polar plot label at top       */
/* Note:  Odd order of these BM definitions is for compat with
*  older versions of the library.  Design decree:  code will
*  assume values > 1 byte are coerced to natural system byte
*  order by input code.  BM_COLOR is not implemented in oldplot
*  library.  */
#define BM_BW        0     /* Bitmap is black & white       */
#define BM_GS        1     /* Bitmap is 8-bit grayscale     */
#define BM_GS16      2     /* Bitmap is 16-bit grayscale    */
#define BM_C48       3     /* Bitmap is 16-16-16 RGB colored*/
#define BM_COLOR     4     /* Bitmap is index colored       */
#define BM_C8        5     /* Bitmap is 2-3-3 BGR colored   */
#define BM_C16       6     /* Bitmap is 5-5-5 RGB colored   */
#define BM_C24       7     /* Bitmap is 8-8-8 RGB colored   */
#define BM_MASK      7     /* Mask to extract above modes   */
#define BM_NSME      8     /* Bitmap--no submap extraction  */
/* The following type code (possibly with other identifiers)
*  labels an image whose format cannot be described by the
*  above codes -- used in CNS for real sensory values */
#define BM_NONSTD 0x40     /* Nonstandard image format flag */
#define BM_BAD    0x80     /* Unsupported image format      */
#define GM_SET       0     /* Set pixels from current color */
#define GM_XOR       1     /* XOR pixels with current color */
#define GM_AND       2     /* AND pixels with current color */
#define GM_CLR       3     /* Clear existing pixel data     */
#define MD_MATRIX    0     /* Movie device is MATRIX        */
#define MD_BEEPER    1     /* Movie device is beeper        */
/* Bits for movie_mode field in button messages */
#define MM_NOCHG     0     /* Movie mode no change          */
#define MM_STILL     1     /* Enter still mode              */
#define MM_MOVIE     2     /* Enter movie mode              */
#define MM_BATCH     3     /* Enter batch mode              */
/* Bits for intxeq field in button messages */
#define BUT_INTX  0x01     /* Interrupt button was pressed  */
#define BUT_QUIT  0x02     /* Quit button was pressed       */
#define BUT_CLOSE 0x04     /* Close this window             */
#define BUT_DRAG  0x10     /* Frame was dragged             */
#define BUT_NVXX  0x20     /* Frame shape was changed       */
#define BUT_SLCT  0x40     /* User selected an object       */
#define BUT_DEAD  0x80     /* mfdraw died or mfsr killed it */
/* Bits for newplt kout argument.
*  Controls for X graphics and metafile are defined as
*  "skip" rather than "do" flags so existing code that
*  does not use them will still produce plots.  */
#define SKP_META     4     /* Skip metafile this frame      */
#define SKP_XG       8     /* Skip X graphics this frame    */
#define DO_MVEXP    16     /* Do movie exposure             */

#endif /* Not defined _PXDEF_H_ */