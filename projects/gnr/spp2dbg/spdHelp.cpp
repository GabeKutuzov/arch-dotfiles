/* (c) Copyright 2009, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                             spdHelp.cpp                              *
*                                                                      *
*  This file implements the actions triggered by the Help menu.        *
*----------------------------------------------------------------------*
*  V1A, 08/13/09, New program by G.N. Reeke                            *
***********************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <QString>

#include "spp2dbg.h"


/*=====================================================================*
*                          DisplayAboutInfo                            *
*                                                                      *
*  This routine brings up a message box that displays the name,        *
*  author, and version of this program.                                *
*=====================================================================*/

void DbgMainWindow::DisplayAboutInfo() {

   QString AbtTitle = "About spp2dbg";
   QString AbtText  = "This is spp2dbg version 1.0\n"
      "Written by George Reeke\n";

   QMessageBox::information(this, AbtTitle, AbtText,
      QMessageBox::Ok, QMessageBox::Ok);
   }


/*=====================================================================*
*                        DisplayCommandSummary                         *
*=====================================================================*/

void DbgMainWindow::DisplayCommandSummary() {

   QString HowTitle = "Debugger Command Summary";
   QString HowText =
      "a addr\tAdvance to address or label.\n"
      "b addr\tBreak at address or label.\n"
      "c\tContinue.\n"
      "cd dir\tChange source/data directory.\n"
      "d bkpt\tDisable breakpoint number 'bkpt'.\n"
      "e bkpt\tEnable breakpoint number 'bkpt'.\n"
      "h\tHelp.  Print this help information.\n"
      "i\tInfo.  Print list of breakpoints.\n"
      "j addr\tJump (set program counter) to 'addr'.\n"
      "load file\tLoad data memory from 'file'.\n"
      "mdr rec\tLoad data memory from record 'rec' of leaded file.\n"
      "n\tExecute next instruction.\n"
      "obj file\tSet 'file' as object file to be debugged.\n"
      "p addr\tPrint contents of memory at 'addr'.\n"
      "prad addr\tPush return address on call stack.\n"
      "pwd\tPrint name of working directory.\n"
      "q\tQuit.\n"
      "r\tRun program.\n"
      "set regn=value\tSet register 'n' to 'value'.\n"
      "set addr=value\tSet data memory at 'addr' to 'value'.\n"
      "spi addr=value\tSet program instruction at 'addr' to'value'.\n"
      "src file\tSet 'file' as source file to be debugged.\n"
      "\n"
      "The PC, AC, stack, registers, and flags may be changed\n"
      "at any time by entering new values into the display.\n";

   QMessageBox::information(this, HowTitle, HowText,
      QMessageBox::Ok, QMessageBox::Ok);
   }

