
/* (c) Copyright 1988 by George N. Reeke, Jr.  *** CONFIDENTIAL ***   */
/* For use only in testing performance of computers on a simplified   */
/* version of a Darwin III simulation.  Please do not distribute      */
/* to any third parties without permission.                           */
/*--------------------------------------------------------------------*/
/*                                                                    */
/*    Darwin III "Benchmark" Program                                  */
/*                                                                    */
/*    This program is designed to be a "miniature" version of         */
/*    Darwin III, incorporating the most time-consuming inner-        */
/*    loop calculations.  It should be a suitable basis for           */
/*    developing specialized versions for various parallel-           */
/*    architecture machines.                                          */
/*                                                                    */
/*    V1A, 05/05/88, G. N. Reeke                                      */
/*    V1B, 05/26/88, GNR - Fix bug allocating CONNDATA structures     */
/*                                                                    */
/*--------------------------------------------------------------------*/

/* Note that integer arithmetic will be used for all calculations,
   as in the current program.  This will be a realistic test for the
   kind of modelling we expect to do.  In the comments that follow,
   parenthesized expressions of the form '(Sn)' will be used to
   indicate that a quantity is understood to have n fraction bits.
   Long int will be assumed 32 bits; short int and int, 16 bits       */

/* The program will read one setup file, whose name must be given
   as a command-line argument.  The file contains:

   Line 1: NCELLTYPES, NITERATIONS, SEED
   where:  NCELLTYPES = Number of types of cells to be simulated
           NITERATIONS = Number of times to iterate entire model
           SEED = Random number generating seed

   For each cell type, the following line is read, followed
   immediately by NCONNTYPES lines of connection information:
           NCELLS, NCONNTYPES, DELTA
   where:  NCELLS = Number of cells of this type (NCELLS < 65536)
           NCONNTYPES = Number of connection types for these cells
           DELTA = Amplification parameter (zero to disable amp)

   For each of NCONNTYPE connection types, a line of the following
   format is read:
           NCONNS, SOURCE, RULE, SCALE
   where:  NCONNS = Number of connections of this type
           SOURCE = Integer between 1 and NCELLTYPES, giving the
                        type of source cells for these connections
           RULE = 0 for completely random connections
                  1 for sequential ("partitioned") connections
           SCALE = scale factor for these connections
                                                                      */

/* Include standard library functions */

   #include "stdio.h"
   #include "stdlib.h"
   #include "time.h"

/* Define parameters that aren't worth varying in input file
      PT = positive firing threshold for a cell
      MTI = modification threshold on output of a cell
      MTJ = modification threshold on input to a connection
      ET = excitatory threshold for an input to a connection          */

   #define  PT  0.0
   #define MTI  0.0
   #define MTJ  0.0
   #define  ET  0.0

/* Define structure types used in the simulation
   Note: In the real simulation, all of the structures contain
   many more variables.                                               */

   typedef unsigned char byte;

   struct CONNDATA {          /* Data for a single connection */
      short int lij;          /* Number of source cell */
      byte cij;               /* Strength of connection (S7) */
      };

   struct CONNBLK {           /* Connection-type information block */
      struct CONNBLK * pct;   /* Pointer to next CONNBLK */
      struct CELLBLK * psrc;  /* Pointer to source cell type */
      double chunksize;       /* Range of a generated connection */
      long int nconns;        /* Number of connections */
      int source;             /* Serial number of source cell type */
      int rule;               /* Connection generating rule */
      long int scale;         /* Scale factor (S8) */
      long int das[4];        /* Detailed amp stats */
      long int ax;            /* Intermediate afferent sum (S15) */
      };

   struct CELLBLK {           /* Cell-type information block */
      struct CELLBLK * play;  /* Pointer to next CELLBLK */
      struct CONNBLK * pct1;  /* Pointer to first CONNBLK */
      struct CONNDATA * prd;  /* Pointer to actual connection data */
      long int ncells;        /* Number of cells of this type */
      long int locell;        /* NCUBE Low cell of this type on this node */
      long int mcells;        /* NCUBE # of cells of this type on this node */
      long int nconntypes;    /* Number of connection types */
      long int nconntotal;    /* Total number of connections */
      long int delta;         /* Amplification factor (S10) */
      byte * psi;             /* Pointer to s(i) data */
      byte * ps2;             /* Pointer to alternate (new) s(i) */
/* From here to the end of the struct definition are variables added for
 * use in doing crude graphical output on the NCUBE. */
      long ngx;              /* Number of cells to plot along X */
      long ngy;              /* Number of cells to plot along Y */
      long xoffset;          /* Left boundary for this CELLBLK */
      long yoffset;          /* Lower boundary for this CELLBLK */
      long x_size;           /* Length in pixels along X of plotting area 
                                 for this CELLBLK. */
      long y_size;           /* Length in pixels along Y of plotting area
                                 for this CELLBLK. */
      long c_size;           /* Length in pixels of square that will
                                 be plotted for each cell in the 
                                 CELLBLK. */   
      };

/*--------------------------------------------------------------------*/
/*                                                                    */
/*                         MAIN PROCEDURE                             */
/*                                                                    */
/*--------------------------------------------------------------------*/

main(int argc,char *argv[])
{

/* Declarations: */

   int ncelltypes;            /* Number of cell types in simulation */
   int niterations;           /* Number of iterations to perform */
   unsigned long seed;        /* Random number seed */
   long int pt;               /* Working output threshold */
   int mti,mtj,et;            /* Working thresholds (S8) */
   float tdelta,tscale;       /* Temporary delta, scale */
   int trule;                 /* Temporary copy of 'rule' */
   int ji,jl,js,jx,jc;        /* Iter, cell, conn type & data counters */
   struct CELLBLK *play1;     /* Pointer to first cell block */
   struct CELLBLK *il,*ll;    /* Pointer to current, last CELLBLK */
   struct CONNBLK *ix,*lx;    /* Pointer to current, last CONNBLK */
   struct CONNDATA *lc;       /* Pointer to current connection data */
   register struct CONNDATA *ic;   /* Pointer to current connection data */
   byte *is,*is2,*psj,*psg;   /* Pointers to state variables */
   int wksj,sjmet;            /* NCUBE s(j) (S8); s(j) - et (S8) */
   register int wkcij;        /* NCUBE cij (S7) */
   int simmti,sjmmtj;         /* s(i) - mti (S8); s(j) - mtj (S8) */
   register int ampcase;      /* NCUBE Amplification case for "DAS" */
   long int wksum;            /* Working sum for s(i) */
   unsigned long skips;       /* Number of random numbers to skip */ 
   time_t time1,time2,time3;  /* Starting, setup, ending time */
   FILE *pif;                 /* Input file pointer */
   long jgx,jgy,gx,gy,ys,dy;  /* For graphics */
 
   setbuf(stdout, (char *)malloc(BUFSIZ)); nglobal(); /* NCUBE buffer stdout */

   ginit();          /* Initialize graphic device */
   ginit_();         /* Load graphics nodes on graphics boards */

   ldcolor(0,255,255,255);
   clrnode(0);      /* Clears graphics node memory to value specified.
                        In global mode this will only be done by node 0 */
   gcmd_();          /* This is for the clrnode above and may not be needed. */

/* Note the beginning time */ 
   time(&time1);

/*   bkpt(); */

/* Open the setup file and read the first line */

   if (argc < 2) {
      printf("Setup file name required\n"); exit(1);}
   if ((pif = fopen(argv[1],"r")) == NULL) {
      printf("Cannot open setup file\n"); exit(2);}
   fscanf(pif,"%d%d%d",&ncelltypes,&niterations,&seed);
   printf("Number of celltypes = %d, number of iterations = %d, seed = %d\n",
      ncelltypes,niterations,seed);
   srand(seed);

/* Read the cell type information and create linked list of cell blocks */

   play1 = NULL; ll = NULL;
   for (jl=1;jl<=ncelltypes;jl++) {
      if ((il=(struct CELLBLK *)malloc(sizeof(struct CELLBLK)))==NULL) {
         printf("No mem for CELLBLK\n"); exit(3);}
      if (ll==NULL) play1 = il; else ll->play = il;
      ll = il;
/* Note: ngx and ngy are read in instead of ncells. Additional variables
 * relating to graphics are also read in. */ 
      fscanf(pif,"%ld%ld%ld%f%d%d%d%d%d",&il->ngx,&il->ngy,&il->nconntypes,
       &tdelta,&il->xoffset,&il->yoffset,&il->x_size,&il->y_size,&il->c_size);
      il->ncells = il->ngx * il->ngy;   /* Calculate ncells */
      if (il->c_size == 0) il->c_size = il->x_size/il->ngx;
      printf("   Cell type %d\n",jl);
      printf("      Ncells = %ld, nconntypes = %ld, delta = %f\n",
         il->ncells,il->nconntypes,tdelta);

/* NCUBE Partition cells among nodes */

      maxconntypes = max(maxconntypes, il->nconntypes);
      whoami(&node, &nproc, &nhost, &ncube); total = 1<<ncube;
      il->locell = (int)((double)node * (double)il->ncells /
         (double)(total) + 0.00001);
      il->mcells = (int)((double)(node+1) * (double)il->ncells /
         (double)(total) + 0.00001) - il->locell;

/* NCUBE end */

      il->nconntotal = 0;
      il->delta = (long int)(1024.0*tdelta);

/* Read the connection type information and create connection
   blocks in a linked list chained to the controlling CELLBLK.
   This is done prior to allocating cell data to keep linked-
   list memory better localized */

      lx = NULL;
      for (jx=1;jx<=il->nconntypes;jx++)  {
         if ((ix=(struct CONNBLK *)malloc(sizeof(struct CONNBLK)))==NULL) {
            printf("No mem for CONNBLK\n"); exit(4);}
         if (lx==NULL) il->pct1 = ix; else lx->pct = ix;
         lx = ix;
         fscanf(pif,"%ld%d%d%f",&ix->nconns,&ix->source,&ix->rule,&tscale);
         printf("      Connection type %d\n",jx);
         printf("         Nconns = %ld, source = %d, rule = %d, scale = %f\n",
            ix->nconns,ix->source,ix->rule,tscale);
         ix->scale = (long int)(256.0*tscale);
         il->nconntotal += ix->nconns;
         } /* End reading CONNBLK input lines for this CELLBLK */
      lx->pct = NULL; /* Terminate the connection linked list */

      } /* End reading CELLBLK input lines */
   ll->play = NULL; /* Terminate the cell linked list */
   flush(); /* NCUBE Flush stdout */

/* Allocate space for two vectors of s(i) values (s(i) == state of
   cell i).  psi points to current values, ps2 points to newly cal-
   culated values.  The psi states are given random initial values.   */

   for (il=play1;il!=NULL;il=il->play) {
      if ((il->psi=(byte *)malloc((unsigned int)il->ncells))==NULL) {
         printf("No mem for s(i)\n"); exit(5);}
      if(il->mcells) { /* NCUBE Skip if mcells zero */
      if ((il->ps2=(byte *)malloc((unsigned int)il->mcells))==NULL) {/* NCUBE */
         printf("No mem for s(i)\n"); exit(5);}
      } /* NCUBE */
      for (is=il->psi,js=1;js<=il->ncells;js++)
         *is++ = (byte)rand();}

/* Complete the connection blocks:
      1) Allocate space for the connection data
      2) Locate the relevant source data and save pointers
      3) Fill in random starting values for cij and lij.              */

   skips = 0;  /* Initialize random number skipping */
   for (il=play1;il!=NULL;il=il->play) {

      /* Allocate space for the actual connection data */

      if(il->mcells) { /* NCUBE Skip if mcells zero */
      if ((il->prd=(struct CONNDATA *)malloc((unsigned int)
         il->mcells*il->nconntotal*sizeof(struct CONNDATA)))==NULL) {/* NCUBE */
         printf("No mem for cij,lij\n"); exit(6);}
      } /* NCUBE */
      ic = il->prd;

      /* Locate the appropriate cell blocks and figure chunksize */

      for (ix=il->pct1;ix!=NULL;ix=ix->pct) {

         /* Locate the source cell type block */
         for (jl=1,ll=play1;jl<ix->source;jl++,ll=ll->play) ;
         ix->psrc = ll;

         /* Constant for uniform random connection rule */
         ix->chunksize = (double)ll->ncells/((double)RAND_MAX+1.0);
         /* Constant for partitioned connection rule */
         if (ix->rule) ix->chunksize /= (double)ix->nconns;
         } /* End connection block initialization */

      /* Generate and fill in the actual connection data */

      skips += il->locell*il->nconntotal*2;
      randskip(skips);  /*It is OK for skips to be 0 */
       
      for (js=1+il->locell;js<=il->locell+il->mcells;js++) { /* NCUBE */
         for (ix=il->pct1;ix!=NULL;ix=ix->pct) { /* Connection types */
            trule = ix->rule;
            for (jc=0;jc<ix->nconns;jc++,ic++) {
               /* Note that the eight bits of cij will be interpreted
                  later as a sign and 7 fraction bits */
               ic->cij = (byte) rand();
               /* Note that the following calculation of lij is not
                  really adequate with a 15-bit random number gene-
                  rator -- try to do better on individual machines.
                  Also, we are forced to use an intermediate double-
                  float variable, since 'C' does not support access
                  to the high-order 32 bits of a 32 x 32 bit product. */
               ic->lij = trule ?
                  /* Partitioned (pseudo-linear) connection rule */
                  (short)(ix->chunksize*((double)jc*(double)RAND_MAX +
                     (double)rand())) :
                  /* Uniformly distributed random connection rule */
                  (short)(ix->chunksize*(double)rand());
               } /* End loop over connections */
            } /* End loop over connection types */
         } /* End loop over cells */
      skips = (il->ncells-(il->locell+il->mcells))*2*il->nconntotal;
      } /* End loop over cell types */

/* Make fixed-point working thresholds */

   pt  = (long int) 8388608.0*PT; /* PT with 23 fraction bits */
   mti = (int) 256.0*MTI;
   mtj = (int) 256.0*MTJ;
   et  = (int) 256.0*ET;

/* NCUBE Allocate buffers for summing 'das' statistics */

   if((dassum = (long *)malloc(maxconntypes*sizeof(ix->das))) == NULL) {
      printf("No mem for dassum\n"); exit(6);}
   if((loary = (long *)malloc((total+1)*sizeof(*loary))) == NULL) {
      printf("No mem for loary\n"); exit(6);}

/* Note the end-of-setup time */

   syncx(); /* NCUBE */
   time(&time2);
   printf("Setup required %f seconds\n",difftime(time2,time1));
   flush(); /* NCUBE */

   d3colors();

/*--------------------------------------------------------------------*/
/*                                                                    */
/*                  PERFORM THE ACTUAL SIMULATION                     */
/*                                                                    */
/*--------------------------------------------------------------------*/

/* Perform 'niterations' iterations.  Note that in general, it is not
   possible to parallelize by having different processors work on
   different iterations because the state of the environment will
   depend on outputs generated by the automaton in the previous step. */

   for (ji=1;ji<=niterations;ji++) {
       frmnum(ji);
       dmansi_(1);

#if 0
   flush(); nlocal(); /* NCUBE */
/* DEBUG DEBUG DEBUG */
      printf("State at iteration %d\n",ji);
      for (jl=1,il=play1;il!=NULL;il=il->play,jl++) {
         ic = il->prd; lc = ic;
         for (js=0,is=il->psi+il->locell;js<il->mcells;js++) { /* NCUBE */
            printf("   s(%d,%d) = %6.3f\n",jl,js+il->locell /* NCUBE */
            ,(float)(*is++)/256.0); /* NCUBE */
            for (jx=1,ix=il->pct1;ix!=NULL;ix=ix->pct,jx++) {
               printf("     l(%d): ",jx);
               for (jc=0;jc<ix->nconns;jc++,lc++)
                  printf("%7d",lc->lij);
               printf("\n");
               printf("     c(%d): ",jx);
               for (jc=0;jc<ix->nconns;jc++,ic++) {
                  wkcij = ic->cij >= 128 ? ic->cij - 256 : ic->cij;
                  printf("%7.3f",(float)wkcij/128.0);}
               printf("\n");}}}
/* ENDDEBUG ENDDEBUG ENDDEBUG */
      flush(); /* NCUBE */
#endif

/* Evaluate responses for all defined cell types.  In general, one
   will want to allow different processors to work on different cell-
   types.  This is complicated by the fact that different celltypes
   have different numbers of connections, so the processors will not
   all finish together.  In present models, the order of evaluation
   of celltypes is deterministic and the resulting states are used
   for evaluation of celltypes defined later.  This restriction may
   be relaxed in the future.  In any event, cells can be made to
   appear to be updated simultaneously by keeping multiple state
   vectors and switching pointers at appropriate times, as is done
   in the present program.  In the real Darwin III program, there
   is an 'UPWITH' option to specify that a group of celltypes are
   to be updated simultaneously.                                      */

/* In systems with relatively few processors, it will probably be
   sufficient to process cell types sequentially, exercising parallelism
   at the level of cells of the same type.  In the connection machine,
   there will typically be fewer cells of one type than there are
   processors.  Designs to consider are (1) to process the entire
   network simultaneously, with one or more cells of whatever type
   assigned to a processor, masking each processor off when it has
   completed the relevant number of connections; or (2) to distribute
   connections, rather than cells, over the processors.  This would
   assure a very uniform load distribution, but would have the added
   complication of needing to bring the results together to update the
   cell variables.                                                    */

      for (il=play1;il!=NULL;il=il->play) {

/* For each iteration, the "DAS" statistics are collected in the
   CONNBLKs.  This calculation is included in the benchmark mainly
   to see how different parallel architectures deal with possibly
   asynchronous updating of a variable from a number of different
   processes (in this case, different connections getting updated).
   At this time, the statistics must be initialized to zero.          */

         for (ix=il->pct1;ix!=NULL;ix=ix->pct) {
            ix->das[0]=0;ix->das[1]=0;ix->das[2]=0;ix->das[3]=0;}

/* Evaluate responses for all cells of this type */

         ic = il->prd;

/******************* NCUBE This loop is done in parallel **********************/

/* NCUBE */ for (js=1,is=il->psi+il->locell,is2=il->ps2;js<=il->mcells;js++) {

/* Evaluate responses for all connection types associated with each
   cell type.                                                         */

            wksum = 0; /* Clear the working s(i) sum */
            simmti = (int)*is++ - mti; /* Temp term needed for amplif */
            for (ix=il->pct1;ix!=NULL;ix=ix->pct) {

/* Calculate contributions of all connections of current type         */

               ix->ax = 0; /* Clear the connection-type sum.  This is
                              stored in the data block for other uses */
               psj = ix->psrc->psi; /* Locate the source cells */
               for (jc=0;jc<ix->nconns;jc++,ic++) {
                  wksj  = (int)psj[ic->lij];
                  sjmet = wksj - et;
                  wkcij = ic->cij; /*!!*/
                  if (sjmet > 0) ix->ax += sjmet*wkcij;

/* Perform amplification if delta != zero.  Only the "4-way" rule
   is implemented in this "benchmark".  To make life simpler,
   the 'phi' factor is ignored (i.e. taken to be 1.0), and scaling
   is fudged to make intermediate results fit in 32-bit registers.
   "DAS" statistics are always collected.                             */

                  if (il->delta) {
                     sjmmtj = wksj - mtj;
                     ampcase = 0;
                     if (simmti < 0) ampcase += 2;
                     if (sjmmtj < 0) ampcase += 1;
                     ix->das[ampcase]++;
                     wkcij += (il->delta*simmti*sjmmtj)>>19;
                     wkcij &= 255;       /*!!*/
                     ic->cij = (byte) wkcij;}

                  } /* End loop over individual connections */

               /* Scale the connection-type contribution and add it in */
               ix->ax *= ix->scale;
               wksum += ix->ax;
               } /* End loop over connection types */

            /* Force the final s(i) into range 0 to 255 */
            if (wksum < pt) wksum = 0;
            else {wksum = wksum>>15;  wksum &= 255;}  /*!!*/
            *is2++ = (byte)wksum;
            } /* End loop over cells of current type */

/****************** NCUBE This loop was done in parallel *********************/

         /* Reverse the new and old states of the cells */

/*         psj = il->psi; il->psi = il->ps2; il->ps2 = psj; NCUBE not needed */
           gather(il); sumdas(il);                                  /* NCUBE */

	   ys = (il->ngy*node)/total;
           dy = (il->ngy*(node+1))/total - ys;
           gx = il->xoffset;
	   gy = il->yoffset + ys*il->c_size;
           ldrwp_(gx,gy);    /* This call specifies where in buffer memory
                                to write. */ 
           dwt_(il->psi,il->ngx,0,ys,il->ngx,dy,il->c_size);
           dmansi_(0);

         } /* End loop over cell types */
      } /* End loop over requested number of iterations */

/* Report time required for simulation */

   time(&time3);              /* Note ending time */
   printf("%d iterations required %f seconds\n",
      niterations,difftime(time3,time2));

/* Report "DAS" statistics for final iteration only. */

   printf("DAS statistics for final iteration:\n");
   for (jl=1,il=play1;il!=NULL;il=il->play,jl++) {
      printf("   Celltype %d\n",jl);
      for (jx=1,ix=il->pct1;ix!=NULL;ix=ix->pct,jx++)
         printf("      Conntype %3d: %8ld%8ld%8ld%8ld\n",
         jx,ix->das[0],ix->das[1],ix->das[2],ix->das[3]);}

   } /* END MAIN */

time(time1)   /* NCUBE Obtain time in seconds */
float *time1;
{
	*time1 = ((double) ntime())*(1024./8000000.);
}

gather(il)                       /* NCUBE Gather results from all nodes */
struct CELLBLK *il;
{
	register byte *r, *s, *t;
	int i, *p;
	int chan, mask, nsend, isend, next, nrecv, irecv, length, type=0x7515;
	double factor;

/* First get 'loary' - an array of the first cell number on each node
   (first cell is 0) */

	p = loary;
	factor = (double)il->ncells / (double)total;
	for (i=0; i<=total; i++) {*p++ = (int)((double)i * factor + 0.00001);}

/* Now take the new s(i) data from il->ps2 on this node and assemble
   it in il->psi on all nodes */

	r = il->psi + il->locell;
	t = il->ps2 + il->mcells;
	for (s = il->ps2; s < t; *r++ = *s++);

	for (chan=1; chan<total; chan <<= 1) {
	  mask = ~(chan - 1);
	  nsend = node & mask;
	  isend = *(loary + nsend);
	  next = node ^ chan;
	  nrecv = next & mask;
	  irecv = *(loary + nrecv);
	  length = (*(loary + nsend + chan) - isend) * sizeof(*il->psi);
	  nwrite(il->psi + isend, length, next, type);
	  nread(il->psi + irecv, 0x7fffffff, &next, &type);
	}
}

sumdas(il)                   /* NCUBE sum "DAS" statistics over all nodes */
struct CELLBLK *il;
{
	register struct CONNBLK *ix;
	register long *r, *s;
	long *t, *ss;
	int chan, next, length, type=0x7516;

        for (r=dassum,ix=il->pct1;ix!=NULL;ix=ix->pct) {
	  *r++ = ix->das[0];
	  *r++ = ix->das[1];
	  *r++ = ix->das[2];
	  *r++ = ix->das[3];
	}
	length = il->nconntypes*sizeof(ix->das);
	t = dassum+il->nconntypes*4;
	for (chan=1; chan<total; chan <<= 1) {
	  next = node ^ chan;
	  nwrite(dassum, length, next, type);
	  nreadp(&ss, length, &next, &type); s=ss;
	  for (r=dassum; r<t; *r++ += *s++);
	  nrelp(ss);
	}		
        for (s=dassum,ix=il->pct1;ix!=NULL;ix=ix->pct) {
	  ix->das[0] = *s++;
	  ix->das[1] = *s++;
	  ix->das[2] = *s++;
	  ix->das[3] = *s++;
	}
}


syncx()	/* does not return until all nodes have called it */
{
	int node, ncube, total, rest, junk, sync = 0x751c, any, zero = 0;

	whoami(&node, &junk, &junk, &ncube);
	total = 1<<ncube;

	if(node == 0) {
	  for(rest=1; rest < total; rest++) {
	    any = -1;
	    nread(&junk, sizeof junk, &any, &sync);
	  }
	} else {
	  nwrite(&junk, sizeof junk, zero, sync);
	}

	if(node == 0) {
	  for(rest=1; rest < total; rest++) {
	    nwrite(&junk, sizeof junk, rest, sync);
	  }
	} else {
	  nread(&junk, sizeof junk, &zero, &sync);
	}
}
/* Crude random number generator - taken from Kernigan & Ritchie,
   second edition, page 46.  This generates only 15-bit random
   numbers, which is good enough for the benchmark.                   */
/* Rev, 10/22/88, GNR - randskip added */

#DEFINE RAND_MAX 32767

unsigned long rnext = 1;

/*-------------------------------------------------------------------*/
/*                    srand: set seed for rand()                     */
/*-------------------------------------------------------------------*/

   srand(seed)
   unsigned long seed;
   {

   rnext = seed;
   }

/*-------------------------------------------------------------------*/
/*          rand: return pseudo-random integer on 0..32767           */
/*-------------------------------------------------------------------*/

int rand() {

   rnext =rnext * 1103515245 + 12345;
   return (unsigned long int)(rnext/65536) % 32768;
   }

/*-------------------------------------------------------------------*/
/*          randskip: skip n random numbers in the sequence          */
/*-------------------------------------------------------------------*/

/* Note: Thanks to Joe Brandenberg of Intel Scientific Computers for
   the idea used in this routine.  The formula for the k'th number
   that would be returned by the 'rand' function is
      x(k) = (A**k)*x + C*Sum(j=0 to k-1)(k**j) (mod 2**32)
   where A = 1103515245, C = 12345 (See Knuth's chapter on random
   number generators if this is not obvious.)  This is expressed as
   x(k) = P(k)*x + Q(k) and the values of P and Q are tabulated for
   values of k that are exact powers of two.  The following code
   then obtains x(k) by repeated application of the above formula
   for each tabulated value that corresponds to a one in the binary
   representation of k.  This method runs in time proportional to
   log(2)(k), and represents a compromise between speed and the
   space that would be required to store larger tables of P and Q
   values.  The tabulated values of P and Q were calculated using
   the program 'randtabl c' and the smaller ones were checked with a
   REXX program that did the same calculation with explicit multi-
   precision arithmetic.  The Q coefficients are obtained by adding
   up the geometric series for Q rather than by using the well-known
   closed form for the sum in order to avoid the need to perform a
   multi-precision quotient by A-1.  Note that the last two values
   of P are both 1, which means that after 2**30 calls, this random
   number generator enters a simple arithmetic sequence from which
   it never escapes.  This condition is not important for the uses
   to which the this routine will be put.  The values are included
   in the table so the program can operate with no danger of having
   an excessive subscript into one of these tables.  No test is made
   for reaching the arithmetic sequence region.                      */

   unsigned long P[32] = {
      1103515245, 3265436265, 3993403153, 3487424289,
      1601471041, 2335052929, 1979738369,  387043841,
      3194463233, 3722397697, 1073647617, 2432507905,
      1710899201, 3690233857, 4159242241, 4023517185,
      3752067073, 3209166849, 2123366401, 4246732801,
      4198498305, 4102029313, 3909091329, 3523215361,
      2751463425, 1207959553, 2415919105,  536870913,
      1073741825, 2147483649,          1,          1 } ;
   unsigned long Q[32] = {
           12345, 3554416254, 3596950572, 3441282840,
      1695770928, 1680572000,  422948032, 3058047360,
       519516928,  530212352, 2246364160,  646551552,
      3088265216,  472276992, 3897344000, 2425978880,
       556990464, 1113980928, 2227961856,  160956416,
       321912832,  643825664, 1287651328, 2575302656,
       855638016, 1711276032, 3422552064, 2550136832,
       805306368, 1610612736, 3221225472, 2147483648 } ;

   randskip(n) 
   unsigned long n;
   {

   register int k;            /* Power of two being considered */
   register unsigned long rn; /* Working copy of n */

   k = 0;
   rn = n;
   while (rn) {
      if (rn & 1) rnext = P[k]*rnext + Q[k];
      k++; rn >>= 1;
      }
   }


/***************************************************************************
 *  Routine to load colour map for Darwin3 benchmark.
 *
 *  Divides up remaining 255 color map entries NCOLORS ways:
 *    the first group of entries all get set to have RGB values
 *    given by the first entry in the arrays red,green and blue.
 *    And so on for the subsequent groups.
 *
 *  The arrays red[], green[] and blue[] are declared as globals
 *    so that they can be initialized in a nice way. The CFG
 *    compiler for the ncube will only allow scalar automatics
 *    to be initialized in a declaration statement. 
 *************************************************************************/ 

#define NCOLORS 8
int   red[NCOLORS] = { 0x80, 0xc0, 0xf0, 0xf0, 0x00, 0x00, 0xf0, 0xf0};
int green[NCOLORS] = { 0x00, 0x00, 0x40, 0xa0, 0xe0, 0xd0, 0x00, 0xf0};
int  blue[NCOLORS] = { 0x00, 0x20, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xf0};

d3colors()
{
   int map_entry, ic, is, ie, cnt;
   unsigned char cbar[10];

   cnt = 256/NCOLORS; 

   for (is=1,ie=cnt,ic=0; ic<NCOLORS; ic++) {

 
    for (map_entry=is; map_entry<ie; map_entry++)
         ldcolor(map_entry,red[ic],green[ic],blue[ic]);
     is = ie;
     ie += cnt;
     if (ie > 255) ie=255; 
     } 

   ldcolor(0,0,0,0);   /* Set background to colour 0 */

   cbar[0] = 1;
   for (ic=1; ic<NCOLORS; ic++) cbar[ic] = ic*cnt;
   ldrwp_(680,390);
   dwt_(cbar,1,0,0,1,NCOLORS,30);
}  /* End of d3colors() */

frmnum(frame)
int frame;
{
   char string[8];

   sprintf(string,"%d",frame);
   amove(512,738);
   dcolor(0,255); 
   dtextp(string,2,0);
   gcmd();

   return 0;
}


static short start_dma[5] = {    30,  /* Command number, see page 59
                                        of NCUBE manual. */
                             0x800,  /* Node address */
                                 0,  /* Frame buffer address */
                            0x6000,  /* Length of DMA transfer */
                                 1}; /* Direction of transfer:
                                        0  for node to video
                                        1  for video to node */

static short check_dma[3] = {23, 0, 0};  /* Send this message to cause
                                            message to be sent back from
                                            graphics nodes indicating
                                            status of DMA transfer. */
static short dma_reply[8];

#define DMA_DEST 0x8107
#define DMA_TYPE 0x7ad0 

dmansi_(trans)
int trans;
{
   int dest,type;
 
   dest=DMA_DEST;
   type=DMA_TYPE;

   if (node == 0) {
      if (trans) nwrite(start_dma,10,dest,type);

      else {for (;;) {
                nwrite(check_dma,6,dest,type);
                nread(dma_reply,16,&dest,&type);
                if ((dma_reply[0] & 0x400) == 0) break;
                }
      /*nsum(0)*/gsync();
         }
      }
   else if (!trans) /*nsum(0)*/gsync();

}

gsync()
{
	int chan, junk=6, next, type=0x751c;

	for (chan=1; chan<total; chan <<= 1) {
	  next = node ^ chan;
	  nwrite(&junk,sizeof junk, next, type);
	  nread( &junk, sizeof junk, &next, &type);
	}
}
