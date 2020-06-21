/* (c) 1992 Neurosciences Research Foundation */
/**********************************************************************/
/*                                                                    */
/*                             nsi_cpp                                */
/*                                                                    */
/*  SYNOPSIS:                                                         */
/*      nsi_cpp [ -Dname[=val] ] [ -Iinclude-dir ] [ -ooutput-file ]\ */  
/*      input-file                                                    */
/*                                                                    */
/*  DESCRIPTION:                                                      */
/*       nsi_cpp is a limited version of the C language preprocessor. */
/*       Its purpose is to strip unnecessary source code from the     */
/*       source file without expanding macros or removing comments.   */ 
/*       Internally, #defines with no variable arguments are expanded */
/*       so that dependent #if's and #ifdef's can be correctly        */
/*       analyzed; however, these expansions are not copied to the    */
/*       output file.  Similarly, included header files are examined  */
/*       for #defines, which may affect later tests in the file       */
/*       being processed, but the text of such included files is NOT  */
/*       copied to the output.                                        */
/*                                                                    */
/* Version 1, 11/12/92, ROZ                                           */
/**********************************************************************/

#define _NSI_CPP_C

#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <stddef.h>
#include <memory.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>


#include "nsi_cpp.h"


/* Functions prototype */
void define_sym();
void nsi_define(char *av,long vl);
void nsi_dir(char *av);
void nsi_init();
void process_args(int ac,char **av);
long constant_exp();
long log_and_exp();
long log_or_exp() ;
int header_handler();
int else_stat();
int elif_stat();
int get_new_line();
int next_token();
int next_dir();
int end_stat();
int lookup_sym(char *symbol,long *val);
int if_stat();
int factor();

/* Process #define and put variable in the symbol table. The function 
   axcept two arguments - av and vl. av is the name of the variable
   and vl is its value */
void nsi_define(char *av,long vl) {
 
   long loc,value;

   loc=lookup_sym(av,&value);
   if ((loc == NOTFOUND) || (loc == NEWSYM)) {

      if (SYMBOLS.numelm >= SYMBOLS.sytlen) {
         SYMBOLS.nsisymt = 
            (SYMTAB *)realloc(SYMBOLS.nsisymt,NSIARLEN*sizeof(SYMTAB)+
            SYMBOLS.numelm);
         SYMBOLS.sytlen += NSIARLEN;
         }
   
      SYMBOLS.nsisymt[SYMBOLS.numelm].def_name = (char *)malloc(strlen(av)+1);
      strcpy(SYMBOLS.nsisymt[SYMBOLS.numelm].def_name,av);
      SYMBOLS.nsisymt[SYMBOLS.numelm].def_value = vl;
#ifdef DEBUG
      printf("%s = %d \n",
       SYMBOLS.nsisymt[SYMBOLS.numelm].def_name,
       SYMBOLS.nsisymt[SYMBOLS.numelm].def_value);
#endif
      SYMBOLS.numelm++;
      }
   else {
      SYMBOLS.nsisymt[loc].def_value = vl;
      }

   }


/* This funcion accept the name of directory and store it in a table
   which is a part of the directory table. */

void nsi_dir(char *av) {
   
   if (NSICDIR.numelm >= SYMBOLS.sytlen) {
      NSICDIR.inc_dir = (char **)
         realloc(NSICDIR.inc_dir,NSIARLEN*sizeof(NSICDIR.inc_dir)+
         NSICDIR.numelm);
      NSICDIR.numelm += NSIARLEN;
      }
   NSICDIR.inc_dir[NSICDIR.numelm] = (char *)malloc(strlen(av)+1);
   strcpy(NSICDIR.inc_dir[NSICDIR.numelm],av);
   NSICDIR.numelm++;
   }


/* Stores some keywords in the symbol table and initializes global
   variables and structures */
void nsi_init() {

   NSTK  = (STKCC *)malloc(NSIARLEN*sizeof(STKCC));
   NSIHDF  = (HINFO *)malloc(NSIARLEN*sizeof(HINFO));
   SYMBOLS.nsisymt = (SYMTAB *)malloc(NSIARLEN*sizeof(SYMTAB));
   NSICDIR.inc_dir = (char **)malloc(NSIARLEN*sizeof(NSICDIR.inc_dir));
   
   SYMBOLS.sytlen = NSIARLEN;
   NSICDIR.tablen = NSIARLEN;
   PARSE.proc_head = 0;
   NFI.stkindx = 0;
   NSTK[NFI.stkindx].numelm = 0;
   NSTK[NFI.stkindx].arrlen = STKMAX;
   NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= CP_BLK;
   NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= CP_LINE;
   NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= PRO_ELSE;
   NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= PRO_DEF;
   nsi_define("#",1);
   nsi_define("if",1);
   nsi_define("ifdef",1);
   nsi_define("defined",1);
   nsi_define("define",1);
   nsi_define("!",1);
   nsi_define("||",1);
   nsi_define("&&",1);
   nsi_define("elif",1);
   nsi_define("else",1);
   nsi_define("endif",1);
   }

/* Process command line arguments in the following mannar:
   nsi_cpp [ -Dname[=val] ] [ -Iinclude-dir ] [ -o output-file ] input-file
   where the options are:
     -Dname=val
     -Dname
          Define 'name' as equal to 'val' and declare 'name' as
          relevant for preprocessing.  
     -Iinclude-dir
          Directory include-dir, more than one -I option can be given.
     -o output-file
          Output is written to file 'output_file'.  If this option
          is not given, output is written to stdout. 
     input-file
          This parameter gives the name of the input file to be
          processed.  This parameter is required.
*/

void process_args(int ac,char **av) {
   FILE *fd;
   int i,j,val,inp=1,outp=0;
   char name[NSIBFLEN];

   
   if (ac < 3) {
      printf("Usage: nsi_cpp [-Dname] [-Iinclude-dir] infile outfile\n");
      exit(1);
      }
   NFI.outfd = stderr;
   for (i=1;i<ac;i++) {
      if (av[i][0]== '-') {
         switch(av[i][1]) {
            case 'D':{
               int j;
               j=2;
               while(av[i][j]!=' ' && av[i][j]!='\0' && av[i][j]!='=') {
                  name[j-2] =  av[i][j];
                  j++;
                  }
               name[j-2] = '\0';
               if (av[i][j] == '=') {
                  val = atoi(&av[i][j+1]);
                  }
               else {
                  val = 1;
                  }
               nsi_define(name,val);
               }
               break;
            case 'I':
               nsi_dir(&av[i][2]);
               break;
            case 'o':
               strcpy(NFI.outfn,&av[i][2]);
               if ((NFI.outfd = fopen(NFI.outfn,"w")) == NULL) {
                  fprintf(stderr,"Open %s failed \n",NFI.outfn);
                  exit(1);
                  }
               break;
            }
         }
      else if (inp) {
         strcpy(NFI.infn,av[i]);
         if ((NFI.infd = fopen(NFI.infn,"r")) == NULL) {
            fprintf(stderr,"Open %s failed \n",NFI.infn);
            exit(1);
            }
         strcpy(NFI.prgfn,NFI.infn);
         NFI.tmpfd = NFI.infd;
         NFI.worfd = NFI.infd;
         inp = 0;
         }
      else {
         fprintf(stderr,"Misplaced argument!!! \n");
         exit(1);
         }
      }
   getwd(NSICDIR.cur_dir);
   }
 
/* Pasre #else statement and return 1 if processed */
int else_stat() {

   int stat;

   if (match("#else") && 
      (NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] & PRO_ELSE)) {
      stat = PARSE.else_value;
      if (stat == 1) {
         if (NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm-1] & CP_BLK) {
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= CP_BLK;
            }
         else {
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_BLK;
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
            }
         }
      else if (stat == 0) {
         if (NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm-1] & CP_BLK) {
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_BLK;
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
            }
         else {
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_BLK;
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
            }
         }
      else if (stat == IGNORE) {
         if (NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm-1] & CP_BLK) {
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= CP_LINE;
            }
         else {
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_BLK;
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
            }
         }
      return 1;
      }
   }

/* Pasre #elif statement and return 1 if processed */
int elif_stat() {

   int stat;

   if (match("#elif") &&
      (NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] & PRO_ELSE)) {
      stat = constant_exp();
      if (stat == 1) {
         if (NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm-1] & CP_BLK) {
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= CP_BLK;
            }
         else {
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_BLK;
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
            }
         }
      else if (stat == 0) {
         if (NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm-1] & CP_BLK) {
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_BLK;
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
            }
         else {
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_BLK;
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
            }
         }
      else if (stat == IGNORE) {
         if (NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm-1] & CP_BLK) {
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= CP_LINE;
            }
         else {
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_BLK;
            NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
            }
         }
      return 1;
      }
   }

/* Pasre #endif statement and return 1 if processed */
int end_stat() {
   if (match("#endif")) {
      return 1;
      }
   else {
      return 0;
      }
   }


     
/* Pasre the variuos #if statements and return 1 if processed */
int if_stat() {

   long stat;

   if (match("#if")) {
      stat = constant_exp();
      }
   else if (match("#ifdef")) {
      next_token();
      stat = factor();
      }
   else if (match("#ifndef")) {
      next_token();
      stat = factor();
      if (stat == 0) stat = 1;
      else if (stat == 1) stat = 0;
      }
   else return 0;

   NSTK[NFI.stkindx].numelm++;
   if (stat == 1) {
      PARSE.else_value = 0;
      if (NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm-1] & CP_BLK) {
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= PRO_DEF;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_ENIF;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= CP_BLK;
         }
      else {
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~PRO_DEF;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_BLK;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_ENIF;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
         }
      }
   else if (stat == 0) {
      PARSE.else_value = 1;
      if (NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm-1] & CP_BLK) {
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= PRO_DEF;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_BLK;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_ENIF;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
         }
    else {
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~PRO_DEF;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_BLK;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_ENIF;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
         }
       }
   else if (stat == IGNORE) {
      if (NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm-1] & CP_BLK) {
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= CP_BLK;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= CP_LINE;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= CP_ENIF;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~PRO_ELSE;
         }
      else {
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~PRO_DEF;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_BLK;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_ENIF;
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
         }
      }
   return 1;
   }

/* Pasre constant expression and return a long value if processed */
long constant_exp() {

   return log_or_exp();
   }

/* Pasre logical OR expression and return a long value if processed */
long log_or_exp() {

   long value,tempval;

   value = log_and_exp();
   if (value == IGNORE) {
      return IGNORE;
      }
   if (match("||")) {
      tempval =  log_or_exp();
      return (value || tempval);
      }
   if (match("==")) {
      tempval =  log_or_exp();
      return (!(value - tempval));
      }
   else {
      return value;
      }
   }

/* Pasre logical and expression and return a long value if processed */
long log_and_exp() {

   long value,tempval;

   value = factor();
   if (value == IGNORE) {
      return IGNORE;
      }
   next_token();
   if (match("&&")) {
      tempval = log_and_exp();
      return (value && tempval);
      }
   else {
      return value;
      }
   }


/* Pasre factor and return a long value if processed */
int factor() {

   int stat;
   long value;

   next_token();
   if(isdigit(PARSE.Token[0])) {
      return ((long)atoi(PARSE.Token));
      }
   else if(match("(")){
      value = constant_exp();
      if(match(")")){
         return value;
         }
      else {
         fprintf(stderr,"%s: %d: Undefined control %s\n",
         NFI.prgfn,NFI.wor_line[NFI.stkindx],PARSE.Token);
         return IGNORE;
         }
      }
   else if(match("defined")){
      return (factor());
      }
   else if(match("!")){
      next_token();
      if(match("defined")) {
         stat = factor();
         if (stat == 1){
            return 0;
            }
         else if (stat == 0) {
            return 1;
            }
         else {
            return IGNORE;
            }
         }
      else {
         fprintf(stderr,"%s: %d: Undefined control %s\n",
         NFI.prgfn,NFI.wor_line[NFI.stkindx],PARSE.Token);
         exit(1);
         }
      }
   else {
      if (lookup_sym(PARSE.Token,&cond_value) == NOTFOUND) {
         return IGNORE;
         }
      else {
         return cond_value;
         }
      }
   }

/* Get the next token */
int next_token() {

   static int pos;
   int i;

   if (PARSE.new_line) {
      pos = 0;
      if (PARSE.line[pos] == '#') {
         i=0;
         while (((PARSE.line[pos] != '\n') &&
                (PARSE.line[pos] != ' '))) {
            PARSE.Token[i] = PARSE.line[pos];
            pos++;
            i++;
            }
         PARSE.Token[i] = '\0';
         PARSE.new_line =0;
         return;
         }
      PARSE.Token[0] = '\0';
      return;
      }

   i=0;
   if (!feof(NFI.worfd)) {
      while (PARSE.line[pos] == '\t' || PARSE.line[pos] == ' ') {
         pos++;
         }

      if (PARSE.line[pos] == '\n') {
         return;
         }

      if (PARSE.line[pos] == '/') {
         pos++;
         if (PARSE.line[pos] == '*') {
            return;
            }
         else if (PARSE.line[pos] == '='){
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = PARSE.line[pos];
            PARSE.Token[2] = '\0';
            pos++;
            return;
            }
         else {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = '\0';
            return;
            }
         }

      if (PARSE.line[pos] == '=') {
         pos++;
         if (PARSE.line[pos] == '=') {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = PARSE.line[pos];
            PARSE.Token[2] = '\0';
            pos++;
            return;
            }
         else {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = '\0';
            return;
            }
         }

      if (PARSE.line[pos] == '|') {
         pos++;
         if (PARSE.line[pos] == '|') {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = PARSE.line[pos];
            PARSE.Token[2] = '\0';
            pos++;
            return;
            }
         else if (PARSE.line[pos] == '=') {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = PARSE.line[pos];
            PARSE.Token[2] = '\0';
            pos++;
            return;
            }
         else {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = '\0';
            return;
            }
         }

      if (PARSE.line[pos] == '&') {
         pos++;
         if (PARSE.line[pos] == '&') {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = PARSE.line[pos];
            PARSE.Token[2] = '\0';
            pos++;
            return;
            }
         else if (PARSE.line[pos] == '=') {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = PARSE.line[pos];
            PARSE.Token[2] = '\0';
            pos++;
            return;
            }
         else {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = '\0';
            return;
            }
         }

      if (PARSE.line[pos] == '^') {
         pos++;
         if (PARSE.line[pos] == '=') {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = PARSE.line[pos];
            PARSE.Token[2] = '\0';
            pos++;
            return;
            }
         else {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = '\0';
            return;
            }
         }

      if (PARSE.line[pos] == '~') {
         pos++;
         if (PARSE.line[pos] == '=') {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = PARSE.line[pos];
            PARSE.Token[2] = '\0';
            pos++;
            return;
            }
         else {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = '\0';
            return;
            }
         }

      if (PARSE.line[pos] == '<') {
         pos++;
         if (PARSE.line[pos] == '=') {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = PARSE.line[pos];
            PARSE.Token[2] = '\0';
            pos++;
            return;
            }
         else {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = '\0';
            return;
            }
         }

      if (PARSE.line[pos] == '>') {
         pos++;
         if (PARSE.line[pos] == '=') {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = PARSE.line[pos];
            PARSE.Token[2] = '\0';
            pos++;
            return;
            }
         else {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = '\0';
            return;
            }
         }

      if (PARSE.line[pos] == '*') {
         pos++;
         if (PARSE.line[pos] == '=') {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = PARSE.line[pos];
            PARSE.Token[2] = '\0';
            pos++;
            return;
            }
         else {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = '\0';
            return;
            }
         }

      if (PARSE.line[pos] == '!') {
         pos++;
         if (PARSE.line[pos] == '=') {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = PARSE.line[pos];
            PARSE.Token[2] = '\0';
            pos++;
            return;
            }
         else {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = '\0';
            return;
            }
         }

      if (PARSE.line[pos] == '-') {
         pos++;
         if (PARSE.line[pos] == '=') {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = PARSE.line[pos];
            PARSE.Token[2] = '\0';
            pos++;
            return;
            }
         else {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = '\0';
            return;
            }
         }

      if (PARSE.line[pos] == '(') {
         PARSE.Token[0] = PARSE.line[pos];
         PARSE.Token[1] = '\0';
         pos++;
         return;
         }

      if (PARSE.line[pos] == ')') {
         PARSE.Token[0] = PARSE.line[pos];
         PARSE.Token[1] = '\0';
         pos++;
         return;
         }

      if (PARSE.line[pos] == '/') {
         pos++;
         if (PARSE.line[pos] == '=') {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = PARSE.line[pos];
            PARSE.Token[2] = '\0';
            pos++;
            return;
            }
         else {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = '\0';
            return;
            }
         }

      if (PARSE.line[pos] == '%') {
         pos++;
         if (PARSE.line[pos] == '=') {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = PARSE.line[pos];
            PARSE.Token[2] = '\0';
            pos++;
            return;
            }
         else {
            PARSE.Token[0] = PARSE.line[pos-1];
            PARSE.Token[1] = '\0';
            return;
            }
         }

      i=0;
      if (isalpha(PARSE.line[pos])) {
         while (isalnum(PARSE.line[pos])) {
            PARSE.Token[i] = PARSE.line[pos];
            pos++;
            i++;
            }
         PARSE.Token[i] = '\0';
         return;
         }

      i=0;
      if (isdigit(PARSE.line[pos])) {
         while (isdigit(PARSE.line[pos])) {
            PARSE.Token[i] = PARSE.line[pos];
            pos++;
            i++;
            }
         PARSE.Token[i] = '\0';
         return;
         }

      i=0;
      if (PARSE.line[pos] == '"') {
         pos++;
         while ((PARSE.line[pos] != '"') && (PARSE.line[pos] != '\n')) {
            PARSE.Token[i] = PARSE.line[pos];
            pos++;
            i++;
            }
         PARSE.Token[i] = '\0';
         return;
         }
      }
   }

/* Check if defined variable is relevant and if it is, insert it in 
   the symbol table */
void define_sym() {
   
   long val,i=7;
   next_token();
   if (NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] & PRO_DEF) {
      while (PARSE.line[i] == '\t' || PARSE.line[i] == ' ') {
         i++;
         }
      i += strlen(PARSE.Token);
      while (PARSE.line[i] == '\t' || PARSE.line[i] == ' ') {
         i++;
         }
      if (PARSE.line[i] == '\n' || PARSE.line[i] == '/') {
         if (lookup_sym(PARSE.Token,&val) == NOTFOUND) { 
            nsi_define(PARSE.Token,1);
            }
         else {
            nsi_define(PARSE.Token,val);
            }
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
         }
      else if (NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] & CP_BLK) {
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= CP_LINE;
         }
      }
   }

/* Find the next directory to apply to searching in a 
   predefined order */
int next_dir() {
   
   if (!NSICDIR.nxt_dir) {
      strcpy(NSICDIR.wor_dir,NSICDIR.cur_dir);
      strcpy(NFI.worfn,NSICDIR.wor_dir);
      strcat(NFI.worfn,"/");
      if (PARSE.Token[0] == '/') {
         strcpy(NFI.worfn,PARSE.Token);
         }
      else {
         strcat(NFI.worfn,PARSE.Token);
         }
      NSICDIR.nxt_dir ++;
      return 1;
      }
   else if (NSICDIR.nxt_dir <= NSICDIR.numelm) {
      strcpy(NSICDIR.wor_dir,NSICDIR.inc_dir[NSICDIR.nxt_dir-1]);
      strcpy(NFI.worfn,NSICDIR.wor_dir);
      strcat(NFI.worfn,"/");
      if (PARSE.Token[0] == '/') {
         strcpy(NFI.worfn,PARSE.Token);
         }
      else {
         strcat(NFI.worfn,PARSE.Token);
         }
      NSICDIR.nxt_dir ++;
      return 1;
      }
   else {
      NSICDIR.nxt_dir = 0;
      return 0;
      }
   }

/* Process header files without copying the code to output */
int header_handler() {

   int t=0,wor_line;
   FILE *fd;

   next_token();
   fd = NFI.worfd;
   if (PARSE.Token[0] == '<') {
      NSICDIR.nxt_dir = 0;
      NFI.worfd = fd;
      return 1;
      }
   if (!next_dir()) {
      fprintf(stderr,"%s: %d: Can't find include file %s\n",
      NFI.prgfn,NFI.wor_line[NFI.stkindx],PARSE.Token);
      exit(1);
      }
      
   NFI.worfd = fopen(NFI.worfn,"r"); 
   while (NFI.worfd == NULL) {
      if (!next_dir()) {
         fprintf(stderr,"%s: %d: Can't find include file %s\n",
         NFI.prgfn,NFI.wor_line[NFI.stkindx],PARSE.Token);
         exit(47);
         }
      NFI.worfd = fopen(NFI.worfn,"r");
      }
   strcpy(NFI.prgfn,NFI.worfn);
   NFI.stkindx++;
   NSTK[NFI.stkindx].numelm = 0;
   NSTK[NFI.stkindx].arrlen = STKMAX;
   NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= CP_BLK;
   NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= CP_LINE;
   while(!feof(NFI.worfd) && !t){
      PARSE.proc_head = 1;
      NSICDIR.nxt_dir = 0;
      t = parse();
      }
   NFI.stkindx--;
   NSICDIR.nxt_dir = 0;
   NFI.worfd = fd;
   return 1;
   }
   
int lookup_sym(char *symbol,long *val) {
   
   int count=0;
  
   if (SYMBOLS.numelm == 0) {
      return NEWSYM;
      }
   while (count < SYMBOLS.numelm) {
      if (strcmp(SYMBOLS.nsisymt[count].def_name,symbol)==0) {
         *val = SYMBOLS.nsisymt[count].def_value;
         return count;
         }
      count++;
      }
   return NOTFOUND;
   }

/* Close all related files and free all allocated memory before leaving
   the program */
void exit_program() {

   free(NSIHDF);
   free(SYMBOLS.nsisymt);
   free(NSICDIR.inc_dir);
   if (NFI.infd) fclose(NFI.infd);
   if (NFI.outfd) fclose(NFI.outfd);
   if (NFI.worfd) fclose(NFI.worfd);
   if (NFI.tmpfd) fclose(NFI.tmpfd);
   }

/* Match the argument to the function with the ckurrent token */
int match(char *tkn) {

   if (strcmp(PARSE.Token,tkn) == 0){
      return 1;  
      }
   return 0;
   }

/* Get a new line */
int get_new_line() {

   NFI.wor_line[NFI.stkindx]++;
   PARSE.new_line = 1;
   if (!feof(NFI.worfd)) {
      fgets(PARSE.line,(NSIBFLEN-1),NFI.worfd);
      return 1;
      }
   else {
      return ENOFILE;
      }
   }

/* This is the core of the program and the actual parsing is done */
int parse() {

   int copy_line,proc_head;
   char line_buff[NSIBFLEN];

   copy_line =  
       NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] & CP_LINE;
   proc_head  = PARSE.proc_head;
   if (!get_new_line()) {
      return 1;
      }
   strcpy(line_buff,PARSE.line);
   next_token();

   if (match("#define")) {
      define_sym();
      copy_line =  
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] & CP_LINE;
      if (copy_line && !proc_head && !feof(NFI.worfd)) {
         fprintf(NFI.outfd,"%s",line_buff);
         }
      copy_line = 0;
      }
   else if (match("#include")) {
      if (copy_line && !proc_head) {
         fprintf(NFI.outfd,"%s",line_buff);
         }
      PARSE.proc_head = proc_head = 1;
      header_handler();
      if (NFI.worfd == NFI.infd) {
         NFI.stkindx = 0;
         PARSE.proc_head = proc_head = 0;
         }
      }
   else if (if_stat()) {
      int t=0;
      copy_line =  
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] & CP_LINE;
      proc_head  = PARSE.proc_head;
      if (copy_line && !proc_head) {
         fprintf(NFI.outfd,"%s",line_buff);
         }
      copy_line = 0;
      }
   else if (elif_stat()) {
      copy_line =  
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] & CP_LINE;
      proc_head  = PARSE.proc_head;
      if (copy_line && !PARSE.proc_head) {
         fprintf(NFI.outfd,"%s",line_buff);
         }
      copy_line = 0;
      }
   else if (else_stat()) {
      copy_line =  
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] & CP_LINE;
      proc_head  = PARSE.proc_head;
      if (copy_line && !proc_head) {
         fprintf(NFI.outfd,"%s",line_buff);
         }
      copy_line = 0;
      }
   else if (end_stat()) {
      proc_head  = PARSE.proc_head;
      copy_line =  
         NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] & CP_ENIF;
      NSTK[NFI.stkindx].numelm--;
      if (copy_line && !proc_head && !feof(NFI.worfd)) {
         fprintf(NFI.outfd,"%s",line_buff);
         }
      copy_line = 0;
      }
   else if (copy_line && !proc_head && !feof(NFI.worfd)) {
      fprintf(NFI.outfd,"%s",line_buff);
      }

   if ((NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] & CP_BLK)) {
      NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] |= CP_LINE;
      }
   else {
      NSTK[NFI.stkindx].flg[NSTK[NFI.stkindx].numelm] &= ~CP_LINE;
      }
   return 0;
   }


main(int argc, char *argv[]){

   nsi_init();
   process_args(argc,argv);
   while(!feof(NFI.worfd)) {
      parse();
      }
   exit_program();
   }

