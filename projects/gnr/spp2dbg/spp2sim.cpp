/* (c) Copyright 2009, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                             spp2sim.cpp                              *
*                                                                      *
*  This file contains a simulator (virtual machine) for the SPP2       *
*  language described in the NSI publication "SPP2 Assembly Language   *
*  Guide, Revision 2.1".  It simulates the documented effects of the   *
*  SPP2 machine instructions, but not any aspects of the internal      *
*  state changes or timing of these instructions.                      *
*                                                                      *
*  Multiple vm's may be constructed and manipulated in parallel using  *
*  the C++ 'new' operator and the class methods provided in spp2sim.h. *
*  The various 'set' functions are implemented with value checking     *
*  rather than as in-lines.                                            *
*                                                                      *
*  The calling syntax and use of each routine is described below at    *
*  the start of each routine.  Prototypes, declarations, and defini-   *
*  tions of inline routines are in spp2sim.h                           *
*                                                                      *
*  Abend error codes 520-529 are assigned to this module.  spp2sim     *
*  assumes the application that uses it (e.g. spp2dbg) provides        *
*  versions of the ROCKS library abexit() and abexitm() routines to    *
*  service these errors (throwing an error was tried, but it is not    *
*  straightforward to catch these in a Qt application).                *
*                                                                      *
*  Availability of crk header files and library routines written by    *
*  GNR is assumed.                                                     *
*----------------------------------------------------------------------*
*  V1A, 07/02/09, New program by G.N. Reeke                            *
*  V1B, 07/23/09, Update instructions per V2.3 documentation           *
*  V1C, 09/16/09, Add JUMP, CALL, RET per R.Schermerhorn email 9/11/09 *
*  Rev, 09/18/09, Add lmad mechanism so caller can display mem chngs   *
***********************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <memory.h>
#include "spp2sim.h"
#include "rksubs.h"

/*---------------------------------------------------------------------*
*                         Constructor: spp2vm                          *
*                                                                      *
*  Arguments:                                                          *
*     codesize    Size of space to allocate for program code in words. *
*                 (Machine instruction words are 18 bits wide--        *
*                 a 32-bit integer is allocated for each word.)        *
*     memsize     Size of space to allocate for program data in words. *
*                 (Data words are 16 bits each.)                       *
*---------------------------------------------------------------------*/

spp2vm::spp2vm(size_t codesize, size_t memsize) {
   if (codesize > MAX_IADDR+1) abexit(521);
   if (memsize > MAX_DADDR+1) abexit(524);
   memset((char *)this, 0, sizeof(spp2vm));
   lcode = codesize;
   lmem  = memsize;
   pgm = (int *)callocv(codesize, sizeof(int), "Program space");
   mem = (si16 *)callocv(memsize, sizeof(si16), "Data space");
   } /* End spp2vm constructor */


/*---------------------------------------------------------------------*
*                         Destructor: ~spp2vm                          *
*---------------------------------------------------------------------*/

spp2vm::~spp2vm() {
   if (lcode) freev(pgm, "Program space");
   if (lmem)  freev(mem, "Data space");
   } /* End spp2vm destructor */


/*---------------------------------------------------------------------*
*                  Set Program Memory Word:  set_pgm                   *
*                                                                      *
*  Arguments:                                                          *
*     addr        Address in program memory where instruction is       *
*                 to be stored.                                        *
*     instr       18-bit program instruction stored in low-order       *
*                 bit of a 32-bit integer.                             *
*                                                                      *
*  Note:                                                               *
*     Although only the low-order 18 bits are used for SPP2 instruc-   *
*  tions, the high-order bits may be used for debugger breakpoint      *
*  information, therefore, the contents of these bits are preserved.   *
*---------------------------------------------------------------------*/

void spp2vm::set_pgm(size_t addr, int instr) {
   if (addr > lcode) abexit(521);
   pgm[addr] = instr;
   } /* End set_pgm() */


/*---------------------------------------------------------------------*
*                      Set Memory Word:  set_mem                       *
*                                                                      *
*  Arguments:                                                          *
*     addr        Address of 16-bit memory location to be changed.     *
*     memval      Value to be stored in location addr.                 *
*---------------------------------------------------------------------*/

void spp2vm::set_mem(size_t addr, int memval) {
   if (addr > lmem) abexit(524);
   if (abs(memval) > UI16_MAX) abexit(520);
   mem[addr] = memval;
   } /* End set_mem() */


/*---------------------------------------------------------------------*
*                     Set Program Counter: set_pc                      *
*                                                                      *
*  Argument:                                                           *
*     addr        Address from which next instruction will be taken.   *
*                 This is the address of an 18-bit instruction word,   *
*                 not a byte.  Virtual memory starts at location 0.    *
*---------------------------------------------------------------------*/

void spp2vm::set_pc(size_t addr) {
   if (addr > lcode) abexit(521);
   PC = (int)addr;
   } /* End set_pc() */


/*---------------------------------------------------------------------*
*                      Set Accumulator:  set_ac                        *
*                                                                      *
*  Arguments:                                                          *
*     high        16-bit signed value of high accumulator word.        *
*     mid         16-bit unsigned value of middle accumulator word.    *
*     low         16-bit unsigned value of low accumulator word.       *
*---------------------------------------------------------------------*/

void spp2vm::set_ac(int high, unsigned int mid, unsigned int low) {
   if (abs(high) > SHRT_MAX || mid > UI16_MAX || low > UI16_MAX)
      abexit(520);
   AC.h = (si16)high, AC.m = (ui16)mid, AC.l = (ui16)low;
   } /* End set_ac() */


/*---------------------------------------------------------------------*
*                   Set Accumulator Stack:  set_stk                    *
*                                                                      *
*  Arguments:                                                          *
*     i           Number of the stack accumulator to be set            *
*                    (0 < i <= STACK_DEPTH).                           *
*     high        16-bit signed value of high stack ac word.           *
*     mid         16-bit unsigned value of middle stack ac word.       *
*     low         16-bit unsigned value of low stack ac word.          *
*                                                                      *
*  The user is reminded that the SPP2 assembly langage does not        *
*  provide instructions to access the accumulator stack, only push     *
*  and pop operations from the normal accumulator.  This function      *
*  is provided so a debugger can initialize or modify the stack        *
*  for test purposes.                                                  *
*---------------------------------------------------------------------*/

void spp2vm::set_stk(int i, int high, unsigned int mid,
      unsigned int low) {
   if (i <= 0 || i > STACK_DEPTH) abexit(522);
   if (abs(high) > SHRT_MAX || mid > UI16_MAX || low > UI16_MAX)
      abexit(520);
   stk[i-1].h = (si16)high;
   stk[i-1].m = (ui16)mid;
   stk[i-1].l = (ui16)low;
   } /* End set_stk() */


/*---------------------------------------------------------------------*
*                 Set Return Address Stack:  set_radd                  *
*                                                                      *
*  Arguments:                                                          *
*     i           Number of the return address stack value to be set   *
*                    (0 < i <= RADD_DEPTH).                            *
*     addr        Address to be stored on the stack.                   *
*                                                                      *
*  The user is reminded that the SPP2 assembly langage does not        *
*  provide instructions to access the return address stack, other      *
*  than the subroutine CALL and RET indstructions.  This function      *
*  is provided so a debugger can initialize or modify the stack        *
*  for test purposes.                                                  *
*---------------------------------------------------------------------*/

void spp2vm::set_radd(int i, size_t addr) {
   if (i <= 0 || i > RADD_DEPTH) abexit(522);
   if (addr > lcode) abexit(521);
   radd[i-1] = (int)addr;
   } /* End set_radd() */


/*---------------------------------------------------------------------*
*                     Set THI register:  set_thi                       *
*                                                                      *
*  Argument:                                                           *
*     thival      16-bit signed value to be stored in thi register     *
*---------------------------------------------------------------------*/

void spp2vm::set_thi(int thival) {
   if (abs(thival) > SHRT_MAX) abexit(520);
   thi = thival;
   } /* End set_thi() */


/*---------------------------------------------------------------------*
*                       Set Register:  set_reg                         *
*                                                                      *
*  Arguments:                                                          *
*     r           Number of the register to be set (0 <= r < N_REGS).  *
*     regval      16-bit signed value to be stored in register r.      *
*---------------------------------------------------------------------*/

void spp2vm::set_reg(int r, int regval) {
   if (r < 0 || r >= N_REGS) abexit(523);
   if (abs(regval) > SHRT_MAX) abexit(520);
   reg[r] = (si16)regval;
   } /* End set_reg() */


/*---------------------------------------------------------------------*
*              Query CALL return address stack:  q_radd                *
*                                                                      *
*  Argument:                                                           *
*     i           Number of the return address stack element to be     *
*                    queried (0 < i <= RADD_DEPTH).                    *
*---------------------------------------------------------------------*/

int spp2vm::q_radd(int i) {
   if (i <= 0 || i > RADD_DEPTH) abexit(522);
   return radd[i-1];
   } /* End q_radd() */


/*---------------------------------------------------------------------*
*                   Query Stack High Word:  q_stkhi                    *
*                                                                      *
*  Argument:                                                           *
*     i           Number of the stack accumulator to be queried        *
*                    (0 < i <= STACK_DEPTH).                           *
*---------------------------------------------------------------------*/

int spp2vm::q_stkhi(int i) {
   if (i <= 0 || i > STACK_DEPTH) abexit(522);
   return (int)stk[i-1].h;
   } /* End q_stkhi() */


/*---------------------------------------------------------------------*
*                  Query Stack Middle Word:  q_stkmd                   *
*                                                                      *
*  Argument:                                                           *
*     i           Number of the stack accumulator to be queried        *
*                    (0 < i <= STACK_DEPTH).                           *
*---------------------------------------------------------------------*/

int spp2vm::q_stkmd(int i) {
   if (i <= 0 || i > STACK_DEPTH) abexit(522);
   return (int)stk[i-1].m;
   } /* End q_stkmd() */


/*---------------------------------------------------------------------*
*                   Query Stack Low Word:  q_stklo                     *
*                                                                      *
*  Argument:                                                           *
*     i           Number of the stack accumulator to be queried        *
*                    (0 < i <= STACK_DEPTH).                           *
*---------------------------------------------------------------------*/

int spp2vm::q_stklo(int i) {
   if (i <= 0 || i > STACK_DEPTH) abexit(522);
   return (int)stk[i-1].l;
   } /* End q_stklo() */


/*---------------------------------------------------------------------*
*                       Query Register:  q_reg                         *
*                                                                      *
*  Argument:                                                           *
*     r           Number of the register to be queried                 *
*                    (0 <= r < N_REGS)                                 *
*---------------------------------------------------------------------*/

int spp2vm::q_reg(int r) {
   if (r < 0 || r >= N_REGS) abexit(523);
   return reg[r];
   } /* End q_reg() */


/*---------------------------------------------------------------------*
*                  Query Program Instruction:  q_pgm                   *
*                                                                      *
*  Argument:                                                           *
*     addr        Address in program memory to be queried              *
*---------------------------------------------------------------------*/

int spp2vm::q_pgm(size_t addr) {
   if (addr > lcode) abexit(521);
   return pgm[addr];
   } /* End q_pgm() */


/*---------------------------------------------------------------------*
*                      Query Memory Word:  q_mem                       *
*                                                                      *
*  Argument:                                                           *
*     addr        Address in working variable memory to be queried     *
*---------------------------------------------------------------------*/

int spp2vm::q_mem(size_t addr) {
   if (addr > lmem) abexit(524);
   return mem[addr];
   } /* End q_mem() */


/*---------------------------------------------------------------------*
*             Simulate One Machine Instruction:  sim_next              *
*                                                                      *
*  There are no arguments.  The instruction at the program memory      *
*  location pointed to by the current value of the PC is executed      *
*  and the PC is incremented.  All affected internal vm registers      *
*  and memory locations are updated in accord with the definition      *
*  of the SPP2 language.                                               *
*                                                                      *
*  Note:  The language definition manual does not state that the       *
*  three flags are set after ADDACC, SUBTACC, or MULTACC operations.   *
*  Accordingly, the sim_next() code does not set these flags.  The     *
*  algorithms may need to be revised to preserve carry status in the   *
*  AC.h word for setting the flag if that feature is added later.      *
*---------------------------------------------------------------------*/

// Define masks for checking that required zero fields are indeed zero
#define ZMASK_NOP    0x00000FFF  /* No operands */
#define ZMASK_ADDR   0x00000C00  /* Jump address only */
#define ZMASK_1REG   0x000000FF  /* Regx only */
#define ZMASK_2REG   0x0000000F  /* Regx and Regy present */
#define ZMASK_IMM    0x00000F00  /* Immediate data in opcode */

int spp2vm::sim_next() {
   int rc = 0;                   /* Return code */
   int instr = pgm[PC] & MAX_INSTR;
   enum opcode op = get_op(instr);
   switch (op) {
   case NOP:                     /* No operation */
      if (instr & ZMASK_NOP) abexit(527);
      break;
   case CLRP:                    /* Set AC to 0 */
      if (instr & ZMASK_NOP) abexit(527);
      AC.h = AC.m = AC.l = 0;
      rc = (int)ChAC;
      break;
   case IMM:                     /* Place constant in temp register */
      if (instr & ZMASK_IMM) abexit(527);
      thi = get_im(instr) << SPP2_BPB;
      rc = (int)ChThi;
      break;
   case ADDACC:                  /* Add register to AC */
      if (instr & ZMASK_1REG) abexit(527);
#ifdef HAS_I64
      {  si64 op = reg[get_rx(instr)];
         AC.l = (ui16)(op += AC.l);
         op = SRA(op, SPP2_BPW);
         AC.m = (ui16)(op += AC.m);
         op = SRA(op, SPP2_BPW);
         AC.h = (si16)(op += AC.h);
         }
#else
      {  si32 op = reg[get_rx(instr)]; /* (sign extended) */
         ui32 tac,ophi = (ui32)(SRA(op, SPP2_BPW));
         AC.l = (ui16)(tac = AC.l + ((ui32)op & UI16_MAX));
         AC.m = (ui16)(tac = AC.m + ophi + (tac >> SPP2_BPW));
         AC.h = (si16)(AC.h + (tac >> SPP2_BPW));
         }
#endif
      rc = (int)ChAC;
      break;
   case SUBTACC:                 /* Subtract register from AC */
      if (instr & ZMASK_1REG) abexit(527);
      /* Implemented as change sign and add */
#ifdef HAS_I64
      {  si64 op = -reg[get_rx(instr)];
         AC.l = (ui16)(op += AC.l);
         op = SRA(op, SPP2_BPW);
         AC.m = (ui16)(op += AC.m);
         op = SRA(op, SPP2_BPW);
         AC.h = (si16)(op += AC.h);
         }
#else
      {  si32 op = -reg[get_rx(instr)]; /* (sign extended) */
         ui32 tac,ophi = (ui32)(SRA(op, SPP2_BPW));
         AC.l = (ui16)(tac = AC.l + ((ui32)op & UI16_MAX));
         AC.m = (ui16)(tac = AC.m + ophi + (tac >> SPP2_BPW));
         AC.h = (si16)(AC.h + (tac >> SPP2_BPW));
         }
#endif
      rc = (int)ChAC;
      break;
   case MULT:                    /* AC = Regx * Regy */
      if (instr & ZMASK_2REG) abexit(527);
      {  si32 tac = reg[get_rx(instr)] * reg[get_ry(instr)];
         AC.l = (ui16)tac;
         AC.m = (ui16)(tac = SRA(tac, SPP2_BPW));
         AC.h = (si16)(SRA(tac, SPP2_BPW));
         }
      rc = (int)ChAC;
      break;
   case MULTACC:                 /* AC = AC + Regx * Regy */
      if (instr & ZMASK_2REG) abexit(527);
#ifdef HAS_I64
      {  si64 op = reg[get_rx(instr)] * reg[get_ry(instr)];
         AC.l = (ui16)(op += AC.l);
         op = SRA(op, SPP2_BPW);
         AC.m = (ui16)(op += AC.m);
         op = SRA(op, SPP2_BPW);
         AC.h = (si16)(op += AC.h);
         }
#else
      {  si32 op = reg[get_rx(instr)] * reg[get_ry(instr)];
         ui32 tac,ophi = (ui32)(SRA(op, SPP2_BPW));
         AC.l = (ui16)(tac = AC.l + ((ui32)op & UI16_MAX));
         AC.m = (ui16)(tac = AC.m + ophi + (tac >> SPP2_BPW));
         AC.h = (si16)(AC.h + (tac >> SPP2_BPW));
         }
#endif
      rc = (int)ChAC;
      break;
   case LOAD_REG:                /* Regx = Regy */
      if (instr & ZMASK_2REG) abexit(527);
      reg[get_rx(instr)] = reg[get_ry(instr)];
      rc = (int)ChReg;
      break;
   case LOAD_IMM:                /* Regx = immediate data */
      reg[get_rx(instr)] = thi | get_im(instr);
      rc = (int)ChReg;
      break;
   case COMPARE_REG:             /* Set NF,ZF per Regx-Regy */
      if (instr & ZMASK_2REG) abexit(527);
      {  si32 tac = reg[get_rx(instr)] - reg[get_ry(instr)];
         NF = tac < 0;
         ZF = tac == 0;
         }
      rc = (int)ChFlg;
      break;
   case COMPARE_IMM:             /* Set NF,ZF per Regx - const */
      {  si16 tac = reg[get_rx(instr)] - (thi | get_im(instr));
         NF = tac < 0;
         ZF = tac == 0;
         }
      rc = (int)ChFlg;
      break;
   case STASH:                   /* Mem(Regy) = Regx */
      if (instr & ZMASK_2REG) abexit(527);
      {  int addr = reg[get_ry(instr)];
         if (addr > MAX_DADDR) abexit(524);
         mem[addr] = reg[get_rx(instr)];
         lmad = addr;
         }
      rc = (int)ChMem;
      break;
   case PUSHP:                   /* Push AC onto stack */
      if (instr & ZMASK_NOP) abexit(527);
      for (int i=STACK_DEPTH-1; i>0; --i) stk[i] = stk[i-1];
      stk[0] = AC;
      rc = (int)ChStk;
      break;
   case POPP:                    /* Pop stack onto AC */
      if (instr & ZMASK_NOP) abexit(527);
      AC = stk[0];
      for (int i=1; i<STACK_DEPTH; ++i) stk[i-1] = stk[i];
      rc = (int)ChAC | (int)ChStk;
      break;
   case TESTP:                   /* Set flags per AC contents */
      /* N.B.  Document says flags are set per AC(lo).  However,
      *  what is implemented here is what was stated in emails
      *  from R.Schermerhorn to GNR dated July 20 & 23, 2009. */
      if (instr & ZMASK_NOP) abexit(527);
      NF = AC.h >> SPP2_STU & 1;
      CF = (NF ^ 1) & (AC.l >> SPP2_STU | AC.m | AC.m >> 1);
      ZF = (AC.l | AC.m | AC.h) == 0;
      rc = (int)ChFlg;
      break;
   case SHFTP:                   /* Shift AC right 17 */
      if (instr & ZMASK_NOP) abexit(527);
      AC.l = AC.m >> 1 | AC.h << SPP2_STU;
      AC.m = SRA(AC.h,1);
      AC.h = SRA(AC.h,SPP2_STU);
      rc = (int)ChAC;
      break;
   case INC:                     /* Increment Regx and set flags */
      if (instr & ZMASK_1REG) abexit(527);
      {  int r = get_rx(instr);
         si32 tac = (si32)reg[r] + 1;
         reg[r] = (si16)tac;
         ZF = tac == 0;
         NF = tac < 0;
         tac >>= SPP2_STU;
         CF = (tac ^ (tac >> 1)) & 1;
         }
      rc = (int)ChReg | (int)ChFlg;
      break;
   case DEC:                     /* Decrement Regx and set flags */
      if (instr & ZMASK_1REG) abexit(527);
      {  int r = get_rx(instr);
         si32 tac = (si32)reg[r] - 1;
         reg[r] = (si16)tac;
         ZF = tac == 0;
         NF = tac < 0;
         tac >>= SPP2_STU;
         CF = (tac ^ (tac >> 1)) & 1;
         }
      rc = (int)ChReg | (int)ChFlg;
      break;
   case SAVEPLO:                 /* AC(lo) to Regx */
      if (instr & ZMASK_1REG) abexit(527);
      reg[get_rx(instr)] = AC.l;
      rc = (int)ChReg;
      break;
   case SAVEPMD:                 /* AC(mid) to Regx */
      if (instr & ZMASK_1REG) abexit(527);
      reg[get_rx(instr)] = AC.m;
      rc = (int)ChReg;
      break;
   case SAVEPHI:                 /* AC(hi) to Regx */
      if (instr & ZMASK_1REG) abexit(527);
      reg[get_rx(instr)] = AC.h;
      rc = (int)ChReg;
      break;
   case JUMPCY:                  /* Jump if CF is set */
      if (instr & ZMASK_ADDR) abexit(527);
      if (CF) {
         PC = get_ja(instr);
         rc = (int)ChPC; }
      break;
   case JUMPN:                   /* Jump if NF is set */
      if (instr & ZMASK_ADDR) abexit(527);
      if (NF) {
         PC = get_ja(instr);
         rc = (int)ChPC; }
      break;
   case JUMPNZ:                  /* Jump if ZF is not set */
      if (instr & ZMASK_ADDR) abexit(527);
      if (!ZF) {
         PC = get_ja(instr);
         rc = (int)ChPC; }
      break;
   case JUMPZ:                   /* Jump if ZF is set */
      if (instr & ZMASK_ADDR) abexit(527);
      if (ZF) {
         PC = get_ja(instr);
         rc = (int)ChPC; }
      break;
   case JUMP:                    /* Unconditional Jump */
      if (instr & ZMASK_ADDR) abexit(527);
      PC = get_ja(instr);
      rc = (int)ChPC;
      break;
   case CALL:                    /* Call a subroutine */
      if (instr & ZMASK_ADDR) abexit(527);
      radd[1] = radd[0];
      radd[0] = PC + 1;
      PC = get_ja(instr);
      rc = (int)ChRadd | (int)ChPC;
      break;
   case RET:                     /* Return from a subroutine */
      if (instr & ZMASK_NOP) abexit(627);
      PC = radd[0];
      radd[0] = radd[1];
      rc = (int)ChRadd | (int)ChPC;
      break;
   case OR_REG:                  /* OR Regy into Regx */
      if (instr & ZMASK_2REG) abexit(527);
      reg[get_rx(instr)] |= reg[get_ry(instr)];
      rc = (int)ChReg;
      break;
   case OR_IMM:                  /* Or immediate data into Regx */
      reg[get_rx(instr)] |= thi | get_im(instr);
      rc = (int)ChReg;
      break;
   case AND_REG:                 /* AND Regy into Regx */
      if (instr & ZMASK_2REG) abexit(527);
      reg[get_rx(instr)] &= reg[get_ry(instr)];
      rc = (int)ChReg;
      break;
   case AND_IMM:                 /* AND immediate data into Regx */
      reg[get_rx(instr)] &= thi | get_im(instr);
      rc = (int)ChReg;
      break;
   case FETCH_REG:               /* Regx = Mem(Regy) */
      if (instr & ZMASK_2REG) abexit(527);
      {  int addr = reg[get_ry(instr)];
         if (addr > MAX_DADDR) abexit(524);
         reg[get_rx(instr)] = mem[addr];
         lmad = addr;
         }
      rc = (int)ChReg;
      break;
   case FETCH_ADDR:              /* Regx = Mem(IMM) */
      {  int addr = (int)(thi | get_im(instr)) & MAX_DADDR;
         reg[get_rx(instr)] = mem[addr];
         lmad = addr;
         }
      rc = (int)ChReg;
      break;
   case SHFTBR:                  /* MSB to LSB, no flags */
      if (instr & ZMASK_1REG) abexit(527);
      {  int r = get_rx(instr);
         reg[r] = reg[r] >> 4 & 15;
         }
      rc = (int)ChReg;
      break;
   case SHFTAR:                  /* Arith shift right Regx */
      if (instr & ZMASK_1REG) abexit(527);
      {  int r = get_rx(instr);
         CF = reg[r] & 1;
         reg[r] = SRA(reg[r],1);
         }
      rc = (int)ChReg | (int)ChFlg;
      break;
   case SHFTLL:                  /* Logical left shift Regx */
      if (instr & ZMASK_1REG) abexit(527);
      {  int r = get_rx(instr);
         CF = (reg[r] >> SPP2_STU) & 1;
         reg[r] <<= 1;
         }
      rc = (int)ChReg | (int)ChFlg;
      break;
   case SHFTLR:                  /* Logical right shift Regx */
      if (instr & ZMASK_1REG) abexit(527);
      {  int r = get_rx(instr);
         CF = reg[r] & 1;
         reg[r] = reg[r] >> 1 & SHRT_MAX;
         }
      rc = (int)ChReg | (int)ChFlg;
      break;
   case ENDPGM:                  /* End-of-program indicator */
      rc = ChEnd;
      break;
   default:                      /* Not a known opcode */
      abexit(526);
      } /* End big opcode switch */
   if (!(rc & (int)ChPC)) PC = PC + 1 & MAX_IADDR;
   return rc;
   } /* End sim_next() */
