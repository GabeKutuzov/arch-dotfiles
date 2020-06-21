/* (c) Copyright 1998-2009, The Rockefeller University *11115* */
/* $Id: freepv.c 3 2017-01-16 19:29:37Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                               freepv                                 *
*                                                                      *
*  This routine is the freev() analog in a shared memory management    *
*  package for parallel computers.  It should not be called on comp    *
*  nodes or to free memory in the Static pool.  It should be used      *
*  on serial computers to free memory allocated with this package.     *
*                                                                      *
*  Synopsis:                                                           *
*     void freepv(void *ptr, char *emsg)                               *
*                                                                      *
*  Arguments:                                                          *
*     ptr      pointer to data object to be released.                  *
*     emsg     String to incorporate in error message that is written  *
*              if ptr is bad (max 48 chars).                           *
*                                                                      *
*  Return value:                                                       *
*     none     If an attempt is made to free memory that was not       *
*              allocated with the shared memory package, a message     *
*              incorporating the string 'emsg' is printed and the      *
*              calling program is terminated by a call to abexitm      *
*              with abexit code MP_FREEP_FAIL (=31).  The 'emsg'       *
*              string should indicate what was being freed.            *
*                                                                      *
************************************************************************
*  V1A, 06/27/98, GNR - Newly written                                  *
*  V1B, 03/04/00, GNR - Add HostString, use same calls for serial and  *
*                       parallel systems                               *
*  V1C, 03/23/00, GNR - No pmblk on comp nodes, add mbdptr, datptr,    *
*                       mbsize, negative mbtype for structured data    *
*  V1D, 06/10/00, GNR - Use a signal handler to detect bad pointers    *
*  V1F, 06/23/01, GNR - Handle deletion of pool change blocks          *
*  ==>, 10/04/16, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "sysdef.h"
#include "mpitools.h"
#include "memshare.h"
#include "mempools.h"
#include "rksubs.h"

#if 0   /*** DEBUG ***/
#ifdef UNIX
#include <setjmp.h>
#include <signal.h>
static jmp_buf MP_freesegvret;

/*=====================================================================*
*                           MP_freesegvhand                            *
*                                                                      *
*             Detect attempts to free from a bad pointer               *
e*=====================================================================*/

static void MP_freesegvhand(int sig) {

   if (sig != SIGSEGV) exit(sig);   /* JIC */
   longjmp(MP_freesegvret, TRUE);
   }
#endif
#endif /*** DEBUG ***/

/*=====================================================================*
*                               freepv                                 *
*=====================================================================*/

void freepv(void *ptr, char *emsg) {

#ifdef PARn

   abexit(MP_COMP_ALLOC);     /* Don't call on comp nodes */

#else

   struct mblkdef *pa;        /* Ptr to allocated area */
   struct mbdlist *pl;        /* Ptr to list entry for this area */
   size_t       ldata;        /* Length of data */
#ifdef PAR
   memtype       kmem;        /* Memory pool containing block */
#endif

/* Verify that ptr points to a valid block and extract kmem */

   pa = mbdptr(ptr);
#ifdef UNIX
#if 0   /*** DEBUG ***/
   /* Under UNIX, enable interception of bad pointer errors */
   void (*oldsegvhand)(int) = signal(SIGSEGV, MP_freesegvhand);
   if (setjmp(MP_freesegvret) || (pa->info.magic & MPMMask) != MPMAGIC)
      abexitm(MP_FREEP_FAIL, ssprintf(NULL, "Attempted to free "
         "unowned memory at %p for %48s.", ptr,
         emsg ? emsg : "(unspecifed)"));
   signal(SIGSEGV, oldsegvhand);
#endif  /*** DEBUG ***/
#else
   if ((pa->info.magic & MPMMask) != MPMAGIC)
      abexitm(MP_FREEP_FAIL, ssprintf(NULL, "Attempted to free "
         "unowned memory at %p for %48s.", ptr,
         emsg ? emsg : "(unspecifed)"));
#endif
#ifdef PAR
   kmem = (memtype)(pa->info.magic & PoolMask);
#endif

/* Unlink the block from whichever queue it is in */

   pl = allounlk(pa);

/* Determine block size and decrement global pool size */

   ldata = mbsize(pa);
   pl->lallmblks -= (LMBDUP + ALIGN_UP(ldata));

#ifdef PAR
/*---------------------------------------------------------------------*
*            In a parallel computer, there are four cases:             *
*---------------------------------------------------------------------*/

   switch (pa->info.magic & CodeMask) {

/* (1) If block was on the host base list, just free it.  If it was
*      on another base list, use realloc to release the data portion,
*      leaving the mblkdef.  Link the mblkdef into the mods list as
*      an MPM_DEL block so comp nodes will get a message to free the
*      original block.  Note that an MPM_CHP block can never occur
*      on the Host list, but the test here is harmless.  */

   case 0:
      if (kmem == Host)
         free(pa);
      else {
         pa = (struct mblkdef *)
            reallocv((char *)pa, LMBDUP, "Free mod record");
         allodel0(pa);
         }
      break;

/* (2) If block was already on the mods list as a realloc, find the
*      preceding MPM_LOC block and unlink it.  If on the host, just
*      delete it.  Otherwise, change it into an MPM_DEL block and
*      relink it so comp nodes will get a message to release the
*      original area.  Then free the MPM_CHG block.  (The block
*      is unnecessarily unlinked and relinked if (not Host and
*      not MPM_CHP) to keep the code shorter.  */

   case MPM_CHG:
      {  struct mblkdef *pp = allogetp(pa);
         if (kmem == Host)
            free(pp);
         else
            allodel0(pp);
         } /* End MPM_CHG local scope */
      /* Drop through to next case ... */

/* (3) If block was an MPM_ADD or MPM_CHG block, just free it.  */

   case MPM_ADD:
      free(pa);
      break;

/* (4) If block was anything else, something is wrong.  */

   default:
      abexit(MP_FREEP_FAIL);     /* Just in case... */

      } /* End switch */

#else
/*---------------------------------------------------------------------*
*             On a serial computer, just free the memory               *
*---------------------------------------------------------------------*/

   free(pa);

#endif   /* !PAR */
#endif   /* !PARn */

   } /* End freepv() */
