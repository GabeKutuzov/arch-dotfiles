/* (c) Copyright 1992-2016, The Rockefeller University *21115* */
/* $Id: aninit.c 6 2018-02-01 20:18:47Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               ANINIT                                 *
*                                                                      *
*  Routine to fill in NCINFO structure, which contains information     *
*  about cluster size and logical node number of current process.      *
*  Also responsible for starting up andmsg (to print debug messages)   *
*  and mfsr (to collate graphics command buffers) if requested.        *
*                                                                      *
*  This is for the MPI environment and is a replacement for ncinfo(),  *
*  renamed because of the addition of arguments.                       *
*                                                                      *
*  Arguments (replaced on PARn nodes with PAR0 values):                *
*     opts     Flags to control spawning of andmsg and/or mfsr procs.  *
*              ANI_DBMSG is always required if appexit() is used.      *
*     debug    Debug flags for the mpitools routines.                  *
*     mfdbg    Debug flags to be passed to mfsr and mfdraw.            *
*                                                                      *
*  Design Note:                                                        *
*     In order to do an MPI_Comm_spawn, all nodes must participate.    *
*     But in a typical application (CNS), the code to start mfsr is    *
*     initially only known on PAR0.  Therefore, aninit() first starts  *
*     up MPI communications, then broadcasts the three parameters,     *
*     then does the spawning.                                          *
************************************************************************
*  V2A, 12/18/92, ABP - First hybrid version, based on NCUBE.          *
*  New, 06/28/16, GNR - Total rewrite for MPI                          *
*  ==>, 10/03/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "rkarith.h"

#define NARGS 3            /* Number of arguments */

void aninit(ui32 opts, ui32 debug, ui32 mfdbg) {

   ui32 allargs[NARGS];

#if 0
   if (1) {    /*** EMERGENCY EARLY DEBUG ***/
      volatile int wct = 1;
      while (wct)    /* Wait for debugger to set wct = 0 */
         sleep(1);
      }
#endif

   MPI_Info mpin;     /*** TEST CODE ***/

   /* Protect against multiple calls */
   int initflag;
   MPI_Initialized(&initflag);
   if (initflag) return;

   MPI_Init(NULL, NULL);
   memset(&NC, 0, sizeof(struct NCINFO));
   NC.commc = MPI_COMM_WORLD;
   NC.procid = (int)getpid();
   if (MPI_Comm_size(NC.commc, &NC.total)) abexit(91);
   NC.headnode = 1;
   NC.tailnode = NC.total - 1;
   NC.cnodes = NC.tailnode - NC.headnode + 1;
   NC.dim = bitszs32(NC.cnodes);
   if (MPI_Comm_rank(NC.commc, &NC.node)) abexit(91);

   /* Broadcast call args to all nodes once communication is up */
   if (NC.total > 1) {
      allargs[0] = opts, allargs[1] = debug, allargs[2] = mfdbg;
      MPI_Bcast(allargs, NARGS, MPI_UNSIGNED, NC.hostid, NC.commc);
      opts = allargs[0], debug = allargs[1], mfdbg = allargs[2];
      }
   NC.debug = debug;

   /***TEST CODE*** See whether specifying host will
   *  solve runtime problem on pegasus  */
   if (opts & ANI_HOST) {
      MPI_Info_create(&mpin);
      }

   /* Set up process to receive debug messages if requested */
   if (opts & ANI_DBMSG) {
      char *dargv[3];
      int  rc,sperr;
      char argv1[UI32_SIZE];
      char argv2[UI32_SIZE];
      ssprintf(argv1, "%8d", NC.cnodes+1);
      ssprintf(argv2, "%8jd", debug);
      dargv[0] = argv1;
      dargv[1] = argv2;
      dargv[2] = NULL;
      if (opts & ANI_HOST) {
         MPI_Info_set(mpin, "host", "pegasus:22");
         rc = MPI_Comm_spawn(DMSGPGM, dargv, 1, mpin,
            NC.hostid, NC.commc, &NC.commd, &sperr);
         }
      else
         rc = MPI_Comm_spawn(DMSGPGM, dargv, 1, MPI_INFO_NULL,
            NC.hostid, NC.commc, &NC.commd, &sperr);
      if (NC.node == NC.hostid && debug & DBG_START) {
         fputs(ssprintf(NULL, "Spawn argv[0] = %s\n", dargv[0]),
            stderr);
         fputs(ssprintf(NULL, "Spawn argv[1] = %s\n", dargv[1]),
            stderr);
         fputs(ssprintf(NULL, "Spawn(andmsg) returned %d and "
            "child error %d\n", rc, sperr), stderr);
         fflush(stderr);
         }
      }

   /* Set up mfsr for parallel graphics if requested */
   if (opts & ANI_MFSR) {
      char *dargv[3];
      int  rc,sperr;
      char argv1[UI32_SIZE];
      char argv2[UI32_SIZE];
      ssprintf(argv1, "%8d", NC.cnodes+1);
      ssprintf(argv2, "%8jx", mfdbg);
      dargv[0] = argv1;
      dargv[1] = argv2;
      dargv[2] = NULL;
      if (opts & ANI_HOST) {
         MPI_Info_set(mpin, "host", "pegasus:23");
         rc = MPI_Comm_spawn(MFSRPGM, dargv, 1, mpin,
            NC.hostid, NC.commc, &NC.commmf, &sperr);
         }
      else
         rc = MPI_Comm_spawn(MFSRPGM, dargv, 1, MPI_INFO_NULL,
            NC.hostid, NC.commc, &NC.commmf, &sperr);
      if (NC.node == NC.hostid && debug & DBG_START) {
         fputs(ssprintf(NULL, "Spawn argv[0] = %s\n", dargv[0]),
            stderr);
         fputs(ssprintf(NULL, "Spawn argv[1] = %s\n", dargv[1]),
            stderr);
         fputs(ssprintf(NULL, "Spawn(mfsr) returned %d and "
            "child error %d\n", rc, sperr), stderr);
         fflush(stderr);
         }
      }

   /* Debug print:  show args received on all nodes */
   if (NC.debug & DBG_START) dbgprt(ssprintf(NULL, "Node %d got "
      "startup bcast, opts = %jd, debug = %jd, mfdbg = %jd\n",
      NC.node, opts, debug, mfdbg));

   /* Allow MPI functions to return errors to our code */
   MPI_Comm_set_errhandler(NC.commc, MPI_ERRORS_RETURN);

   /* Wait for debugger if requested */
   if (NC.debug & DBG_WTALL ||
      (NC.debug & DBG_DBGWT && NC.node == NC.debug >> 8)) {
      volatile int wct = 1;
      while (wct)    /* Wait for debugger to set wct = 0 */
         sleep(1);
      }

   return;

   } /* End aninit() */























