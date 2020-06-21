/* (c) Copyright 1992-2017, The Rockefeller University *21116* */
/* $Id: mfio.h 64 2018-05-07 21:58:15Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                             MFIO.H                                   *
*   Plot library header for working with binary graphics metafiles     *
*                                                                      *
*  This version is now usable for both the new and old plot libraries. *
*  The old definitions will be in effect if __GLUH__ is defined first. *
************************************************************************
*  Version 1, 05/05/92, ROZ                                            *
*  Rev, 02/09/94,  LC - Added defines for error bits in ack to host.   *
*  Rev, 03/05/94, GNR - Added MFDRAWNAME,MFDRAWPATH.                   *
*  Rev, 04/01/98, GNR - Clarify difference between errors 263 and 270. *
*  V2A, 12/29/98, GNR - Move button structure to glu.h, add MFB_ATOM   *
*  Rev, 02/23/06, GNR - Change MFDRAWPORT to 10090                     *
*  Rev, 02/23/08, GNR - Better comments, use err 284, previously same  *
*                       as err 265, for metafile exceeds 2GB           *
*  Rev, 08/26/11, GNR - Change longs for 64-bit compilation            *
*  Rev, 01/18/15, GNR - Update for current library spec                *
*  Rev, 08/03/16, GNR - Add MFDSUMsg struct definition                 *
*  ==>, 08/03/16, GNR - Last date before committing to svn repository  *
*  Rev, 08/04/16, GNR - Move metafile graphics types to 0x3200 range   *
*  Rev, 04/07/17, GNR - Define combined ack + msgbutt msg for mfsr     *
***********************************************************************/

#ifndef __MFIO2__
#define __MFIO2__

#include "swap.h"

/* Code in mfsr requires that MFDRAWIDEN is 3 chars + 1 or 2 digits */
#ifdef __GLUH__
#define MFDRAWIDEN "MFD8"     /* Identifier for old mfdraw startup */
#else
#define MFDRAWIDEN "MFD9"     /* Identifier for mfdraw startup msg */
#endif
#define MFDRAWNAME "mfdraw"   /* Name to load mfdraw */
#define MFDRAWPORT  10090     /* Default port for calling mfdraw */
#define LocalHostIP "127.0.0.1"

/* Bits for high-order of buffer count word */
#define MFB_ATOM   0x10000000 /* Marks buffers atomically continued */
#define MFB_INIT   0x20000000 /* Marks start-up buffer */
#define MFB_COMPLT 0x40000000 /* Marks last buffer for this frame */
#define MFB_CLOSE  0x80000000 /* Marks last buffer for this session */
/* The following bits are used to tell mfsr which outputs to write--
*  in serial code, that decision is made in mfwrite.  */
#define MFB_SKPXG  0x08       /* Skip XG output */
#define MFB_SKPMF  0x04       /* Skip metafile output */
#define MFB_OPMSK  0xfc       /* Mask to extract op code from cmd */
#define MFB_COUNT  0x03ffffff /* Mask to extract count from header */
#define MFDRAW_DBG 0xc0000000 /* Mfdraw's dbxwait mask */

/* Type codes for processor to processor messages */
#define MFSRSUM_MSG  0x3200   /* Startup mfsr */
#define MFSRACK_MSG  0x3201   /* mfsr startup ack */
#define MSDRACK_MSG  0x3202   /* mfsr ack for mfdraw startup */
#define METFILE_MSG  0x3203   /* Metafile I/O message type */
#define METFACK_MSG  0x3204   /* mfsr ack for METFILE_MSG */
#define METBUTT_MSG  0x3205   /* mfsr sending buttons to PAR0 */
#define MFSYNCH_MSG  0x3206   /* Synchronize processors */
#define MFBCBUT_MSG  0x3207   /* Broadcast button message */
#define MFRKGSI_MSG  0x3208   /* Broadcast RKGSdef structure */
#define MFNEWWN_MSG  0x3209   /* Broadcast new window number */

#define ERRBEG        210     /* 1st mfsr, plot err in errtab */
#define ERRTOT         28     /* Total mfsr, plot errors */

/* Error numbers and error bits processed by mfckerr(), designed to
*  allow multiple errors to be packaged in one word in messages from
*  MFSR to host.  These errors are moved to range 210-239 in new plot
*  library intermixed with errors from the base plot library.  Errors
*  EXEC, FORK, PIPE, OCMF, CCMF, WCMF, BUFS, BUFT, SACK are not used
*  in any current oldplot or mfsr code and have been removed.  Be sure
*  to define any new error message codes such that the bit rotate in
*  mfckerr() will work correctly.  */
#define ERR_MEMA    (1<<0)    /* Unable to allocate memory.     210 */
#define ERR_HOST    (1<<1)    /* Unable to identify MFDRAW host.211 */
#define ERR_HOME    (1<<2)    /* CREATE: Cannot find env HOME.  212 */
#define ERR_BUTT    (1<<3)    /* SYNCH: Error reading butt msg. 213 */
#define ERR_BUFB    (1<<4)    /* MFBCHK: MF buff too small.     214 */
#define ERR_RMFS    (1<<5)    /* Error reading ack from MFSR.   215 */
#define ERR_RHST    (1<<6)    /* MFSR: Err parsing startup msg. 216 */
#define ERR_MFDO    (1<<7)    /* Parm too long for startup msg. 217 */
#define ERR_TMMB    (1<<8)    /* More acks than frames          218 */
#define ERR_RCHD    (1<<9)    /* Err reading ack from mfdraw.   219 */
#define ERR_RBUT   (1<<10)    /* Xgl error returned by mdraw.   220 */
#define ERR_PROBE  (1<<11)    /* MFSYNCH: Select or Iprobe err  221 */
#define ERR_WRSK   (1<<12)    /* MFSR: Err writing to socket.   222 */
#define ERR_CRSK   (1<<13)    /* MFSR: Err creating socket.     223 */
#define ERR_OSMF   (1<<14)    /* Err opening metafile           224 */
#define ERR_CNSK   (1<<15)    /* MFSR: Err connecting socket.   225 */
#define ERR_CSMF   (1<<16)    /* Err closing metafile.          226 */
#define ERR_WSMF   (1<<17)    /* Err writing to metafile.       227 */
#define ERR_CLSK   (1<<18)    /* MFSR: Err closing socket.      228 */
#define ERR_MFMT   (1<<19)    /* Unrecognized MF instruction.   229 */
#define ERR_MACK   (1<<20)    /* Read/write mfsr ack msg error. 230 */
#define ERR_MPIR   (1<<21)    /* MFSR: MPI buffer read error.   231 */
#define ERR_2GMF   (1<<22)    /* Metafile exceeds 2GB.          232 */
#define ERR_DISP   (1<<23)    /* Cannot find or open DISPLAY.   233 */
#define ERR_FNM0   (1<<24)    /* File name was a null string.   234 */
#define ERR_BWIN   (1<<25)    /* Butt msg rcvd for bad win num  235 */
#define ERR_PWIN   (1<<26)    /* Bad window num newpltw,newwin  236 */
#define ERR_TMWN   (1<<27)    /* Tried to allocate > 2^16 Wins  237 */
/* 238-239 available for future MFSR messages (mfckerr mechanism) */

/* MFSR and startup messages in the range 260-289 were moved down to
*  the above 210-239 range in order to be reportable as iexit codes.
*  The old range 260-289 is now available for errors from the new
*  plot library reportable via abexit[m].  These are listed in the
*  src/crk/errtab file and do not have #define names assigned.  */

/* These structs combine information from the old mfsr ack and
*  msgbutt returned from mfdraw, with new names to avoid conflict
*  when this header is included in old programs.  */
struct mfdbm {          /* mfdraw button message */
   byte movie_mode;        /* This field formerly movie, see MM_xxx
                              *  in plotdefs.h for possible values.  */
   char next;              /* 1 for next frame */
   byte intxeq;            /* See BUT_xxx in plotdefs.h */
   char snapshot;          /* 1 for snapshot--obsolete */
   /* Following only used in new plot library */
   ui16 gfrm,gwin;         /* Graphics frame and window (big-endian) */
   };
struct ackbutt {        /* Combined ack and mfdraw buttons */
   char ackc[FMJSIZE];     /* 0 if OK, else error code (big-endian) */
   struct mfdbm mfdb;      /* See above */
   };

/* Startup message sent to mfsr (built in metafile buffer if serial).
*  These fields are now all ASCII by definition so no swapping.  */
#define MXSUML   999          /* Max length of a startup msg field */
#define MXSUBL 99999          /* Max length of buffer length field */
struct MFSRSUMsg {
   char m_doXG[2];            /* X graphics switch: '0' no XG,
                              *  '1' do XG, 'X' immediate exit */
   char m_doMF[2];            /* Metafile switch */
   char m_l_disp_name[4];     /* Length of display name */
   char m_l_meta_name[4];     /* Length of metafile name */
   char m_l_win_title[4];     /* Length of window title */
   char m_cmd_buff_len[6];    /* Length of command buffer */
   char m_meta_blk_len[6];    /* Length of metafile block */
   char m_movie_mode[2];      /* Movie mode */
   /* The next 10 chars were the never-used "ratio" field--
   *  now allocated for icon name width and version ID.  */
   char m_l_icon_name[4];     /* Length of icon name */
   char m_version[6];         /* Version identifier */
   char m_debug[9];           /* mfdraw debug info */
   char m_data[1];            /* One pad blank, then data */
   } ;

/* Startup message sent by mfcreate or mfsr to mfdraw.  This is
*  not documented anywhere I can find--this version taken from
*  mfdrawsockserver.cxx as of 08/03/16 to serve as standard for
*  senders.  Variable-length names follow last item in struct.  All
*  fields are ASCII strings.  Not all fields are currently used.
*
*  N.B.  Icon name specified in setmf() call was not honored by
*  smarx version of mfdraw.  Never-used "aspect_ratio" field has
*  been replaced by l_icon_name and version.  The added icon name
*  string is ignored by current mfdraw code but is ready to use
*  when mfdraw is rewritten for binary metafile spec.  The m_version
*  field is currently string version of vers, but may be changed
*  for some future purpose.  */
struct MFDSUMsg {
      char vers[4];              /* Version of interface (I4) */
      char l_disp_name[4];       /* Length of display name */
      char l_home_name[4];       /* Length of home directory name */
      char l_win_title[4];       /* Length of window title */
      char cmd_buff_len[6];      /* Length of command buffer */
      char i_movie_mode[2];      /* Movie mode */
      char l_icon_name[4];       /* Length of icon name */
      char m_version[6];         /* Version identifier (6H) */
      char z_debug[9];           /* Debug flags */
      char m_pad[1];
      } ;

/* Function prototypes */
#ifdef __GLUH__                  /* Old prototypes */
void mfcreate(char *fn, long blklen, float ratio);
void mfwrite(char *buff,long nbytes);
int  mfbchk(int len);
int  mfhchk(int len);
#define NEWBUFF   1              /* mf[bh]chk:  New buffer started */
#define OLDBUFF   0              /* mf[bh]chk:  Continuing current */
void mfst(int index, float xsz, float ysz, float xi,
         float yi, const char *chart);
#else                            /* New prototypes */
void mfcreate(void);
void mfflush(void);
void mfstart(const char *chart);
#endif                        /* End different prototype versions */
void mfckerr(ui32 error);
void mfclose(void);
void mfhead(void);
void mfstate(void);
int  mfsynch(int ksyn);
/* Bits for ksyn: */
#define KSYN_COMPLT  0x01     /* Send MFB_COMPLT */
#define KSYN_FLUSH   0x02     /* Flush buffer */
#define KSYN_SYNC    0x04     /* Perform isynch */
#define KSYN_PWAIT   0x08     /* Wait if frame pending */
#define KSYN_WAIT    0x10     /* Always wait */
#define KSYN_BCMB    0x20     /* Broadcast message buttons */
#define KSYN_CLOSE   0x40     /* Send MFB_CLOSE */

#endif /* __MFIO2__ */

