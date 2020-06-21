/* (c) Copyright 2009, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                             spdDass.cpp                              *
*                                                                      *
*  This file implements a simple disassembler for the NRF SPP2         *
*  assembly language.  (The Qt documentation tells us it is cheap      *
*  to return a QString this way.)                                      *
*                                                                      *
*  Argument:                                                           *
*     xcmd           Hexadecimal command                               *
*  Returns:                                                            *
*     qscmd          Human-readable command in QString format          *
*----------------------------------------------------------------------*
*  V1A, 12/05/09, New program by G.N. Reeke                            *
***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "spp2dbg.h"

QString DbgMainWindow::Disassemble(int xcmd) {

   QString qscmd,qsops;
   int opcode, itype, reg1, reg2, idata, iaddr;
   char inst10[11];
   /* List of instruction names.  I'm guessing a good old char
   *  array will have less overhead here  */
   static const char *insts[64] = {
      "nop", "clrp", "imm", 0, "addacc", "subtacc", "mult", "multacc",
      "load", "load", "compare", "compare", 0, 0, "stash", 0,
      "pushp", "popp", "testp", "shftp", "inc", 0, "dec", 0,
      0, "saveplo", "savepmd", "savephi", 0, 0, 0, 0,
      "jumpcy", "jumpn", "jumpnz", "jumpz", "jump", 0, "call", "ret",
      "or", "or", "and", "and", 0, 0, "fetch", "fetch",
      "shftbr", "shftar", "shftll", "shftlr", 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0 };
   /* Instruction type codes */
   enum dassopt { Bad, Nops, IDat, Reg, RR, RDat, Addr };
   static const enum dassopt optypes[64] = {
      Nops, Nops, IDat, Bad,  Reg,  Reg,  RR,   RR,
      RR,   RDat, RR,   RDat, Bad,  Bad,  Reg,  Bad,
      Nops, Nops, Nops, Nops, Reg,  Bad,  Reg,  Bad,
      Bad,  Reg,  Reg,  Reg,  Bad,  Bad,  Bad,  Bad,
      Addr, Addr, Addr, Addr, Addr, Bad,  Addr, Nops,
      RR,   RDat, RR,   RDat, Bad,  Bad,  RR,   RDat,
      Reg,  Reg,  Reg,  Reg,  Bad,  Bad,  Bad,  Bad,
      Bad,  Bad,  Bad,  Bad,  Bad,  Bad,  Bad,  Bad };

   /* Extract the opcode, put name into result */
   opcode = xcmd >> 12 & 077;
   itype = optypes[opcode];
   if (itype == Bad) return QString("BadInstr");
   sprintf(inst10, "  %-8s", insts[opcode]);
   qscmd = QString(inst10);

   /* Extract the operands according to the instruction type */
   switch (itype) {
   case Nops:                 /* Instruction has no operands */
      break;
   case IDat:                 /* Immediate data */
      idata = xcmd & MAX_IMMED;
      qscmd += qsops.sprintf("%#3x", idata);
      break;
   case Reg:                  /* One register */
      reg1 = xcmd >> 8 & MAX_REGNO;
      qscmd += qsops.sprintf("reg%x", reg1);
      break;
   case RR:                   /* Two registers */
      reg1 = xcmd >> 8 & MAX_REGNO;
      reg2 = xcmd >> 4 & MAX_REGNO;
      qscmd += qsops.sprintf("reg%x reg%x", reg1, reg2);
      break;
   case RDat:                 /* Register plus immediate data */
      reg1 = xcmd >> 8 & MAX_REGNO;
      idata = xcmd & MAX_IMMED;
      qscmd += qsops.sprintf("reg%x %#3x", reg1, idata);
      break;
   case Addr:                 /* Jump address */
      iaddr = xcmd & MAX_IADDR;
      qscmd += qsops.sprintf("%#3x", iaddr);
#if 0
      // This code removed because often causes text line to
      // overflow in the code viewer
      if (PgmLocHash->contains(iaddr))
         qscmd += " (" + PgmLocHash->value(iaddr) + ")";
#endif
      break;
      } /* End itype switch */

   return qscmd;
   } /* End Disassemble() */
