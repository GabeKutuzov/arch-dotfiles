/* PROGRAM TO CONVERT PH1I8 SAVENET FILE TO NEW FORMAT */
/* Originally by M. Cook, revised to handle 4-character
*  object names, run as main pgm, 04/04/92, GNR
*
*  Synopsis: rstcnvrt oldfile newfile
*/

/*
*     Common block definitions:
*/
#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <memory.h>
#include "sysdef.h"
#include "d3global.h"
#include "rocks.h"

#include "newsvblk.h"
#include "savblk.h"

#define SAVRES_ERR   73

void main(int argc, char *argv[]) {

   struct REPBLOCK *ir;
   struct CELLBLK *il;
   struct CONNTYPE *ix;

   struct CONNTYPE *curr_ix;
   int fdes;
   int oldfdes;
   int i,ii;
   int count;

   s_type *p1;
   rd_type *pd;
   char *bptr;

   long cflg;
   long nn;
   long firstsi;
   long size,wsize;
   long sizes[17];
   char * junk;
   struct SEEDBLK {
      long cseed;
      long lseed;
      long nseed;
      long rseed;
      } *seeds;


seeds = (struct SEEDBLK *)mallocv(sizeof(struct SEEDBLK),
      "SEEDBLK.");
oldhdr = (struct OLDSAVBLK *)mallocv(sizeof(struct OLDSAVBLK),
      "OLDSAVBLK.");
hdr = (struct SAVBLK *)mallocv(sizeof(struct SAVBLK),
      "SAVBLK.");
ilhdr = (struct CELHDR *)mallocv(sizeof(struct CELHDR),
      "CELHDR.");
xhdr = (struct CONNHDR *)mallocv(sizeof(struct CONNHDR),
      "CONNHDR.");

ix = (struct CONNTYPE *)mallocv(sizeof(struct CONNTYPE),
      "CONNTYPE.");
il = (struct CELLBLK *)mallocv(sizeof(struct CELLBLK),
      "CELLBLOCK.");

junk = (char *)mallocv(MAX_MSG_LENGTH,"junk.");

memset(il, 0, sizeof(struct CELLBLK));
memset(ix, 0, sizeof(struct CONNTYPE));

/* Get file names from command line, open them */

   RK.iexit = 0;
   if (argc != 3) {
      cryout(RK_P1,"0***Synopsis: rstcnvrt oldfile newfile",
         RK_LN2,NULL);
      exit();
      }
   oldfdes = (int) n_fopen(argv[1],2,NULL,NULL,0);
   fdes = (int) n_fcreate(argv[2],17,1);
   if (oldfdes < 0) {
      cryout(RK_P1,"0***CAN'T OPEN OLDSAVENET FILE.",RK_LN2,NULL);
      RK.iexit = 1;
      }
   if (fdes < 0) {
      cryout(RK_P1,"0***CAN'T OPEN NEW SAVENET FILE.",RK_LN2,NULL);
      RK.iexit = 1;
      }
   if (RK.iexit) exit();

/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */


/* write out general header in new file */
   hdr->vers = 2;
   hdr->numrep = 0;
   hdr->numcel = 17;
   hdr->spare = 0;
   strncpy(hdr->title,"NEW TRACKING RUN ph1i8, 4-char names",
      MAX_TITLE_LEN);
   strncpy(hdr->timestamp,"911025100000",DATE_LEN);

   n_fwrite (fdes,hdr,sizeof(struct SAVBLK),0,0);

/*   
 *
 * BASIC FLOW OF THIS PROGRAM:
 *
 * for each header in old file
 *    read header
 *    read seeds;
 *    create new header
 *    write out new header
 *    read and throw away data
 * endfor
 *
 * At this point have the header info for new file
 *
 *  NEXT: read old savenet file again: 
 *
 * position old file at beginning
 *
 * for each header in old file
 *    read header
 *    read data
 *    write data
 * endfor
 *
 */


     firstsi = sizeof(struct SAVBLK) +
               sizeof(struct CELHDR)*17 +
               sizeof(struct CONNHDR)*29;


/* CELLTYPE RM  - one CONNTYPE TV 1 */

         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);

            il->nelt = 256;
            il->lel = 48; 

    strncpy(ilhdr->repnam,"E1",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 16;       /* by hand */
         ilhdr->rngy    = 16;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 44444;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   = firstsi;    /* by hand */
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;

   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);
/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"TV",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"1",LSNAME); /*hand*/
            xhdr->numconns= 16;              /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 4;               /* by hand */
            xhdr->xkgen =   0x24002102;      /* by hand */
            xhdr->src_ngx = 64;      /* by hand */
            xhdr->src_ngy = 64;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 4096;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */

   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);
/* CELLTYPE FR  - three CONNTYPE TV's*/
/* Now skip over the data to the next header -  */
            size = il->nelt + il->lel*il->nelt;
            count = 0;
            sizes[count++] = size;
printf("\n");
printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);


    strncpy(ilhdr->repnam,"FR",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 22;       /* by hand */
         ilhdr->rngy    = 11;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 155548;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel ; /* by hand */
            il->nelt = 242;
            il->lel = 144; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;

   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);
/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"TV",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"1",LSNAME); /*hand*/
            xhdr->numconns= 16;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x24002102;        /* by hand */
            xhdr->src_ngx = 64;      /* by hand */
            xhdr->src_ngy = 64;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 4096;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"TV",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"1",LSNAME); /*hand*/
            xhdr->numconns= 16;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x24002102;        /* by hand */
            xhdr->src_ngx = 64;      /* by hand */
            xhdr->src_ngy = 64;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 4096;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"TV",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"1",LSNAME); /*hand*/
            xhdr->numconns= 16;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x24002102;        /* by hand */
            xhdr->src_ngx = 64;      /* by hand */
            xhdr->src_ngy = 64;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 4096;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/* CELLTYPE FT  - three CONNTYPE */
/* Now skip over the data to the next header -  */
            size = il->nelt + il->lel*il->nelt;
            sizes[count++] = size;

printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);


    strncpy(ilhdr->repnam,"FT",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 11;       /* by hand */
         ilhdr->rngy    = 11;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 466634;        /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel ; /* by hand */
            il->nelt = 121;
            il->lel = 144; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;
   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"TV",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"1",LSNAME); /*hand*/
            xhdr->numconns= 16;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x24002102;        /* by hand */
            xhdr->src_ngx = 64;      /* by hand */
            xhdr->src_ngy = 64;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 4096;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"TV",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"1",LSNAME); /*hand*/
            xhdr->numconns= 16;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x24002102;        /* by hand */
            xhdr->src_ngx = 64;      /* by hand */
            xhdr->src_ngy = 64;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 4096;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"TV",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"1",LSNAME); /*hand*/
            xhdr->numconns= 16;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x24002102;        /* by hand */
            xhdr->src_ngx = 64;      /* by hand */
            xhdr->src_ngy = 64;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 4096;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/* CELLTYPE V3  - one CONNTYPE */
/* Now skip over the data to the next header -  */
            size = il->nelt + il->lel*il->nelt;
            sizes[count++] = size;

printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);


    strncpy(ilhdr->repnam,"V3",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 10;       /* by hand */
         ilhdr->rngy    = 10;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 777720;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel ; /* by hand */
            il->nelt = 100;
            il->lel = 4; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;
   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"TV",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"1",LSNAME); /*hand*/
            xhdr->numconns= 1;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x024002102;        /* by hand */
            xhdr->src_ngx = 64;      /* by hand */
            xhdr->src_ngy = 64;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 4096;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/* CELLTYPE V4  - one CONNTYPE */
/* Now skip over the data to the next header -  */
            size = il->nelt + il->lel*il->nelt;
            sizes[count++] = size;

printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);


    strncpy(ilhdr->repnam,"V4",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 20;       /* by hand */
         ilhdr->rngy    = 10;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 888824;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel ; /* by hand */
            il->nelt = 200;
            il->lel = 12; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;
   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"TV",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"1",LSNAME); /*hand*/
            xhdr->numconns= 4;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x24002102;        /* by hand */
            xhdr->src_ngx = 64;      /* by hand */
            xhdr->src_ngy = 64;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 4096;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/* CELLTYPE V5  - one CONNTYPE */
/* Now skip over the data to the next header -  */
            size = il->nelt + il->lel*il->nelt;
            sizes[count++] = size;

printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);


    strncpy(ilhdr->repnam,"V5",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 10;       /* by hand */
         ilhdr->rngy    = 64;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 999928;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel ; /* by hand */
            il->nelt = 640;
            il->lel = 4; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;
   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"TV",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"1",LSNAME); /*hand*/
            xhdr->numconns= 1;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x24002102;        /* by hand */
            xhdr->src_ngx = 64;      /* by hand */
            xhdr->src_ngy = 64;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 4096;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/* CELLTYPE NL  - zero CONNTYPEs */
/* Now skip over the data to the next header -  */
            size = il->nelt + il->lel*il->nelt;
            sizes[count++] = size;

printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);


    strncpy(ilhdr->repnam,"XL",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 1;       /* by hand */
         ilhdr->rngy    = 1;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 1111032;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel ; /* by hand */
            il->nelt = 1;
            il->lel = 1; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;
   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);

/* CELLTYPE NR  - one CONNTYPE */
/* Now skip over the data to the next header -  */
#define FUDGE 1  /* size is 2, but in the file is 3 */
            size = il->nelt + il->lel*il->nelt + FUDGE;
            sizes[count++] = size;

printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);


    strncpy(ilhdr->repnam,"XR",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 1;       /* by hand */
         ilhdr->rngy    = 1;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 1122145;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel + FUDGE;
            il->nelt = 1;
            il->lel = 2; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;
   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"XL",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"NL",LSNAME); /*hand*/
            xhdr->numconns= 1;          /* by hand */
            xhdr->len =     2;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x2102;        /* by hand */
            xhdr->src_ngx = 1;      /* by hand */
            xhdr->src_ngy = 1;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 1;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/* CELLTYPE NU  - one CONNTYPE */
/* Now skip over the data to the next header -  */
            size = il->nelt + il->lel*il->nelt;
            sizes[count++] = size;

printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);


    strncpy(ilhdr->repnam,"NU",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 1;       /* by hand */
         ilhdr->rngy    = 1;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 1233249;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel ; /* by hand */
            il->nelt = 1;
            il->lel = 768; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;
   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"E1",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"RM",LSNAME); /*hand*/
            xhdr->numconns= 256;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 8;         /* by hand */
            xhdr->xkgen =   0x40402;        /* by hand */
            xhdr->src_ngx = 16;      /* by hand */
            xhdr->src_ngy = 16;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 256;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/* CELLTYPE NL  - four CONNTYPE */
/* Now skip over the data to the next header -  */
            size = il->nelt + il->lel*il->nelt;
            sizes[count++] = size;

printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);



    strncpy(ilhdr->repnam,"NL",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 1;       /* by hand */
         ilhdr->rngy    = 1;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 1344353;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel ; /* by hand */
            il->nelt = 1;
            il->lel = 1110; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;
   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"E1",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"RM",LSNAME); /*hand*/
            xhdr->numconns= 256;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 8;         /* by hand */
            xhdr->xkgen =   0x40402;        /* by hand */
            xhdr->src_ngx = 16;      /* by hand */
            xhdr->src_ngy = 16;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 256;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"XL",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"NL",LSNAME); /*hand*/
            xhdr->numconns= 1;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x40402;        /* by hand */
            xhdr->src_ngx = 1;      /* by hand */
            xhdr->src_ngy = 1;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 1;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"NR",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"NR",LSNAME); /*hand*/
            xhdr->numconns= 1;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x40402;        /* by hand */
            xhdr->src_ngx = 1;      /* by hand */
            xhdr->src_ngy = 1;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 1;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"E1",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"RM",LSNAME); /*hand*/
            xhdr->numconns= 112;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 8;         /* by hand */
            xhdr->xkgen =   0x40402;        /* by hand */
            xhdr->src_ngx = 16;      /* by hand */
            xhdr->src_ngy = 16;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 256;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/* CELLTYPE NR  - four CONNTYPE */
/* Now skip over the data to the next header -  */
            size = il->nelt + il->lel*il->nelt;
            sizes[count++] = size;

printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);


    strncpy(ilhdr->repnam,"NR",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 1;       /* by hand */
         ilhdr->rngy    = 1;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 1755430;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel ; /* by hand */
            il->nelt = 1;
            il->lel = 1110; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;
   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"E1",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"RM",LSNAME); /*hand*/
            xhdr->numconns= 256;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 8;         /* by hand */
            xhdr->xkgen =   0x40402;        /* by hand */
            xhdr->src_ngx = 16;      /* by hand */
            xhdr->src_ngy = 16;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 256;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"XR",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"NR",LSNAME); /*hand*/
            xhdr->numconns= 1;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x40402;        /* by hand */
            xhdr->src_ngx = 1;      /* by hand */
            xhdr->src_ngy = 1;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 1;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"NL",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"NL",LSNAME); /*hand*/
            xhdr->numconns= 1;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x40402;        /* by hand */
            xhdr->src_ngx = 1;      /* by hand */
            xhdr->src_ngy = 1;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 1;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"E1",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"RM",LSNAME); /*hand*/
            xhdr->numconns= 112;          /* by hand */
            xhdr->len =     3;          /* by hand */
            xhdr->numbits = 8;         /* by hand */
            xhdr->xkgen =   0x40402;        /* by hand */
            xhdr->src_ngx = 16;      /* by hand */
            xhdr->src_ngy = 16;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 256;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/* CELLTYPE X2  - zero CONNTYPE */
/* Now skip over the data to the next header -  */
            size = il->nelt + il->lel*il->nelt;
            sizes[count++] = size;

printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);


    strncpy(ilhdr->repnam,"X2",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 1;       /* by hand */
         ilhdr->rngy    = 1;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 2166507;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel ; /* by hand */
            il->nelt = 1;
            il->lel = 0; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;
   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);


/* CELLTYPE UU  - three CONNTYPE */
/* Now skip over the data to the next header -  */
            size = il->nelt + il->lel*il->nelt;
            sizes[count++] = size;

printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);


    strncpy(ilhdr->repnam,"UU",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 1;       /* by hand */
         ilhdr->rngy    = 1;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 2177620;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel ; /* by hand */
            il->nelt = 9;
            il->lel = 1478; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;
   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"E1",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"RM",LSNAME); /*hand*/
            xhdr->numconns= 256;          /* by hand */
            xhdr->len =     4;          /* by hand */
            xhdr->numbits = 8;         /* by hand */
            xhdr->xkgen =   0x40402;        /* by hand */
            xhdr->src_ngx = 16;      /* by hand */
            xhdr->src_ngy = 16;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 256;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"E1",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"RM",LSNAME); /*hand*/
            xhdr->numconns= 112;          /* by hand */
            xhdr->len =     4;          /* by hand */
            xhdr->numbits = 8;         /* by hand */
            xhdr->xkgen =   0x40402;        /* by hand */
            xhdr->src_ngx = 16;      /* by hand */
            xhdr->src_ngy = 16;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 256;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"NU",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"NU",LSNAME); /*hand*/
            xhdr->numconns= 1;          /* by hand */
            xhdr->len =     4;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x2102;        /* by hand */
            xhdr->src_ngx = 1;      /* by hand */
            xhdr->src_ngy = 1;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 1;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/* CELLTYPE DD  - zero CONNTYPE */
/* Now skip over the data to the next header -  */
            size = il->nelt + il->lel*il->nelt;
            sizes[count++] = size;

printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);


    strncpy(ilhdr->repnam,"DD",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 1;       /* by hand */
         ilhdr->rngy    = 1;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 2488706;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel ; /* by hand */
            il->nelt = 9;
            il->lel = 0; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;
   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);


/* CELLTYPE RR  - three CONNTYPE */
/* Now skip over the data to the next header -  */
            size = il->nelt + il->lel*il->nelt;
            sizes[count++] = size;

printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);


    strncpy(ilhdr->repnam,"RR",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 1;       /* by hand */
         ilhdr->rngy    = 1;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 2499819;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel ; /* by hand */
            il->nelt = 9;
            il->lel = 1038; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;
   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"E1",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"RM",LSNAME); /*hand*/
            xhdr->numconns= 256;          /* by hand */
            xhdr->len =     4;          /* by hand */
            xhdr->numbits = 8;         /* by hand */
            xhdr->xkgen =   0x40402;        /* by hand */
            xhdr->src_ngx = 16;      /* by hand */
            xhdr->src_ngy = 16;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 256;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"NR",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"NR",LSNAME); /*hand*/
            xhdr->numconns= 1;          /* by hand */
            xhdr->len =     4;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x2102;        /* by hand */
            xhdr->src_ngx = 1;      /* by hand */
            xhdr->src_ngy = 1;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 1;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"LL",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"LL",LSNAME); /*hand*/
            xhdr->numconns= 2;          /* by hand */
            xhdr->len =     4;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x2102;        /* by hand */
            xhdr->src_ngx = 1;      /* by hand */
            xhdr->src_ngy = 1;      /* by hand */
            xhdr->src_nel = 9;      /* by hand */
            xhdr->src_nelt= 9;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */

            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/* CELLTYPE LL  - three CONNTYPE */
/* Now skip over the data to the next header -  */
            size = il->nelt + il->lel*il->nelt;
            sizes[count++] = size;

printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);


    strncpy(ilhdr->repnam,"LL",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 1;       /* by hand */
         ilhdr->rngy    = 1;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 2810905;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel ; /* by hand */
            il->nelt = 9;
            il->lel = 1038; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;
   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"E1",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"RM",LSNAME); /*hand*/
            xhdr->numconns= 256;          /* by hand */
            xhdr->len =     4;          /* by hand */
            xhdr->numbits = 8;         /* by hand */
            xhdr->xkgen =   0x40402;        /* by hand */
            xhdr->src_ngx = 16;      /* by hand */
            xhdr->src_ngy = 16;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 256;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"NL",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"NL",LSNAME); /*hand*/
            xhdr->numconns= 1;          /* by hand */
            xhdr->len =     4;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x2102;        /* by hand */
            xhdr->src_ngx = 1;      /* by hand */
            xhdr->src_ngy = 1;      /* by hand */
            xhdr->src_nel = 1;      /* by hand */
            xhdr->src_nelt= 1;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/*          FOR EACH CONNTYPE in savenet file */
         if (n_fread(oldfdes,(char*)seeds,
                     sizeof(struct SEEDBLK),0,0) < 0)
                abexit(SAVRES_ERR);
     strncpy(xhdr->srcsnam,"RR",LSNAME); /*hand*/
     strncpy(xhdr->srclnam,"RR",LSNAME); /*hand*/
            xhdr->numconns= 2;          /* by hand */
            xhdr->len =     4;          /* by hand */
            xhdr->numbits = 4;         /* by hand */
            xhdr->xkgen =   0x2102;        /* by hand */
            xhdr->src_ngx = 1;      /* by hand */
            xhdr->src_ngy = 1;      /* by hand */
            xhdr->src_nel = 9;    /* by hand */
            xhdr->src_nelt= 9;     /* by hand */
            xhdr->c_seed =  seeds->cseed;       /* by hand */
            xhdr->l_seed =  seeds->lseed;       /* by hand */
            xhdr->n_seed =  seeds->nseed;       /* by hand */
            xhdr->r_seed =  seeds->rseed;       /* by hand */
            xhdr->n_seed =  ix->nseed;       /* by hand */
            xhdr->r_seed =  ix->rseed;       /* by hand */
            xhdr->off_prd = ilhdr->offprd;       /* by hand */
   n_fwrite (fdes,xhdr,sizeof(struct CONNHDR),0,0);

/* CELLTYPE X1  - zero CONNTYPE */
/* Now skip over the data to the next header -  */
            size = il->nelt + il->lel*il->nelt;
            sizes[count++] = size;

printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);
            while (wsize = min(size,MAX_MSG_LENGTH)) {
               n_fread(oldfdes,junk,wsize,0,0);  
               size -= wsize;
               }
/* READ NEXT HEADER */
         if (n_fread(oldfdes,(char*)oldhdr,
                     sizeof(struct OLDSAVBLK),0,0) < 0)
                abexit(SAVRES_ERR);



    strncpy(ilhdr->repnam,"X1",LSNAME); /*hand*/
    strncpy(ilhdr->celnam,oldhdr->typenm,LSNAME);
         ilhdr->rngx    = 1;       /* by hand */
         ilhdr->rngy    = 1;       /* by hand */
         ilhdr->celgrp  = oldhdr->celgrp;
         ilhdr->xperel  = oldhdr->xperel;
         ilhdr->lencel  = oldhdr->lencel;
         ilhdr->lenel   = oldhdr->lenel;
         ilhdr->lenlay  = oldhdr->lenlay;
         ilhdr->nseed   = 3121991;       /* by hand */
         ilhdr->pseed   = il->phiseed;   /* by hand */
         ilhdr->xtype   = oldhdr->xtype;
         ilhdr->pshft   = 0;             /* by hand */
         ilhdr->offsi   += il->nelt + il->nelt*il->lel ; /* by hand */
            il->nelt = 1;
            il->lel = 0; 
         ilhdr->offprd  = ilhdr->offsi + il->nelt;    /* by hand */
         ilhdr->cbits[0]  = oldhdr->cbits[0];
         ilhdr->cbits[1]  = oldhdr->cbits[1];
         ilhdr->cbits[2]  = oldhdr->cbits[2];
         ilhdr->pad     = 0;
   n_fwrite (fdes,ilhdr,sizeof(struct CELHDR),0,0);

            size = il->nelt + il->lel*il->nelt;
            sizes[count++] = size;
printf("offsi,offprd,size = %d,%d,%d\n",ilhdr->offsi,ilhdr->offprd,size);
fflush(stdout);


/* position old file at beginning
for each header in old file
   read header
   read data
   write data
endfor
*/

/* Position old file at beginning, and go through again,
 * writing out the si and prd data this time */
   n_fseek (oldfdes, 0);

for (ii=0; ii< 17; ii++) {
/* READ HEADER */
   if (n_fread(oldfdes,(char*)oldhdr,
               sizeof(struct OLDSAVBLK),0,0) < 0)
          abexit(SAVRES_ERR);

/* READ SEEDS */
   for (i = 0; i<oldhdr->xtype; i++) {
      if (n_fread(oldfdes,(char*)seeds,
                  sizeof(struct SEEDBLK),0,0) < 0)
         abexit(SAVRES_ERR);
      }

/* READ AND WRITE DATA */
   size = sizes[ii];  /* = il->nelt + il->nelt*il->lel */
   while (wsize = min(size,MAX_MSG_LENGTH)) {
      n_fread(oldfdes,junk,wsize,0,0);  
      n_fwrite (fdes,junk,wsize,0,0);
      size -= wsize;
      }
   }
   n_fclose(fdes);
   } /* End of whole routine */

