/* (c) Copyright 1992-1999, The Rockefeller University *11113* */
/***********************************************************************
*                               d2u-u2d                                *
*                                                                      *
*  Synopsis:  d2u  [-a] [-d] [-i] [-v] [-y] [-] path1 [path2]          *
*             u2d  [-a] [-d] [-i] [-v] [-y] [-] path1 [path2]          *
*                                                                      *
*     This source file can be compiled to yield GNR-enhanced versions  *
*  of the standard SunOS utilities dos2unix (convert text file with    *
*  DOS newlines to UNIX newlines--default compilation) and unix2dos    *
*  (convert text file with UNIX newlines to DOS newlines--compile with *
*  U2D preprocessor variable defined).  The most critical difference   *
*  is that both versions always preserve the timestamps of the input   *
*  files during conversion.  In addition, V2 programs can convert      *
*  multiple files and whole directories in a single execution.         *
*                                                                      *
*  Switches:                                                           *
*     -a    Convert all input files regardless of the timestamps of    *
*           any existing files with the same names in <path2>.  The    *
*           default is to convert only files that are newer and files  *
*           that are absent in <path2>.  When the program is in -i     *
*           mode, the input file(s) are converted regardless of the    *
*           timestamp(s) of the output file(s) or the setting of the   *
*           -a switch.                                                 *
*     -d    Suppress the warning that would normally be issued when    *
*           <path1> contains one or more bare directories.             *
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
*               UNIX by shell globbing, under DOS or Windows by pgm.   *
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
*  Initial version, 08/14/92, G.N. Reeke                               *
*  V2A, 03/21/99, GNR - Add multiple-file capability and command-line  *
*                 switches, do conversion explicitly, do not execute   *
*                 original Sun utilities (not present on all systems). *
*  V2B, 09/20/99, GNR - Add -y switch.                                 *
***********************************************************************/

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sysdef.h"
#include "rksubs.h"
#include "rfdef.h"
#ifdef DOS
#include <dir.h>
#include <dos.h>
#include <io.h>
#endif
#ifdef UNIX
#include <unistd.h>
#include <dirent.h>
#endif

/* Configuration parameters */
#define INPB_MAX     8        /* Max size of response */
#ifndef PATH_MAX
#define PATH_MAX   255        /* Max path name length */
#endif
#define MYBUFLEN  4096        /* I/O buffer size */

/* System definitions */
#define DOSEOF      26        /* DOS End-Of-File character */
#if defined DOS || defined WINDOWS
#define PSEP '\\'             /* Path separator */
#define LDRIVE       2        /* Length of drive specifier */
#else
#define PSEP '/'
#endif
#define ATIME        0        /* Position of atime in oldtimes[] */
#define MTIME        1        /* Position of mtime in oldtimes[] */

/* Define option flags */
#define OP_ALL       1        /* Convert all input files */
#define OP_DIROK     2        /* Suppress directory warning */
#define OP_INPLACE   4        /* Convert files in place */
#define OP_VERBOSE   8        /* Print names of files converted */
#define OP_AUTOYES  16        /* Answer "yes" to most questions */
#define OP_RQDIR    32        /* Path has directory form */
#define OP_ISDIR    64        /* Output path is a directory */

/* Define error exits */
#define XPERM_ERR    1        /* Negative response to getok() */
#define NOSRC_ERR    2        /* Too few arguments */
#define BADSW_ERR    3        /* Bad command-line switch */
#define NOWRT_ERR    4        /* No write permission for output */
#define PATHL_ERR    5        /* Path name is too long */
#define MKDIR_ERR    6        /* Unable to create output directory */
#define EXDIR_ERR    7        /* Nonexistent directory specified */
#define MAXIN_ERR    8        /* Too many inputs */
#define HOMEX_ERR    9        /* Unable to expand home directory */
#define INACC_ERR   10        /* Unable to access input file */
#define MATCH_ERR   11        /* No input found matching file spec */
#define SAMEF_ERR   12        /* Input and output files are same */
#define RENAM_ERR   13        /* Unable to rename temp to input fn */
#define FTIME_ERR   14        /* Unable to get old timestamp */
#define STIME_ERR   15        /* Unable to reset timestamp */
#define OGCWD_ERR   16        /* Unable to get initial cwd */
#define NOCWD_ERR   17        /* Unable to getcurdir on O/P drive */
#define BADFN_ERR   18        /* Illegally composed path name */
#define OPNID_ERR   19        /* Unable to open input directory */
#define READD_ERR   20        /* Error reading input directory */
#define PARSE_ERR   21        /* Something wrong in parsing logic */
#define SYSEX_ERR   22        /* Unable to execute a system program */

/* Global data */
static struct RFdef inrf;         /* Input file descriptor */
static struct RFdef outrf;        /* Output file descriptor */
static int   kop;                 /* Option codes */
static int   nconv;               /* Number of files converted */
static char  cwd0[PATH_MAX+1];    /* Working directory on entry */
static char  dir1[PATH_MAX+1];    /* Input directory (w/PSEP) */
static char  file1[PATH_MAX+1];   /* Input file */
static char  dir2[PATH_MAX+1];    /* Output directory (w/PSEP) */
static char  file2[PATH_MAX+1];   /* Output file */

/*=====================================================================*
*                          Fatal error exits                           *
*                                                                      *
*  By using the names "abexit" and "abexitm", we intercept errors      *
*  from ROCKS library routines as well as our own errors.              *
*=====================================================================*/

int abexloop;

void abexit(int rc) {
   if (!abexloop) {
      abexloop = TRUE;
      fputs(ssprintf(NULL, "\n***Program terminated"
         " with abend code %d\n", rc), stderr);
      fflush(stderr);
      }
   exit(rc);
   } /* End abexit() */

void abexitm(int rc, char *msg) {
   if (!abexloop) {
      abexloop = TRUE;
      fputs("\n***", stderr);
      fputs(msg,  stderr);
      fputs("\n", stderr);
      fflush(stderr);
      }
   exit(rc);
   } /* End abexitm() */

void abexitme(int rc, char *msg) {
   if (!abexloop) {
      abexloop = TRUE;
      fputs("\n***", stderr);
      fputs(msg,  stderr);
      fputs("\n", stderr);
      fputs(ssprintf(NULL, "***The system error code is %d\n",
         errno), stderr);
      fflush(stderr);
      }
   exit(rc);
   } /* End abexitme() */

/*=====================================================================*
*                          Path length exit                            *
*=====================================================================*/

static void plexitm(char *path) {
   fputs("\n", stderr);
   fputs(path, stderr);
   abexitm(PATHL_ERR, "Above path name is too long");
   } /* End plexitm() */

#ifdef UNIX
/*=====================================================================*
*       Run a UNIX system program (safer version than system())        *
*=====================================================================*/

static int my_system (char *command) {
    int pid, status;

    if (command == 0)
       return 1;
    pid = fork();
    if (pid == -1)
       return -1;
    if (pid == 0) {
       char *argv[4];
       argv[0] = "sh";
       argv[1] = "-c";
       argv[2] = command;
       argv[3] = 0;
       execv("/bin/sh", argv);
       abexit(SYSEX_ERR);
       }
    do {
       if (waitpid(pid, &status, 0) == -1) {
          if (errno != EINTR) return -1; }
       else
          return status;
    } while(1);
} /* End my_system() */
#endif

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
*  This routine makes sure the path has the appropriate drive letter   *
*  (DOS), inserts the original working directory if no directory was   *
*  specified, and inserts the original working directory or one of     *
*  its parents if a string of one or more dots was specified.  This    *
*  is done on input as well as output, because earlier arguments may   *
*  change the current working directory.  If a string of dots is not   *
*  the first element in the path name, no special test is done and     *
*  the error should be detected later.                                 *
*=====================================================================*/

static void expandPath(char *argpath, char *exppath) {

   char *epe = exppath;       /* End of reconstructed path */
   char *psep;                /* Location of a separator */
   int lep = PATH_MAX;        /* Size remaining in output area */
   int len;                   /* Length of a string */

/* Check first for an initial string of dots, as these
*  supersede tests for drive and absolute directory.
*  (Under DOS, cwd0 will always include the drive.)  */

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
#if defined DOS || defined WINDOWS
         abexitm(BADFN_ERR, ssprintf(NULL, "Invalid file name %s",
            argpath));
#else /* Not DOS */
         /* Absolute dir is missing, use cwd0 */
         if ((len = strlen(cwd0)) > lep) plexitm(cwd0);
         strcpy(epe, cwd0);
         epe += len, lep -= len;
#endif
         }
      }

#ifdef UNIX
#if 0  /* LINUX does this in shell--if other UNIXs do too, kill this code */
/* Under UNIX, expand an initial squiggle.
*  N.B.  This code currently won't handle ~xyz.  */

   else if (argpath[0] == '~') {
      if (argpath[1] == PSEP &&
            (psep = getenv("HOME")) != NULL) {
         if ((len = strlen(psep)) > lep) plexitm(psep);
         strcpy(epe, psep);
         epe += len, lep -= len;
         if (epe[-1] != PSEP)
            *epe++ = PSEP, lep -= 1;
         argpath += 2;
         }
      else
         abexitm(HOMEX_ERR, "Unable to expand home directory");
      }
#endif
#endif

/* Name does not start with a string of dots or a squiggle */

   else {

#if defined DOS || defined WINDOWS
      /* In a PC, be sure the drive is specified */
      if (argpath[LDRIVE-1] == ':') {
         /* Got absolute drive, use it and skip over it */
         memcpy(epe, argpath, LDRIVE);
         argpath += LDRIVE;
         }
      else {
         /* Absolute drive not specified, use current working drive */
         memcpy(epe, cwd0, LDRIVE);
         }
      epe += LDRIVE, lep -= LDRIVE;
#endif

      /* Be sure the absolute directory is specified */
      *epe = '\0';   /* In case used in err msg */
      if (argpath[0] != PSEP) {
#if defined DOS || defined WINDOWS
         /* Absolute dir is missing, use cwd on specified drive */
         char curdir[MAXDIR+3];
         curdir[0] = PSEP;
         if (getcurdir(exppath[0] & 0x1f, curdir+1)) abexitme(NOCWD_ERR,
            ssprintf(NULL, "Unable to get current directory on %s",
               exppath));
         /* Add terminal separator unless it's a root directory */
         if ((len = strlen(curdir)) > 1) {
            curdir[len] = PSEP;
            curdir[++len] = '\0';
            }
         if (len > lep) plexitm(curdir); /* JIC */
         strcpy(epe, curdir);
         epe += len, lep -= len;
#else    /* Not DOS */
         /* Absolute dir is missing, use cwd0 */
         if ((len = strlen(cwd0)) > lep) plexitm(cwd0);
         strcpy(epe, cwd0);
         epe += len, lep -= len;
#endif
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
*                Perform one actual format conversion                  *
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
#ifdef UNIX
   time_t oldtimes[2];        /* Old access, modify times */
#endif
#if defined DOS || defined WINDOWS
   struct ftime oldtime;      /* Old modify time */
#endif
   char *sstr,*estr;          /* Start & end of a text string */
   char *tfn;                 /* Temp ptr to output file name */
   long lstr;                 /* Length of a text string */
#ifndef U2D
   int  gotcr;                /* TRUE if CR found on last read */
#endif
   char ipath[PATH_MAX+1];    /* Full input path name */
   char opath[PATH_MAX+1];    /* Full output path name */
   static char CRLF[2] = "\r\n";

/* Generate the fully expanded input path name for testing equality
*  with output file.  Get status information on the input file,
*  including timestamps (under DOS, file must be open for this).
*  Attempt to open the file.  If this fails, skip the file.  */

   if (strlen(dir1) + strlen(fn) > PATH_MAX) plexitm(fn);
   strcpy(ipath, dir1);
   strcat(ipath, fn);
   if (stat(ipath, &instat) || !rfopen(&inrf, ipath, READ, BINARY,
         SEQUENTIAL, TOP, LOOKAHEAD, REWIND, RETAIN_BUFF, MYBUFLEN,
         IGNORE, IGNORE, NO_ABORT)) {
      puts(ssprintf(NULL, "Skipping %s, unable to access.", ipath));
      return FALSE;
      }
#ifdef UNIX
   oldtimes[ATIME] = instat.st_atime;
   oldtimes[MTIME] = instat.st_mtime;
#endif
#if defined DOS || defined WINDOWS
   if (getftime(inrf.frwd, &oldtime)) abexitme(FTIME_ERR, ssprintf(NULL,
      "Unable to get timestamp for file %s", ipath));
#endif

/* If converting in place, create a temporary output file name.
*  If output is to a directory, make output file name same as
*  input.  Otherwise, use given file name.  Terminate if just
*  one output file was specified and it has already been written.
*/

   if (kop & OP_INPLACE) {       /* Converting in place */
      tfn = tmpnam(NULL);
#ifdef UNIX
      if (strlen(tfn) > PATH_MAX) plexitm(tfn);
      strcpy(opath, tfn);
#else
      if (strlen(dir1) + strlen(tfn) > PATH_MAX) plexitm(dir1);
      strcpy(opath, dir1);
      strcat(opath, tfn);
#endif
      }
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

#if defined DOS || defined WINDOWS || defined MVS || defined VM
   if (!strcmpi(ipath, opath))
      abexitm(SAMEF_ERR, "Use -i to convert a file in place.");
#else
   if (!strcmp(ipath, opath))
      abexitm(SAMEF_ERR, "Use -i to convert a file in place.");
#endif

/* Get status information on the output file.  (Next
*  two checks are done only if output already exists.)  */

   if (!stat(opath, &outstat)) {

/* If the output file exists, we are not converting in place,
*  and -a was not specified, compare the two timestamps and
*  skip this file if the output is newer.  */

   if (!(kop & (OP_INPLACE|OP_ALL)) &&
         (outstat.st_mtime >= instat.st_mtime)) {
      rfclose(&inrf, REWIND, RETAIN_BUFF, ABORT);
      return FALSE;
      }

/* If the output file exists, an attempt can be made to detect
*  whether it is the same as the input using rfqsame.  In order
*  not to destroy it, we first open it for input, do the rfqsame
*  test, then close it and reopen it for output.  If the file
*  cannot be opened, or it is the same as the input file, abort.
*
*  N.B.  The rfqsame() routine under DOS is only known to work
*  for stdout/stderr (it may also work other pairs of files opened
*  for output), but we do not want to force write access to the
*  input file, therefore, this test is probably useless, but it
*  is left here as a template for future improvement.  The test
*  for equality of names performed above is expected to work most
*  of the time since the names have all been fully expanded.  */

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

/* Finally, open the output file for output.  */

   rfopen(&outrf, opath, WRITE, BINARY, SEQUENTIAL, TOP, LOOKAHEAD,
      REWIND, RETAIN_BUFF, MYBUFLEN, IGNORE, IGNORE, ABORT);

#ifdef U2D

/*---------------------------------------------------------------------*
*                             UNIX TO DOS                              *
*                                                                      *
*  Copy the input to the output, expanding each LF to a CR-LF combo.   *
*  Use internals of rfread to avoid copying everything twice.          *
*---------------------------------------------------------------------*/

   for (;;) {
      if (inrf.lbsr == 0) rfgetb(&inrf, ABORT);
      if (inrf.lbsr == ATEOF) break;
      sstr = inrf.bptr;
      estr = sstr + inrf.lbsr;
      for ( ; sstr<estr && *sstr!='\n'; ++sstr) ;
      if (lstr = sstr - inrf.bptr) {   /* Assignment intended */
         rfwrite(&outrf, inrf.bptr, lstr, ABORT);
         inrf.bptr += lstr;
         inrf.aoff += lstr;
         inrf.lbsr -= lstr;
         }
      if (sstr < estr) {
         rfwrite(&outrf, CRLF, 2, ABORT);
         inrf.bptr += 1;
         inrf.aoff += 1;
         inrf.lbsr -= 1;
         }
      }

#else

/*---------------------------------------------------------------------*
*                             DOS TO UNIX                              *
*                                                                      *
*  Copy the input to the output, removing the extra CR's.              *
*  Use internals of rfread to avoid copying everything twice.          *
*  The logic has to handle the case of a CR-LF that is split.          *
*---------------------------------------------------------------------*/

   for (gotcr=FALSE;;) {
      if (inrf.lbsr == 0) rfgetb(&inrf, ABORT);
      /* If CR not followed by LF, put it back */
      if (gotcr && (inrf.lbsr <= 0 || *inrf.bptr != '\n'))
         rfwrite(&outrf, CRLF, 1, ABORT);
      if (inrf.lbsr == ATEOF) break;
      sstr = inrf.bptr;
      estr = sstr + inrf.lbsr;
      for ( ; sstr<estr && *sstr!='\r' && *sstr!=DOSEOF; ++sstr) ;
      if (lstr = sstr - inrf.bptr) {   /* Assignment intended */
         rfwrite(&outrf, inrf.bptr, lstr, ABORT);
         inrf.bptr += lstr;
         inrf.aoff += lstr;
         inrf.lbsr -= lstr;
         }
      gotcr = FALSE;
      if (sstr >= estr) continue;
      if (*sstr == DOSEOF) break;
      if (*sstr == '\r') {
         /* Swallow the carriage return */
         inrf.bptr += 1;
         inrf.aoff += 1;
         inrf.lbsr -= 1;
         gotcr = TRUE;
         }
      }

#endif

#if defined DOS || defined WINDOWS

/* Restore the original timestamp to the (open) DOS output file.
*  Must flush first, otherwise when file is closed, writing the
*  last bit causes the timestamp to be changed to current time!  */

   rfflush(&outrf, ABORT);
   if (setftime(outrf.frwd, &oldtime)) abexitme(STIME_ERR, ssprintf(
      NULL, "Unable to set timestamp for %s", opath));

#endif

/* Close both files, abort on error */

   rfclose(&inrf,  REWIND, RETAIN_BUFF, ABORT);
   rfclose(&outrf, REWIND, RETAIN_BUFF, ABORT);

/* If converting in place, rename temp file to replace input file.
*  In UNIX, temp file may be on a different file system, use mv.  */

   if (kop & OP_INPLACE) {
#ifdef UNIX
      char mvc[12+PATH_MAX+PATH_MAX];
      strcpy(mvc, "/bin/mv ");
      strcat(mvc, opath);
      strcat(mvc, " ");
      strcat(mvc, ipath);
      if (my_system(mvc) != 0) abexitme(RENAM_ERR, ssprintf(NULL,
         "Unable to replace %s", ipath));
      strcpy(opath, ipath);
#endif
#if defined DOS || defined WINDOWS
      if (remove(ipath)) abexitme(RENAM_ERR, ssprintf(NULL,
         "Unable to remove original %s", ipath));
      if (rename(opath, ipath)) abexitme(RENAM_ERR, ssprintf(NULL,
         "Unable to write over %s", ipath));
#endif
      }

#ifdef UNIX

/* Restore the original timestamp to the (closed) UNIX output file */

   if (utime(opath, oldtimes)) puts(ssprintf(NULL,
      "Unable to set timestamp for %s, errno = %d", opath, errno));

#endif

/* Print the name of the file if verbose,
*  increment the conversion count and return.  */

   if (kop & OP_VERBOSE) puts(ipath);
   ++nconv;
   return TRUE;

   } /* End do1file() */

/*=====================================================================*
*                                                                      *
*                             d2u or u2d                               *
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
#if defined DOS || defined WINDOWS
   struct ffblk ffb;          /* File-found info */
#endif
#ifdef UNIX
   DIR *pdir;                 /* Ptr to directory struct */
   struct dirent *pdirent;    /* Ptr to directory entry struct */
#endif
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
         case 'a':
            kop |= OP_ALL;
            break;
         case 'd':
            kop |= OP_DIROK;
            break;
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
#ifdef U2D
      puts("Synopsis: u2d [-adiv] [-] path1 [path2]");
#else
      puts("Synopsis: d2u [-adiv] [-] path1 [path2]");
#endif
      exit(NOSRC_ERR);
      }

/* Save offsets of file arguments */

   ip1 = iarg;
   ip2 = argc - 1;

/* Save working directory for constructing default paths.
*  Because systems may differ, be sure it is ended with PSEP.
*  TurboC always provides the drive letter with the directory.  */

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

/* If working in place, use last argument as another input */

   if (kop & OP_INPLACE)
      ip2 += 1;

/* Otherwise, copy output specifier to work, expanding it
*  to include full drive and directory specs (which will
*  be needed when the input is from a different location).
*  Analyze name in work according to whether or not it exists
*  and, if not, whether user included a final separator to
*  indicate a directory was intended.  When a non-existent
*  directory is requested, ask permission to create it.
*  Finally, parse the components so the full directory part
*  of the path is in dir2 (with added final separator) and
*  the base file name is in file2.  */

   else {
      path2 = argv[ip2];
      expandPath(path2, work);

      /* Determine status of output file or directory */
#if defined DOS || defined WINDOWS
      /* Under DOS (or anyway TURBOC), stat() returns an error if
      *  given just a drive letter.  Handle this case separately.  */
      if (work[LDRIVE-1] == ':' && work[LDRIVE] == '\0') {
/****
** Here we can add code to check whether the drive letter exists
** and perhaps whether the drive is writeable.
****/
         kop |= OP_ISDIR;
         strcpy(dir2, work);
         } else
#endif
#ifdef UNIX
      if (work[0] == '\0') {
         /* The root directory is a special case, because our other
         *  logic reduces its name in work to a null string.  */
         if (access("/", W_OK) != 0) abexitm(NOWRT_ERR,
            "You do not have write permission"
            " for the output file or directory");
         kop |= OP_ISDIR;
         strcpy(dir2, work);
         } else
#endif
      if (stat(work, &outstat) == 0) {
         /* Output item exists.  Whatever it is, we must be able
         *  to write to it.  Note:  Under DOS, stat() does not
         *  determine whether directories are writeable.  Under
         *  UNIX, we make a separate call to access(), which checks
         *  whether current user, as opposed to owner, can write.  */
#ifdef UNIX
         if (access(work, W_OK) != 0) abexitm(NOWRT_ERR,
            "You do not have write permission"
            " for the output file or directory");
#endif
         if (outstat.st_mode & S_IFDIR) {
            kop |= OP_ISDIR;
            strcpy(dir2, work);
            }
         else {
            /* Output is an existing file.  For safety,
            *  ask permission to write over it.  */
            if (!getok("\nSpecified output file already"
                  " exists.\nShall I write over it? (y/n):"))
               exit(XPERM_ERR);
#if defined DOS || defined WINDOWS
            if (!(outstat.st_mode & S_IWRITE)) abexitm(NOWRT_ERR,
               "You do not have write permission"
               " for the output file");
#endif
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
#ifdef DOS
         if (mkdir(work))
#else
         if (mkdir(work, 0755))
#endif
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
#if defined DOS || defined WINDOWS
         if (dir2[LDRIVE-1] == ':' && dir2[LDRIVE] == '\0') {
/****
** Here we can add code to check whether the drive letter exists
** and perhaps whether the drive is writeable.
****/
            ; } else
#endif
         if (stat(dir2, &outstat)) abexitm(EXDIR_ERR,
            "A nonexistent directory was specified in which"
            " to write the output file");
#ifdef UNIX
         if (!(outstat.st_mode & S_IWRITE)) abexitm(NOWRT_ERR,
            "You do not have write permission in the"
            " parent directory of the output file");
#endif
         } /* End handling nonexistent output file */
      /* Restore the separator after dir2 */
      if ((len = strlen(dir2)) + 1 > PATH_MAX) plexitm(dir2);
      dir2[len] = PSEP; dir2[len+1] = '\0';
      } /* End checking output for non-i mode */

/*---------------------------------------------------------------------*
*                          Main process loop                           *
*                                                                      *
*  N.B.  Testing for input same as output is deferred until both files *
*  are open in do1file() routine, where a proper test can be done that *
*  sees through links, etc.                                            *
*---------------------------------------------------------------------*/

   for (iarg=ip1; iarg<ip2; ++iarg) {

/* Expand the input specification, dealing with drive letters,
*  dot and squiggle directories, remove any final separator.  */

      path1 = argv[iarg];
      expandPath(path1, work);

/* Determine whether input is a file or a directory */

      if (stat(work, &instat)) abexitme(INACC_ERR, ssprintf(NULL,
         "Unable to access %s", work));
      if ((instat.st_mode & S_IFMT) == S_IFDIR) {

/* Process an input directory.  Find all the files in the
*  directory and call do1file() individually for each file.  */

         if (!(kop & OP_DIROK) && !getok(ssprintf(NULL,
               "\nInput %s is a directory.  Do you want\n to con"
               "vert all files in this directory? (y/n):", work)))
            exit(XPERM_ERR);
         strcpy(dir1, work);
         if ((len = strlen(dir1)) + 1 > PATH_MAX) plexitm(dir1);
         dir1[len] = PSEP; dir1[++len] = '\0';

#ifdef UNIX
         /* See note below in DOS code regarding need to scan
         *  and store list before converting any files.  */
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
            if ((instat.st_mode & S_IFMT) == S_IFDIR) continue;
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
#endif

#if defined DOS || defined WINDOWS
         if (strlen(work) + 4 > PATH_MAX) plexitm(work);
         strcat(work, "\\*.*");
#endif

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

#ifdef UNIX

/* In the UNIX case, globbing is done by the shell, therefore
*  there is just one file to process at this point.  */

         do1file(file1);

#endif

         } /* End handling a single input file */

#if defined DOS || defined WINDOWS

/* Under DOS, there is no shell globbing and so the cases of
*  "single" files (which might have wildcards) and directories
*  must both be expanded now.  Naturally, it is necessary to
*  expand the list before processing any files, otherwise each
*  one gets converted twice (this mistake does not generate an
*  infinite loop, probably because the third copy gets stored
*  over the first, earlier in the directory structure).  Rather
*  than write some fancy code to buffer the list in blocks, we
*  just malloc a separate flist struct for each one.  */

      ppfl = &pfl0;
      if (findfirst(work, &ffb, 0)) abexitm(MATCH_ERR, ssprintf(
         NULL, "No files found matching input spec %s", work));
      do {
         *ppfl = pfl = mallocv(sizeof(struct flist) +
            strlen(ffb.ff_name), "input file list");
         strcpy(pfl->name, ffb.ff_name);
         ppfl = &pfl->pnfl;
         } while (!findnext(&ffb));
      pfl->pnfl = NULL;

      for (pfl=pfl0; pfl; pfl=pfl1) {
         pfl1 = pfl->pnfl;
         do1file(pfl->name);
         free(pfl);
         }

#endif

      } /* End loop over input arguments */

   puts(ssprintf(NULL, "%d files converted", nconv));
   return 0;                  /* Success */

   } /* End d2u() or u2d() */
