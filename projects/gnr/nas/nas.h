/*******************************************************************************
* Copyright 2007 by Jack Lloyd, All Rights Reserved
*                   jack@stormyskies.net
*
* FILE: nas.h
* DESC: Application header file for the NPU Assembler (nas).
*
* REVISION HISTORY (Date, Modifier: Description)
* ----------------------------------------------
*
* 2007-01-28, Jack Lloyd:
* Creation.
*
* 2007-07-09, Jack Lloyd:
* Version 1.2.
*
* 2007-08-14, Jack Lloyd:
* Version 1.3.
*
* 2007-08-16, Jack Lloyd:
* -Corrected opcode for fetch <reg> <reg> instruction.
* -Version 1.4.
*
* 2007-08-19, Jack Lloyd
* Version 1.5.
*
* 2007-08-19, Jack Lloyd
* Version 1.6.
*
* 2007-10-07, Jack Lloyd:
* [2.x] Added new instructions (see "GPU_CODES_2007_10_03.pdf").
*
* 2008-01-27, Jack Lloyd:
* 2.1:  Modifications to output format code:
*    - UCF Format fix
*    - Dual UCF/VHDL bug fix
*    - addition of new MEM output format
*
* 2009-04-04, Jack Lloyd:
* [2.x] Added new instruction, shftbr
*       Added new constraint for SAVEPxx and POPP/PUSHP
*
* 2009-09-14, Jack Lloyd:
* [2.5] Added new instructions JUMP, CALL, RET
*       Added ability to dump "symbol table"
*
* Rev, 10/30/09, GNR - Modify to compile under Linux
* Rev, 11/04/09, GNR - Add 'IsLabel' field to label_record_t
*******************************************************************************/

#ifndef NAS_H_INCLUDED
#define NAS_H_INCLUDED

#ifndef PCLINUX
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "resource.h"
#endif // !defined(PCLINUX)

#include "stdarg.h"

#define _NAS_VERSION "2.6"

// code generation state
typedef enum {
   NAS_OUTPUT_STATE_First_Pass,
   NAS_OUTPUT_STATE_Second_Pass,
   NAS_OUTPUT_STATE_Error_Found,
   NAS_OUTPUT_STATE_Clean_Up,
   NAS_OUTPUT_STATE_Ignoring_Output_Requests
   } nas_output_state_t;

////////////////////////////////////////////////////////////////////////
// goodies for bison ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

// instructions...actually used by more than bison...all one needs to
// do is shift in the arguments and dump the bytes and you'll have an
// NPU machine language command.
typedef enum {
   NAKED_INSTR_nop       = 0x00000,
   NAKED_INSTR_clrp      = 0x01000,
   NAKED_INSTR_imm       = 0x02000,
   NAKED_INSTR_addacc    = 0x04000,
   NAKED_INSTR_subtacc   = 0x05000,
   NAKED_INSTR_mult      = 0x06000,
   NAKED_INSTR_multacc   = 0x07000,
   NAKED_INSTR_loadRC    = 0x09000,
   NAKED_INSTR_loadRR    = 0x08000,
   NAKED_INSTR_compareRR = 0x0a000,
   NAKED_INSTR_compareRC = 0x0b000,
   NAKED_INSTR_stash     = 0x0e000,
   NAKED_INSTR_pushp     = 0x10000,
   NAKED_INSTR_popp      = 0x11000,
   NAKED_INSTR_testp     = 0x12000,
   NAKED_INSTR_shftp     = 0x13000,
   NAKED_INSTR_inc       = 0x14000,
   NAKED_INSTR_dec       = 0x16000,
   NAKED_INSTR_saveplo   = 0x19000,
   NAKED_INSTR_savepmd   = 0x1a000,
   NAKED_INSTR_savephi   = 0x1b000,
   NAKED_INSTR_jump      = 0x24000,
   NAKED_INSTR_jumpcy    = 0x20000,
   NAKED_INSTR_jumpn     = 0x21000,
   NAKED_INSTR_jumpnz    = 0x22000,
   NAKED_INSTR_jumpz     = 0x23000,
   NAKED_INSTR_call      = 0x26000,
   NAKED_INSTR_ret       = 0x27000,
   NAKED_INSTR_orRR      = 0x28000,
   NAKED_INSTR_orRC      = 0x29000,
   NAKED_INSTR_andRR     = 0x2a000,
   NAKED_INSTR_andRC     = 0x2b000,
   NAKED_INSTR_fetchRR   = 0x2e000,
   NAKED_INSTR_fetchRD   = 0x2f000,
   NAKED_INSTR_shftbr    = 0x30000,
   NAKED_INSTR_shftar    = 0x31000,
   NAKED_INSTR_shftll    = 0x32000,
   NAKED_INSTR_shftlr    = 0x33000,

   NAKED_INSTR_mask      = 0x3f000,
   NAKED_INSTR_no_instruction = 0xffffffff//,
   } naked_instruction_t; // an instruction with zeroed out args

// an instruction with args masked in
typedef unsigned long complete_instruction_t;
#define OPCODE_LO_BITS(opcode) ((opcode) & 0xffff)
#define OPCODE_HI_BITS(opcode) (((opcode) & 0x30000) >> 16)
#define OPCODE_INST_BITS(opcode) (((opcode) & 0x3f000) >> 12)

// used to keep track of labels seen (either declared or referenced)
typedef unsigned long int nas_address_t;
typedef struct label_record_node {
   const char * name;
   nas_address_t address;
   bool fUsed;
   bool IsLabel;
   struct label_record_node * next;
   } label_record_t;

// used to indicate location of tokens and groupings
typedef struct YYLTYPE {
   char * fileName;
   unsigned lineNum;
   } YYLTYPE;

// bison wraps its default definition of YYLTYPE in a
// #if !defined YYLTYPE
// ...
// #endif block.
// MSVC++ seems to take the defined preprocessor directive to refer to
// macros, not identifiers, hence we must define a macro to prevent a
// compile error in parser.tab.h. ... and it must be defined to the
// typedef we just used, and not nil, so that declarations work...
#define YYLTYPE YYLTYPE
// malloc and free also defined in this manor to prevent bison from
// declaring them and causing warnings about dllexport
#define malloc malloc  
#define free free

#define YYLLOC_DEFAULT(Current, Rhs, N)             \
do {                                                \
   (Current).fileName = YYRHSLOC(Rhs, 1).fileName; \
   (Current).lineNum = YYRHSLOC(Rhs, 1).lineNum;   \
   } while (0)

// scolding the user
void nas_errorP(YYLTYPE * loc, const char *text, ...);
void nas_errorPv(YYLTYPE * loc, const char *text, va_list args);
void nas_warnP(YYLTYPE * loc, const char *text, ...);
void nas_warnPv(YYLTYPE * loc, const char *text, va_list args);

// parser.tab.c gets compiled and linked by MSVC as a C program,
// which has implications for the symbols it exports and uses,
// therefore any symbols it uses must be declared as C stuff
// for the linker
#ifdef __cplusplus
extern "C" {
#endif

// global flag indicating the current pass
extern nas_output_state_t nasOutputState;

// for some reason, bison likes to generate code that passes by value...
void nas_error(YYLTYPE loc, const char *text, ...);
void nas_warn(YYLTYPE loc, const char *text, ...);

// dumping output
void nas_output(complete_instruction_t val);

// the lexar
int yylex(void);
void yyerror(const char *s);

// tracking labels
label_record_t * get_label_record(const char * name);
label_record_t * put_label_record(label_record_t * new_label_record);
label_record_t * generate_new_label_record(const char * new_label_name);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // !defined(NAS_H_INCLUDED)
