/*=====================================================================*
*                              mdkssrtx                                *
*                                                                      *
*  Sort a list of data points in increasing order along some dimension *
*  using a radix sort (like an old card sorter) with time proportional *
*  to n*sizeof(Xdat).                                                  *
*=====================================================================*/

static void vfnname(mdkssrt,VL)(Wkhd *phd, int jx, int jk) {

   Link *ptl,**ppl;        /* Ptrs to data links */
   Bin  *pb0,*pbe;         /* Ptrs to sorting bin info start,end */
   int  i,ii,isz;          /* Indexes for loops over sort digits */
   int  qle = phd->qle;    /* 1 Little-endian, 0 Big-endian */
   /* Values for flipping values so negatives sort correctly */
   unsigned int pflip = 0, nflip = (1 << NBPB) - 1;

   isz = sizeof(Xdat)-1;
   pb0 = phd->pBin0;
   pbe = pb0 + NBINS;

   /* Loop over bytes in the radix sort */
   for (ii=0; ii<=isz; ++ii) {
      Bin *pb;                /* Ptr to a sorting bin */
      i = qle ? ii : isz - ii;
      if (ii == isz) pflip = 1 << (NBPB-1);

      /* Initialize sorting bins for this key digit */
      for (pb=pb0; pb<pbe; ++pb) {
         pb->head = NULL;     /* Empty bin signal */
         pb->tail = (Link *)&pb->head;
         }

      /* Traverse linked list, extracting current key digit and
      *  assigning each record to top of appropriate bin.  */
      for (ptl=phd->pLnk1[jx]; ptl; ptl=ptl->pnxt) {
         Xdat const *pdk = ptl->pdat + jk;
         unsigned int ibin = (unsigned int)((byte const *)pdk)[i];
         ibin ^= (*pdk < 0) ? nflip : pflip;
         pb = pb0 + ibin;
         pb->tail->pnxt = ptl;
         pb->tail = ptl;
         } /* End loop over data records */

      /* End of pass, stack up the bins in order */
      ppl = &phd->pLnk1[jx];
      for (pb=pb0; pb<pbe; ++pb) if (pb->head) {
         *ppl = pb->head;
         ppl = &pb->tail->pnxt; }
      *ppl = NULL;            /* Terminate linked list */
      } /* End loop over bytes in Xk coords */

   return;
   } /* End mdkssrtx() */

