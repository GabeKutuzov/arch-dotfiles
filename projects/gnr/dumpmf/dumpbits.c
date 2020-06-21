/* (c) Copyright 20xx, The Rockefeller University *nnnnn* */
/*********************************************************************
*                                                                    *
* SYNOPSIS:                                                          *
*    int extnum(char *str, int i)                                    *
*                                                                    *
*    void dmpsub(ui16 tvar, void *var, ui32 lvar, char *fmt,         * 
*     ui32 i, ui32 j, ui16 ofs) {                                    *
*                                                                    *
*    void dumpbits(MFS *s, char *fmt, ...)                           *
*                                                                    *
* DESCRIPTION:                                                       *
*    The purpose of this file is to dump a character string          *
*    with proper formatting as per specification.                    *
*    The way strings are formatted is slightly similar to the        *
*    way printf/sprintf() handle it. Whenever a '%' character is     *
*    reached, the following character is checked to determine the    *
*    type of parameter encoding that is to be read from the MFS      * 
*    filepointer. If a '`' key is preceeding the alphabetical        *
*    character, the next "variadic" parameter's value will be        *
*    updated to the decoded value.                                   *
*                                                                    *
*     'k<n>'   - unsigned integer                                    *
*     's'      - signed integer                                      *
*     'x,y'    - coordinate                                          *
*     'a'      - angle                                               *
*     'g'      - general                                             *
*     'r,w,h'  - size                                                * 
*     'c<n>'   - character                                           *
*     'j'      - bitmap value                                        *
*                                                                    *
*    The function 'extnum' is used to read the following number      *
*    of a character "e.x. k<n>" where the value of <n> is returned.  *
*                                                                    *
*    The function dmpsub() is used as a helper function to           *
*    get the substring from the 'fmt' text and concatenate the       *
*    value of the parameter.                                         *
*                                                                    *
ARGUMENTS:                                                           *
*  mfs      Pointer to metafile shared data.                         *
*  fmt      Pointer to formatted string.                             *
*  ...      Variadic parameters (should be addresses) that are       *
*           updated when value is requested.                         *
*                                                                    *
**********************************************************************
*  Version 1, 00/00/19, GMK                                          *
*********************************************************************/

/* TODO: Fix sign-bit to actually act like a sign-bit. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>

#include "sysdef.h"
#include "readmf.h"

#define strnxti(n) readbitsi(mfs, n)
#define strnxtf(n) readbitsf(mfs, n)

/*=====================================================================*
*                              extnum()                                *
*   Return number from a string at a given index. If no number is      *
*   found, return 0. Otherwise, return the extracted value. update     *
*   value of pointer 'dig' (count of digits).                          *
*=====================================================================*/

int extnum(char *str, int i, ui32 *dig) {

   int digits = 0; /* amount of digits in number */
   char *numstr = malloc(1);
   while (str[i] >= '0' && str[i] <= '9') {
      numstr = realloc(numstr,digits+1);
      numstr[digits] = str[i];
      digits++;
      i++;
   }
   if(digits > 0) {
      uintmax_t num = strtoumax(numstr, NULL, 10);
      *(dig) = digits;
      free(numstr);
      return num;
   }
   free(numstr);
   return 0;
} /* End of extnum */

/*=====================================================================*
*                              dmpsub()                                *
*  Print parameter info by substring from the formatted text.          *
*=====================================================================*/

void dmpsub(ui16 tvar, void *var, ui32 lvar, char *fmt, 
      ui32 i, ui32 j, ui16 ofs) {
   
   char *msg;
   int sov = j-ofs;
   msg = malloc(sizeof(byte)+lvar+sov);

   memcpy(msg,fmt+(i-j)+ofs,sov);

   switch (tvar) {
      case UIVAL:
      sprintf(msg+sov,"%d",*((ui32*)var));
      break;
      case SIVAL:
      sprintf(msg+sov,"%x",*((si32*)var));
      break;
      case FLVAL:
      sprintf(msg+sov,"%.2f",*((float*)var));
      break;
      case STVAL:
      sprintf(msg+sov,"%s",(char*)var);
      break;
   }

   fwrite(msg,1,j+lvar-ofs,stdout);
   free(msg);
}

/*=====================================================================*
*                              dumpbits()                              *
*  Formatted printing for plotting commands.                           *
*=====================================================================*/

void dumpbits(MFS *mfs, char *fmt, ...) {

   Window win;     /* Data from curent window             */
   Frame frm;      /* Data from current viewport          */
   ui16 cfrm,cwin; /* Index for current viewport, window  */

   ui16 chk;
   ui32 i,j,wp,ofs,dgts;
   int nums;
   int flen;

   si32 sival;
   ui32 uival;
   ui32 nclen0,nclen; /* length of character string */
   float fval;
   char *strval;
   char cchar; /* Current/checking character */

   va_list ap;

   cwin = mfs->nodes[mfs->cnode].cwin;
   win  = mfs->windows[cwin];
   cfrm = win.cframe;
   frm  = win.frames[win.cframe];

   i=j=wp=ofs=dgts = 0;
   flen = strlen(fmt);
   va_start(ap,fmt);

   for (i=0; i<flen; i++) {
      dgts = 0; /* Reset digits for next offset */
      if (fmt[i] == '%') {
         wp = 0;
         if (fmt[i+1] == '`') {
            wp = 1;
         }
         cchar = fmt[i+wp+1]; /* Update current character */
         /* Character after reference '%' and reference check */
         switch (cchar) {
         /* Singned integer parameter */
         case ('s'):
         chk = strnxti(2);
         switch (chk) {
            case 0x00:
            sival = (si32)(chk+strnxti(6));
            break;
            case 0x01:
            sival = (si32)(chk+strnxti(14));
            break;
            case 0x10:
            sival = (si32)(chk+strnxti(22));
            break;
            case 0x11:
            sival = (si32)(chk+strnxti(30));
            break;
         }
         /* print message here */
         nclen0 = abs(sival);
         break;
         /* Unsigned integer parameter */
         case ('k'):
         nums = extnum(fmt,i+wp+2,&dgts); /* Check following number */
         if(nums > 0) {
            uival = strnxti(nums);
         }
         else {
            chk = strnxti(2);
            switch (chk) {
            case 0x00:                         /* Type 00 integer */
               uival = (ui32)(chk+strnxti(6));
               // chk fix (not actual addition here) //
               break;
            case 0x01:                         /* Type 01 integer */
               uival = (ui32)(chk+strnxti(14));
               break;
            case 0x10:                         /* Type 10 integer */
               uival = (ui32)(chk+strnxti(22));
               break;
            case 0x11:                         /* Type 11 integer */
               uival = (ui32)(chk+strnxti(30));
               break;
            }
         }
         /* Check for next value update */
         if(wp == 1) *(va_arg(ap,ui32*)) = uival;

         /* Print formatted message */
         dmpsub(UIVAL,&uival,sizeof(uival),fmt,i,j,ofs);
         j = 0;
         ofs = wp+dgts+2; /* offset characters for next message     */
         nclen0 = uival;  /* update nclen0 for string param. length */
         break;

         /* Coordinate parameter */
         case ('x'):
         case ('y'):
            chk = strnxti(2);
            switch (chk) {
            case 0x00:
               if (cchar=='x') fval = (frm.x += strnxtf(mfs->lcf)+1);
               else            fval = (frm.y += strnxtf(mfs->lcf)+1);
               break;
            case 0x01:
               if (cchar=='x') fval = (frm.x -= strnxtf(mfs->lcf)-1);
               else            fval = (frm.y -= strnxtf(mfs->lcf)-1);
               break;
            case 0x10:
               if (cchar=='x') fval = frm.px;
               else            fval = frm.py;
               break;
            case 0x11:
               fval = (strnxti(1)+strnxti(mfs->lci)+strnxtf(mfs->lcf));
               break;
         }
         if (cchar=='x') frm.px = fval;
         else            frm.py = fval;

         /* Check for next value update */
         if(wp == 1) *(va_arg(ap,float*)) = fval;
         /* Print formatted message */
         dmpsub(FLVAL,&fval,sizeof(fval),fmt,i,j,ofs);
         /* Prepare for next cycle */
         j = 0;
         ofs = wp+2;
         break;

         /* Size parameter */
         case ('r'):
         case ('w'):
         case ('h'):
            chk = strnxti(2);
            switch (chk) {
            case 0:
            case 1:
               if (cchar=='r') 
                  fval = (frm.r += strnxtf(mfs->lcf)+1);
               else if(cchar=='w')
                  fval = (frm.w += strnxtf(mfs->lcf)+1);
               else if(cchar=='h')
                  fval = (frm.h += strnxtf(mfs->lcf)+1);
               break;
            case 2:
               if (cchar=='r')
                  fval = frm.px;
               else if (cchar=='w')
                  fval = frm.pw;
               else if (cchar=='h')
                  fval = frm.ph;
               break;
            case 3:
               fval = (strnxti(mfs->lci)+strnxtf(mfs->lcf));
               break;
         }
         if      (cchar=='r') frm.pr = fval;
         else if (cchar=='w') frm.pw = fval;
         else if (cchar=='h') frm.ph = fval;

         /* Check for next value update */
         if(wp == 1) *(va_arg(ap,float*)) = fval;
         /* Print formatted message */
         dmpsub(FLVAL,&fval,sizeof(fval),fmt,i,j,ofs);
         /* Prepare for next cycle */
         j = 0;
         ofs = wp+2;
         break;
         /* Angle parameter */
         case ('a'):
            fval = strnxtf(4);
            /* Check for next value update */
            if(wp == 1) *(va_arg(ap,float*)) = fval;
            /* print formatted here */
            dmpsub(FLVAL,&fval,sizeof(fval),fmt,i,j,ofs);
            j = 0;
            ofs = wp+2;
            break;
         /* General number */
         case ('g'):
            chk = strnxti(1);
            if (chk == 0) {
               chk = strnxti(1);
               if (chk == 0) {   /* 0b00 */
                  fval = 0;
               }
               else {
                  chk = strnxti(1);
                  switch (chk) {
                     case 0:        /* 0b010 */
                     fval = 1;
                     break;
                     case 1:        /* 0b011 */
                     fval = -1;
                     break;
                  }
               }
            }
            else {
               chk = strnxti(1);
               if (chk == 0) {      /* 0b10 */
                  fval = strnxtf(16);
               }
               else {               /* 0b11 */
                  chk = strnxti(1);
                  switch(chk) {
                     case 0:        /* 0b110 */
                     fval = strnxti(1)+strnxti(8)+strnxtf(16);
                     break;
                     case 1:        /* 0b111 */
                     fval = strnxti(1)+strnxti(20)+strnxtf(16);
                     break;
                  }
               }
            }
            /* Check for next value update */
            if(wp==1) *(va_arg(ap,float*)) = fval;
            /* Print formatted message */
            dmpsub(FLVAL,&fval,sizeof(fval),fmt,i,j,ofs);
            /* offset and cleanup for next run */
            j = 0;
            ofs = wp+2;
            break;
         /* Bitmap value */
         case ('j'):
         break;
         /* Character-string parameter */
         case ('c'):
         /* Get length of character string */
         if (fmt[i+wp+2] == 'n') nclen = nclen0;
         else nclen = extnum(fmt,i+wp+2,&dgts);

         strval = readbytes(mfs,nclen);

         #if 0
         /* Check for next value update */
         if(wp==1) {
           /* va_arg(ap,char*) = malloc(sizeof(nclen)); */
           *(va_arg(ap,char*)) = (char*)strval;
         } 
         #endif
         /* Print message here */
         dmpsub(STVAL,strval,nclen,fmt,i,j,ofs);
         free(strval);
         j = 0;
         ofs = wp+2;
         break;
         }
      }

      j++;

   }  /* End of for loop */

   va_end(ap);
} /* End of dumpbts */
