/* (c) Copyright 1990-2002, The Rockefeller University *11115* */
/* $Id: dials.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                                                                      *
*                               DIALS.H                                *
*                                                                      *
***********************************************************************/

/* This file defines the C-language interface to the DIALS.ASM package
   of routines for using the GNR/RU dials box with serial interface.

   V1A, 10/06/90, GNR - Newly written
   V1B, 12/06/90, GNR - Add dialrst prototype
*  ==>, 07/01/02, GNR - Last date before committing to svn repository
*/

#define NDIALS 8              /* Number of dials available */

/* Global variables: */

extern int DIALCNTS[NDIALS];

/* Function prototypes: */

void dialopn(void);           /* Install interrupt handler */
int dialval(int i);           /* Read a single dial */
int dialwait(void);           /* Wait for next dial change */
int  dialrst(void);           /* Check for reset button */
void dialdis(void);           /* Disable dials temporarily */
void dialenb(void);           /* Reenable after dialdis */
void dialcls(void);           /* Remove interrupt handler */


