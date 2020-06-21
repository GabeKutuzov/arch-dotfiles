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

/*---------------------------------------------------------------------*
*      THIS IS THE TI MVP MASTER PROCESSOR (MP) SERVER COMPONENT       *
*---------------------------------------------------------------------*/

/* Note that integer arithmetic will be used for all calculations,
   as in the current program.  This will be a realistic test for the
   kind of modelling we expect to do.  In the comments that follow,
   parenthesized expressions of the form '(Sn)' will be used to
   indicate that a quantity is understood to have n fraction bits.    */

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

#include <stdlib.h>        /* MVP Standard Definitions */
#include <stdarg.h>
#include "stdio.h"
#include "time.h"

/* Include MVP files */
#include <mvp.h>           /* MVP hardware functions */
#include <mvp_hw.h>        /* MVP hardware parameters */
#include <task.h>          /* MP multitasking kernel */
#include <mp_int.h>        /* MP interrupts */

/* Local (GIC) include files */
#include "bios.h"          /* Basic I/O functions */
#include "kerror.h"        /* Kernel error definitions, etc */
#include "host.h"          /* HOST Communication functions */
#include "timemgr.h"       /* Timer functions */
#include "dspAPI.h"        /* DSP Command interface */

/* Additional include files */
#include "commtest.h"   /* Whacky structure def's */
#include "param.h"

/* Define parameters that aren't worth varying in input file
      PT = positive firing threshold for a cell
      MTI = modification threshold on output of a cell
      MTJ = modification threshold on input to a connection
      ET = excitatory threshold for an input to a connection          */

#define  PT  0.25
#define MTI  0.3
#define MTJ  0.4
#define  ET  0.2

/* Define structure types used in the simulation
   Note: In the real simulation, all of the structures contain
   many more variables.                                               */

   typedef unsigned char byte;

   struct CONNDATA {          /* Data for a single connection */
      short int lij;          /* Number of source cell */
      byte cij;               /* Strength of connection (S7) */
      };

   struct CONNBLK {           /* Connection-type information block */
      struct CONNBLK  *pct;   /* Pointer to next CONNBLK */
      struct CELLBLK  *psrc;  /* Pointer to source cell type */
      double chunksize;       /* Range of a generated connection */
      long int nconns;        /* Number of connections */
      int source;             /* Serial number of source cell type */
      int rule;               /* Connection generating rule */
      long int scale;         /* Scale factor (S8) */
      long int das[4];        /* Detailed amp stats */
      long int ax;            /* Intermediate afferent sum (S15) */
      };

   struct CELLBLK {           /* Cell-type information block */
      struct CELLBLK  *play;  /* Pointer to next CELLBLK */
      struct CONNBLK  *pct1;  /* Pointer to first CONNBLK */
      struct CONNDATA *prd;  /* Pointer to actual connection data */
      long int ncells;        /* Number of cells of this type */
      long int nconntypes;    /* Number of connection types */
      long int nconntotal;    /* Total number of connections */
      long int delta;         /* Amplification factor (S10) */
      byte  *psi;             /* Pointer to s(i) data */
      byte  *ps2;             /* Pointer to alternate (new) s(i) */
      };

/*** DEBUG replacement for malloc--report amount requested ***/
#ifndef SIM
void *mallocv(unsigned int n) {
   void *mret;
   printf("malloc requesting %u bytes\n", n);
   mret = memalign(MVP_CACHE_SUBBLOCK, n);
   printf ("malloc returned %x\n", mret);
   return mret;
   }
#else
#define mallocv(n) malloc(n)
#endif
   
#ifndef SIM
/**** rptlij DEBUG ****/
void rptlij(struct CELLBLK *play1) {
   int jl,jx;
   struct CELLBLK *il;
   struct CONNBLK *ix;
   struct CONNDATA *ic;

   for (jl=1,il=play1; il; il=il->play,jl++) {
      ic = il->prd;
      for (jx=1,ix=il->pct1; ix; ix=ix->pct,jx++) {
         printf("For celltype %d, conntype %d, first lij is %d,"
           " first cij is %d\n", jl, jx, (int)ic->lij, (int)ic->cij);
         ic += ix->nconns;
         } /* End connblk loop */
      } /* End cellblk loop */
   } /* End rptlij */
/*** ENDDEBUG ROUTINE ***/
#endif

/*--------------------------------------------------------------------*/
/*                                                                    */
/*                         MAIN PROCEDURE                             */
/*                                                                    */
/*--------------------------------------------------------------------*/

unsigned long int next = 1;
extern char buffer1[], buffer2[];
char buffer1[128], buffer2[128];
shared void test_pp( DSPARG * ); /* My DSP test function */

userMain(int argc, char *argv[]) {

/* Declarations: */

   int ncelltypes;            /* Number of cell types in simulation */
   int niterations;           /* Number of iterations to perform */
   unsigned int seed;         /* Random number seed */
   long int pt;               /* Working output threshold */
   int mti,mtj,et;            /* Working thresholds (S8) */
   float tdelta,tscale;       /* Temporary delta, scale */
   int trule;                 /* Temporary copy of 'rule' */
   int ji,jl,js,jx,jc;        /* Iter, cell, conn type & data counters */
   struct CELLBLK  *play1;    /* Pointer to first cell block */
   struct CELLBLK *il,*ll;    /* Pointer to current, last CELLBLK */
   struct CONNBLK  *ix,*lx, *xtemp;   /* Pointer to current, last CONNBLK */
   struct CONNDATA  *ic,*lc,*ctemp;   /* Pointer to current connection data */
   byte *is,*is2,*psj;        /* Pointers to state variables */
   int wksj,sjmet,wkcij;      /* s(j) (S8), s(j) - et (S8), cij (S7) */
   int simmti,sjmmtj;         /* s(i) - mti (S8), s(j) - mtj (S8) */
   int ampcase;               /* Amplification case for "DAS" */
   long int wksum;            /* Working sum for s(i) */
   double time1,time2,time3;  /* Starting, setup, ending time */
   int i,j;
   int test;
   char dum[100];
   long lp[2];
   int ptr=0;
   int oldlij[4], lij[4];
   
   char *input[] = {
     "4 10 1009",
     "300 4 0.1",
        "400 1 0 0.2",
        "100 2 1 0.3",
        "40  3 0 0.25",
        "20  4 1 0.18",
     "120 4 0.2",
        "200 1 1 0.2",
        "100 2 0 0.25",
        "40  3 1 0.18",
        "20  4 0 0.25",
     "50 4 0.1",
        "400 1 1 0.2",
        "150 2 0 0.25",
        "40  3 0 0.33",
        "24 4 1 0.25",
     "30 4 0.2",
        "120 1 1 0.1",
        "10  2 0 0.2",
        "40  3 1 0.25",
        "20  4 0 0.18",
        "\0"};

/*   char *input[] = {
     "2 1 1009",
     "5 2 0.1",
        "2 1 1 0.2",
        "2 2 0 0.25",
     "3 2 0.2",
        "2 1 1 0.1",
        "2  2 0 0.2",
        "\0"};*/

/* Set up communications */

/*   commNodeControl(CNC_REGISTER);
   commNodeControl(CNC_NOTIFY, NOTIFY_WAIT);*/

/* Note the beginning time */

   time1 = GICTimerGet();

/* Read header info for setting up loops */

   sscanf(input[ptr++], "%d %d %u", &ncelltypes, &niterations, &seed);

#ifdef SIM
   sprintf(dum,
   "Number of celltypes = %d, number of iterations = %d, seed = %u\n",
      ncelltypes,niterations,seed);
#else
   printf(
   "Number of celltypes = %d, number of iterations = %d, seed = %u\n",
      ncelltypes,niterations,seed);
#endif
   srand(seed);

   play1 = NULL; ll = NULL; j=0;
   for (jl=1;jl<=ncelltypes;jl++, j++) {
      if ((il=(struct CELLBLK *)mallocv(sizeof(struct CELLBLK)))==NULL) {
#ifndef SIM
         printf("No mem for cellblk\n");
#endif
         exit(3);
         }

      if (ll==NULL) play1 = il; else ll->play = il;
      ll = il;

      sscanf(input[ptr++], "%ld %ld %f", &(il->ncells), &(il->nconntypes), 
                                          &tdelta);
#ifndef SIM
      printf("   Cell type %d\n",jl);
      printf("      Ncells = %ld, nconntypes = %ld, delta = %f\n",
         il->ncells,il->nconntypes,tdelta);
#endif
      il->nconntotal = 0;
      il->delta = (long int)(1024.0*tdelta);

/* Read the connection type information and create connection
   blocks in a linked list chained to the controlling CELLBLK.
   This is done prior to allocating cell data to keep linked-
   list memory better localized */

      lx = NULL;
      i = 0;
      for (jx=1;jx<=il->nconntypes;jx++, i++)  {
         if ((ix=(struct CONNBLK *)mallocv(sizeof(struct CONNBLK)))==NULL) {
#ifndef SIM
           printf("No mem for CONNBLK\n"); 
#endif
           exit(4);
           }
         if (lx==NULL) il->pct1 = ix; else lx->pct = ix;
         lx = ix;

        sscanf(input[ptr++], "%ld %d %d %f", &(ix->nconns), &(ix->source),
                                       &(ix->rule), &tscale);
#ifndef SIM 
        printf("      Connection type %d\n",jx);
        printf("         Nconns = %ld, source = %d, rule = %d, scale = %f\n", 
                         ix->nconns,ix->source,ix->rule,tscale);
#endif
        ix->scale = (long int)(256.0*tscale);
        il->nconntotal += ix->nconns;
        } /* End reading CONNBLK input lines for this CELLBLK */
     lx->pct = NULL; /* Terminate the connection linked list */

     } /* End reading CELLBLK input lines */
   ll->play = NULL; /* Terminate the cell linked list */

/* Allocate space for two vectors of s(i) values (s(i) == state of
   cell i).  psi points to current values, ps2 points to newly cal-
   culated values.  The psi states are given random initial values.   */

   for (il=play1;il!=NULL;il=il->play) {
      if ((il->psi=(byte *)mallocv((unsigned int)il->ncells))==NULL) {
#ifndef SIM
          printf("No mem for s(i)\n");
#endif
          exit(5);
          }
      if ((il->ps2=(byte *)mallocv((unsigned int)il->ncells))==NULL) {
#ifndef SIM
          printf("No mem for s(i)\n"); 
#endif
          exit(5);
          }
      for (is=il->psi,js=1;js<=il->ncells;js++) 
         *is++ = (byte)rand();

#ifndef SIM
      printf("Flushing psi,ps2 arrays\n");
#endif
      flush(il->psi, il->ncells);
      flush(il->ps2, il->ncells);
      } /* End celltype loop */

/* Complete the connection blocks:
      1) Allocate space for the connection data
      2) Locate the relevant source data and save pointers
      3) Fill in random starting values for cij and lij.              */

#ifndef SIM
   printf("\nStarting cell initialization loop \n"); 
#endif

   for (il=play1;il!=NULL;il=il->play) { /* celltype */

      /* Allocate space for the actual connection data */

      il->prd=(struct CONNDATA *)
               mallocv((unsigned int)
               il->ncells*il->nconntotal*sizeof(struct CONNDATA));
      if (il->prd==NULL) {
#ifndef SIM
           printf("nconns %d ncells %d couldn't alloc cif lij\n",
                    il->nconntotal, il->ncells);
#else
           sprintf(dum,"nconns %d ncells %d couldn't alloc cif lij\n",
                    il->nconntotal, il->ncells);
#endif
           exit(6);
           }
      ic = il->prd;

      /* Locate the appropriate cell blocks and figure chunksize */

      for (ix=il->pct1;ix!=NULL;ix=ix->pct) { /* connblk */

         /* Locate the source cell type block */
#ifndef SIM
         printf("Setting up connections from source %d\n",ix->source);
#endif
         for (jl=1,ll=play1;jl<ix->source;jl++,ll=ll->play) ; 
         ix->psrc = ll;

         /* Constant for uniform random connection rule */
         ix->chunksize = (double)ll->ncells/((double)RAND_MAX+1.0);
         /* Constant for partitioned connection rule */
         if (ix->rule) ix->chunksize /= (double)ix->nconns;
         } /* End connection block initialization */

      /* Generate and fill in the actual connection data */

      for (js=1;js<=il->ncells;js++) { /* Loop over cells */
         for (ix=il->pct1;ix!=NULL;ix=ix->pct) { /* Connection types */

#ifndef SIM
            ctemp = play1->prd; i=1;         
            
            for (xtemp=play1->pct1; xtemp; xtemp=xtemp->pct) {

               lij[i-1] = (int) ctemp->lij;
               if (oldlij[i-1] != lij[i-1]) {             
                   printf(" IN LOOP: celltype 1, ctype %d, 1st lij  %d\n",
                      i, (int)ctemp->lij);
                   oldlij[i-1] = (int) ctemp->lij;
                   }
               i++;
               ctemp += xtemp->nconns;
            }
#endif

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
#ifndef SIM
      printf("Now flushing connection data\n");
#endif
      flush(il->prd,sizeof(struct CONNDATA)*il->ncells*il->nconntotal);
      } /* End loop over cell types */

/* Make fixed-point working thresholds */
#ifndef SIM
   printf("Finished cell type loop\n");
#endif

#ifndef SIM  /*** Print some lijs ***/
   printf("Calling rptlij first time\n");
   rptlij(play1);
#endif

/* Go through all the data again and flush all to real memory */

#ifndef SIM
   printf("Now flushing all celltype and conntype data\n");
#endif
   for (il=play1; il; il=il->play) { /* celltype */
      for (ix=il->pct1;ix!=NULL;ix=ix->pct) { /* connblk */
         flush(ix,sizeof(struct CONNBLK));
         } /* End conntype loop */
      flush(il,sizeof(struct CELLBLK));
      } /* End celltype loop */

/* Note the end-of-setup time */

   sprintf(dum,"Setup required %lf seconds\n",time2-time1);

   lp[0] = (long) niterations;
   lp[1] = (long) play1;

   dspAPIInit();
   i = dspInit(0x1);
   if (i != DSP_OK ) sprintf(dum," dspInit() failed");  
   else {
#ifndef SIM
      printf("Starting PP function\n");
#endif
      time2 = GICTimerGet();
      dspExec(0x1, test_pp, 2L, lp, DSP_WAIT, 0);
      time3 = GICTimerGet();

#ifndef SIM
      printf("PP function returned \n");
#endif
      }
#ifndef SIM
    printf("\nBenchmark time is %lf \n", time3-time2);
#endif

#ifndef SIM  /*** Print some lijs ***/
   printf("Calling rptlij second time\n");
   rptlij(play1);
#endif

/* Print DAS results */

   for (jl=1,il=play1;il!=NULL;il=il->play,jl++) {
#ifndef SIM
      printf(" Celltype %d \n", jl);
#endif
      for (jx=1,ix=il->pct1;ix!=NULL;ix=ix->pct,jx++) {
#ifndef SIM
         printf("      Conntype %3d: %8ld%8ld%8ld%8ld\n",
         jx,ix->das[0],ix->das[1],ix->das[2],ix->das[3]);
#endif
         }
      }
   }
 

int rand(void) {

   next = next * 1103515245 + 12345;
   return (unsigned int)(next>>16) & RAND_MAX;
   }
void srand(unsigned int seed) {

   next = seed;
   }

