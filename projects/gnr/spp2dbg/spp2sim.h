/* (c) Copyright 2009, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                              spp2sim.h                               *
*                                                                      *
*  This header file contains definitions needed to access the SPP2     *
*  simulator.  Source code and instructions for use of those routines  *
*  are contained in the file spp2sim.cpp                               *
*                                                                      *
*----------------------------------------------------------------------*
*  V1A, 07/02/09, New program by G.N. Reeke                            *
*  V1B, 07/23/09, Update instructions per V2.3 documentation           *
*  V1C, 09/16/09, Add JUMP, CALL, RET per R.Schermerhorn email 9/11/09 *
***********************************************************************/

#ifndef SPP2SIM_HEADER_DEFINED
#define SPP2SIM_HEADER_DEFINED

#include "sysdef.h"

// Machine configuration definitions
#define MAX_DADDR 0x000007FF     /* Maximum data address */
#define MAX_IADDR 0x000003FF     /* Maximum instruction address */
#define MAX_IMMED 0x000000FF     /* Maximum immediate data */
#define MAX_INSTR 0x0003FFFF     /* Maximum machine instruction */
#define MAX_REGNO 0x0000000F     /* Maximum register number */
#define N_REGS            16     /* Number of general 16-bit regs */
#define SPP2_BPB           8     /* Bits per byte in SPP2 */
#define SPP2_BPW          16     /* Bits per word in SPP2 */
#define SPP2_STU          15     /* Shift sign to units */
#define STACK_DEPTH        5     /* Depth of AC stack */
#define RADD_DEPTH         2     /* Depth of CALL return stack */

// Opcodes in the Assembler Language
// Names defined in the doc are used except where ambiguous
// ENDPGM is a non-instruction used to indicate that execution
// has reached the end of the stored program.
enum opcode { NOP=000, CLRP=001, IMM=002, ADDACC=004, SUBTACC=005,
   MULT=006, MULTACC=007, LOAD_REG=010, LOAD_IMM=011,
   COMPARE_REG=012, COMPARE_IMM=013, STASH=016, PUSHP=020,
   POPP=021, TESTP=022, SHFTP=023, INC=024, DEC=026,
   SAVEPLO=031, SAVEPMD=032, SAVEPHI=033,
   JUMPCY=040, JUMPN=041, JUMPNZ=042, JUMPZ=043,
   JUMP=044, CALL=046, RET=047,
   OR_REG=050, OR_IMM=051, AND_REG=052, AND_IMM=053,
   FETCH_REG=056, FETCH_ADDR=057, SHFTBR=060,
   SHFTAR=061, SHFTLL=062, SHFTLR=063, ENDPGM=077 };

// Codes returned by sim_next() to indicate registers that changed
enum sregch { ChAC=1, ChStk=2, ChReg=4, ChThi=8, ChFlg=16,
   ChMem=32, ChRadd=64, ChPC=128, ChEnd=256 };

typedef struct ac_t {            /* Define an accumulator */
   si16 h;                          /* High 16 bits */
   ui16 m;                          /* Middle 16 bits */
   ui16 l;                          /* Low 16 bits */
   } act;

class spp2vm {
private:
   int      *pgm;                /* Ptr to program storage */
   si16     *mem;                /* Ptr to data memory */
   size_t   lcode, lmem;         /* Size of pgm, mem storage */
   int      lmad;                /* Last memory address */
   int      PC;                  /* Program counter */
   int      radd[RADD_DEPTH];    /* Return address stack */
   act      AC;                  /* Accumulator */
   act      stk[STACK_DEPTH];    /* Accumulator stack */
   si16     reg[N_REGS];         /* General registers */
   si16     thi;                 /* Temp high byte */
   int      CF;                  /* Carry flag */
   int      NF;                  /* Negative flag */
   int      ZF;                  /* Zero flag */

//Private (inline) functions to extract parts of an instruction
   enum opcode get_op(int instr) {  /* Get op code */
      return (enum opcode)(instr >> 12 & 077); }
   int get_ja(int instr) {          /* Get jump address */
      return instr & MAX_IADDR; }
   int get_rx(int instr) {          /* Get register x */
      return instr >> 8 & MAX_REGNO; }
   int get_ry(int instr) {          /* Get register y */
      return instr >> 4 & MAX_REGNO; }
   int get_im(int instr) {          /* Get immediate constant */
      return instr & MAX_IMMED; }

public:
   spp2vm(size_t codesize, size_t memsize);
   ~spp2vm();
   /* Set virtual machine registers and memory */
   void set_pgm(size_t addr, int instr);
   void set_mem(size_t addr, int memval);
   void set_pc(size_t addr);
   void set_ac(int high, unsigned int mid, unsigned int low);
   void set_radd(int i, size_t addr);
   void set_stk(int i, int high, unsigned int mid, unsigned int low);
   void set_thi(int thival);
   void set_reg(int r, int regval);
   void set_CF(int cf) { CF = cf; }    /* Set carry flag */
   void set_NF(int nf) { NF = nf; }    /* Set negative flag */
   void set_ZF(int zf) { ZF = zf; }    /* Set zero flag */
   /* Query virtual machine registers and memory */
   static size_t q_max_iaddr() { return (MAX_IADDR+1); }
   static size_t q_max_daddr() { return (MAX_DADDR+1); }
   int q_pc()   { return PC; }
   int q_lmad() { return lmad; }
   int q_achi() { return (int)AC.h; }
   int q_acmd() { return (int)AC.m; }
   int q_aclo() { return (int)AC.l; }
   int q_radd(int i);
   int q_stkhi(int i);
   int q_stkmd(int i);
   int q_stklo(int i);
   int q_thi() { return thi; }
   int q_CF() { return (CF != 0); }
   int q_NF() { return (NF != 0); }
   int q_ZF() { return (ZF != 0); }
   int q_reg(int r);
   int q_pgm(size_t addr);
   int q_mem(size_t addr);
   int sim_next();                     /* Simulate next instruction */
   };

#endif /* not defined SPP2SIM_HEADER_DEFINED */
