/* (c) Copyright 1993, Neurosciences Research Foundation, Inc. */
/* $Id: findcell.c 3 2008-03-11 19:50:00Z  $ */
/*---------------------------------------------------------------------*
*                             findcell()                               *
*                                                                      *
*  Purpose:  Locate node where state variables for a particular cell   *
*              are stored.                                             *
*  Synopsis: long findcell(struct CELLTYPE *il, long cell)             *
*  Returns:  Node where data for cell 'cell' are stored.  Terminates   *
*              execution if requested cell does not exist.             *
*                                                                      *
*  Notes:    All calculations of node numbers should now be done in    *
*              this function.  By putting the variables 'node1' and    *
*              'nodes' in the CELLTYPE, we are now prepared to add a   *
*              load optimizer that allocates cell types to nodes in    *
*              ways other than the traditional distribution over all   *
*              nodes.                                                  *
*                                                                      *
*  This function is not used in the serial version of CNS              *
*                                                                      *
*  V6C, 08/14/93, GNR - Initial version                                *
*  ==>, 09/04/06, GNR - Last mod before committing to svn repository   *
*---------------------------------------------------------------------*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sysdef.h"
#include "d3global.h"

long findcell(struct CELLTYPE* il, long cell) {

   /* This check is not really needed--all cell numbers occurring
   *  in CNS should be checked at input time for validity.  May be
   *  removed for a slight performance gain if desired...  */
   if (cell < 0 || cell >= il->nelt)
      d3exit(BADCELL_ERR, fmturlnm(il), cell);

   /* Cells are divided as equally as possible among il->nodes nodes
   *  beginning at node il->node1.  There are il->cpn cells per node.
   *  If the total number of cells, il->nelt, is not a multiple of
   *  il->nodes, then equal division is not possible, and one extra
   *  cell is placed on each of the first il->crn nodes.  */

   if (cell < il->cut) return (cell/(il->cpn + 1) + il->node1);
   else                return ((cell - il->crn)/il->cpn + il->node1);
   } /* End findcell */

