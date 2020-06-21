/* (c) Copyright 2001 Neurosciences Research Foundation, Inc. */
/* $Id: d3uprg.c 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                              d3uprg.c                                *
*                                                                      *
*  Function d3uprg is used to open a user library file and load a      *
*  function from that file into memory for use during CNS execution.   *
*                                                                      *
*  Function d3upcl is used to close any user libraries that remain     *
*  open at the end of CNS processing (do not close until there are     *
*  no more calls to the loaded functions!).                            *
*                                                                      *
*  Multiple user libraries may be declared on the Group 0 UPROGLIB     *
*  control card.  The library where a given function is to be found    *
*  can be declared in the UPROG control card option that loads that    *
*  function.  When a UPROGLIB file is opened, it stays open until      *
*  another one is opened or until d3upcl() is called.  rfxxxx()        *
*  routines are not used--variables in RFdef are used to keep          *
*  track of the file name and whether it is open.                      *
************************************************************************
*  V8B, 10/07/01, GNR - New CNS functionality                          *
*  Rev, 01/04/80, GNR - Deal with absence of RTLD_NOW on SUNOS         *
*  ==>, 10/25/07, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#if defined(UNIX) && !defined(SUNOS)
#include <dlfcn.h>
#endif
#include "d3global.h"
#include "d3fdef.h"
#include "rocks.h"
#include "rkxtra.h"

/*=====================================================================*
*                              d3uprg()                                *
*                                                                      *
*     Load a user-provided function to be called from within CNS       *
*                                                                      *
*  Synopsis:  void *d3uprg(int kpar)                                   *
*                                                                      *
*  Arguments:                                                          *
*     kpar        TRUE if an error should be generated if this call    *
*                 is made on a parallel computer (i.e. the function    *
*                 would be called from a comp node).                   *
*                                                                      *
*  Assumptions:                                                        *
*     The caller is processing a control card option of the form       *
*     'UPROG=(usage, fnname, libno)' and has already called cdscan()   *
*     and processed the 'usage' keyword to determine the purpose of    *
*     the user function.  The 'fnname' and 'libno' parameters are      *
*     processed by d3uprg().  'libno' defaults to 1 if not entered.    *
*                                                                      *
*  Returns:                                                            *
*     A pointer to the user-supplied function.  It must be cast to     *
*     the appropriate type by the caller.  If 'fnname' is "OFF",       *
*     a NULL pointer is returned.                                      *
*                                                                      *
*  Errors:                                                             *
*     (1) A UPROGLIB entry has not been provided for the given         *
*     'libno' parameter.  (2) The specified library cannot be          *
*     opened.  (3) The specified function is not in the library.       *
*     (4) 'kpar' is TRUE and the program is running on a parallel      *
*     computer.  (5) The two required input fields are not in          *
*     parentheses.  (6) 'libno' is not followed by a right paren.      *
*                                                                      *
*=====================================================================*/

void *d3uprg(int kpar) {

#if defined(UNIX) && !defined(SUNOS)
   struct RFdef *pfd;      /* Ptr to RFdef struct */
   char *pdle;             /* Ptr to dl library error msg */
   void *puprog;           /* Ptr to requested user program */
   int hfnnm;              /* Text locator for function name */
   int libno = 1;          /* Library number */
   int iln;                /* Libno counter */

#ifdef PAR
   if (kpar) d3exit(UPROG_ERR, NULL, 0);
#endif

/* Read in the two positional parameters.  Because of paren
*  level checking, actually it is necessary to push back the
*  first argument (scanned by the caller), then scan it again
*  and throw it away, using iln as a temporary dump.  */

   scanagn();
   inform("(SV(2*T" qLUPRGNM ",NV>IV))", &iln, &hfnnm, &libno, NULL);

/* Check for "OFF" option */

   if (ssmatch(getrktxt(hfnnm), "OFF", 3))
      return NULL;

/* Locate the relevant UPROGLIB file */

   pfd = d3file[UPROGLIB];
   for (iln=1; pfd && iln<libno; pfd=pfd->nf,++iln) ;
   if (!pfd) {
      convrt("(P1,'0***FEWER THAN ',J1I8,'UPROGLIB FILES"
         " WERE SPECIFIED.')", &libno, NULL);
      RK.iexit |= 1;
      return NULL;
      }

/* If the file is not already open, open it and store the
*  "handle" returned by dlopen in bptr in the RFdef block.  */

   dlerror();                 /* Clear any previous errors */
   if (!(pfd->iamopen & IAM_ANYOPEN)) {
#ifdef SUNOS
      pfd->bptr = (char *)dlopen(pfd->fname, RTLD_LAZY);
#else
      pfd->bptr = (char *)dlopen(pfd->fname, RTLD_NOW);
#endif
      if (pdle = dlerror()) { /* Assignment intended */
         cryout(RK_E1, "0***FAILED TO OPEN ", RK_LN2,
            pdle, RK_CCL, NULL);
         return NULL;
         }
      pfd->iamopen |= IAM_DLOPEN;
      }

/* Find the requested symbol */

   puprog = dlsym(pfd->bptr, getrktxt(hfnnm));
   if (pdle = dlerror()) {    /* Assignment intended */
      cryout(RK_E1, "0***FUNCTION \"", RK_LN2, getrktxt(hfnnm), RK_CCL,
         "\" NOT FOUND IN ", RK_CCL, pdle, RK_CCL, NULL);
      return NULL;
      }

   return puprog;
#else
   d3exit(UPROG_ERR, NULL, 0);
#endif
   } /* End d3uprg() */

/*=====================================================================*
*                              d3upcl()                                *
*                                                                      *
*    Close any UPROGLIB that has been left open by a previous call     *
*                            to d3uprg().                              *
*=====================================================================*/

void d3upcl(void) {

#if defined(UNIX) && !defined(SUNOS)
   struct RFdef *pfd;      /* Ptr to RFdef struct */

   for (pfd = d3file[UPROGLIB]; pfd; pfd=pfd->nf)
      if (pfd->iamopen & IAM_ANYOPEN) {
         dlclose(pfd->bptr);
         pfd->iamopen = NO;
         }
#endif

   } /* End d3upcl() */
