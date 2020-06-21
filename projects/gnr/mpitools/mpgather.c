/* (c) Copyright 2017, The Rockefeller University *11115* */
/* $Id: mpgather.c 8 2018-08-15 20:01:33Z  $ */
/***********************************************************************
*                          MPI Tools Library                           *
*                             mpgather()                               *
*                                                                      *
*  This is a routine for a parallel computer which implements what is  *
*  commonly known as a 'gather' operation, that is, portions of a      *
*  large array computed in parallel on different nodes are gathered    *
*  together on a host node (always PAR0 here).  The algorithm used     *
*  performs a number of data transfers proportional to the log2 of     *
*  the number of nodes.  This is NOT an 'all-gather'.                  *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*  Synopsis:                                                           *
*  void mpgather(char *pbuf, void *pdat, int myoff, int ldat,          *
*     int tblks, int nblks, int type)                                  *
*                                                                      *
*  Arguments:                                                          *
*  pbuf     On the host node, this is a pointer to where the final     *
*           gathered data will end up.  On other nodes, it points      *
*           to a work area that is large enough to hold, worst case,   *
*           all the data.  This work area can be used between calls    *
*           for other purposes.                                        *
*  pdat     Pointer to the contribution of the current node to the     *
*           global data being gathered.  These data are required to    *
*           be consecutive on consecutive nodes, that is, when re-     *
*           ceived at the end of the data on the next lower-numbered   *
*           partner node, a single larger block is created.  May be    *
*           NULL if nblks == 0.                                        *
*  myoff    COMP NODES:  Offset of data from this node in the final    *
*           array in blocks of length lblk.  Space can be reserved     *
*           for existing data on the host node by suitable choice of   *
*           the myoff parameters (if host has data not in pbuf, they   *
*           will be copied there (do not overlap pbuf and pdat)).      *
*  myoff    HOST NODE:  Offset of data received on the host node is    *
*           always understood to be 0.  Instead, myoff >= lblk is      *
*           taken as the stride to be introduced between data blocks.  *
*           This may be useful to leave space between data blocks to   *
*           place linked-list pointers or other data.                  *
*  lblk     Length of one block on this node (adjusted up to multiple  *
*           of the required memory alignment for these blocks).        *
*  tblks    Total number of blocks in the gathering.                   *
*  nblks    Number of blocks contributed by this node.                 *
*  type     Message type code for this operation.                      *
*                                                                      *
*  Prototype is in collect.h                                           *
*                                                                      *
*  N.B.  All internal working of this program treats data as unaligned *
*  bytes.  Caller should assure that pbuf, pdat, and lblk are multi-   *
*  ples of any required data alignment.                                *
*                                                                      *
*  Errors:                                                             *
*  All errors are non-recoverable and result in abexit() termination.  *
*  Error codes in the range 580-589 are assigned to this package.      *
*                                                                      *
*----------------------------------------------------------------------*
*  Design considerations were:                                         *
*  -- No node should receive from more than one other node in each     *
*     cycle, to avoid serialization bottlenecks.                       *
*  -- It is assumed that transfer latency is more important than byte  *
*     transfer time and memory is plentiful on modern machines, hence  *
*     the transfers are not divided into smaller blocks to reduce the  *
*     requirement for a large work area.                               *
*  -- MPI structured data types are used to send content description   *
*     info just before the data in each transfer, keeping those data   *
*     out of the collection work area.                                 *
*  -- Data are transferred in units of blocks.  Originally, this was   *
*     part of an idea to allow data to be inserted blockwise into      *
*     larger structures on the host, e.g. to associate the data with   *
*     pointers for sorting, but now it is just to aid in maintaining   *
*     alignment and to facilitate any future development where blocks  *
*     might be useful.                                                 *
*  -- Offset and size parms are ints, consistent with MPI call args.   *
*  -- This code makes the historical assumption that message byte      *
*     order and alignment of items are same on all comp processor      *
*     nodes.  If this ever changes, code must be rewritten to use      *
*     lemfm/bemfm routines or MPI library functions with similar       *
*     semantics.                                                       *
************************************************************************
*  V1A, 01/27/17, GNR - New routine                                    *
*  V1B, 01/31/17, GNR - Add ability to space gathered elements         *
*  ==>, 01/30/17, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "mpitools.h"
#include "collect.h"

/*=====================================================================*
*                              mpgather                                *
*=====================================================================*/

#define NMPIBlks  3                 /* Number blocks in MPI struct */
enum blknms { Hdr, Blk, Pkt };

void mpgather(char *pbuf, void *pdat, int myoff, int lblk,
   int tblks, int nblks, int type) {

   int chan;                        /* Channel for log collection */
   int ncnodes = NC.cnodes + 1;     /* Number nodes in gather */
   int nrcvb = 0;                   /* Number of received blocks */
   int rc;                          /* Send/receive return code */
   int setoffOK = !(myoff|nblks);   /* TRUE if OK to use pard's myoff */
   int stride = lblk;               /* Host node stride if myoff != 0 */

   /* Information for packing message info with data:
   *  GHdr on sending nodes will describe contents being sent,
   *       on receiving nodes will be value received from sender.  */
   struct GHdr_t {                  /* Contents of an MPHdr */
      int GHdato;                   /* Offset of sent block */
      int GHnblks;                  /* Number of blocks sent */
      } GHdr;

   MPI_Datatype MPBlk,MPPkt;
   /* Element counts, displacements, data types for MPPkt struct */
   int GHect[NMPIBlks] = { 1, 0, 0 };
   MPI_Datatype GHtyp[NMPIBlks];
   MPI_Aint GHoff[NMPIBlks];

   MPI_Type_contiguous(2, MPI_UNSIGNED, &GHtyp[0]);
   MPI_Type_commit(&GHtyp[0]);
   MPI_Type_contiguous(lblk, MPI_UNSIGNED_CHAR, &MPBlk);
   MPI_Type_commit(&MPBlk);

   /* Put node 0 data in result without messaging.  Pretend these
   *  were received in an earlier cycle.  */
   if (NC.node == NC.hostid) {
      if (myoff != 0) stride = myoff, myoff = 0;
      if (nblks > 0) {
         if (pdat != pbuf) memcpy(pbuf, pdat, nblks*lblk);
         tblks -= nrcvb = nblks; nblks = 0;
         }
      setoffOK = FALSE;
      }

   /* Location of GHdr remains constant through all channel cycles */
   MPI_Get_address(&GHdr, GHoff);

   /* Perform data combination by a hypercube-like algorithm */
   for (chan=1; chan<ncnodes; chan<<=1) {
      int pard = NC.node ^ chan;    /* Partner relative node number */

      if (NC.node & chan) {
         /* Nodes with odd numbers in the current channel send to
         *  their next lower partner, then drop out.  */
         int impb = Blk;            /* Index into MPPkt components */
         if (nblks > 0) {           /* Any of my own data? */
            GHect[impb] = nblks;
            MPI_Get_address(pdat, GHoff+impb);
            GHtyp[impb] = MPBlk;
            ++impb;
            }
         if (nrcvb > 0) {           /* Any received data? */
            GHect[impb] = nrcvb;
            MPI_Get_address(pbuf, GHoff+impb);
            GHtyp[impb] = MPBlk;
            ++impb;
            }
         /* Build header describing data to be sent */
         GHdr.GHdato = myoff;
         GHdr.GHnblks = nblks /* mine */ + nrcvb /* received */ ;
         MPI_Type_create_struct(impb, GHect, GHoff, GHtyp, &MPPkt);
         MPI_Type_commit(&MPPkt);
         rc = MPI_Send(MPI_BOTTOM, 1, MPPkt, pard, type, NC.commc);
         if (rc) appexit("MPGATHER: MPI_Send error", 48, rc);
         break;
         }

      else if (pard < ncnodes) {
         MPI_Status rstat;
         /* Nodes with even numbers in the current channel read a
         *  message from the partner node (unless the partner is above
         *  ncnodes and therefore does not exist) into the pbuf work-
         *  space.  This area will end up containing all the data
         *  except the contribution from this node, which is not
         *  moved.  On the host node, received data are placed
         *  directly into the caller data area, whose address is
         *  given in the pbuf argument.  */
         MPI_Get_address(pbuf + nrcvb * stride, GHoff+Blk);
         if (stride > lblk) {
            GHect[Blk] = 1;
            MPI_Type_create_hvector(tblks, 1, stride, MPBlk,
               GHtyp+Blk);
            }
         else {
            GHect[Blk] = tblks;           /* Max blocks */
            GHtyp[Blk] = MPBlk;
            }
         MPI_Type_create_struct(2, GHect, GHoff, GHtyp, &MPPkt);
         MPI_Type_commit(&MPPkt);
         rc = MPI_Recv(MPI_BOTTOM, 1, MPPkt, pard, type, NC.commc,
            &rstat);
         if (rc) appexit("MPGATHER: MPI_Recv error", 49, rc);
         /* We would like to check the received packet against the
         *  expected data, but there is no way to predict the number
         *  of blocks to be expected because that depends on the
         *  arrangement of caller's data on other nodes.  So all we
         *  can do is check that the offset immediately follows the
         *  data we already have in our pbuf buffer.  Furthermore,
         *  it is annoying for caller to be required to provide myoff
         *  when nblks == 0, so we omit the test in that case.  */
         /*** Side note:  In this situation, MPI_Get_elements appears
         *  by testing to return the total number of bytes received in
         *  both Hdr and Blk structures.  The documentation is not
         *  clear on this, but anyway there is no present way to test
         *  this information.  ***/
         if (GHdr.GHnblks > 0) {
            /* If this node originally had no data, and no myoff was
            *  given in the cell, its myoff can be updated based on
            *  data received from a higher node that has data.  */
            if (setoffOK) myoff = GHdr.GHdato, setoffOK = FALSE;
            if (GHdr.GHdato != myoff + nblks + nrcvb)
               appexit("MPGATHER: Receive data offset mismatch",
                  MPG_RDOMME, GHdr.GHdato);
            }
         /* Update for next cycle */
         nrcvb += GHdr.GHnblks;
         tblks -= GHdr.GHnblks;
         } /* End else on a read channel */
      } /* End of loop over channels */

   } /* End of mpgather() */
