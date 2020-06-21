/***********************************************************************
* Copyright 2007 by Jack Lloyd, All Rights Reserved
*                   jack@stormyskies.net
*
* FILE: nas.cpp
* DESC: main() for the NPU Assembler, and associated glue code.
*
* REVISION HISTORY (Date, Modifier: Description)
* ----------------------------------------------
*
* 2007-01-28, Jack Lloyd:
* Creation.
*
* 2007-03-21, Jack Lloyd:
*   - new register keyword logic (reg0, reg1, .... vs. r[0-9a-f]{1,2})
*   - still record label declarations in the face of errors (so as not
*     to declare undue error when a declared label is used); involves
*     different logic for NAS_OUTPUT_STATE_First_Pass
*   - delete output files when error detected and we quit generating
*     output
*   - UCF output now requires a memref argument
*   - various bugs fixed in VHDL and UCF output
*
* 2007-08-14, Jack Lloyd:
* Reversed output of VHDL/UCF format so that each instruction is padded
*  from the left instead of the right per the specification.
*
* 2007-10-07, Jack Lloyd:
* [2.x] Added new instructions (see "GPU_CODES_2007_10_03.pdf").
*
* 2008-01-27, Jack Lloyd:
* Modifications to output format code:
*    - UCF Format fix
*    - Dual UCF/VHDL bug fix
*    - addition of new MEM output format
*
* 2009-09-14, Jack Lloyd:
* [2.5] Added new instructions JUMP, CALL, RET
*       Added ability to dump "symbol table"
*
* Rev, 10/30/09, GNR - Modify for gcc compilation
* Rev, 11/04/09, GNR - Add 'IsLabel' field to label_record_t
* Rev, 11/04/09, GNR - Add neutral_count to eliminate unnecessary NOPs
* Rev, 11/26/09, GNR - Count ticks, not instructions, for NOP addition
***********************************************************************/

#ifndef PCLINUX
#include "stdafx.h"
#endif
#include "stdio.h"
#include "stdarg.h"
#ifdef PCLINUX
#include "stdlib.h"
#include <string>
#include <iostream>
#include "malloc.h"
#include "unistd.h"
#endif
#include "nas.h"

#ifdef PCLINUX
// GNR Additions -- apparent usages from Visual C?
#define DeleteFile unlink
#define TCHAR char
#define _strdup strdup
#define _tmain main
#endif

extern "C" {
#include "parser.tab.h" // bison generated header for token values
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef PCLINUX
// The one and only application object
CWinApp theApp;
#endif

using namespace std;

// input file
static char * nas_in_filename = NULL;

// output files
static char * nas_raw_out_filename = NULL;
static FILE * nas_out_raw  = NULL;
static char * nas_vhdl_out_filename = NULL;
static FILE * nas_out_vhdl = NULL;
static char * nas_ucf_out_filename = NULL;
static FILE * nas_out_ucf  = NULL;
static char * nas_ucf_mem_ref = NULL;
static char * nas_mem_out_filename = NULL;
static FILE * nas_out_mem  = NULL;
static char * nas_mem_preamble = NULL;

// indicates what state of code generation we're in
nas_output_state_t nasOutputState = NAS_OUTPUT_STATE_First_Pass;

// keeping track of errors and warnings
static unsigned int total_errors_found = 0;
static unsigned int total_warnings_found = 0;
static unsigned int total_NOPs_inserted = 0;
static unsigned int total_NOP_locations = 0;

// passback variable between lexer and parser
struct YYLTYPE yylloc;

// Linked list of labels and their definitions
static label_record_t * labels = NULL;

// tracks absolute machine code instruction offset
static nas_address_t nas_next_instruction_address = 0;

// Counts innocuous instructions after ACC instructions
static int neutral_count = 0;

// Flag to control detaild listing of NOP insertions
static bool qListNOPs = true;

/***********************************************************************
 * FUNCTION: nas_getline
 *
 * DESCRIPTION:
 * Iterates through each file specified on the command-line in turn and
 * returns one line per call.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * STATE:
 *
 * FILE * currFile
 * File currently reading from, NULL if no such file.
 *
 * int curFileIndex
 * Next command-line argument to look at for a file name in the face of
 * a NULL currFile.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ARGUMENTS:  void
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * RETURN:
 * A string representing the next line in the file, NULL if all contents
 * from all files have been exhausted.  Note that space for the returned
 * string is statically allocated in a global buffer, so calling this
 * function causes strings from previous calls to be lost (unless the
 * caller has copied).
 *
***********************************************************************/
#define LINE_BUF_SIZE 1024
static char lineBuffer[LINE_BUF_SIZE];
static char * nas_getline(void)
{
    static FILE * inFile = NULL;
    char * retLine = NULL;

    // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    // Open the input file if it's not opened already.
    // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    if (NULL == inFile)
    {
      if (NULL == (inFile = fopen(nas_in_filename, "r")))
        nas_errorP(NULL, "can't open file %s", nas_in_filename);
      else
      {
        yylloc.fileName = nas_in_filename;
        yylloc.lineNum = 0;
      }
    }

    // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    // Look for lines marked by CR, LF, or CR-LF.  Don't bother
    // returning blank lines.
    // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    while ((NULL != inFile) && (NULL == retLine))
    {
      unsigned lineBufIdx = 0;
      int c;

      // keep reading in chars to make up the new line until we see the
      // end of line character or EOF
      while ((EOF != (c = fgetc(inFile))) && ('\r' != c) && ('\n' != c))
      {
        if (LINE_BUF_SIZE - 1 <= lineBufIdx)
        {
          nas_errorP(&yylloc, "line too big!");
          lineBufIdx = 0;
          break;
        }
        else lineBuffer[lineBufIdx++] = c;
      }
      if (0 < lineBufIdx)
          {
                  retLine = lineBuffer;
                  lineBuffer[lineBufIdx] = '\0';
          }
      ++(yylloc.lineNum); // increment parsers line number

      // see about closing this file (only if we're about to return
      // NULL, thus telling caller that we're done...this fixes the
      // infinite loop when the input file's last line has no CR/CRLF)
      if ((EOF == c) && (NULL == retLine))
      {
        if (fclose(inFile)) nas_warnP(&yylloc, "can't close file %s",
         nas_in_filename);
        inFile = NULL;
      }
    }

    return retLine;
}

/***********************************************************************
 * FUNCTION: get_label_record
 *
 * DESCRIPTION:
 * Attempts to retrieve a label record based on a given name.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * STATE:
 *
 * label_record_t * labels
 * Looks through this linked list of labels in its search.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ARGUMENTS:
 *
 * const char * name
 * Name to search for.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * RETURN:
 *
 * A pointer to the first matching label record in labels, NULL if no
 * match is found.
 *
***********************************************************************/
label_record_t * get_label_record(const char * name)
{
  label_record_t * candidate = NULL;

  if (NULL != name)
  {
    for (candidate = labels;
        ((NULL != candidate) && (0 != strcmp(candidate->name, name)));
        candidate = candidate->next) /* nada */;
  }

  return candidate;
}

/***********************************************************************
 * FUNCTION: put_label_record
 *
 * DESCRIPTION:
 * Places a given label record into the global linked list of label
 * records (at the head).
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * STATE:
 *
 * label_record_t * labels
 * Adds a label record to this linked-list of label records.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ARGUMENTS:
 *
 * label_record_t * new_label_record
 * Record to add to the global list.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * RETURN:
 *
 * A pointer to the newly added (and NOT copied!) label record, which
 * will also be the head of the global list of label records.  If
 * anything goes wrong (which it shouldn't unless the caller screwed
 * something up) NULL is returned.
 *
***********************************************************************/
label_record_t * put_label_record(label_record_t * new_label_record)
{
  if (NULL != new_label_record)
  {
    // NB: when adding to empty list, labels will be NULL...
    new_label_record->next = labels;
    labels = new_label_record;
  }

  return new_label_record;
}

/***********************************************************************
 * FUNCTION: generate_new_label_record
 *
 * DESCRIPTION:
 * Generates (and creates memory for) a new label based on the current
 * address in the current nas assembler run and a given name.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * STATE:
 *
 * unsigned int nas_next_instruction_address
 * Reads this to determine what address to use in the newly generated
 * label record.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ARGUMENTS:
 *
 * const char * new_label_name
 * Name for the new label.  Caller must ensure the memory used for the
 * name is already allocated; this function only uses pointers.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * RETURN:
 *
 * A pointer to the newly created record, NULL if the record could not
 * be generated for some reason (only way could be out of memory!).
 *
***********************************************************************/
label_record_t * generate_new_label_record(const char * new_label_name)
{
  label_record_t * new_record = NULL;

  if (NULL != (new_record =
   (label_record_t * )malloc(sizeof(*new_record))))
  {
    // NB: certainly not incremented here...
    new_record->address = nas_next_instruction_address;
    new_record->name    = new_label_name;
    new_record->fUsed   = false;
    new_record->IsLabel = false;
  }

  return new_record;
}

/***********************************************************************
 * FUNCTION: yylex
 *
 * DESCRIPTION:
 * Lexical analyzer front end for nas.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * STATE:
 *
 * char *currLine
 * Line we're currently parsing, NULL if a new line needs to be fetched.
 *
 * char *next_token
 * Used to horde tokens in some cases involving :.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ARGUMENTS: void
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * RETURN:
 *
 * int representing next token for GNU Bison generated parser to deal
 * with.  0 or less indicates end of all tokens.  Anything less than 256
 * is an ASCII character.  Higher than that are Bison generated token
 * #defines based on the GNU Bison grammar file in parser.y.  These are
 * defined in the GNU Bison generated header parser.tab.h.
 *
***********************************************************************/
int yylex(void)
{
  static char *currLine = NULL;
  // geez, I need to look at lex or flex...
  static char *colonLocation = NULL;
  static char *next_token = NULL;
  int retCode = 0;
  bool fCurrLineChanged = false;
  char * token = NULL;
  char * endChar;

  // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
  // replace any nulled : from the previous call and reprocess with a
  // possibly hoarded token
  // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
  if (NULL != colonLocation)
  {
    *colonLocation = ':';
    next_token = colonLocation;
    colonLocation = NULL;
  }

  // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
  // see if we have a token horded from a previous call
  // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
  if (NULL != next_token)
  {
    token = next_token;
    next_token = NULL;
  }

  // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
  // see about getting a line if we don't have one
  // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
  else if (NULL == currLine)
  {
    while (NULL != (currLine = nas_getline()))
    {
      char * startOfComment;

      // truncate any comments
      if (NULL != (startOfComment = strchr(currLine, ';')))
         *startOfComment = '\0';

      // if non-commentary text remains, break out so we can lexically
      // analyze it
      if ('\0' != *currLine) break;
    }

    fCurrLineChanged = true;
  }

  // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
  // if we have no new input and nothing horded, then we're pretty much
  // done here
  // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
  if ((NULL == token) && (NULL == currLine))
   retCode = 0; // signifies end of tokens

  // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
  // see we already have a token horded or if we can get one
  // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
  else if ((NULL != token) ||
      (NULL != (token = strtok((fCurrLineChanged ? currLine : NULL),
      " \t"))))
  {
    //  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :
    // colon processing (need to look into flex or lex)...by the nature
    // of our strtok() calls, the : comes either at the beginning,
    // middle, or end of token, or alone by itself
    //  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :  :

    // : alone
    if (0 == strcmp(token, ":")) retCode = ':';

    // : at beginning of token
    else if ((colonLocation = strchr(token, ':')) == token)
    {
      // don't bother storing this because we're about to return :
      colonLocation = NULL;
      // NB: this returns as many : as there are in ::::foo, or
      // foo::bar, or foo : : bar, etc.
      next_token = token + 1;
      retCode = ':';
    }

    else // no : or non-simple :
    {
      // : in the middle of or at the end of token...: nulled in this
      // call so we can analyze what comes before it, and restored and
      // reprocessed in one of the two casses above in the next call
      if (NULL != colonLocation) *colonLocation = '\0';

      // look for instructions NB: no semantic value for these; sytax =
      // semantics here
           if (0 == strcmp(token, "nop"))      retCode = NOP;
      else if (0 == strcmp(token, "clrp"))     retCode = CLRP;
      else if (0 == strcmp(token, "imm"))      retCode = IMM;
      else if (0 == strcmp(token, "addacc"))   retCode = ADDACC;
      else if (0 == strcmp(token, "subtacc"))  retCode = SUBTACC;
      else if (0 == strcmp(token, "mult"))     retCode = MULT;
      else if (0 == strcmp(token, "multacc"))  retCode = MULTACC;
      else if (0 == strcmp(token, "load"))     retCode = LOAD;
      else if (0 == strcmp(token, "wload"))    retCode = WLOAD;
      else if (0 == strcmp(token, "compare"))  retCode = COMPARE;
      else if (0 == strcmp(token, "wcompare")) retCode = WCOMPARE;
      else if (0 == strcmp(token, "stash"))    retCode = STASH;
      else if (0 == strcmp(token, "pushp"))    retCode = PUSHP;
      else if (0 == strcmp(token, "popp"))     retCode = POPP;
      else if (0 == strcmp(token, "testp"))    retCode = TESTP;
      else if (0 == strcmp(token, "shftp"))    retCode = SHFTP;
      else if (0 == strcmp(token, "inc"))      retCode = INC;
      else if (0 == strcmp(token, "dec"))      retCode = DEC;
      else if (0 == strcmp(token, "saveplo"))  retCode = SAVEPLO;
      else if (0 == strcmp(token, "savepmd"))  retCode = SAVEPMD;
      else if (0 == strcmp(token, "savephi"))  retCode = SAVEPHI;
      else if (0 == strcmp(token, "jump"))     retCode = JUMP;
      else if (0 == strcmp(token, "jumpcy"))   retCode = JUMPCY;
      else if (0 == strcmp(token, "jumpn"))    retCode = JUMPN;
      else if (0 == strcmp(token, "jumpnz"))   retCode = JUMPNZ;
      else if (0 == strcmp(token, "jumpz"))    retCode = JUMPZ;
      else if (0 == strcmp(token, "call"))     retCode = CALL;
      else if (0 == strcmp(token, "ret"))      retCode = RET;
      else if (0 == strcmp(token, "or"))       retCode = OR;
      else if (0 == strcmp(token, "wor"))      retCode = WOR;
      else if (0 == strcmp(token, "and"))      retCode = AND;
      else if (0 == strcmp(token, "wand"))     retCode = WAND;
      else if (0 == strcmp(token, "fetch"))    retCode = FETCH;
      else if (0 == strcmp(token, "wfetch"))   retCode = WFETCH;
      else if (0 == strcmp(token, "shftar"))   retCode = SHFTAR;
      else if (0 == strcmp(token, "shftbr"))   retCode = SHFTBR;
      else if (0 == strcmp(token, "shftll"))   retCode = SHFTLL;
      else if (0 == strcmp(token, "shftlr"))   retCode = SHFTLR;

      // look for register designation next
      else if (0 == strcmp(token, "reg0"))
         { retCode = REG; yylval.operand = 0x0; }
      else if (0 == strcmp(token, "reg1"))
         { retCode = REG; yylval.operand = 0x1; }
      else if (0 == strcmp(token, "reg2"))
         { retCode = REG; yylval.operand = 0x2; }
      else if (0 == strcmp(token, "reg3"))
         { retCode = REG; yylval.operand = 0x3; }
      else if (0 == strcmp(token, "reg4"))
         { retCode = REG; yylval.operand = 0x4; }
      else if (0 == strcmp(token, "reg5"))
         { retCode = REG; yylval.operand = 0x5; }
      else if (0 == strcmp(token, "reg6"))
         { retCode = REG; yylval.operand = 0x6; }
      else if (0 == strcmp(token, "reg7"))
         { retCode = REG; yylval.operand = 0x7; }
      else if (0 == strcmp(token, "reg8"))
         { retCode = REG; yylval.operand = 0x8; }
      else if (0 == strcmp(token, "reg9"))
         { retCode = REG; yylval.operand = 0x9; }
      else if (0 == strcmp(token, "rega"))
         { retCode = REG; yylval.operand = 0xa; }
      else if (0 == strcmp(token, "regb"))
         { retCode = REG; yylval.operand = 0xb; }
      else if (0 == strcmp(token, "regc"))
         { retCode = REG; yylval.operand = 0xc; }
      else if (0 == strcmp(token, "regd"))
         { retCode = REG; yylval.operand = 0xd; }
      else if (0 == strcmp(token, "rege"))
         { retCode = REG; yylval.operand = 0xe; }
      else if (0 == strcmp(token, "regf"))
         { retCode = REG; yylval.operand = 0xf; }

      // address mnemonic def
      else if (0 == strcmp(token, "defNum")) retCode = DEF_NUM;

      // look for a number
      else
      {
        yylval.operand = strtoul(token, &endChar, 0);
        if ('\0' == *endChar) retCode = NUM;
        else // number doesn't make sense, so call it a label (though
             // it could just be some random bullsh*t without :)
        {
          retCode = LABEL;
          yylval.string = _strdup(token);
        } // no, a label
      } // a number, perhaps?
    } // no : or non-simple :
  } // we had or got a token to process

  else // token is NULL, so we must be at the end of the line
  {
    retCode = '\n';
    currLine = NULL; // forces read of next line when called again
  }

  return retCode;
}

/***********************************************************************
 * FUNCTION: yyerror
 *
 * DESCRIPTION:
 * Called by the parser on a parsing error (generally a syntax error).
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * STATE: stateless
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ARGUMENTS:
 *
 * const char *s
 * GNU Bison generated error string.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * RETURN: void
 *
***********************************************************************/
void yyerror(const char *s)
{
    nas_errorP(&yylloc, "%s", s);
}

/***********************************************************************
 * FUNCTION: nas_error
 *
 * DESCRIPTION:
 * Reports error to user (and halts output processing).
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * STATE: stateless
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ARGUMENTS:
 *
 * YYLTYPE * loc
 * Location of the offending text.  If NULL, location taken to be the
 * top-level.
 *
 * const char *text
 * Description of what's wrong.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * RETURN: void
 *
***********************************************************************/
void nas_error(YYLTYPE loc, const char *text, ...) // bison accomodation
{
  va_list args;

  va_start(args, text);
  nas_errorPv(&loc, text, args);
  va_end(args);
}
void nas_errorP(YYLTYPE * loc, const char *text, ...)
{
  va_list args;

  va_start(args, text);
  nas_errorPv(loc, text, args);
  va_end(args);
}
void nas_errorPv(YYLTYPE * loc, const char *text, va_list args)
{
  ++total_errors_found;

  fprintf(stderr, "nas: ERROR: (");
  if (NULL == loc) fprintf(stderr, "top-level");
  else fprintf(stderr, "%s, %u", loc->fileName, loc->lineNum);
  fprintf(stderr, "): ");
  vfprintf(stderr, text, args);
  fprintf(stderr, "\n");

  // stop outputing stuff if we're not on the first pass....if we're on
  // the first pass, keep on going as we want to record label
  // declarations
  if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
    nasOutputState = NAS_OUTPUT_STATE_Error_Found;
}

/***********************************************************************
 * FUNCTION: nas_warn
 *
 * DESCRIPTION:
 * Warns the user (but doesn't halts output processing).
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * STATE: stateless
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ARGUMENTS:
 *
 * YYLTYPE * loc
 * Location of the offending text.  If NULL, location taken to be the
 * top-level.
 *
 * const char *text
 * Description of what's wrong.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * RETURN: void
 *
***********************************************************************/
void nas_warn(YYLTYPE loc, const char *text, ...) // bison accomodation
{
  va_list args;

  va_start(args, text);
  nas_warnPv(&loc, text, args);
  va_end(args);
}
void nas_warnP(YYLTYPE * loc, const char *text, ...)
{
  va_list args;

  va_start(args, text);
  nas_warnPv(loc, text, args);
  va_end(args);
}
void nas_warnPv(YYLTYPE * loc, const char *text, va_list args)
{
  ++total_warnings_found;

  fprintf(stderr, "nas: warning: (");
  if (NULL == loc) fprintf(stderr, "top-level");
  else fprintf(stderr, "%s, %u", loc->fileName, loc->lineNum);
  fprintf(stderr, "): ");
  vfprintf(stderr, text, args);
  fprintf(stderr, "\n");
}

/***********************************************************************
 * FUNCTION: nas_output_vhdl, nas_output_ucf
 *
 * DESCRIPTION:
 * Output VHDL or UCF init commands to a file based on how nas organizes
 * NPU opcodes into VHDL and UCF commands.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * STATE:
 *
 * Raw commands are buffered until enough instructions are added to
 * produce a complete INIT / INITP set.  This buffer can be flushed
 * and 0's padded into an INIT / INITP pair based on arguments to this
 * function (see below).  Also, index values for INIT and INITP
 * commands, as well as index values for NPU opcodes in the INIT and
 * INITP commands persist between non-flushing calls.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ARGUMENTS:
 *
 * FILE *f
 * File to output VHDL or UCF commands to.  Call with NULL to indicate
 * that commands should be buffered and dumped to the same file as
 * before.  Call with non-NULL to start a new output sequence (with new
 * index values for the INIT / INITP / NPU OpCode indicies).  Calling
 * with non-NULL does NOT flush any partially formed INIT / INITP
 * commands.  To do that, see the next argument.
 *
 * complete_instruction_t val
 * NPU machine code to buffer/output.  Call with
 * NAKED_INSTR_no_instruction to cause any partially formed INIT /
 * INITP pairs to be padded and dumped.  If called with
 * NAKED_INSTR_no_instruction and with f non-NULL, nothing happens.
 *
 * char * ucf [nas_output_ucf only]
 * Memory reference string used in a UCF initialization instruction.
 * This argument is ignored when flushing or buffering.  This
 * argument is only used when changing files, so it need only be
 * passed once when the file handle is given and can be called with
 * NULL thereafter.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * RETURN: void
 *
***********************************************************************/

// Converts order of instruction execution to VHDL/UCF ("mem init")
// order.  Observe that when looking at VHDL or UCF initialization
// commands, the opcodes should appear in the following order:
//
//   15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
//   31 30 29 28 27 26 25 24 23 22 21 29 19 18 17 16
//   47 46 45 44 43 42 41 40 39 38 37 36 35 34 33 32
//   ...
//
// So some time with an excel spreadsheet should convince one that the
// conversion is:
//
//                                 |  i   |
//   M = (15 - (i mod 16)) + (16 * | ---- | )
//                                 |_ 16 _|
//
// where i is the execution order and M is the memory init order.
// Without further ado...
#define EXEC_TO_MEM_INIT_ORDER(i) \
  ((15 - ((i) % 16)) + (16 * ((i)/16)))

// Of course, that just covers the INIT_xx commands, not the
// INITP_xx commands (which contain the high-order two bits for
// 128 instructions).  The conversion for that is simple:  M = 127 - i:
#define EXEC_TO_MEM_INITP_ORDER(i) (127 - (i))

static void nas_output_vhdl(FILE *f, complete_instruction_t val)
{
  static FILE * current_out_file = NULL;
  static complete_instruction_t buffered_opcodes[128];
  static unsigned char npu_opcode_idx = 0;
  static unsigned int initp_idx = 0;
  int i;

  // changing files?
  if (NULL != f)
  {
    initp_idx = 0;
    npu_opcode_idx = 0;
    current_out_file = f;
  }

  // do we have a file to write to?
  if (NULL != current_out_file)
  {
    // flushing opcode buffer?
    // forced flush case
    if (((NAKED_INSTR_no_instruction == val) && (0 != npu_opcode_idx))
      || (128 == npu_opcode_idx))   // must flush case
    {
      // fill any unused parts of the opcode buffer with 0's so that
      // it outputs with padded INIT and INITP instructions
      for (i = npu_opcode_idx; 128 > i; ++i)
         buffered_opcodes[i] = NAKED_INSTR_nop;

      // dump INIT part of buffered instructions
      for (i = 0; 128 > i; ++i)
      {
        if (0 == i % 16)
        {
          // unless this is the very first statement in the VHDL file,
          // output a coma and whitespace to separate it from the last
          if ((0 != initp_idx) || (0 != i))
            fprintf(current_out_file, ",\n");
#ifdef PCLINUX
          fprintf(current_out_file, "INIT_%02X => X\"%4.4lx",
            (i / 16) + (8 * initp_idx), OPCODE_LO_BITS(
            buffered_opcodes[EXEC_TO_MEM_INIT_ORDER(i)]));
#else
          fprintf(current_out_file, "INIT_%02X => X\"%4.4x",
            (i / 16) + (8 * initp_idx), OPCODE_LO_BITS(
            buffered_opcodes[EXEC_TO_MEM_INIT_ORDER(i)]));
#endif
        }
        else if (15 != i % 16)
#ifdef PCLINUX
          fprintf(current_out_file, "%4.4lx", OPCODE_LO_BITS(
            buffered_opcodes[EXEC_TO_MEM_INIT_ORDER(i)]));
#else
          fprintf(current_out_file, "%4.4x", OPCODE_LO_BITS(
            buffered_opcodes[EXEC_TO_MEM_INIT_ORDER(i)]));
#endif
        else // last opcode in this INIT statement
#ifdef PCLINUX
          fprintf(current_out_file, "%4.4lx\"", OPCODE_LO_BITS(
            buffered_opcodes[EXEC_TO_MEM_INIT_ORDER(i)]));
#else
          fprintf(current_out_file, "%4.4x\"", OPCODE_LO_BITS(
            buffered_opcodes[EXEC_TO_MEM_INIT_ORDER(i)]));
#endif
      }

      // dump INITP part of buffered instructions
      for (i = 0; 128 > i; i += 4)
      {
        if (0 == i)
        {
          // NB: this cannot possibly be the very first statement in
          // the VHDL file, so output , and CR unconditionally
          fprintf(current_out_file, ",\n");
#ifdef PCLINUX
          fprintf(current_out_file, "INITP_%02X => X\"%2.2lx",
            initp_idx,
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER( i )]) << 6) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+1)]) << 4) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+2)]) << 2) |
             OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+3)]));
#else
          fprintf(current_out_file, "INITP_%02X => X\"%2.2x",
            initp_idx,
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER( i )]) << 6) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+1)]) << 4) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+2)]) << 2) |
             OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+3)]));
#endif
        }
        else if (124 != i)
#ifdef PCLINUX
          fprintf(current_out_file, "%2.2lx",
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER( i )]) << 6) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+1)]) << 4) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+2)]) << 2) |
             OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+3)]));
#else
          fprintf(current_out_file, "%2.2xlx",
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER( i )]) << 6) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+1)]) << 4) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+2)]) << 2) |
             OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+3)]));
#endif
        else // last 4 opcodes in this INITP statement
        {
#ifdef PCLINUX
          fprintf(current_out_file, "%2.2lx\"",
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER( i )]) << 6) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+1)]) << 4) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+2)]) << 2) |
             OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+3)]));
#else
          fprintf(current_out_file, "%2.2x\"",
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER( i )]) << 6) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+1)]) << 4) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+2)]) << 2) |
             OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+3)]));
#endif
        }
      }

      // get ready for next INITP group
      ++initp_idx;
      npu_opcode_idx = 0;
    }

    // buffer opcode so long as we weren't doing a forced flush
    if (NAKED_INSTR_no_instruction != val)
      buffered_opcodes[npu_opcode_idx++] = val;
  }

  return;
}

static void nas_output_ucf(FILE *f, complete_instruction_t val,
      char * mem_ref)
{
  static FILE * current_out_file = NULL;
  static char * ucf_mem_ref = NULL;
  static complete_instruction_t buffered_opcodes[128];
  static unsigned char npu_opcode_idx = 0;
  static unsigned int initp_idx = 0;
  int i;

  // changing files?
  if (NULL != f)
  {
    initp_idx = 0;
    npu_opcode_idx = 0;
    current_out_file = f;
    if (NULL == (ucf_mem_ref = mem_ref))
    {
      nas_warnP(NULL, "internal error; UCF mem ref somehow NULL!\r\n");
      ucf_mem_ref = "!!! BAD UCF MEM REF !!!";
    }
  }

  // do we have a file to write to?
  if (NULL != current_out_file)
  {
    // flushing opcode buffer?
    // forced flush case
    if (((NAKED_INSTR_no_instruction == val) && (0 != npu_opcode_idx))
      || (128 == npu_opcode_idx))   // must flush case
    {
      // fill any unused parts of the opcode buffer with 0's so that
      // it outputs with padded INIT and INITP instructions
      for (i = npu_opcode_idx; 128 > i; ++i)
         buffered_opcodes[i] = NAKED_INSTR_nop;

      // dump INIT part of buffered instructions
      for (i = 0; 128 > i; ++i)
      {
        if (0 == i % 16)
        {
#ifdef PCLINUX
          fprintf(current_out_file, "INST \"%s\" INIT_%02X = 0x%4.4lx",
            ucf_mem_ref, (i / 16) + (8 * initp_idx),
            OPCODE_LO_BITS(buffered_opcodes[EXEC_TO_MEM_INIT_ORDER(i)]));
#else
          fprintf(current_out_file, "INST \"%s\" INIT_%02X = 0x%4.4x",
            ucf_mem_ref, (i / 16) + (8 * initp_idx),
            OPCODE_LO_BITS(buffered_opcodes[EXEC_TO_MEM_INIT_ORDER(i)]));
#endif
        }
        else if (15 != i % 16)
#ifdef PCLINUX
          fprintf(current_out_file, "%4.4lx",
            OPCODE_LO_BITS(buffered_opcodes[EXEC_TO_MEM_INIT_ORDER(i)]));
#else
          fprintf(current_out_file, "%4.4x",
            OPCODE_LO_BITS(buffered_opcodes[EXEC_TO_MEM_INIT_ORDER(i)]));
#endif
        else // last opcode in this INIT statement
#ifdef PCLINUX
          fprintf(current_out_file, "%4.4lx;\n",
            OPCODE_LO_BITS(buffered_opcodes[EXEC_TO_MEM_INIT_ORDER(i)]));
#else
          fprintf(current_out_file, "%4.4x;\n",
            OPCODE_LO_BITS(buffered_opcodes[EXEC_TO_MEM_INIT_ORDER(i)]));
#endif
      }

      // dump INITP part of buffered instructions
      for (i = 0; 128 > i; i += 4)
      {
        if (0 == i)
        {
#ifdef PCLINUX
          fprintf(current_out_file, "INST \"%s\" INITP_%02X = 0x%2.2lx",
            ucf_mem_ref, initp_idx,
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER( i )]) << 6) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+1)]) << 4) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+2)]) << 2) |
            OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+3)]));
#else
          fprintf(current_out_file, "INST \"%s\" INITP_%02X = 0x%2.2x",
            ucf_mem_ref, initp_idx,
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER( i )]) << 6) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+1)]) << 4) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+2)]) << 2) |
            OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+3)]));
#endif
        }
        else if (124 != i)
#ifdef PCLINUX
          fprintf(current_out_file, "%2.2lx",
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER( i )]) << 6) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+1)]) << 4) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+2)]) << 2) |
            OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+3)]));
#else
          fprintf(current_out_file, "%2.2lx",
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER( i )]) << 6) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+1)]) << 4) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+2)]) << 2) |
            OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+3)]));
#endif
        else // last 4 opcodes in this INITP statement
        {
#ifdef PCLINUX
          fprintf(current_out_file, "%2.2lx;\n",
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER( i )]) << 6) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+1)]) << 4) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+2)]) << 2) |
            OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+3)]));
#else
          fprintf(current_out_file, "%2.2x;\n",
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER( i )]) << 6) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+1)]) << 4) |
            (OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+2)]) << 2) |
            OPCODE_HI_BITS(buffered_opcodes[EXEC_TO_MEM_INITP_ORDER(i+3)]));
#endif
        }
      }

      // get ready for next INITP group
      ++initp_idx;
      npu_opcode_idx = 0;
    }

    // buffer opcode so long as we weren't doing a forced flush
    if (NAKED_INSTR_no_instruction != val)
      buffered_opcodes[npu_opcode_idx++] = val;
  }

  return;
}

/***********************************************************************
 * FUNCTION: nas_cleanup
 *
 * DESCRIPTION:
 * Do whatever's necessary to close down nas, including closing output
 * files, optionally deleting output files, etc.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * STATE: stateless
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ARGUMENTS:
 *
 * bool fTrashOutputFiles
 * Flag indicating whether or not caller wants any open output files
 * deleted as well as closed.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * RETURN: void
 *
***********************************************************************/
static void nas_cleanup(bool fTrashOutputFiles)
{
  if (NULL != nas_out_raw)
  {
    if (fclose(nas_out_raw))
      nas_warnP(NULL, "can't close file %s", nas_raw_out_filename);
    nas_out_raw = NULL;

    if (fTrashOutputFiles)
    {
      if ( ! DeleteFile(nas_raw_out_filename))
        nas_warnP(NULL, "can't delete file %s", nas_raw_out_filename);
      nas_raw_out_filename = NULL;
    }
  }

  if (NULL != nas_out_vhdl)
  {
    if ( ! fTrashOutputFiles)
      nas_output_vhdl(NULL, NAKED_INSTR_no_instruction);

    if (fclose(nas_out_vhdl))
      nas_warnP(NULL, "can't close file %s", nas_vhdl_out_filename);
    nas_out_vhdl = NULL;

    if (fTrashOutputFiles)
    {
      if ( ! DeleteFile(nas_vhdl_out_filename))
        nas_warnP(NULL, "can't delete file %s", nas_vhdl_out_filename);
      nas_vhdl_out_filename = NULL;
    }
  }

  if (NULL != nas_out_ucf)
  {
    if ( ! fTrashOutputFiles) nas_output_ucf(NULL,
      NAKED_INSTR_no_instruction, nas_ucf_mem_ref);

    if (fclose(nas_out_ucf))
      nas_warnP(NULL, "can't close file %s", nas_ucf_out_filename);
    nas_out_ucf = NULL;

    if (fTrashOutputFiles)
    {
      if ( ! DeleteFile(nas_ucf_out_filename))
        nas_warnP(NULL, "can't delete file %s", nas_ucf_out_filename);
      nas_ucf_out_filename = NULL;
    }
  }

  if (NULL != nas_out_mem)
  {
    if (fclose(nas_out_mem))
      nas_warnP(NULL, "can't close file %s", nas_mem_out_filename);
    nas_out_mem = NULL;

    if (fTrashOutputFiles)
    {
      if ( ! DeleteFile(nas_mem_out_filename))
        nas_warnP(NULL, "can't delete file %s", nas_mem_out_filename);
      nas_mem_out_filename = NULL;
    }
  }
}

/***********************************************************************
 * FUNCTION: nas_is_accum_instr
 *
 * This function deleted by GNR, 11/27/09.  It is replaced by
 * checking that the ticks_used count is zero.
 *
***********************************************************************/



/***********************************************************************
 * FUNCTION: nas_output
 *
 * DESCRIPTION:
 * Generates output based on semanteme, how nas was called to output,
 * and existance of any previous errors.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * STATE:
 *
 * nas_output_state_t nasOutputState
 * Flag indicating whether or not we should bother with output.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ARGUMENTS:
 *
 * complete_instruction_t val
 * Instruction to output.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * RETURN: void
 *
***********************************************************************/
void nas_output(complete_instruction_t val) {

  // Table of clock ticks used by various instructions.  A Zero
  // signifies a non-instruction or one that must reset neutral_count.
  // For jumps that use variable time, the shortest time is listed.
  static unsigned char ticks_used[64] = {
/* 0  1  2  3  4  5  6  7   0  1  2  3  4  5  6  7  */
   1, 1, 1, 0, 0, 0, 0, 0,  1, 1, 1, 1, 0, 0, 1, 0,
   1, 1, 1, 0, 2, 0, 2, 0,  0, 1, 1, 1, 0, 0, 0, 0,
   2, 2, 2, 2, 2, 0, 2, 2,  2, 2, 2, 2, 0, 0, 2, 2,
   2, 2, 2, 2, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0 };

  // when we output VHDL or UCF, we call a function with a file handle
  // for the first instruction, and a NULL file handle thereafter
  FILE * vhdl_output_file = NULL;
  FILE * ucf_output_file = NULL;
  complete_instruction_t valop = val & NAKED_INSTR_mask;
  static complete_instruction_t last_instr = NAKED_INSTR_nop;
  int op_code_index = OPCODE_INST_BITS(val);
  int ticks = ticks_used[op_code_index];

  //--------------------------------------------------------------------
  // This block checks the inter-relationships between instructions as
  // they are generated in output.  If this gets big, it should be its
  // own function.  This is done here regardless of pass since the
  // first pass needs to see any inserted instructions so that it can
  // accomodate instruction addressing for label functionality.
  //--------------------------------------------------------------------

  // -     -     -     -     -     -     -     -     -     -     -     -
  // If the instruction at hand is a SAVEPxx instruction, pad with NOPs
  // if we've just seen accumulator instructions
  // Rev, 11/04/09, GNR - Only add NOPs if neutral instructions not
  //                      already in place
  // -     -     -     -     -     -     -     -     -     -     -     -
  if (NAKED_INSTR_saveplo == valop ||
      NAKED_INSTR_savepmd == valop ||
      NAKED_INSTR_savephi == valop) {
      if (neutral_count < 2) {
         /* Caution:  Recursive call manipulates neutral_count,
         *  do not use it to control loop here.  */
         int nops2add = 2 - neutral_count;
         if (NAS_OUTPUT_STATE_First_Pass != nasOutputState) {
            total_NOP_locations += 1;
            total_NOPs_inserted += nops2add;
            if (qListNOPs) nas_warnP(NULL,
               "Adding %d NOPs at %#5.3lx before a SAVEPxx\n.",
               nops2add, nas_next_instruction_address);
            }
         while (nops2add--)
            nas_output(NAKED_INSTR_nop);
         }
      }
  if (ticks) neutral_count += ticks;
  else       neutral_count = 0;

  // -     -     -     -     -     -     -     -     -     -     -     -
  // In the unlikely event that a POP after a PUSH, insert a NOP
  // -     -     -     -     -     -     -     -     -     -     -     -
  if ((NAKED_INSTR_popp == (val & NAKED_INSTR_mask)) &&
      (NAKED_INSTR_pushp == (last_instr & NAKED_INSTR_mask)))
  {
      if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
          nas_warnP(NULL, "Adding NOP between PUSHP and POPP "
          "instructions.");
      nas_output(NAKED_INSTR_nop);
  }

  //--------------------------------------------------------------------
  // END OUTPUT CONSISTENCY CHECKS
  //--------------------------------------------------------------------

  ++nas_next_instruction_address;
  switch (nasOutputState)
  {
    case NAS_OUTPUT_STATE_First_Pass:
      break;

    case NAS_OUTPUT_STATE_Second_Pass:
      {
        // -     -     -     -     -     -     -     -     -     -     -
        // If we need to and we haven't, open the raw output file.
        // -     -     -     -     -     -     -     -     -     -     -
        if ((NULL != nas_raw_out_filename) && (NULL == nas_out_raw))
        {
          if (NULL == (nas_out_raw = fopen(nas_raw_out_filename, "w")))
          {
            // this way we only tell the user once
            nas_raw_out_filename = NULL;
            nas_errorP(NULL, "can't open file %s",
               nas_raw_out_filename);
          }
        }

        // -     -     -     -     -     -     -     -     -     -     -
        // If we need to and we haven't, open the VHDL output file.
        // -     -     -     -     -     -     -     -     -     -     -
        if ((NULL != nas_vhdl_out_filename) && (NULL == nas_out_vhdl))
        {
          if (NULL == (nas_out_vhdl =
               fopen(nas_vhdl_out_filename, "w")))
          {
            // this way we only tell the user once
            nas_vhdl_out_filename = NULL;
            nas_errorP(NULL, "can't open file %s",
               nas_vhdl_out_filename);
          }
          else vhdl_output_file = nas_out_vhdl;
        }

        // -     -     -     -     -     -     -     -     -     -     -
        // If we need to and we haven't, open the UCF output file.
        // -     -     -     -     -     -     -     -     -     -     -
        if ((NULL != nas_ucf_out_filename) && (NULL == nas_out_ucf))
        {
          if (NULL == (nas_out_ucf = fopen(nas_ucf_out_filename, "w")))
          {
            // this way we only tell the user once
            nas_ucf_out_filename = NULL;
            nas_errorP(NULL, "can't open file %s",
               nas_ucf_out_filename);
          }
          else ucf_output_file = nas_out_ucf;
        }

        // -     -     -     -     -     -     -     -     -     -     -
        // If we need to and we haven't, open the mem output file and
        // write the standard preamble in it.
        // -     -     -     -     -     -     -     -     -     -     -
        if ((NULL != nas_mem_out_filename) && (NULL == nas_out_mem))
        {
          if (NULL == (nas_out_mem = fopen(nas_mem_out_filename, "w")))
          {
            // this way we only tell the user once
            nas_mem_out_filename = NULL;
            nas_errorP(NULL, "can't open file %s",
               nas_mem_out_filename);
          }
          else fprintf(nas_out_mem, "%s\n", nas_mem_preamble);
        }

        // -     -     -     -     -     -     -     -     -     -     -
        // If we need to, write assembled instruction to raw output.
        // -     -     -     -     -     -     -     -     -     -     -
        if (NULL != nas_out_raw)
        {
#ifdef PCLINUX
          fprintf(nas_out_raw, "%5.5lx\n", val);
#else
          fprintf(nas_out_raw, "%5.5x\n", val);
#endif
        }

        // -     -     -     -     -     -     -     -     -     -     -
        // If we need to, write assembled instruction to VHDL output.
        // -     -     -     -     -     -     -     -     -     -     -
        if (NULL != nas_out_vhdl)
            nas_output_vhdl(vhdl_output_file, val);

        // -     -     -     -     -     -     -     -     -     -     -
        // If we need to, write assembled instruction to UCF output.
        // -     -     -     -     -     -     -     -     -     -     -
        if (NULL != nas_out_ucf) nas_output_ucf(ucf_output_file, val,
            nas_ucf_mem_ref);

        // -     -     -     -     -     -     -     -     -     -     -
        // If we need to, write assembled instruction to mem output.
        // -     -     -     -     -     -     -     -     -     -     -
        if (NULL != nas_out_mem)
        {
#ifdef PCLINUX
          fprintf(nas_out_mem, "%5.5lx\n", val);
#else
          fprintf(nas_out_mem, "%5.5x\n", val);
#endif
        }
      }
      break;

    case NAS_OUTPUT_STATE_Clean_Up:
      {
        nas_cleanup(false);
      }
      break;

    default:
      {
        nas_errorP(NULL, "internal error; unknown output state; "
         "aborting");
        nasOutputState = NAS_OUTPUT_STATE_Error_Found;
      }
      // fall through

    case NAS_OUTPUT_STATE_Error_Found:
      {
        nas_cleanup(true);
        nasOutputState = NAS_OUTPUT_STATE_Ignoring_Output_Requests;
      }
      // fall through

    case NAS_OUTPUT_STATE_Ignoring_Output_Requests:
      {
        // nil
      }
      break;
  }

  // -     -     -     -     -     -     -     -     -     -     -     -
  // Store off last instruction
  // -     -     -     -     -     -     -     -     -     -     -     -
  last_instr = val;
}

/***********************************************************************
 * FUNCTION: print_usage
 *
 * DESCRIPTION:
 * Prints usage of nas to a given output stream.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * STATE: stateless
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ARGUMENTS:
 *
 * ostream stream
 * Stream to which usage text is output.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * RETURN: void
 *
***********************************************************************/
#ifdef PCLINUX
void print_usage(ostream& stream)
#else
void print_usage(ostream stream)
#endif
{
    stream << "nas usage: nas [-r <raw output file>]\n";
    stream << "               [-v <VHDL output file>]\n";
    stream << "               [-u <UCF mem ref> <UCF output file>]\n";
    stream << "               [-m <MEM preamble, e.g., @0000> <MEM output file>]\n";
    stream << "               [-h] [-n] [-s]\n";
    stream << "               <input file>\n";
    stream << "\t- switch arguments can be given in any order\n";
    stream << "\t- if no output files are specified, output is raw to nas_raw.out\n";
    stream << "\t- any assembler warnings or errors are dumped to standard error\n";
    stream << "\t- the -h flag prints this help message\n";
    stream << "\t- the -n flag disables individual warnings for inserted NOPs\n";
    stream << "\t- the -s flag writes a symbol table to <input_file>_nas.sym\n";
    stream << "Neural Processing Unit (NPU) Assembler (NAS) version ";
    stream << _NAS_VERSION;
    stream << "\n";
}

/***********************************************************************
 * FUNCTION: _tmain
 *
 * DESCRIPTION:
 * Entry point and command-line processor for nas.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * STATE: stateless
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * ARGUMENTS:
 *
 * int argc
 * Number of arguments given on the command line
 * (if just the command 1 == argc).
 *
 * TCHAR* argv[]
 * Command-line argument strings (argv[0] will be some direct or
 * indirect reference to nas.exe).
 *
 * TCHAR* envp[]
 * The command shell environment.  Not used.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * RETURN:
 *
 * int representing error code; 0 means all is well.
 *
***********************************************************************/
// FIXME: for some reason, bison doesn't put this declaration in
//   parser.tab.h...
extern "C" int yyparse ();
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
  int nRetCode = 0;
  int i;
#ifndef PCLINUX
  bool fFileFormatFlagsNotSeen = true;
#endif
  bool fDumpSymbolTable = false;
  FILE * symbolDumpFile = NULL;

#ifndef PCLINUX
  // initialize MFC and print and error on failure
  if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
  {
    cerr << _T("Fatal Error: MFC initialization failed") << endl;
    nRetCode = 1;
  }
  else
#endif
  {
    // enough args?
    if (1 >= argc)
    {
      nas_errorP(NULL, "Not enough arguments, see usage below:");
      print_usage(cerr);
      nRetCode = 8;
    }

    // process arguments
    else
    {
      // iterate through all the arguments
      for (i = 1; argc > i; ++i)
      {
        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // help flag
        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        if (0 == strcmp("-h", argv[i]))
        {
          print_usage(cout);
          if ((argc - 1 != i) || (1 != i))
          {
            nas_warnP(NULL, "-h flag is not the only argument; "
               "ignoring the others");
          }
          return nRetCode;
        }

        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // raw output flag
        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        else if (0 == strncmp("-r", argv[i], 2))
        {
          if (i + 1 >= argc) nas_warnP(NULL,
            "-r flag specified with no file name; ignoring");
          else if (NULL != nas_raw_out_filename) nas_warnP(NULL,
            "ignoring duplicate raw output file %s", argv[i]);
          else nas_raw_out_filename = argv[++i];
        }

        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // VHDL output flag
        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        else if (0 == strncmp("-v", argv[i], 2))
        {
          if (i + 1 >= argc) nas_warnP(NULL,
            "-v flag specified with no file name; ignoring");
          else if (NULL != nas_vhdl_out_filename) nas_warnP(NULL,
            "ignoring duplicate VHDL output file %s", argv[i]);
          else nas_vhdl_out_filename = argv[++i];
        }

        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // UCF output flag
        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        else if (0 == strncmp("-u", argv[i], 2))
        {
          if (i + 1 >= argc) nas_warnP(NULL,
            "-u flag specified with no ucf mem reference; ignoring");
          else if (i + 2 >= argc) nas_warnP(NULL,
            "-u flag specified with no file name; ignoring");
          else if (NULL != nas_ucf_out_filename) nas_warnP(NULL,
            "ignoring duplicate UCF output file %s", argv[i]);
          else
          {
            nas_ucf_mem_ref = argv[++i];
            nas_ucf_out_filename = argv[++i];
          }
        }

        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // mem output flag
        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        else if (0 == strncmp("-m", argv[i], 2))
        {
          if (i + 1 >= argc) nas_warnP(NULL,
            "-m flag specified with no mem preamble; ignoring");
          else if (i + 2 >= argc) nas_warnP(NULL,
            "-m flag specified with no file name; ignoring");
          else if (NULL != nas_mem_out_filename) nas_warnP(NULL,
            "ignoring duplicate MEM output file %s", argv[i+1]);
          else
          {
            nas_mem_preamble = argv[++i];
            nas_mem_out_filename = argv[++i];
          }
        }

        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // dump symbal table flag
        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        else if (0 == strncmp("-s", argv[i], 2))
          fDumpSymbolTable = true;

        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // omit individual NOP listings flag
        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        else if (0 == strncmp("-n", argv[i], 2))
          qListNOPs = false;

        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // unknown flag
        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        else if ('-' == argv[i][0]) nas_warnP(NULL,
            "unknown flag %s ignored", argv[i]);

        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // input file name (assumed)
        // -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        else
        {
          if (NULL != nas_in_filename) nas_warnP(NULL,
            "ignoring duplicate input file %s", argv[i]);
          else
          {
            nas_in_filename = argv[i];

            if (fDumpSymbolTable)
            {
              char symbolDumpFileName[257];

              sprintf(symbolDumpFileName, "%s.sym", nas_in_filename);
              if (NULL == (symbolDumpFile =
                  fopen(symbolDumpFileName, "w")))
                nas_warnP(NULL,
                  "can't open file %s for symbol dump",
                  symbolDumpFileName);
            }
          }
        }
      }

      // default output file if none specified
      if ((NULL == nas_raw_out_filename) &&
          (NULL == nas_vhdl_out_filename) &&
          (NULL == nas_ucf_out_filename) &&
          (NULL == nas_mem_out_filename))
        nas_raw_out_filename = "nas_raw.out";

      // make sure we have something to assemble
      if (NULL == nas_in_filename) nas_errorP(NULL,
         "no input file specified");

      // it's all good...until it's not...
      else
      {
        // first pass (looking for label declarations)
        nasOutputState = NAS_OUTPUT_STATE_First_Pass;
        yyparse();

        // second pass
        //  (assembing, but only if first pass didn't blow up)
        if (NAS_OUTPUT_STATE_First_Pass == nasOutputState)
        {
          nasOutputState = NAS_OUTPUT_STATE_Second_Pass;
          nas_next_instruction_address = 0;
          yyparse();
        }

        // dump any partial files
        nasOutputState = NAS_OUTPUT_STATE_Clean_Up;
        nas_output(NAKED_INSTR_no_instruction);
      }
    }
  }

  // free up dynamic memory and dump the symbol table, if requested
  while (NULL != labels)
  {
    label_record_t * l = labels->next;
    if (labels->fUsed == false) nas_warnP(NULL,
      "label '%s' unused.", labels->name);
    if (NULL != symbolDumpFile)
      fprintf(symbolDumpFile, "%s,%u,%c,%c\n", labels->name,
         (unsigned int)labels->address,
         (labels->fUsed) ? 'U' : 'N', (labels->IsLabel) ? 'L' : 'D');
    free((void *)(labels->name));
    free(labels);
    labels = l;
  }

  // close the symbol dump file if open
  if (NULL != symbolDumpFile)
    fclose(symbolDumpFile);

  // advise user of all errors and warnings found
  fprintf(stdout, "\n\nnas: Total program length %u instructions.\n",
    (unsigned int)nas_next_instruction_address);
  fprintf(stdout, "     %s: %u error(s) and %u warning(s) found.\n",
    nas_in_filename, total_errors_found, total_warnings_found);
  fprintf(stdout, "     %u NOPs inserted in %u locations.\n",
   total_NOPs_inserted, total_NOP_locations);

  return nRetCode;
}
