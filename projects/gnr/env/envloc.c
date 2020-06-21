/* (c) Copyright 1989-2010, The Rockefeller University *21115* */
/* $Id: envloc.c 23 2017-01-16 19:28:30Z  $ */
/***********************************************************************
*              Darwin III Environment Simulation Package               *
*                               ENVLOC                                 *
*                                                                      *
*  This function searches the library and dynamic shape list (if any)  *
*  for a required shape ID or shape number and initializes a "handle"  *
*  that envlod() can use to find the corresponding pixel data.         *
*                                                                      *
*  Synopsis: int envloc(struct ENVDEF *Eptr, char *id, char ityp,      *
*              struct LIBHANDLE *handle)                               *
*                                                                      *
*  Input:      Eptr  = ptr to current environment block.               *
*              id    = Shape id name to be located or NULL to indicate *
*                          that 'ityp' is a library shape number.      *
*              ityp  = if 'id' is not NULL, indicates scope of search: *
*                          SEARCH_LIB to search for library object     *
*                          only (SELECT), SEARCH_ALL to search for     *
*                          library or dynamic object (CREATE).         *
*                      if 'id' is NULL, gives number of library object *
*                          to be located.  (This function is trivial,  *
*                          but is included in interest of abstracting  *
*                          the LIBHANDLE type.)                        *
*              handle= ptr to LIBHANDLE struct where result is placed. *
*                                                                      *
*  Output:  If successful, returns zero and fills in 'handle' struct.  *
*           Otherwise, returns NOT_FOUND.                              *
************************************************************************
*  V4A, 01/08/89, JWT - Translated into C                              *
*  Rev, 08/09/92, GNR - Introduce LIBHANDLE for portability            *                                                                 *
*  ==>, 02/22/03, GNR - Last mod before committing to svn repository   *
*  V8H, 12/01/10, GNR - Separate env.h,envglob.h w/ECOLMAXCH change    *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "envglob.h"

int envloc(struct ENVDEF *Eptr, char *id, int ityp,
   LIBHANDLE *handle) {

   struct DSLBDEF  *Shptr;    /* Dynamic shape list traversal ptr */
   struct LIBINDEF *libptr;   /* Ptr to library index array */
   int lower, upper;          /* Upper lower bounds of LIBID search  */
   int midnum;                /* Library index record being examined */
   int cmp;                   /* strncmp() return value used for
                              *  directing boundary search */

/* If 'id' is NULL, build handle directly from 'ityp' */

   if (!id) {
      handle->pmemsh = NULL;
      handle->libsh = ityp;
      return ENV_NORMAL;
      }

/* Search user-defined shapes first if ityp != 0 */

   if (ityp == SEARCH_ALL) {
      for (Shptr=Eptr->pexshp; Shptr; Shptr=Shptr->pnxshp)
         if (!strncmp(id,Shptr->nshp,SHP_NAME_SIZE)) {
            handle->pmemsh = Shptr;
            handle->libsh = 0;
            return ENV_NORMAL;
            } /* End string comparison */
      } /* End user-defined shape search */

/* Not a user-defined shape... do binary search the LIBID array.
*  Note: if found, return the item number rather than the record
*        number because SELECT items need item numbers for
*        cycling (because of variable num of records per item) */

   libptr = Eptr->plibid;
   lower = 0;
   upper = Eptr->libnum - 1;
   while (lower <= upper) {
      midnum = (lower+upper)>>1;
      cmp = strncmp(id,libptr[midnum].shape_name,SHP_NAME_SIZE);
      if (!cmp) {
         handle->pmemsh = NULL;
         handle->libsh = midnum;
         return ENV_NORMAL;
         }
      /* Search above current item */
      else if (cmp > 0) lower = midnum + 1;
      /* Search below current item */
      else upper = midnum - 1;
      }

/* Error return (item not found) */

   return NOT_FOUND;
   } /* End envloc() */


