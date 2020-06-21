/* (c) Copyright 2017, The Rockefeller University *11113* */
/***********************************************************************
*                               keyedit                                *
*                                                                      *
*  Synopsis:  keyedit [-c] [-d] [-e] [-i] [-v] [-y] [-] path1 path2    *
*                                                                      *
*  This program may be used to add or remove acknowledgment and        *
*  license statements from source code files to avoid having the       *
*  files cluttered with those statements during normal maintenance.    *
*  Comment lines filled with hyphens will be inserted/removed before   *
*  and after the inserted text.                                        *
*                                                                      *
*  The possible messages are included in the source code of this key-  *
*  edit program.  The desired texts are specified at the end of the    *
*  copyright statement in line 1 with the following codes:             *
*     ...copyright; *alpqn*    where:                                  *
*  a is an acknowledgment message identifier:                          *
*     0 = No acknowledgment message.                                   *
*     1 = Standard GNR/NRF/RU message.                                 *
*  l is a license identifier number:                                   *
*     0 = No license.                                                  *
*     1 = standard GPU license statement.                              *
*  p = punctuation code for left of each line:                         *
*     1 = '*  '                                                        *
*     2 = '#  '                                                        *
*     3 = '%  '                                                        *
*     4 = '/* '                                                        *
*     5 = '// '                                                        *
*     6 = ';'                                                          *
*  q = punctuation code for right end of each line:                    *
*     0 = Do not add anything at end of each line.                     *
*     1 = '*' in col. 72.                                              *
*     2 = '#' in col. 72.                                              *
*     3 = '%' in col. 72.                                              *
*     4 = '*' in col. 71, '/' in col. 72.                              *
*  n = number of lines to skip to reach insertion point of text.       *
*                                                                      *
*  Additional codes may be added in each category as needed.           *
*                                                                      *
*  Switches:                                                           *
*     -c    Contract (remove) the text from each file.                 *
*     -d    Suppress the warning that would normally be issued when    *
*           <path1> contains one or more bare directories.             *
*     -e    Expand (insert) the text into each file (default).         *
*     -i    Convert files in place, i.e. write the converted output    *
*           over the input files.  In this case, all arguments are     *
*           taken to be files or directories to be converted, and no   *
*           <path2> argument is expected.  When the -i switch is not   *
*           given, it is an error to write over an input file except   *
*           when there is only a single argument (see below).          *
*     -v    Print the names of all files converted.                    *
*     -y    Internally supply a "yes" answer to queries normally       *
*           issued to the user regarding overwriting output files.     *
*     -     Indicates end of option switches, allowing first entry     *
*           in <path1> to begin with a hyphen.                         *
*                                                                      *
*  Arguments:                                                          *
*     path1 specifies the file(s) to be converted.  It may be any one  *
*           or more of the following:                                  *
*           (1) full path name--convert just that one file.            *
*           (2) directory name--convert all files in that directory.   *
*               (Ask user's permission unless -d is specified.)        *
*           (3) names with wildcards--wildcards are expanded under     *
*               UNIX by shell globbing.                                *
*     path2 specifies the location of the output file(s).  The last    *
*           argument must always be <path2> unless the -i switch has   *
*           been specified.  <path2> is normally a directory, but if   *
*           <path1> specifies a single file, then <path2> may be a     *
*           full path name (makes the file as specified) or a file     *
*           name only (makes named file in current directory).         *
*                                                                      *
*  Special case:                                                       *
*     If only a single argument is given, then pgm assumes the <path1> *
*     file or directory is to be converted in place.  If the -i switch *
*     has not been entered, the program asks permission before copying *
*     the input over itself.                                           *
*                                                                      *
************************************************************************
*  Initial version, 01/10/17, G.N. Reeke, code from d2u-u2d.c, no DOS  *
***********************************************************************/

#define MAIN

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <regex.h>
#include "sysdef.h"
#include "rocks.h"
#include "rksubs.h"
#include "rfdef.h"

/* Configuration parameters */
#define INPB_MAX     8        /* Max size of response */
#ifndef PATH_MAX
#define PATH_MAX   255        /* Max path name length */
#endif
#define MYBUFLEN  4096        /* I/O buffer size */

/* System definitions */
#define KEYLEN       7        /* Number of characters expected in
                              *  key string.  */
#define STDLLEN     72        /* Standard source code line length--
                              *  could be cmdline param if needed */
#define PSEP '/'
#define ATIME        0        /* Position of atime in oldtimes[] */
#define MTIME        1        /* Position of mtime in oldtimes[] */

/* Define option flags */
#define OP_CONTR     1        /* Contract text in input files */
#define OP_DIROK     2        /* Suppress directory warning */
#define OP_INPLACE   4        /* Convert files in place */
#define OP_VERBOSE   8        /* Print names of files converted */
#define OP_AUTOYES  16        /* Answer "yes" to most questions */
#define OP_RQDIR    32        /* Path has directory form */
#define OP_ISDIR    64        /* Output path is a directory */

/* Define error exits */
#define XPERM_ERR  701        /* Negative response to getok() */
#define NOSRC_ERR  702        /* Too few arguments */
#define BADSW_ERR  703        /* Bad command-line switch */
#define NOWRT_ERR  704        /* No write permission for output */
#define PATHL_ERR  705        /* Path name is too long */
#define MKDIR_ERR  706        /* Unable to create output directory */
#define EXDIR_ERR  707        /* Nonexistent directory specified */
#define MAXIN_ERR  708        /* Too many inputs */
#define HOMEX_ERR  709        /* Unable to expand home directory */
#define INACC_ERR  710        /* Unable to access input file */
#define MATCH_ERR  711        /* No input found matching file spec */
#define SAMEF_ERR  712        /* Input and output files are same */
#define RENAM_ERR  713        /* Unable to rename temp to input fn */
#define FTIME_ERR  714        /* Unable to get old timestamp */
#define STIME_ERR  715        /* Unable to reset timestamp */
#define OGCWD_ERR  716        /* Unable to get initial cwd */
#define NOCWD_ERR  717        /* Unable to getcurdir on O/P drive */
#define BADFN_ERR  718        /* Illegally composed path name */
#define OPNID_ERR  719        /* Unable to open input directory */
#define READD_ERR  720        /* Error reading input directory */
#define PARSE_ERR  721        /* Something wrong in parsing logic */
#define SYSEX_ERR  722        /* Unable to execute a system program */
#define REGEX_ERR  723        /* Problem parsing key search */

/* Global data */
/* Note:  rfopen() is called without rfallo() -- these RFdefs
*  must be initialized to all zeros -- done by making them global. */
static struct RFdef inrf;         /* Input crk file descriptor */
static struct RFdef outrf;        /* Output crk file descriptor */
static regex_t keyrex;            /* Compiled regex for key */
static int   kop;                 /* Option codes */
static int   nconv;               /* Number of files converted */
static char  cwd0[PATH_MAX+1];    /* Working directory on entry */
static char  dir1[PATH_MAX+1];    /* Input directory (w/PSEP) */
static char  file1[PATH_MAX+1];   /* Input file */
static char  dir2[PATH_MAX+1];    /* Output directory (w/PSEP) */
static char  file2[PATH_MAX+1];   /* Output file */
static char  lbuff[CDSIZE];       /* Generous space for input lines */

/* Here are the selectable acknowledgment and license texts */
static char *acktxt1[] = {
   "This software was written by George N. Reeke in the Laboratory of",
   "Biological Modelling at The Rockefeller University with support",
   "from The Neurosciences Research Foundation and The Rockefeller",
   "University.  Please send any corrections or suggestions to the",
   "author by email to reeke@rockefeller.edu for possible inclusion",
   "in future revisions.",
   "  "
   };
static char *acktxt2[] = {
   "This software was written by or under the direction of George N.",
   "Reeke in the Laboratory of Biological Modelling at The Rockefeller",
   "University with support from The Neurosciences Research Foundation",
   "and The Rockefeller University.  Please send any corrections or",
   "suggestions to the author by email to reeke@rockefeller.edu for",
   "possible inclusion in future revisions.",
   "  "
   };
static char *lictxt1[] = {
   "This software is distributed under GPL, version 2.  This program is",
   "free software; you can redistribute it and/or modify it under the",
   "terms of the GNU General Public License as published by the Free",
   "Software Foundation; either version 2 of the License, or (at your",
   "option) any later version. Accordingly, this program is distributed",
   "in the hope that it will be useful, but WITHOUT ANY WARRANTY; with-",
   "out even the implied warranty of MERCHANTABILITY or FITNESS FOR A",
   "PARTICULAR PURPOSE.  See the GNU General Public License for more",
   "details. You should have received a copy of the GNU General Public",
   "License along with this program.  If not, see",
   "<http://www.gnu.org/licenses/>."
   };
static char *lfttxt[] = { "", "*  ", "#  ", "%  ", "/* ", "// ", ";" };
static char *rgttxt[] = { "", "*", "#", "%", "*/" };

/*=====================================================================*
*                          Path length exit                            *
*=====================================================================*/

static void plexitm(char *path) {
   fputs("\n", stderr);
   fputs(path, stderr);
   abexitm(PATHL_ERR, "Above path name is too long");
   } /* End plexitm() */

/*=====================================================================*
*                Get user's permission to do something                 *
*=====================================================================*/

static int getok(char *msg) {
   char *pnl;              /* Ptr to newline */
   char inbuf[INPB_MAX];
   if (kop & OP_AUTOYES) return YES;
   fputs(msg, stdout);
   fflush(stdout);
   fgets(inbuf, INPB_MAX, stdin);
   if (pnl = strrchr(inbuf, '\n'))  /* Assignment intended */
      *pnl = '\0';
   if (ssmatch(inbuf, "ok", 1)) return YES;
   if (ssmatch(inbuf, "yes", 1)) return YES;
   return NO;
   } /* End getok() */

/*=====================================================================*
*        Split output path into file and directory components          *
*=====================================================================*/

static void splitpw2(char *pp) {

   char *psep = strrchr(pp, PSEP);  /* Ptr to dir/file separator */
   int  len;                        /* Length of name component */
   if (psep) {
      len = psep - pp;
      memcpy(dir2, pp, len);        /* Copy directory */
      dir2[len] = '\0';
      psep += 1;
      strcpy(file2, psep);          /* Copy file name */
      }
   else
      abexitm(PARSE_ERR, "Internal logic error, check source code");
   } /* End splitpw2() */

/*=====================================================================*
*                             Expand path                              *
*                                                                      *
*  This routine inserts the original working directory in the path     *
*  name if no directory was specified, and inserts the original        *
*  working directory or one of its parents if a string of one or more  *
*  dots was specified.  This is done on input as well as output,       *
*  because earlier arguments may change the current working directory. *
*  If a string of dots is not the first element in the path name, no   *
*  special test is done and the error should be detected later.        *
*=====================================================================*/

static void expandPath(char *argpath, char *exppath) {

   char *epe = exppath;       /* End of reconstructed path */
   char *psep;                /* Location of a separator */
   int lep = PATH_MAX;        /* Size remaining in output area */
   int len;                   /* Length of a string */

/* Check first for an initial string of dots, as these
*  supersede tests for drive and absolute directory.  */

   if (argpath[0] == '.') {
      /* Under UNIX, a long string of dots might be part of a file
      *  name, in which case we should get out of this code.  Under
      *  DOS, this type of filename would be ill-constructed.  */
      for (psep=argpath; psep[1] == '.'; ++psep) ;
      if (psep[1] == PSEP || psep[1] == '\0') {
         len = strlen(cwd0);
         while (*++argpath == '.') {
            cwd0[len-1] = '\0';  /* Temporarily delete final separator */
            psep = strrchr(cwd0, PSEP);
            cwd0[len-1] = PSEP;
            if (psep)
               len = psep - cwd0 + 1;
            else
               abexitm(EXDIR_ERR, "Too many dots in directory string");
            }
         if (len > lep) plexitm(cwd0);    /* JIC */
         memcpy(epe, cwd0, len);
         epe += len, lep -= len;
         if (argpath[0] == PSEP) ++argpath;
         }
      else {
         /* Absolute dir is missing, use cwd0 */
         if ((len = strlen(cwd0)) > lep) plexitm(cwd0);
         strcpy(epe, cwd0);
         epe += len, lep -= len;
         }
      }

/* Name does not start with a string of dots or a squiggle */

   else {
      /* Be sure the absolute directory is specified */
      *epe = '\0';   /* In case used in err msg */
      if (argpath[0] != PSEP) {
         /* Absolute dir is missing, use cwd0 */
         if ((len = strlen(cwd0)) > lep) plexitm(cwd0);
         strcpy(epe, cwd0);
         epe += len, lep -= len;
         }
      }

   /* Append the rest of the output specification */
   if ((len = strlen(argpath)) > lep) plexitm(argpath);
   strcpy(epe, argpath);
   epe += len, lep -= len;

   /* Remove any final separator indicating a directory,
   *  as this prevents stat() from working, at least in DOS.  */
   if (epe[-1] == PSEP) {
      *--epe = '\0';
      kop |= OP_RQDIR; }

   } /* End expandPath() */

/*=====================================================================*
*                   Perform one actual file rewrite                    *
*                                                                      *
*  Assumptions:                                                        *
*  (1) fn is the name of the input file (sans path).                   *
*  (2) This file is located in directory dir1.                         *
*  (3) If working in place (-i mode), the output file replaces the     *
*      input file.  Otherwise, dir2 is the name of the directory       *
*      where the output is to be written.                              *
*  (4) If the OP_ISDIR flag is set, the name of the output file is     *
*      made to be the same as that of the input file.  Otherwise,      *
*      file2 is the name of the output file and do1file makes sure     *
*      this output file will not be written more than once.            *
*                                                                      *
*  Returns TRUE if file was converted, otherwise FALSE                 *
*=====================================================================*/

static int do1file(char *fn) {

   struct stat instat;        /* Input file status */
   struct stat outstat;       /* Output path status */
   regmatch_t keyloc;         /* Key string match info */
   time_t oldtimes[2];        /* Old access, modify times */
   char **pack,**plic;        /* Ptrs to selected ack/lic texts */
   char *plft,*prgt;          /* Ptrs to selected end texts */
   char *pkey;                /* Ptr to key */
   long lstr;                 /* Length of a text string */
   int  ka,kl,kp,kq,nn;       /* Interpreted key codes */
   int  kopn;                 /* Mode of opening input file */
   int  llft,lrgt;            /* Lengths of left,right strings */
   int  nack,nlic;            /* Rows in ack,lic texts */
   char ipath[PATH_MAX+1];    /* Full input path name */
   char opath[PATH_MAX+1];    /* Full output path name */

/* Generate the fully expanded input path name for testing equality
*  with output file.  Get status information on the input file,
*  including timestamps.  Attempt to open the file.  If this fails,
*  skip the file.  */

   if (strlen(dir1) + strlen(fn) > PATH_MAX) plexitm(fn);
   strcpy(ipath, dir1);
   strcat(ipath, fn);
   kopn = (kop & OP_INPLACE) ? READWRITE : READ;
   if (stat(ipath, &instat) || !rfopen(&inrf, ipath, kopn, BINARY,
         SEQUENTIAL, TOP, LOOKAHEAD, REWIND, RETAIN_BUFF, MYBUFLEN,
         IGNORE, IGNORE, NO_ABORT)) {
      puts(ssprintf(NULL, "Skipping %s, unable to access.", ipath));
      return FALSE;
      }
   oldtimes[ATIME] = instat.st_atime;
   oldtimes[MTIME] = instat.st_mtime;

/* If converting in place, use temp file created in main program.
*  If output is to a directory, make output file name same as
*  input.  Otherwise, use given file name.  Terminate if just
*  one output file was specified and it has already been written.
*/

   if (kop & OP_INPLACE)         /* Converting in place */
      goto ProcessInputFile;
   else if (kop & OP_ISDIR) {    /* Output to a directory */
      if (strlen(dir2) + strlen(fn) > PATH_MAX) plexitm(dir1);
      strcpy(opath, dir2);
      strcat(opath, fn);
      }
   else if (nconv > 0)           /* > 1 output to same file */
      abexitm(MAXIN_ERR, "Attempted to convert more than one"
         " input file to a single output file.");
   else {                        /* Output to full path */
      if (strlen(dir2) + strlen(file2) > PATH_MAX) plexitm(dir1);
      strcpy(opath, dir2);
      strcat(opath, file2);
      }

/* If the input and output file names are the same, abort.  */

   if (!strcmp(ipath, opath))
      abexitm(SAMEF_ERR, "Use -i to convert a file in place.");

/* If the output file already exists, an attempt can be made to detect
*  whether it is the same as the input using rfqsame.  In order not to
*  destroy it, we first open it for input, do the rfqsame test, then
*  close it and reopen it for output.  If the file cannot be opened,
*  or it is the same as the input file, abort.
*
*  Note:  The test for equality of names performed above is expected
*  to work most of the time since the names have all been fully
*  expanded.  */

   if (!stat(opath, &outstat)) {
      rfopen(&outrf, opath, READ, BINARY, SEQUENTIAL, TOP, NO_LOOKAHEAD,
         REWIND, RETAIN_BUFF, MYBUFLEN, IGNORE, IGNORE, ABORT);
      if (rfqsame(&inrf, &outrf)) {
         fputs(ssprintf(NULL,
            "Input and output files %s are the same\n", opath), stderr);
         fputs("  in spite of having different names.\n", stderr);
         abexit(SAMEF_ERR);
         }
      rfclose(&outrf, REWIND, RETAIN_BUFF, ABORT);
      } /* End checking of existing output files */

/* Read the first line of the input file and interpret the key code.
*  This first version code assumes the five elements are all single-
*  digit integers.  This can be extended to include other codes as
*  needed by changing the regexp string and the parsing here.  */

ProcessInputFile:
   lstr = rfgets(&inrf, lbuff, CDSIZE, ABORT);
   if (lstr == 0) return FALSE;
   if (regexec(&keyrex, lbuff, 1, &keyloc, 0)) {
      /* The key code is not there.  Just leave this file alone */
      return FALSE;
      }
   if (keyloc.rm_eo - keyloc.rm_so != KEYLEN)
      abexitm(REGEX_ERR, "Key string in file is not " qqvar(KEYLEN)
         "characters.");
   /* OK to proceed, now open output file if not temp */
   if (!(kop & OP_INPLACE))
      rfopen(&outrf, opath, WRITE, BINARY, SEQUENTIAL, TOP, LOOKAHEAD,
         REWIND, RETAIN_BUFF, MYBUFLEN, IGNORE, IGNORE, ABORT);
   pkey = lbuff + keyloc.rm_so;
   if (!isdigit(pkey[1]) || !isdigit(pkey[2]) || !isdigit(pkey[3]) ||
         !isdigit(pkey[4]) || !isdigit(pkey[5]))
      abexitm(PARSE_ERR, "Key string has an unexpected character.");
   ka = pkey[1] - '0';
   kl = pkey[2] - '0';
   kp = pkey[3] - '0';
   kq = pkey[4] - '0';
   nn = pkey[5] - '0';
   /* These limits are hard-coded here--change if new options added */
   if (ka > 2 || kl > 1 || kp > 6 || kq > 4 || nn > 9)
      abexitm(PARSE_ERR, "Undefined key string numeric code.");
   plft = lfttxt[kp]; llft = strlen(plft);
   prgt = rgttxt[kq]; lrgt = strlen(prgt);
   /* Switches are used here as only way to capture lengths of
   *  different text selections, which may differ.  */
   switch (ka) {
   case 0:
      nack = 0;
      pack = NULL; break;
   case 1:
      nack = sizeof(acktxt1)/sizeof(char *);
      pack = acktxt1; break;
   case 2:
      nack = sizeof(acktxt2)/sizeof(char *);
      pack = acktxt2; break;
      } /* End ka switch */
   switch (kl) {
   case 0:
      nlic = 0;
      plic = NULL; break;
   case 1:
      nlic = sizeof(lictxt1)/sizeof(char *);
      plic = lictxt1; break;
      } /* End kl switch */

/* Copy first nn lines to the output */

   lbuff[lstr-1] = '\n';
   rfwrite(&outrf, lbuff, lstr, ABORT);
   while (nn-- > 0) {
      lstr = rfgets(&inrf, lbuff, CDSIZE, ABORT);
      lbuff[lstr-1] = '\n';
      rfwrite(&outrf, lbuff, lstr, ABORT);
      }

   if (kop & OP_CONTR) {
      /* Contract option.  As a sanity check, be sure next line
      *  matches the expected start-of-line code.  If so, use
      *  line counts from stored standard texts to skip over what
      *  should be that text.  If not, or after doing the skips,
      *  break out to copy rest of file to the output.  */
      lstr = rfgets(&inrf, lbuff, CDSIZE, ABORT);
      if (lstr > llft && lbuff[0] == plft[0]) {
         nn = 2;        /* Count comment lines */
         if (ka) nn += nack;
         if (kl) nn += nlic;
         while (--nn > 0) rfgets(&inrf, lbuff, CDSIZE, ABORT);
         }
      }

   else if (ka|kl) {
      /* Expand option.  Copy selected text to output file with
      *  requested start-of-line and end-of-line decorations.  */
      int itopbot = 0;
      while (1) {
         /* Make top comment line */
         int irow, icol = 0;
         if (kq) {         /* Make full-line comment */
            if (plft[0])        lbuff[icol++] = plft[0];
            if (plft[1] != ' ') lbuff[icol++] = plft[1];
            while (icol < STDLLEN - lrgt) lbuff[icol++] = '-';
            if (prgt[0])        lbuff[icol++] = prgt[0];
            if (prgt[1])        lbuff[icol++] = prgt[1];
            lbuff[icol++] = '\n';
            rfwrite(&outrf, lbuff, icol, ABORT);
            }
         else {
            memcpy(lbuff, plft, llft);
            lbuff[llft] = '\n';
            rfwrite(&outrf, lbuff, llft+1, ABORT);
            }
         if (itopbot) break;
         memset(lbuff, ' ', STDLLEN);
         /* Copy acknowledgment text (null loop if nack == 0) */
         for (irow=0; irow<nack; ++irow) {
            if (kp) rfwrite(&outrf, plft, llft, ABORT);
            nn = strlen(pack[irow]);
            rfwrite(&outrf, pack[irow], nn, ABORT);
            if (kq) {
               int nblk = STDLLEN - llft - nn - lrgt;
               rfwrite(&outrf, lbuff, nblk, ABORT);
               rfwrite(&outrf, prgt, lrgt, ABORT);
               rfwrite(&outrf, "\n", 1, ABORT);
               }
            }
         /* Copy license text (null loop if nlic == 0) */
         for (irow=0; irow<nlic; ++irow) {
            if (kp) rfwrite(&outrf, plft, llft, ABORT);
            nn = strlen(plic[irow]);
            rfwrite(&outrf, plic[irow], nn, ABORT);
            if (kq) {
               int nblk = STDLLEN - llft - nn - lrgt;
               rfwrite(&outrf, lbuff, nblk, ABORT);
               rfwrite(&outrf, prgt, lrgt, ABORT);
               rfwrite(&outrf, "\n", 1, ABORT);
               }
            }
         /* Go back to make bottom comment line */
         itopbot = 1;
         } /* End itopbot loop */
      } /* End else (expanding) */

/* Copy rest of file to output */

   while ((lstr = rfgets(&inrf, lbuff, CDSIZE, ABORT)) > 0) {
      lbuff[lstr-1] = '\n';
      rfwrite(&outrf, lbuff, lstr, ABORT);
      }

/* If editing in place, rewind input and output (temp) files, copy
*  temp file back over input file (which was opened above for
*  READWRITE), close input (now output) file, rewind temp file
*  for reuse on the next file, restore original timestamp to the
*  (closed) rewritten input file.  */

   if (kop & OP_INPLACE) {
      rfseek(&inrf, 0, SEEKABS, ABORT);
      rfflush(&outrf, ABORT);
      rfseek(&outrf, 0, SEEKABS, ABORT);
      while ((lstr = rfgetb(&outrf, ABORT)) > 0)
         rfwrite(&inrf, outrf.buff, lstr, ABORT);
      rfclose(&inrf, REWIND, RETAIN_BUFF, ABORT);
      rfseek(&outrf, 0, SEEKABS, ABORT);
      ftruncate(outrf.frwd, 0);
      if (utime(ipath, oldtimes)) puts(ssprintf(NULL,
         "Unable to set timestamp for %s, errno = %d", ipath, errno));
      }

/* Otherwise, if not editing in place, just close both files and
*  restore the original timestamp to the (closed) output file.  */

   else {
      rfclose(&inrf,  REWIND, RETAIN_BUFF, ABORT);
      rfclose(&outrf, REWIND, RETAIN_BUFF, ABORT);
      if (utime(opath, oldtimes)) puts(ssprintf(NULL,
         "Unable to set timestamp for %s, errno = %d", opath, errno));
      }

/* Print the name of the file if verbose,
*  increment the conversion count and return.  */

   if (kop & OP_VERBOSE) puts(ipath);
   ++nconv;
   return TRUE;

   } /* End do1file() */

/*=====================================================================*
*                                                                      *
*                        keyedit main program                          *
*                                                                      *
*=====================================================================*/

int main(int argc, char *argv[]) {

   struct flist {             /* Struct to hold file list */
      struct flist *pnfl;
      char name[1];           /* This is fake length--malloc space
                              *  for actual file name length */
      } *pfl, *pfl0, *pfl1, **ppfl;
   struct stat instat;        /* Input file status */
   struct stat outstat;       /* Output path status */
   DIR *pdir;                 /* Ptr to directory struct */
   struct dirent *pdirent;    /* Ptr to directory entry struct */
   char *path1, *path2;       /* Ptrs to input, output paths */
   char *psep;                /* Ptr to dir/file separator */
   int   iarg,iac;            /* Index of current argument */
   int   ip1,ip2;             /* Indexes of path1, path2 args */
   int   len;                 /* Length of a string */
   char  work[PATH_MAX+1];    /* Working path */

/* Look for switch arguments */

   kop = nconv = 0;
   for (iarg=1; iarg<argc; ++iarg) {
      if (argv[iarg][0] != '-') break;
      if ((len = strlen(argv[iarg])) == 1) {
         ++iarg; break; }
      for (iac=1; iac<len; ++iac) {
         switch (argv[iarg][iac]) {
         case 'c':
            kop |= OP_CONTR;
            break;
         case 'd':
            kop |= OP_DIROK;
            break;
         case 'e':
            kop &= ~OP_CONTR;
         case 'i':
            kop |= OP_INPLACE;
            break;
         case 'v':
            kop |= OP_VERBOSE;
            break;
         case 'y':
            kop |= OP_AUTOYES;
            break;
         default:
            abexitm(BADSW_ERR, ssprintf(NULL, "Unrecognized"
               " command-line switch %s", argv[iarg]));
            }
         }
      }

/* Terminate if no file arguments are present */

   if (iarg >= argc) {
      puts("Synopsis: keyedit [-cdeivy] [-] path1 [path2]");
      exit(NOSRC_ERR);
      }

/* Save offsets of file arguments */

   ip1 = iarg;
   ip2 = argc - 1;

/* Save working directory for constructing default paths.
*  Because systems may differ, be sure it is ended with PSEP.  */

   if (!getcwd(cwd0, PATH_MAX)) abexitme(OGCWD_ERR,
      "Unable to ascertain working directory");
   len = strlen(cwd0);
   if (cwd0[len-1] != PSEP) {
      cwd0[len] = PSEP; cwd0[len+1] = '\0';
      }

/* If there is only one argument (and -i not specified),
*  ask user for permission to work in -i mode.  */

   if (!(kop & OP_INPLACE) && (ip1 >= ip2)) {
      if (getok("\nReformat input file(s) in place? (y/n):"))
         kop |= OP_INPLACE;
      else
         exit(XPERM_ERR);
      }

/* If working in place, create a temp file as output and
*  use last argument as another input */

   if (kop & OP_INPLACE) {
      strcpy(file2, "Keyedit-temp-XXXXXX");
      outrf.accmeth = ACC_DEFER;
      rfopen(&outrf, file2, READWRITE, BINARY, MK_TEMP, TOP,
         LOOKAHEAD, NO_REWIND, RELEASE_BUFF, MYBUFLEN, IGNORE,
         IGNORE, ABORT);
      ip2 += 1;
      }

/* Otherwise, copy output specifier to work, expanding it to include
*  full directory specs (which will be needed when the input is from
*  a different location).
*  Analyze name in work according to whether or not it exists and,
*  if not, whether user included a final separator to indicate a
*  directory was intended.  When a non-existent directory is
*  requested, ask permission to create it.
*  Finally, parse the components so the full directory part of the
*  path is in dir2 (with added final separator) and the base file
*  name is in file2.  */

   else {
      path2 = argv[ip2];
      expandPath(path2, work);

      /* Determine status of output file or directory */
      if (work[0] == '\0') {
         /* The root directory is a special case, because our other
         *  logic reduces its name in work to a null string.  */
         if (access("/", W_OK) != 0) abexitm(NOWRT_ERR,
            "You do not have write permission"
            " for the output file or directory");
         kop |= OP_ISDIR;
         strcpy(dir2, work);
         } else
      if (stat(work, &outstat) == 0) {
         /* Output item exists.  Whatever it is, we must be able to
         *  write to it.  We make a separate call to access(), which
         *  checks whether current user, as opposed to owner, can
         *  write.  */
         if (access(work, W_OK) != 0) abexitm(NOWRT_ERR,
            "You do not have write permission"
            " for the output file or directory");
         if (S_ISDIR(outstat.st_mode)) {
            kop |= OP_ISDIR;
            strcpy(dir2, work);
            }
         else {
            /* Output is an existing file.  For safety,
            *  ask permission to write over it.  */
            if (!getok("\nSpecified output file already"
                  " exists.\nShall I write over it? (y/n):"))
               exit(XPERM_ERR);
            splitpw2(work);      /* Get dir2 and file2 */
            }
         } /* End handling existing output path */
      else if (kop & OP_RQDIR) {
         /* Output item is a nonexistent directory, as indicated
         *  by the name's ending in a separator).  Ask the user
         *  for permission to create it and do so.  */
         if (!getok("\nSpecified output directory does not"
               " exist.\nShall I create it? (y/n):"))
            exit(XPERM_ERR);
         if (mkdir(work, 0755))
            abexitme(MKDIR_ERR,
               "Unable to create specified directory");
         kop |= OP_ISDIR;
         strcpy(dir2, work);
         } /* End handling nonexistent output directory */
      else {
         /* Output name specifies a plain file.  Attempts to
         *  write more than one input to it will be detected
         *  in do1file(), but a quick test here won't hurt.
         *  Also, check that we have write permission in the
         *  parent directory.  */
         if (ip2 - ip1 > 1) abexitm(MAXIN_ERR, "With more"
            " than one input, output must be a directory");
         splitpw2(work);        /* Get dir2 and file2 */
         if (stat(dir2, &outstat)) abexitm(EXDIR_ERR,
            "A nonexistent directory was specified in which"
            " to write the output file");
         if (!(outstat.st_mode & S_IWUSR)) abexitm(NOWRT_ERR,
            "You do not have write permission in the"
            " parent directory of the output file");
         } /* End handling nonexistent output file */
      /* Restore the separator after dir2 */
      if ((len = strlen(dir2)) + 1 > PATH_MAX) plexitm(dir2);
      dir2[len] = PSEP; dir2[len+1] = '\0';
      } /* End checking output for non-i mode */

/* Make a regular expression for searching for the key */

   if (regcomp(&keyrex, "\\*[0-9]{5}\\*", REG_EXTENDED))
      abexitm(REGEX_ERR, "Unable to parse key code search string.");

/*---------------------------------------------------------------------*
*                          Main process loop                           *
*                                                                      *
*  N.B.  Testing for input same as output is deferred until both files *
*  are open in do1file() routine, where a proper test can be done that *
*  sees through links, etc.                                            *
*---------------------------------------------------------------------*/

   for (iarg=ip1; iarg<ip2; ++iarg) {

/* Expand the input specification, dealing with dot and squiggle
*  directories, remove any final separator.  */

      path1 = argv[iarg];
      expandPath(path1, work);

/* Determine whether input is a file or a directory */

      if (stat(work, &instat)) abexitme(INACC_ERR, ssprintf(NULL,
         "Unable to access %s", work));
      if (S_ISDIR(instat.st_mode)) {

/* Process an input directory.  Find all the files in the
*  directory and call do1file() individually for each file.  */

         if (!(kop & OP_DIROK) && !getok(ssprintf(NULL,
               "\nInput %s is a directory.  Do you want\n to con"
               "vert all files in this directory? (y/n):", work)))
            exit(XPERM_ERR);
         strcpy(dir1, work);
         if ((len = strlen(dir1)) + 1 > PATH_MAX) plexitm(dir1);
         dir1[len] = PSEP; dir1[++len] = '\0';

         /* With a named directory (rather than globbing), naturally,
         *  it is necessary to expand the file list before processing
         *  any files, otherwise each one gets converted twice (this
         *  mistake does not generate an infinite loop, probably
         *  because the third copy gets stored over the first, earlier
         *  in the directory structure).  Rather than write some fancy
         *  code to buffer the list in blocks, we just malloc a
         *  separate flist struct for each one.  */
         if (!(pdir = opendir(dir1)))  /* Assignment intended */
            abexitme(OPNID_ERR, ssprintf(NULL,
               "Unable to open input directory %s", dir1));
         ppfl = &pfl0;
         /* Apparently readdir() doesn't clear errno.  Doing it
         *  now, we hope, lets us distinguish eof from err.  */
         errno = 0;
         while (pdirent = readdir(pdir)) {
            int ldnm;               /* Length of d_name */
            char ednm[PATH_MAX+1];  /* Expanded d_name */
            if (errno) abexitme(READD_ERR, ssprintf(NULL,
               "Read error in directory %s", dir1));
            /* We are told not to trust any field in the struct
            *  that is returned except d_name, so do a stat on
            *  the name to see if it is a directory.  */
            ldnm = strlen(pdirent->d_name);
            if (ldnm + len > PATH_MAX) plexitm(pdirent->d_name);
            strcpy(ednm, dir1);
            strcat(ednm, pdirent->d_name);
            if (stat(ednm, &instat)) abexitme(INACC_ERR,
               ssprintf(NULL, "Unable to access %s", ednm));
            if (S_ISDIR(instat.st_mode)) continue;
            *ppfl = pfl = mallocv(sizeof(struct flist) + ldnm,
               "input file list");
            strcpy(pfl->name, pdirent->d_name);
            ppfl = &pfl->pnfl;
            } /* End directory scan */
         pfl->pnfl = NULL;

         for (pfl=pfl0; pfl; pfl=pfl1) {
            pfl1 = pfl->pnfl;
            do1file(pfl->name);
            free(pfl);
            }

         } /* End handling input directory */

/* Process an input file.  Parse the given argument into directory
*  and file name.  Directory will already have been filled in if
*  specified indirectly.  If not same as cwd, change to new dir.  */

      else {

         if (psep = strrchr(work, PSEP)) {  /* Assignment intended */
            len = (++psep)-work;
            if (len > PATH_MAX) plexitm(work);
            memcpy(dir1, work, len);
            dir1[len] = '\0';
            if (strlen(psep) > PATH_MAX) plexitm(work);
            strcpy(file1, psep);
            }
         else
            abexitm(PARSE_ERR, "Internal logic error,"
               " check source code");

/* In the UNIX case, globbing is done by the shell, therefore
*  there is just one file to process at this point.  */

         do1file(file1);
         } /* End handling a single input file */

      } /* End loop over input arguments */

   puts(ssprintf(NULL, "%d files converted", nconv));
   if (kop & OP_INPLACE)
      rfclose(&outrf, NO_REWIND, RELEASE_BUFF, ABORT);
   return 0;                  /* Success */

   } /* End d2u() or u2d() */
