/* (c) Copyright 1992-2016, The Rockefeller University *11115* */
/* $Id: mfcreate.c 5 2008-03-13 20:32:23Z  $ */
/***********************************************************************
*                     ROCKS metafile plot library                      *
*                             mfcreate()                               *
*                                                                      *
* SYNOPSIS:                                                            *
*     void mfcreate(void)                                              *
*                                                                      *
* DESCRIPTION:                                                         *
*     This function is executed during the first call to newplt() or   *
*     newwin() to start up the metafile plot library.  It completes    *
*     initialization of the common data structure, _RKG, after setmf() *
*     has had a chance to change defaults such as lci, lcf, or buffer  *
*     length. Then it initializes the connection to mfsr (if parallel) *
*     or mfdraw (if serial).  The output metafile, if requested, is    *
*     opened here if serial, but by mfsr if parallel.  mfcreate() is   *
*     called only from node 0 and and only once during the program.    *
*                                                                      *
*     mfcreate() sets up buffering for the default window and frame.   *
*     If additional windows are opened, new buffers must be allocated  *
*     for them.                                                        *
*                                                                      *
* DESIGN NOTES:                                                        *
*     Parallel versions must communicate with mfsr when either X or    *
*     META graphics is required, but serial versions communicate with  *
*     mfdraw only for X graphics. mfsr is started up by aninit() along *
*     with node-to-node communication.  In principle, mfsr could run   *
*     on a separate host, but at present it is a spawned process on    *
*     PAR0.  On the other hand, mfdraw is started from here, because   *
*     if we are in the serial version, there is no aninit()...  Note   *
*     that the startup reply from mfsr is a full error word, which may *
*     need to be swapped.  The reply from mfdraw is a single -1 byte   *
*     if OK.                                                           *
*                                                                      *
*     This code currently has only been designed for UNIX or UNIX-like *
*     systems (UNIX defined in sysdef.h)--may work on others if        *
*     suitable equivalent libraries are available.                     *
*                                                                      *
* RETURN VALUES:                                                       *
*     None.  Terminates with mfckerr() (parallel) or abexitm()         *
*     (serial) on any kind of error.                                   *
************************************************************************
*  Version 1, 04/03/92, ROZ (Some obsolete revision notes removed)     *
*  Rev, 08/01/93, GNR - Secure against excessively long arg strings    *
*  Rev, 12/06/93,  LC - Added XG and MF flags to mfsr message          *
*  Rev, 03/05/94, GNR - Fix NOXG bug waiting for mfdraw ack, cosmetics *
*  Rev, 03/23/94,  LC - Add MFPLOT_SAV env. var. to mfdraw argv        *
*  Rev, 05/29/94, GNR - Use nsitools swap support                      *
*  Rev, 06/11/97, GNR - Use socket interface to mfdraw, abolish fixed- *
*                       length buffers for display info, entire mfsr   *
*                       startup msg is now ASCII, some fields widened, *
*                       remove last printf/sprintf calls--use ROCKS    *
*  Rev, 08/06/97, GNR - Add wtitl, start mfdraw via inetd              *
*  Rev, 01/23/99, GNR - Support icons, metafile on stdout, remove all  *
*                       args, incl. incorrect aspect ratio             *
*  Rev, 05/23/99, GNR - Use new swapping scheme                        *
*  Rev, 11/17/08, GNR - Add call to ignore SIGPIPE to get err msgs     *
*  Rev, 10/12/11, GNR - Change ibcdwt() calls to wbcdwt()              *
*  Rev, 03/24/13, GNR - Add pound-def __USE_GNU for RedHat 6 compile   *
*  V2A, 04/04/15, GNR - Changes for V2A library (binary metafile)      *
*  Rev, 07/20/16, GNR - Modify for MPI environment, rev mfsr start msg *
*  ==>, 07/21/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include "sysdef.h"
#ifdef UNIX
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define __USE_GNU
#include <netdb.h>
#include <signal.h>
#else
#error File mfcreate.c requires code update for non-UNIX sockets
#endif

#ifdef PARn
#error File mfcreate.c should not be compiled for PARn nodes
#else

#include "mfint.h"
#include "mfio.h"
#ifdef PAR
#include "mpitools.h"
#include "swap.h"
#else
#include "rfdef.h"
#endif

/* Length for temp display name -- not worth a malloc() --
*  long enough to hold max LocalHostIP + :disp.disp  */
#define LTDISP 20

void mfcreate(void) {

   char *c;                   /* Ptr for extending message */
   size_t ldn;                /* Length of display name */
   size_t lfn;                /* Length of metafile name */
   size_t lwt;                /* Length of window title */
   size_t lin;                /* Length of icon name */
   size_t nbytes;             /* Length of startup msg */
#ifdef PAR                 /* Items needed for mfsr */
   struct MFSRSUMsg *pstm;    /* Ptr to mfsr startup message */
   char  ebuf[FMJSIZE];       /* Message buffer for error */
   AN_Status rstat;           /* Msg recv status */
   ui32  error;
   int    rc;
   int    src, type;
   char   errm[FMJSIZE];      /* Buffer for receiving error codes */
#else                      /* Items needed for mfdraw */
   struct MFDSUMsg *pdsm;     /* Ptr to mfdraw startup msg */
   char  *home;               /* mfdraw home directory name */
   char  *tmp;                /* Temp for mfdraw host */
#ifdef UNIX
   struct sockaddr_in client;
   struct hostent *hp, *gethostbyname();
#endif
   int   lhn;                 /* Length of home directory name */
#endif

   /* Ignore pipe signals as discussed above */
#ifdef UNIX
   signal(SIGPIPE, SIG_IGN);
#endif

/*---------------------------------------------------------------------*
*  If neither kind of output is needed, exit now.  Otherwise,          *
*  prepare items that are common to both startup messages.             *
*---------------------------------------------------------------------*/

   if (!(_RKG.s.MFMode & (SGX|METF))) return;

   /* If no display is specified, find the default display in the
   *  environment, and if not there, use the local host.  If there is
   *  a value, it was set in setmf() via mallocv() and is free'd in
   *  mfclose.  So if we change it, we must reallocv() it.  */
   {  char *dip,*dscr;        /* Display IP, display screen */
      int qcolon = 0;         /* Got a colon from setmf or DISPLAY */
      int qdset;              /* Got a display IP from setmf */
      dip = _RKG.MFDdisplay, /* To elim warning */ dscr = NULL;
      qdset = dip && dip[0];
      if (qdset) {
         if (dip[0] == ':') qcolon = strlen(dscr = dip);
         else goto GotDisplay; }
      dip = getenv("DISPLAY");
      if (dip) {
         /* Any :screen in MFDdisplay overrides one in env DISPLAY */
         char *qdcol = strchr(dip, ':');
         if (qcolon) { if (qdcol) *qdcol = '\0'; }
         else if (qdcol == dip) {
            qcolon = strlen(dscr = dip);
            dip = LocalHostIP;
            }
         }
      else
         dip = LocalHostIP;
      /* In all these cases, reallocv will be needed */
      ldn = strlen(dip) + qcolon + 1;
      _RKG.MFDdisplay = reallocv(_RKG.MFDdisplay, ldn, "X Display");
      strcpy(_RKG.MFDdisplay, dip);
      if (qcolon) strcat(_RKG.MFDdisplay, dscr);
      }  /* End display checking local scope */

   /* Determine lengths of display name, icon name, window title */
GotDisplay:
   ldn = _RKG.MFDdisplay ? strnlen(_RKG.MFDdisplay,MXSUML) : 0;
   lwt = _RKG.MFDwtitl ? strnlen(_RKG.MFDwtitl,MXSUML) : 0;
   lin = _RKG.MFDwicon ? strnlen(_RKG.MFDwicon,MXSUML) : 0;

#ifdef PAR

/*---------------------------------------------------------------------*
*  Parallel:  Generate message that will be sent to mfsr               *
*---------------------------------------------------------------------*/

   /* Determine length of filename, up to LFILNM max */
   lfn = _RKG.MFfn ? strnlen(_RKG.MFfn, LFILNM-1) : 0;

   /* Generate an error if any of these parameters would overflow
   *  into padding between mfsr startup message fields.  */
   if (ldn > MXSUML || lfn > MXSUML || lwt > MXSUML ||
         lin > MXSUML || _RKG.s.MFBuffLen >= MXSUBL)
      mfckerr(ERR_MFDO);

   nbytes = sizeof(struct MFSRSUMsg) + ldn + lfn + lin + lwt;

   /* Insert flag for generating X graphics */
   pstm->m_doXG[0] = ' ';
   pstm->m_doXG[1] = (_RKG.s.MFMode & SGX) ? '1' : '0';
   /* Insert flag for writing metafile to disk  */
   pstm->m_doMF[0] = ' ';
   pstm->m_doMF[1] = (_RKG.s.MFMode & METF) ? '1' : '0';
   /* Insert length of display name */
   wbcdwt(&ldn, pstm->m_l_disp_name, RKDI(4));
   /* Insert length of metafile name */
   wbcdwt(&lfn, pstm->m_l_meta_name, RKDI(4));
   /* Insert length of window title */
   wbcdwt(&lwt, pstm->m_l_win_title, RKDI(4));
   /* Insert length of command buffer */
   wbcdwt(&_RKG.s.MFBuffLen, pstm->m_cmd_buff_len, RKD32(6));
   /* For now, just copy this to the m_meta_blk_len field, which
   *  theoretically could be different for an IBM BLKSIZE spec.  */
   memcpy(pstm->m_meta_blk_len, pstm->m_cmd_buff_len,
      sizeof(pstm->m_meta_blk_len));
   /* Insert movie mode */
   wbcdwt(&_RKG.movie_mode, pstm->m_movie_mode, RKDB(2));
   /* Insert length of icon name */
   wbcdwt(&lin, pstm->m_l_icon_name, RKDI(4));
   /* Insert version identifier */
   strncpy(pstm->m_version, MFDRAWIDEN, sizeof(pstm->m_version));
   /* Insert debug mask */
   wbcdwt(&_RKG.s.dbgmask, pstm->m_debug, RK_HEXF+RK_NI32+7);
   /* Insert one-byte pad */
   pstm->m_data[0] = ' ';

   /* Append the variable-length data fields */
   c = pstm->m_data + 1;
   if (ldn > 0) { memcpy(c, _RKG.MFDdisplay, ldn); c += ldn; }
   if (lfn > 0) { memcpy(c, _RKG.MFfn, lfn);       c += lfn; }
   if (lwt > 0) { memcpy(c, _RKG.MFDwtitl, lwt);   c += lwt; }
   if (lin > 0) { memcpy(c, _RKG.MFDwicon, lin);   c += lin; }

   /* Write MFSRSUMsg to mfsr.  Wait for ack.  All messages are
   *  now pure ASCII strings, no need for any swapping.  */
   rc = MPI_Send(pstm, c - (char *)pstm,  MPI_UNSIGNED_CHAR,
      NC.mfsrid, MFSRSUM_MSG, NC.commmf);
   if (rc) appexit("mfsr startup msg", 284, rc);

   rc = MPI_Recv(ebuf, FMJSIZE, MPI_UNSIGNED_CHAR, NC.mfsrid,
      MFSRACK_MSG, NC.commmf, &rstat);
   if (rc) appexit("mfsr startup ack", 284, rstat.MPI_ERROR);
   error = bemtoi4(ebuf);
   mfckerr(error);

   /* If doing XG, there is an extra ack for mfdraw startup */
   if (_RKG.s.MFMode & SGX) {
      rc = MPI_Recv(ebuf, FMJSIZE, MPI_UNSIGNED_CHAR, NC.mfsrid,
         MSDRACK_MSG, NC.commmf, &rstat);
      if (rc) appexit("mfdraw startup ack", 284, rstat.MPI_ERROR);
      error = bemtoi4(ebuf);
      mfckerr(error);
      }

   freev(pstm, "mfsr startup msg");

#else

/*---------------------------------------------------------------------*
*                Serial:  Start mfdraw and/or metafile                 *
*                                                                      *
*  On a serial machine, if X graphics are desired (SGX flag) an        *
*  MFDSUMsg mfdraw startup message is sent.  xinetd will start mfdraw  *
*  when a connection is opened.  If a metafile is desired (METF flag), *
*  the file is opened directly from here.  mfsr is not used.  This     *
*  code currently has only been designed for UNIX-like systems--may    *
*  work on others if suitable equivalent libraries are available.      *
*                                                                      *
*  Note:  We are now preparing a MFDSUMsg separately from an MFSRSUMsg *
*  to allow separate future changes.  Apparently V1 code placed home   *
*  directory name in meta_name field of MFSRSUMsg on serial computers. *
*  This was not used by mfdraw (which might even be running on a       *
*  different computer).  Here we use the MFDRAW_HOME environment       *
*  variable, which is intended for this purpose.                       *
*---------------------------------------------------------------------*/

   if (_RKG.s.MFMode & SGX) {

#ifdef UNIX

/* Create a socket and try to contact mfdraw.  If display name was not
*  specified, look for MFDRAW_HOST in env--if not found, try host we
*  are currently running on.  N.B.  mfdraw is only interested in the
*  screen spec part of the disp name--no need to update the host name
*  part if that is loaded from env or LocalHostIP here.  */

      _RKG.MFDsock = socket(AF_INET, SOCK_STREAM, 0);
      if (_RKG.MFDsock == -1)
         abexitme(223, "Unable to create socket to mfdraw");

      client.sin_family = AF_INET;
      client.sin_port = htons(MFDRAWPORT);

      if (ldn > 0) {
         /* Temporarily zap out the ':' screen identifier, get
         *  host ip, then restore ':'  */
         char *pcolon = strchr(_RKG.MFDdisplay, ':');
         if (pcolon) *pcolon = '\0';
         hp = gethostbyname(_RKG.MFDdisplay);
         if (pcolon) *pcolon = ':';
         }
      else {
         if (!(tmp = getenv("MFDRAW_HOST"))) tmp = LocalHostIP;
         hp = gethostbyname(tmp);
         }
      if (!hp)
         abexitm(211, "Unable to identify mfdraw host");
      memcpy((char *)&client.sin_addr, (char *)hp->h_addr,
         hp->h_length);
      if (connect(_RKG.MFDsock, (struct sockaddr *)&client,
            sizeof(client)) == -1)
         abexitme(225, "Unable to connect to mfdraw");
#endif /* UNIX */


/* Generate the mfdraw startup message and send it */

      /* If MFDRAW_HOME specified, use it as home directory */
      home = getenv("MFDRAW_HOME");
      if (!home) {
         /* Otherwise, support old-style test of MFPLOT_SAV,
         *  which is valid if mfdraw host == mfcreate host */
         if ((tmp = getenv("MFPLOT_SAV")) != NULL) {
            if (!strcasecmp(tmp, "ON")) {
               if (!(home = getenv("HOME")))
                  abexitm(212, "Can't find HOME in environment");
               }
            }
         }
      lhn = home ? strlen(home) : 0;

      /* Generate an error if any of the parameters would overflow
      *  into padding between mfdraw startup message fields.  */
      if (ldn > MXSUML || lhn > MXSUML || lwt > MXSUML ||
            lin > MXSUML || _RKG.s.MFBuffLen >= MXSUBL)
         abexitm(217, "Graphics parameter overflow");

      nbytes = sizeof(struct MFDSUMsg) + ldn + lhn + lwt + lin;
      pdsm = (struct MFDSUMsg *)mallocv(nbytes, "mfdraw startup msg");

      /* Start with identifying header */
      strncpy(pdsm->vers, MFDRAWIDEN+3, 4);
      /* Insert length of display name */
      wbcdwt(&ldn, pdsm->l_disp_name, RKDI(4));
      /* Insert length of home directory name */
      wbcdwt(&lhn, pdsm->l_home_name, RKDI(4));
      /* Insert length of window title */
      wbcdwt(&lwt, pdsm->l_win_title, RKDI(4));
      /* Insert length of command buffer */
      wbcdwt(&_RKG.s.MFBuffLen, pdsm->cmd_buff_len, RKD32(6));
      /* Insert movie mode */
      wbcdwt(&_RKG.movie_mode, pdsm->i_movie_mode, RKDB(2));
      /* Insert length of icon name */
      wbcdwt(&lin, pdsm->l_icon_name, RKDI(4));
      /* Insert version identifier */
      strncpy(pdsm->m_version, MFDRAWIDEN, sizeof(pdsm->m_version));
      /* Insert debug mask */
      wbcdwt(&_RKG.s.dbgmask, pdsm->z_debug, RK_HEXF+RK_NI32+8);
      pdsm->m_pad[0] = ' ';

      /* Append the variable-length data fields */
      c = pdsm->m_pad + 1;
      if (ldn > 0) { memcpy(c, _RKG.MFDdisplay, ldn); c += ldn; }
      if (lhn > 0) { memcpy(c, home, lhn);            c += lhn; }
      if (lwt > 0) { memcpy(c, _RKG.MFDwtitl, lwt);   c += lwt; }
      if (lin > 0) { memcpy(c, _RKG.MFDwicon, lin);   c += lin; }

      if (write(_RKG.MFDsock, pdsm, nbytes) != nbytes) {
         _RKG.MFFlags |= MFF_MFDrawX;
         abexitme(222, "Error writing to mfdraw socket"); }

      /* Wait for mfdraw to ack successful startup */
      if (read(_RKG.MFDsock, pdsm->m_pad, 1) != 1) {
         _RKG.MFFlags |= MFF_MFDrawX;
         abexitme(219, "Error reading mfdraw socket"); }
      if ((schr)pdsm->m_pad[0] != -1)
         abexitm(219, "Bad ack from mfdraw startup");

      freev(pdsm, "mfdraw startup msg");
      } /* End setting up mfdraw */

/* Open ordinary file for metafile output */

   if (_RKG.s.MFMode & METF) {
      _RKG.MFfdes = rfallo(_RKG.MFfn, WRITE, BINARY, SEQUENTIAL, TOP,
         LOOKAHEAD, REWIND, RELEASE_BUFF, _RKG.s.MFBuffLen,
         _RKG.s.MFBuffLen, IGNORE, NO_ABORT);
      if (!_RKG.MFfdes) abexitm(210, "Error allocating metafile");
      if (!rfopen(_RKG.MFfdes, IGNORE, WRITE, BINARY, SEQUENTIAL, TOP,
            LOOKAHEAD, REWIND, RELEASE_BUFF, _RKG.s.MFBuffLen,
            _RKG.s.MFBuffLen, IGNORE, NO_ABORT))
         abexitm(224, "Error opening metafile");
      }
#endif /* not PAR */

   } /* End of mfcreate() */

#endif /* Not PARn */
