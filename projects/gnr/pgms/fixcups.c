/* (c) Copyright 2007, The Rockefeller University *11113* */
/***********************************************************************
*                               fixcups                                *
*                                                                      *
*  Synopsis:  fixcups                                                  *
*                                                                      *
*  This program deals with an apparent problem with the vmware         *
*  installation on this system:  it adds a line                        *
*  "Listen 192.168.86.1:631"                                           *
*  near the end of the file /etc/cups/cupsd.conf.                      *
*  This line prevents cups from starting during boot up because        *
*  that IP address, which is part of the virtual vmnet1, does not      *
*  yet exist.  But the vmnet must start late in boot up due to other   *
*  dependencies.  This program runs from rc.local and just removes     *
*  the offending line before it can cause trouble.                     *
*                                                                      *
*  This is a real "quick-and-dirty" -- everything is hard-wired and    *
*  there is no error checking.                                         *
*                                                                      *
*  New program, 03/29/07, G.N. Reeke                                   *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <memory.h>

int main(void) {

/*   static char ccfname[] = "/etc/cups/cupsd.conf";  */
   static char ccfname[] = "testfile";
   static char lookfor[] = "Listen 192.168";
   static char tfname[] = "fixcupsXXXXXX";
   FILE *fcc;                 /* Ptr to cups conf file info */
   FILE *ffx;                 /* Ptr to temporary output file */
   char ttfname[20];          /* Ptr to temp file name */
   char line[1000];           /* Plenty for one line? */

/* Open the files */

   fcc = fopen(ccfname, "r");
   strcpy(ttfname, tfname);   /* Make a writeable string */
   ffx = fdopen(mkstemp(ttfname), "w+");

/* Copy until eof, deleting the bad line */

   while (fgets(line, 998, fcc)) {
      if (memcmp(line, lookfor, 14))
         fputs(line, ffx);
      }

/* At the end, rename the temp file over the old file */

   fclose(fcc);
   fclose(ffx);
   rename(ttfname, ccfname);

   return 0;
   } /* End fixcups */
