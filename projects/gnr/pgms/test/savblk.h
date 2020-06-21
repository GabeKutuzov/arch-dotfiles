/* (c) Copyright 1991, The Neurosciences Research Foundation */
/***********************************************************************
*                         SAVBLK Header File                           *
*                                                                      *
*  Header data block for SAVENET file                                  *
*  Written, 12/14/90, M. Cook                                          *
*                                                                      *
***********************************************************************/

struct OLDSAVBLK {
   char repnam[D3NAM_LENGTH];    /*   ir->name     */
   long rgrp;                    /*   ir->ngrp     */ 
   char typenm[D3NAM_LENGTH];    /*   il->lname    */
   short xnit;                   /*   il->nit      */
   short xtype;                  /*   il->nct      */
   long xperel;                  /*   il->nce      */
   long lencel;                  /*   il->ls       */
   long lenel;                   /*   il->lel      */
   long lenlay;                  /*   il->llt      */
   long celgrp;                  /*   il->nel      */
   byte cbits[(NDVARS+7)/8];     /*   il->bits     */
   } *oldhdr;

