/* (c) Copyright 2007-2018, The Rockefeller University *11115* */
/* $Id: bbdsinit.c 30 2018-08-15 19:46:51Z  $ */
/***********************************************************************
*                      BBD (Server Side) Package                       *
*                             bbdsinit.c                               *
*                                                                      *
*----------------------------------------------------------------------*
*                              bbdsinit                                *
*         Initialize data structures for a BBD network server          *
*                                                                      *
*  This routine must be called from a BBD server (currently CNS)       *
*  before using any of the bbdsreg[estv] routines.  It does any        *
*  necessary initialization of global variables and registers          *
*  bbdsquit() to run at shutdown time.                                 *
*                                                                      *
*  Synopsis:                                                           *
*  void bbdsinit(void)                                                 *
*----------------------------------------------------------------------*
*                              bbdsfhnm                                *
*                                                                      *
*  This routine checks whether a BBD device client host has already    *
*  been registered.  If so, it returns the address of the BBDsHost     *
*  structure for that host.  If not, it creates that structure and     *
*  returns its address.  This way, any collection of devices can       *
*  occur on any collection of different hosts, shared or not shared.   *
*                                                                      *
*  Synopsis:                                                           *
*  struct BBDsHost *bbdsfhnm(char *phnm)                               *
*                                                                      *
*  Arguments:                                                          *
*     phnm        Ptr to a string containing the name of a BBD device  *
*                 host (suitable for looking up with a gethostbyname() *
*                 call, followed optionally by a colon and the number  *
*                 of the port to be used (default: BBDPortData).       *
************************************************************************
*  V1A, 02/09/07, GNR - New program                                    *
*  ==>, 12/21/07, GNR - Last mod before committing to svn repository   *
*  Rev, 01/15/11, GNR - Move bbdsquit to a separate source file        *
*  R30, 08/04/18, GNR - Remove SUNOS support                           *
***********************************************************************/

#define BBDS

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sysdef.h"
#include "bbd.h"
#include "rksubs.h"
#include "rfdef.h"

struct BBDComData BBDsd;            /* Common BBD data struct */
extern void bbdsquit(void);

/*=====================================================================*
*                              bbdsinit                                *
*=====================================================================*/

void bbdsinit(void) {

/* Initialize linked lists for the six registration types */

   BBDsd.ppfeff = &BBDsd.pfeff;
   BBDsd.ppfsns = &BBDsd.pfsns;
   BBDsd.ppfttv = &BBDsd.pfttv;
   BBDsd.ppfetv = &BBDsd.pfetv;
   BBDsd.ppfstv = &BBDsd.pfstv;
   BBDsd.ppfval = &BBDsd.pfval;
   atexit(bbdsquit);

   } /* End bbdsinit() */


/*=====================================================================*
*                              bbdsfhnm                                *
*=====================================================================*/

struct BBDsHost *bbdsfhnm(char *phnm) {

   struct BBDsHost *ph,**pph;       /* Ptrs to host list */
   char   *pcol;                    /* Ptr to port digits */
   int    hport;                    /* Host port */
   int    lname;                    /* Length of just the name */

/* If name is NULL, use the default.  Point to the first host entry,
*  which should have been filled in from the BBDHOST control card.
*  If that did not happen, it is a terminal error.  */

   if (phnm == NULL) {
      ph = BBDsd.phlist;
      if (!ph) abexitm(137, "BBD Host info not received");
      goto GotBBDsHostEntry;
      }

/* Parse the phnm argument into host name and port.  Code
*  the decimal-to-binary explicitly because this code does
*  not know exactly what environment it is working in.  */

   if (pcol = strchr(phnm, ':')) {  /* Assignment intended */
      lname = pcol - phnm;
      hport = 0;
      while (*(++pcol)) {
         int val = pcol[0] - '0';
         if ((unsigned)val > 9) abexitm(BBDsErrBadPort,
            "A non-digit character in a port number");
         hport *= 10, hport += val;
         }
      }
   else {
      lname = strlen(phnm);
      hport = BBDPortInit+1;        /* Set default port */
      }

/* We expect only a few hosts (usually just one),
*  so a linear search is used.  */

   for (pph=&BBDsd.phlist; ph=*pph; pph=&ph->pnhost) {
      if ((hport != ph->bbdport) ||
         ((lname > 0) ^ (ph->pbbdnm != NULL))) continue;
      if ((lname == 0) || (strncmp(phnm, ph->pbbdnm, lname) == 0))
         goto GotBBDsHostEntry;
      }

   /* Not found, make a new entry on the list */
   ph = *pph = (struct BBDsHost *)callocv(1, sizeof(struct BBDsHost),
      "DDB device host list");

   /* Copy the name string (if there is one), not just the pointer,
   *  because the caller may pass a temporary name string.  */
   if (lname) {
      ph->pbbdnm = mallocv(lname+1, "BBD device host name");
      memcpy(ph->pbbdnm, phnm, lname);
      ph->pbbdnm[lname] = '\0';
      }

   /* Copy the port number (or zero) */
   ph->bbdport = hport;

GotBBDsHostEntry:
   return ph;

   } /* End bbdsfhnm() */

