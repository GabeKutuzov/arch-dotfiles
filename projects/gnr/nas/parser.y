%{

/*******************************************************************************
* Copyright 2007 by Jack Lloyd, All Rights Reserved
*                   jack@stormyskies.net
*
* FILE: parser.y
* DESC: GNU Bison grammar file for the NPU Assembler (nas).
*
* REVISION HISTORY (Date, Modifier: Description)
* ----------------------------------------------
*
* 2007-01-28, Jack Lloyd:
* Creation.
*
* 2007-07-09, Jack Lloyd:
* Added support for labels in the and, wand, compare, wcompare, imm, and wload
* instructions; Corrected the warning check on the 3rd terminal in the LOAD REG
* NUM rule (used to be 12-bit, now its 8-bit).
*
* 2007-08-16, Jack Lloyd:
* Fixed output for LOAD REG LABEL rule.
*
* 2007-08-19, Jack Lloyd:
* wload now loads the upper 8-bits into imm, instead of just 4.
*
* 2007-08-24, Jack Lloyd:
* WAND REG LABEL had a bug corrected where more than the low 8 bits of whatever
* number LABEL referred to were put in the and opcode.
*
* 2007-10-07, Jack Lloyd:
* [2.x] Added new instructions (see "GPU_CODES_2007_10_03.pdf").
* 
* 2009-09-14, Jack Lloyd:
* [2.5] Added new instructions JUMP, CALL, RET
*       Added ability to dump "symbol table"
*
*******************************************************************************/


#include <malloc.h>

// nas.h is included by .c and .cpp files, so we can't include stdafx.h, yet we
// need bool and it's values, so here goes...
#if !defined(bool)
typedef unsigned char bool;
#endif
#if !defined(true)
#define true 0xff
#endif
#if !defined(false)
#define false 0
#endif

#include "nas.h"

static label_record_t * label_record_scratch;

%}

%error-verbose

%union symantic_value {
    complete_instruction_t instruction;
    label_record_t         *label_record;
    unsigned long int      operand;
    char                   *string;
}

/* assembler operand tokens */
%token <operand> REG
%token <operand> NUM

/* assembler command tokens */
%token <instruction> NOP
%token <instruction> CLRP
%token <instruction> IMM
%token <instruction> ADDACC
%token <instruction> SUBTACC
%token <instruction> MULT
%token <instruction> MULTACC
%token <instruction> LOAD
%token <instruction> WLOAD
%token <instruction> COMPARE
%token <instruction> WCOMPARE
%token <instruction> STASH
%token <instruction> PUSHP
%token <instruction> POPP
%token <instruction> TESTP
%token <instruction> SHFTP
%token <instruction> INC
%token <instruction> DEC
%token <instruction> SAVEPLO
%token <instruction> SAVEPMD
%token <instruction> SAVEPHI
%token <instruction> JUMP
%token <instruction> JUMPCY
%token <instruction> JUMPN
%token <instruction> JUMPNZ
%token <instruction> JUMPZ
%token <instruction> CALL
%token <instruction> RET
%token <instruction> OR
%token <instruction> WOR
%token <instruction> AND
%token <instruction> WAND
%token <instruction> FETCH
%token <instruction> WFETCH
%token <instruction> SHFTAR
%token <instruction> SHFTBR
%token <instruction> SHFTLL
%token <instruction> SHFTLR

/* various and sundry tokens */
%token <string> LABEL
%token <instruction> DEF_NUM

/* non-terminals */
%type <instruction> instruction
%type <label_record> label_declaration
%type <label_record> address_declaration

%%
input:    /* empty */
        | input line
;

address_declaration:  DEF_NUM LABEL NUM {
                        if (NAS_OUTPUT_STATE_First_Pass == nasOutputState)
                        {
                          // see if the label already exists by trying to get it;
                          // if it already exists, that's an error
                          if (NULL != ($$ = get_label_record($2)))
                          {
                            if (NAS_OUTPUT_STATE_First_Pass == nasOutputState)
                              nas_error(@1, "Duplicate label declaration for '%s'.", $2);
                          }

                          // otherwise, try to add the new label to the label
                          // table; if we can't that's an error, too
                          else if (NULL == ($$ = put_label_record(generate_new_label_record($2))))
                          {
                            nas_error(@1, "internal: unable to add new label '%s' for some strange reason...freakin' weird, man!!", $2);
                          }
                          else 
			  {
			    $$->address = $3;
			    $$->IsLabel = false;
			  }
                        }
                      }
;

label_declaration:   LABEL ':' {
                       if (NAS_OUTPUT_STATE_First_Pass == nasOutputState)
                       {
                         // see if the label already exists by trying to get it;
                         // if it already exists, that's an error
                         if (NULL != ($$ = get_label_record($1)))
                         {
                           if (NAS_OUTPUT_STATE_First_Pass == nasOutputState)
                             nas_error(@1, "Duplicate label declaration for '%s'.", $1);
                         }

                         // otherwise, try to add the new label to the label
                         // table; if we can't that's an error, too
                         else if (NULL == ($$ = put_label_record(generate_new_label_record($1))))
                         {
                           nas_error(@1, "internal: unable to add new label '%s' for some strange reason...freakin' weird, man!!", $1);
                         }
			 $$->IsLabel = true;
                       }
                     }
;

label_declarations:   label_declaration
                    | label_declarations label_declaration
;

instructions:   instruction                   { if (NAKED_INSTR_no_instruction != $1) nas_output($1); }
              | instructions instruction      { if (NAKED_INSTR_no_instruction != $2) nas_output($2); }
;

line:     '\n'
        | label_declarations '\n'
        | instructions '\n'
        | label_declarations instructions '\n'
        | address_declaration '\n'
        | error '\n' { yyerrok; }
;


instruction:      NOP {
                    $$ = NAKED_INSTR_nop;
                  } /* end nop instruction rule-------------------------------------------------------- */

                | CLRP {
                    $$ = NAKED_INSTR_clrp;
                  } /* end clrp instruction rule------------------------------------------------------- */

                | IMM NUM {
                    if (0xff < $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "constant truncation (8-bit)");
                        $2 &= 0x000000ff;
                    }

                    $$ = NAKED_INSTR_imm | $2;
                  }

                | IMM LABEL {
                    if (NULL == (label_record_scratch = get_label_record($2)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@2, "label '%s' does not exist", $2);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn(@2, "constant truncation (8-bit)");
                          num &= 0x000000ff;
                      }

                      label_record_scratch->fUsed = true;
                      $$ = NAKED_INSTR_imm | num;
                    }
                  } /* end imm instruction rule-------------------------------------------------------- */

                | ADDACC REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    $$ = NAKED_INSTR_addacc | ($2 << 8);
                  } /* end addacc instruction rule----------------------------------------------------- */

                | SUBTACC REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    $$ = NAKED_INSTR_subtacc | ($2 << 8);
                  } /* end subtacc instruction rule---------------------------------------------------- */

                | MULT REG REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (16 <= $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "register truncation [0, 16)");
                        $3 &= 0xf;
                    }

                    $$ = NAKED_INSTR_mult | ($2 << 8) | ($3 << 4);
                  } /* end mult instruction rule------------------------------------------------------- */

                | MULTACC REG REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (16 <= $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "register truncation [0, 16)");
                        $3 &= 0xf;
                    }

                    $$ = NAKED_INSTR_multacc | ($2 << 8) | ($3 << 4);
                  } /* end multacc instruction rule---------------------------------------------------- */

                | LOAD REG NUM {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (0xff < $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "constant truncation (8-bit)");
                        $3 &= 0x000000ff;
                    }

                    $$ = NAKED_INSTR_loadRC | ($2 << 8) |  $3;
                  }

                | WLOAD REG NUM {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (0xffff < $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "constant truncation (16-bit)");
                        $3 &= 0x0000ffff;
                    }

                    nas_output(NAKED_INSTR_imm | (($3 & 0xff00) >> 8));
                    $$ = NAKED_INSTR_loadRC | ($2 << 8) | ($3 & 0xff);
                  }

                | LOAD REG LABEL {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record($3)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@3, "label '%s' does not exist", $3);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xff < num)
                      {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "constant truncation (8-bit)");
                        num &= 0x000000ff;
                      }

                      label_record_scratch->fUsed = true;
                      $$ = NAKED_INSTR_loadRC | ($2 << 8) | num;
                    }
                  }

                | WLOAD REG LABEL {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record($3)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@3, "label '%s' does not exist", $3);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xffff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn(@3, "constant truncation (16-bit)");
                          num &= 0x0000ffff;
                      }

                      label_record_scratch->fUsed = true;
                      nas_output(NAKED_INSTR_imm | ((num & 0xff00) >> 8));
                      $$ = NAKED_INSTR_loadRC | ($2 << 8) | (num & 0xff);
                    }
                  }

                | LOAD REG REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (16 <= $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "register truncation [0, 16)");
                        $3 &= 0xf;
                    }

                    $$ = NAKED_INSTR_loadRR | ($2 << 8) | ($3 << 4);
                  } /* end load instruction rule------------------------------------------------------- */

                | COMPARE REG NUM {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (0xff < $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "constant truncation (8-bit)");
                        $3 &= 0x000000ff;
                    }

                    $$ = NAKED_INSTR_compareRC | ($2 << 8) | $3;
                  }

                | WCOMPARE REG NUM {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (0xffff < $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "constant truncation (16-bit)");
                        $3 &= 0x0000ffff;
                    }

                    nas_output(NAKED_INSTR_imm | (($3 & 0xff00) >> 8));
                    $$ = NAKED_INSTR_compareRC | ($2 << 8) | ($3 & 0xff);
                  }

                | COMPARE REG LABEL {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record($3)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@3, "label '%s' does not exist", $3);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xff < num)
                      {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "data address truncation (8-bit)");
                        num &= 0x00000ff;
                      }

                      label_record_scratch->fUsed = true;
                      $$ = NAKED_INSTR_compareRC | ($2 << 8) | num;
                    }
                  }

                | WCOMPARE REG LABEL {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record($3)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@3, "label '%s' does not exist", $3);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xffff < num)
                      {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "data address truncation (16-bit)");
                        num &= 0x000ffff;
                      }

                      label_record_scratch->fUsed = true;
                      nas_output(NAKED_INSTR_imm | ((num & 0xff00) >> 8));
                      $$ = NAKED_INSTR_compareRC | ($2 << 8) | (num & 0xff);
                    }
                  }

                | COMPARE REG REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (16 <= $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "register truncation [0, 16)");
                        $3 &= 0xf;
                    }

                    $$ = NAKED_INSTR_compareRR | ($2 << 8) | ($3 << 4);
                  } /* end compare instruction rule---------------------------------------------------- */

                | STASH REG REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (16 <= $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "register truncation [0, 16)");
                        $3 &= 0xf;
                    }

                    $$ = NAKED_INSTR_stash | ($2 << 8) | ($3 << 4);
                  } /* end stash instruction rule------------------------------------------------------ */

                | PUSHP {
                    $$ = NAKED_INSTR_pushp;
                  } /* end pushp instruction rule------------------------------------------------------ */

                | POPP {
                    $$ = NAKED_INSTR_popp;
                  } /* end popp instruction rule------------------------------------------------------- */

                | TESTP {
                    $$ = NAKED_INSTR_testp;
                  } /* end testp instruction rule------------------------------------------------------ */

                | SHFTP {
                    $$ = NAKED_INSTR_shftp;
                  } /* end shftp instruction rule------------------------------------------------------ */

                | INC REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    $$ = NAKED_INSTR_inc | ($2 << 8)
                  } /* end inc instruction rule-------------------------------------------------------- */

                | DEC REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    $$ = NAKED_INSTR_dec | ($2 << 8);
                  } /* end dec instruction rule-------------------------------------------------------- */

                | SAVEPLO REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    $$ = NAKED_INSTR_saveplo | ($2 << 8);
                  } /* end saveplo instruction rule---------------------------------------------------- */

                | SAVEPMD REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    $$ = NAKED_INSTR_savepmd | ($2 << 8);
                  } /* end savepmd instruction rule---------------------------------------------------- */

                | SAVEPHI REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    $$ = NAKED_INSTR_savephi | ($2 << 8);
                  } /* end savephi instruction rule---------------------------------------------------- */

                | JUMP NUM {
                    if (0xfff < $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "instruction address truncation (12-bit)");
                        $2 &= 0x000000fff;
                    }

                    $$ = NAKED_INSTR_jump | $2;
                  }

                | JUMP LABEL {
                    if (NULL == (label_record_scratch = get_label_record($2)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@2, "label '%s' does not exist", $2);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      unsigned int num = label_record_scratch->address;

                      if (0xfff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn(@2, "instruction address truncation (12-bit)");
                          num &= 0x000000fff;
                      }

                      label_record_scratch->fUsed = true;
                      $$ = NAKED_INSTR_jump | num;
                    }
                  } /* end jump instruction rule------------------------------------------------------- */

                | JUMPCY NUM {
                    if (0xfff < $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "instruction address truncation (12-bit)");
                        $2 &= 0x000000fff;
                    }

                    $$ = NAKED_INSTR_jumpcy | $2;
                  }

                | JUMPCY LABEL {
                    if (NULL == (label_record_scratch = get_label_record($2)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@2, "label '%s' does not exist", $2);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      unsigned int num = label_record_scratch->address;

                      if (0xfff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn(@2, "instruction address truncation (12-bit)");
                          num &= 0x000000fff;
                      }

                      label_record_scratch->fUsed = true;
                      $$ = NAKED_INSTR_jumpcy | num;
                    }
                  } /* end jumpcy instruction rule----------------------------------------------------- */

                | JUMPN NUM {
                    if (0xfff < $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "instruction address truncation (12-bit)");
                        $2 &= 0x000000fff;
                    }

                    $$ = NAKED_INSTR_jumpn | $2;
                  }

                | JUMPN LABEL {
                    if (NULL == (label_record_scratch = get_label_record($2)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@2, "label '%s' does not exist", $2);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      unsigned int num = label_record_scratch->address;

                      if (0xfff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn(@2, "instruction address truncation (12-bit)");
                          num &= 0x000000fff;
                      }

                      label_record_scratch->fUsed = true;
                      $$ = NAKED_INSTR_jumpn | num;
                    }
                  } /* end jumpn instruction rule------------------------------------------------------ */

                | JUMPNZ NUM {
                    if (0xfff < $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "instruction address truncation (12-bit)");
                        $2 &= 0x000000fff;
                    }

                    $$ = NAKED_INSTR_jumpnz |  $2;
                  }

                | JUMPNZ LABEL {
                    if (NULL == (label_record_scratch = get_label_record($2)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@2, "label '%s' does not exist", $2);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      unsigned int num = label_record_scratch->address;

                      if (0xfff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn(@2, "instruction address truncation (12-bit)");
                          num &= 0x000000fff;
                      }

                      label_record_scratch->fUsed = true;
                      $$ = NAKED_INSTR_jumpnz | num;
                    }
                  } /* end jumpnz instruction rule----------------------------------------------------- */

                | JUMPZ NUM {
                    if (0xfff < $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "instruction address truncation (12-bit)");
                        $2 &= 0x000000fff;
                    }

                    $$ = NAKED_INSTR_jumpz |  $2;
                  }

                | JUMPZ LABEL {
                    if (NULL == (label_record_scratch = get_label_record($2)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@2, "label '%s' does not exist", $2);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      unsigned int num = label_record_scratch->address;

                      if (0xfff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn(@2, "instruction address truncation (12-bit)");
                          num &= 0x000000fff;
                      }

                      label_record_scratch->fUsed = true;
                      $$ = NAKED_INSTR_jumpz | num;
                    }
                  } /* end jumpz instruction rule------------------------------------------------------ */

                | CALL NUM {
                    if (0xfff < $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "instruction address truncation (12-bit)");
                        $2 &= 0x000000fff;
                    }

                    $$ = NAKED_INSTR_call |  $2;
                  }

                | CALL LABEL {
                    if (NULL == (label_record_scratch = get_label_record($2)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@2, "label '%s' does not exist", $2);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      unsigned int num = label_record_scratch->address;

                      if (0xfff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn(@2, "instruction address truncation (12-bit)");
                          num &= 0x000000fff;
                      }

                      label_record_scratch->fUsed = true;
                      $$ = NAKED_INSTR_call | num;
                    }
                  } /* end call instruction rule------------------------------------------------------- */

                | RET {
                    $$ = NAKED_INSTR_ret;
                  } /* end ret instruction rule-------------------------------------------------------- */

                | OR REG NUM {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (0xff < $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "constant truncation (8-bit)");
                        $3 &= 0x000000ff;
                    }

                    $$ = NAKED_INSTR_orRC | ($2 << 8) | $3;
                  }

                | OR REG LABEL {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record($3)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@3, "label '%s' does not exist", $3);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn(@3, "constant truncation (8-bit)");
                          num &= 0x000000ff;
                      }

                      label_record_scratch->fUsed = true;
                      $$ = NAKED_INSTR_orRC | ($2 << 8) | num;
                    }
                  }

                | WOR REG NUM {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (0xffff < $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "constant truncation (16-bit)");
                        $3 &= 0x0000ffff;
                    }

                    nas_output(NAKED_INSTR_imm | (($3 & 0xff00) >> 8));
                    $$ = NAKED_INSTR_orRC | ($2 << 8) | ($3 & 0xff);
                  }

                | WOR REG LABEL {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record($3)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@3, "label '%s' does not exist", $3);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xffff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn(@3, "constant truncation (16-bit)");
                          num &= 0x0000ffff;
                      }

                      label_record_scratch->fUsed = true;
                      nas_output(NAKED_INSTR_imm | ((num & 0xff00) >> 8));
                      $$ = NAKED_INSTR_orRC | ($2 << 8) | (num & 0xff);
                    }
                  }

                | OR REG REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (16 <= $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "register truncation [0, 16)");
                        $3 &= 0xf;
                    }

                    $$ = NAKED_INSTR_orRR | ($2 << 8) | ($3 << 4);
                  } /* end or instruction rule--------------------------------------------------------- */

                | AND REG NUM {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (0xff < $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "constant truncation (8-bit)");
                        $3 &= 0x000000ff;
                    }

                    $$ = NAKED_INSTR_andRC | ($2 << 8) | $3;
                  }

                | AND REG LABEL {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record($3)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@3, "label '%s' does not exist", $3);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn(@3, "constant truncation (8-bit)");
                          num &= 0x000000ff;
                      }

                      label_record_scratch->fUsed = true;
                      $$ = NAKED_INSTR_andRC | ($2 << 8) | num;
                    }
                  }

                | WAND REG NUM {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (0xffff < $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "constant truncation (16-bit)");
                        $3 &= 0x0000ffff;
                    }

                    nas_output(NAKED_INSTR_imm | (($3 & 0xff00) >> 8));
                    $$ = NAKED_INSTR_andRC | ($2 << 8) | ($3 & 0xff);
                  }

                | WAND REG LABEL {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record($3)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@3, "label '%s' does not exist", $3);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xffff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn(@3, "constant truncation (16-bit)");
                          num &= 0x0000ffff;
                      }

                      label_record_scratch->fUsed = true;
                      nas_output(NAKED_INSTR_imm | ((num & 0xff00) >> 8));
                      $$ = NAKED_INSTR_andRC | ($2 << 8) | (num & 0xff);
                    }
                  }

                | AND REG REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (16 <= $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "register truncation [0, 16)");
                        $3 &= 0xf;
                    }

                    $$ = NAKED_INSTR_andRR | ($2 << 8) | ($3 << 4);
                  } /* end and instruction rule-------------------------------------------------------- */

                | FETCH REG REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (16 <= $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "register truncation [0, 16)");
                        $3 &= 0xf;
                    }

                    $$ = NAKED_INSTR_fetchRR   | ($2 << 8) | ($3 << 4);
                  }

                | FETCH REG NUM {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (0xff < $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "data address truncation (12-bit)");
                        $3 &= 0x00000ff;
                    }

                    $$ = NAKED_INSTR_fetchRD | ($2 << 8) |  $3;
                  }

                | WFETCH REG NUM {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (0xfff < $3)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "data address truncation (12-bit)");
                        $3 &= 0x00000fff;
                    }

                    nas_output(NAKED_INSTR_imm | (($3 & 0x0f00) >> 8));
                    $$ = NAKED_INSTR_fetchRD | ($2 << 8) | ($3 & 0xff);
                  }

                | FETCH REG LABEL {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record($3)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@3, "label '%s' does not exist", $3);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xff < num)
                      {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "data address truncation (8-bit)");
                        num &= 0x00000ff;
                      }

                      label_record_scratch->fUsed = true;
                      $$ = NAKED_INSTR_fetchRD | ($2 << 8) | num;
                    }
                  }

                | WFETCH REG LABEL {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record($3)))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error(@3, "label '%s' does not exist", $3);
                        $$ = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xfff < num)
                      {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@3, "data address truncation (12-bit)");
                        num &= 0x00000fff;
                      }

                      label_record_scratch->fUsed = true;
                      nas_output(NAKED_INSTR_imm | ((num & 0x0f00) >> 8));
                      $$ = NAKED_INSTR_fetchRD | ($2 << 8) | (num & 0xff);
                    }
                  } /* end fetch instruction rule------------------------------------------------------ */

                | SHFTAR REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    $$ = NAKED_INSTR_shftar | ($2 << 8);
                  } /* end shftar instruction rule----------------------------------------------------- */

                | SHFTBR REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    $$ = NAKED_INSTR_shftbr | ($2 << 8);
                  } /* end shftbr instruction rule----------------------------------------------------- */

                | SHFTLL REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    $$ = NAKED_INSTR_shftll | ($2 << 8);
                  } /* end shftll instruction rule----------------------------------------------------- */

                | SHFTLR REG {
                    if (16 <= $2)
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn(@2, "register truncation [0, 16)");
                        $2 &= 0xf;
                    }

                    $$ = NAKED_INSTR_shftlr | ($2 << 8);
                  } /* end shftlr instruction rule----------------------------------------------------- */
; /* end of instruction rule */
%%
