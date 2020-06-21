/* (c) Copyright 1991, Neurosciences Research Foundation, Inc. */
/* $Id: findln.c 3 2008-03-11 19:50:00Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                               findln                                 *
*                                                                      *
*  Find a repertoire and layer name in the repertoire tree             *
*                                                                      *
*  Call by: findln(rlnm *rlname)                                       *
*  Where:   'rlname' is a pointer to the internal representation       *
*              of the repertoire and layer names of the cell block     *
*              to which a pointer is desired.                          *
*                                                                      *
*           The value returned is a pointer to the specified           *
*              cell block, or NULL if the cell block is not found.     *
*                                                                      *
*  Method: A simple sequential search is used.  The function is        *
*     not used often enough to justify a hash table.                   *
*                                                                      *
************************************************************************
*  V1A, 12/04/85, G. N. Reeke                                          *
*  V2A, 07/29/88, ??? - Translated to 'C' from version V1A             *
*  V8A, 09/22/98, GNR - Change argument to rlnm pointer type           *
*  ==>, 02/17/05, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "d3global.h"

struct CELLTYPE *findln(rlnm *rlname) {
   struct REPBLOCK *jr;
   struct CELLTYPE *jl;

/* Search for repertoire */

   for (jr=RP->tree;
      jr&&strncmp(rlname->rnm, jr->sname, LSNAME);
      jr=jr->pnrp) ;
   if (!jr) /* Repertoire not found */ return NULL;

/* Search for cell type */

   for (jl=jr->play1; jl; jl=jl->play)
      if (!strncmp(rlname->lnm, jl->lname, LSNAME)) return(jl);

/* Cell type not found */

   return NULL;
   } /* End findln() */
