/* (c) Copyright 2009, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                             spdCmd.cpp                               *
*                                                                      *
*  This file implements the command-line interface for the SPP2        *
*  debugger.  Because native Qt does not seem to provide a basic       *
*  terminal widget, I chose to implement the command line with two     *
*  separate widgets, one for input and one for output, in order to     *
*  prevent the user from attempting to edit the output.                *
*                                                                      *
*  The commands implemented by this program are:                       *
*  a paddr     Advance to program address or label.                    *
*  b paddr     Break at program address or label.                      *
*  c           Continue.                                               *
*  cd dir      Change source/data directory.                           *
*  d bkpt      Disable breakpoint number 'bkpt'.                       *
*  e bkpt      Reenable breakpoint number 'bkpt'.                      *
*  h           Help.  Print this help information.                     *
*  i           Info.  Print list of breakpoints.                       *
*  j paddr     Jump (set program counter) to 'paddr'.                  *
*  load file   Load data memory from 'file'.                           *
*  mdr rec     Load data memory from record 'rec' of loaded file.      *
*  n           Execute next instruction.                               *
*  obj file    Set 'file' as object file to be debugged.               *
*  p maddr     Print contents of data memory at 'maddr'.               *
*  prad paddr  Push program address onto call return stack.            *
*  pwd         Print working directory.                                *
*  q           Quit.                                                   *
*  r           Run program.                                            *
*  set regn=value       Set register 'n' to 'value'.                   *
*  set maddr=value      Set data memory at 'maddr' to 'value'.         *
*  spi paddr=instr      Set program instruction at 'addr' to 'instr'.  *
*  sym file    Read symbol table for target program from 'file'.       *
*                                                                      *
*  [Note that all 'paddr', 'maddr', and 'instr' values are assumed to  *
*  be hexadecimal.  Breakpoint numbers are decimal.  Numerical input   *
*  in 'set' is assumed to be decimal on the current binary scale       *
*  unless prefixed by '0x' to indicate hexadecimal.  When the symbol   *
*  table file for the program being debugged has been loaded, then any *
*  name that is defined in the source as a label can be used to enter  *
*  the corresponding 'paddr' value, and any name defined by defNum     *
*  can be used to enter the corresponding 'maddr' value.  'maddr'      *
*  values can be prefixed with '*' to indicate indirect addressing.]   *
*----------------------------------------------------------------------*
*  V1A, 08/26/09, New program by G.N. Reeke                            *
*  V1B, 11/30/09, GNR - Add symbol table and usage in debug commands   *
***********************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <QApplication>
#include <QBrush>
#include <QFileInfo>
#include <QRegExp>
#include <QStringList>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>

#include "spp2dbg.h"
#include "rocks.h"

#define  BKPTENB  0x00800000     /* Breakpoint enabled bit */
#define  BKPTNUM  0xff000000     /* Breakpoint number field */
#define  BKPTSHFT         24     /* Shift of bkpt num to pgm word */
#ifndef  MaxBkpts
#define  MaxBkpts        100     /* Maximum number of breakpoints */
#endif
#if MaxBkpts > 255
#error MaxBkpts > 255 requires new code in spdCmd
#endif

// Macro to display an error message in the CommandWindow
#define CmdError(txt) dbgr->CommandWindow-> \
   appendPlainText(txt);


/*=====================================================================*
*                            chkPgmLoaded                              *
*  Check whether an object file has been loaded before running         *
*  a program or program instruction                                    *
*=====================================================================*/

int DbgMainWindow::chkPgmLoaded(int kck) {

   if (!ObjFile) {
      popupError("Cannot run program--\n"
         "No program has been loaded into the debugger", 530);
      return -1;
      }
   if (kck) {
      if (!MdatFile)
         CmdError("Warning: Memory data have not been loaded for this "
            "program.\n   Execution will proceed with all-zero data.");
      if (!(FilesLoaded & SymbolsLoaded))
         CmdError("Warning: No symbol table has been loaded for this "
            "program.\n  Only numeric addresses can be referenced.");
      }
   return 0;
   } /* End chkPgmLoaded() */


/*=====================================================================*
*                               getAddr                                *
*    Get a program or user memory address from a debugger command      *
*                                                                      *
*  Arguments:                                                          *
*     QSAddr      The string describing the desired address            *
*                 (a number or a symbol).                              *
*     kad         kad_pgm (0) if this is an address in program         *
*                 memory, kad_mem (1) if this is an address in         *
*                 user data memory.                                    *
*=====================================================================*/

int DbgMainWindow::getAddr(const QString& QSAddr, int kad) {
   QString vQSAddr = QSAddr;  /* A nonconstant copy of QSAddr */
   int  iaddr;                /* Return value */
   bool qind;                 /* TRUE for indirect address */
   static const int kadlim[] = { MAX_IADDR, MAX_DADDR };

   /* Check for indirect addressing */
   qind = vQSAddr.startsWith("*");
   if (qind) {
      if (kad == kad_pgm) {
         CmdError("Indirect address is only allowed with data memory, "
            "not program memory");
         return -1;
         }
      vQSAddr.remove(0,1);
      }

   /* Check for numeric address */
   if (KindAddr->exactMatch(vQSAddr)) {
      /* User input is considered to be a hex number whether or
      *  not prefixed with '0x' -- this could easily be changed.  */
      char lea[DFLT_MAXLEN+1];
      int ioff = vQSAddr.startsWith("0x", Qt::CaseInsensitive) ? 2 : 0;
      if (vQSAddr.size() > DFLT_MAXLEN) {
         CmdError("***Address is too long");
         return -1;
         }
      strncpy(lea, qPrintable(vQSAddr), DFLT_MAXLEN);
      wbcdin(lea+ioff, &iaddr,
         RK_HEXF|RK_QPOS|RK_CTST|RK_NINT|DFLT_MAXLEN);
      } /* End numeric address */

   /* Handle symbolic address */
   else switch (kad) {
   case kad_pgm:
      /* User input is a putative program address */
      if (LabelHash->contains(vQSAddr))
         iaddr = LabelHash->value(vQSAddr);
      else {
         CmdError("***Symbol not recognized");
         iaddr = -1; }
      break;
   case kad_mem:
      /* User input is a putative memory address */
      if (defNumHash->contains(vQSAddr))
         iaddr = defNumHash->value(vQSAddr);
      else {
         CmdError("***Symbol not recognized");
         iaddr = -1; }
      break;
      } /* End kad switch */

   /* Handle indirects, check for invalid results.
   *  N.B.  q_mem() checks for bad iaddr, but gives abexit.
   *  Here we just want a CmdError, so check before calling.  */
   if (qind && iaddr <= kadlim[kad]) iaddr = VM->q_mem(iaddr);
   if (iaddr > kadlim[kad]) {
      CmdError("***Address exceeds NPU capacity");
      iaddr = -1;
      }
   return iaddr;
   } /* End getAddr() */


/*=====================================================================*
*                              getPosInt                               *
*           Get a positive integer from a debugger command             *
*=====================================================================*/

int DbgMainWindow::getPosInt(const QString& QSInt) {
   int  ival;
   char lea[DFLT_MAXLEN+1];
   if (QSInt.size() > DFLT_MAXLEN) {
      CmdError("***Command argument is too long");
      return -1; }
   strncpy(lea, qPrintable(QSInt), DFLT_MAXLEN);
   wbcdin(lea, &ival, RK_IORF|RK_QPOS|RK_CTST|RK_NINT|DFLT_MAXLEN);
   return ival;
   } /* End getPosInt() */


/*=====================================================================*
*                               getFile                                *
*        Get a file or directory name from a debugger command          *
*=====================================================================*/

QString DbgMainWindow::getFile(const QString& Cmd) {
   int jfb = Cmd.indexOf(' ');
   int jff = Cmd.indexOf(QRegExp("[^ ]"), jfb);
   return Cmd.mid(jff);
   } /* End getFile() */


/*=====================================================================*
*                           iaddrToTextLine                            *
*  Returns the line in the InstrViewer where a particular program      *
*  instruction can be found.  If no symbol table has been loaded,      *
*  this routine only needs to add 1 to iaddr to skip over the single   *
*  origin definition to get the code line.  If a symbol table has      *
*  been loaded, the InstLine table is used to find the right line.     *
*  This allows for the label lines that will have been inserted        *
*  where they are defined in the source code.                          *
*     (In current NPU assembler output, only one origin, 0, is ever    *
*  defined, so this routine does not worry about multiple origins.     *
*  If later versions of the assembler ever allow multiple origins,     *
*  this can be handled by extending the function of the InstLine       *
*  table in a suitable fashion.)                                       *
*=====================================================================*/

int DbgMainWindow::iaddrToTextLine(int iaddr) {

   if (InstLine)
      return InstLine[iaddr];
   else
      return iaddr + 1;

   } /* End iaddrToTextLine() */


/*=====================================================================*
*                           markInstruction                            *
*        Marks in yellow the instruction at the given address          *
*=====================================================================*/

void DbgMainWindow::markInstruction(int iaddr) {

   if (chkPgmLoaded(kck_pgm) < 0) return;
   if (iaddr == PrevMarked) return;
   if (PrevMarked >= 0) {
      QTextCursor txtcur = QTextCursor(ObjLastMark);
      QTextBlockFormat txtfmt = ObjLastMark.blockFormat();
      txtfmt.setBackground(QBrush("white"));
      txtcur.movePosition(QTextCursor::EndOfBlock,
         QTextCursor::KeepAnchor);
      txtcur.setBlockFormat(txtfmt);
      }
   {  QTextDocument *instrdoc = dbgr->InstrViewer->document();
      ObjLastMark =
         instrdoc->findBlockByLineNumber(iaddrToTextLine(iaddr));
      QTextCursor txtcur = QTextCursor(ObjLastMark);
      QTextBlockFormat txtfmt = ObjLastMark.blockFormat();
      txtfmt.setBackground(QBrush("yellow"));
      txtcur.movePosition(QTextCursor::EndOfBlock,
         QTextCursor::KeepAnchor);
      txtcur.setBlockFormat(txtfmt);
      dbgr->InstrViewer->setTextCursor(txtcur);
      PrevMarked = iaddr;
      } /* End instrdoc local scope */

   } /* End markInstruction */


/*=====================================================================*
*                          updateMemDisplay                            *
*  Update display of data in program data memory space                 *
*=====================================================================*/

void DbgMainWindow::updateMemDisplay(int imad) {

   int mdat;
   char mdln[LMDLine+1];

   if (imad > MAX_DADDR) ::abexit(533);

   QTextDocument *memdoc = dbgr->DataViewer->document();
   QTextBlock memblk = memdoc->findBlockByLineNumber(imad);
   QTextCursor memcur = QTextCursor(memblk);
   memcur.movePosition(QTextCursor::EndOfBlock,
      QTextCursor::KeepAnchor);

   mdat = VM->q_mem(imad);
   wbcdwt(&imad, mdln, RK_HEXF|RK_NZNW|RK_NUNS|RK_NINT|3);
   mdln[4] = mdln[5] = ' ';
   wbcdwt(&mdat, mdln+6, RK_HEXF|RK_NZNW|RK_NUNS|RK_NHALF|4);
   mdln[11] = '\0';
   memcur.insertText(QString(mdln));

   } /* End updateMemDisplay() */


/*=====================================================================*
*                             RunCommand                               *
*                       Run a debugger command                         *
*=====================================================================*/

void DbgMainWindow::RunCommand() {

   QString UserCmd = dbgr->CommandInput->text();
   QStringList cmdwds;           /* List of command args */
   char aCmd[DFLT_MAXLEN+1];     /* ASCII version of command */
   /* The following four lists much match in number & order */
   static char *dbgcmds[] = { "advance", "break", "c",
      "continue", "cd", "disable", "enable", "help", "info",
      "jump", "load", "mdr", "next", "object", "p", "print",
      "prad", "pwd", "quit", "run", "set", "spi", "sym" };
   enum dbgops { dc_bad=0, dc_adv, dc_brk, dc_c1, dc_c2, dc_cd, dc_dis,
      dc_enb, dc_help, dc_info, dc_jump, dc_load, dc_mdr, dc_next,
      dc_obj, dc_prt1, dc_prt2, dc_prad, dc_pwd, dc_quit, dc_run,
      dc_set, dc_spi, dc_sym } dbo;
   /* Omit nargs test (nargs == 0) when arg can be a path */
   const static byte nargs[] = { 0, 2, 2, 1, 1, 0, 2, 2, 1, 1,
      2, 0, 2, 1, 0, 2, 2, 2, 1, 1, 1, 0, 0, 0 };
   const static byte ckpgm[] = { 0, 2, 1, 2, 2, 0, 1, 1, 0, 1,
      1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0 };

   /* Clean up the command and copy to display widget */
   QString Cmd = UserCmd;
   if (!Cmd.startsWith(">")) Cmd.prepend(">");
   dbgr->CommandWindow->appendPlainText(Cmd);

   /* Restore prompt */
   dbgr->CommandInput->setText(">");

   /* Divide command into words.  The second word can be
   *  interpreted as is for those commands that have a single
   *  argument that is a number.  This will not be correct
   *  for directory or file names, or for the 'set' commands,
   *  which may have embedded blanks.  */
   cmdwds = Cmd.split(' ', QString::SkipEmptyParts);
   if (cmdwds[0].size() > DFLT_MAXLEN) {
      CmdError("***Unrecognized command");
      return;
      }
   strncpy(aCmd, qPrintable(cmdwds[0]), DFLT_MAXLEN);

   /* Do a quick check on number of arguments and whether
   *  a program must have been loaded to run this command.
   *  Then switch according to command.  */
   dbo = (enum dbgops)smatch(RK_NMERR, aCmd+1, dbgcmds, dc_sym);
   if (nargs[dbo] && cmdwds.count() != (int)nargs[dbo]) {
      CmdError("***Wrong number of arguments");
      return;
      }
   if (ckpgm[dbo] > 0 && chkPgmLoaded(ckpgm[dbo] - 1) < 0)
      return;

   switch (dbo) {

   case dc_bad:                  /* Bad command */
      CmdError("***Unrecognized command");
      break;

   case dc_adv: {                /* Advance to address */
      int iaddr = getAddr(cmdwds[1], kad_pgm);
      if (iaddr < 0) break;
      QWidget::grabKeyboard();
      RunMode = SimRunning;
      while (!(RunMode & SimStopRqst || VM->q_pc() == iaddr)) {
         int rc;
         markInstruction(VM->q_pc());
         rc = VM->sim_next();
         spdState(rc);
         if (rc & ChMem) updateMemDisplay(VM->q_lmad());
         QApplication::processEvents();
         }
      RunMode = 0;
      QWidget::releaseKeyboard();
      markInstruction(VM->q_pc());
      break;
      } /* End dc_adc local scope */

   case dc_brk: {                /* Set breakpoint */
      int binst, baddr = getAddr(cmdwds[1], kad_pgm);
      if (baddr < 0) break;
      binst = VM->q_pgm(baddr);
      if (LastBkpt >= MaxBkpts) {
         CmdError("***Too many breakpoints\n");
         break;
         }
      if (binst & (BKPTNUM|BKPTENB)) {
         CmdError("***Already is a breakpoint there\n");
         break;
         }
      if (!BkptAddr) {
         /* This is first breakpoint for this object file--
         *  make a new breakpoint table.  */
         BkptAddr = (int *)callocv(MaxBkpts, sizeof(int),
            "Breakpoint table");
         }
      BkptAddr[LastBkpt++] = baddr;
      VM->set_pgm(baddr,
         LastBkpt << BKPTSHFT | BKPTENB | binst & ~BKPTNUM);
      break;
      } /* End dc_brk local scope */

   case dc_run:                  /* Run program */
      VM->set_pc(0);
      /* Drop through to continue cases ... */
   case dc_c1:                   /* Continue */
   case dc_c2:
      QWidget::grabKeyboard();
      RunMode = SimRunning;
      while (!(RunMode & SimStopRqst)) {
         int rc,ipc,iinst;
         ipc = VM->q_pc();
         markInstruction(ipc);
         iinst = VM->q_pgm(ipc);
         if (iinst & BKPTENB) break;
         rc = VM->sim_next();
         spdState(rc);
         if (rc & ChMem) updateMemDisplay(VM->q_lmad());
         QApplication::processEvents();
         }
      RunMode = 0;
      QWidget::releaseKeyboard();
      markInstruction(VM->q_pc());
      break;

   case dc_cd:                   /* Change working directory */
      if (!Cwd->cd(getFile(Cmd)))
         CmdError("***Directory does not exist or is unreadable");
      break;

   case dc_dis: {                /* Disable breakpoint */
      int bpc, binst, bnum = getPosInt(cmdwds[1]);
      if (bnum <= 0 || bnum > LastBkpt || !BkptAddr) {
         CmdError("***That breakpoint does not exist");
         break;
         }
      bpc = BkptAddr[bnum-1];
      binst = VM->q_pgm(bpc);
      if (bnum << BKPTSHFT != binst & BKPTNUM) {
         CmdError("***Internal breakpoint error, contact GNR");
         break;
         }
      VM->set_pgm(bpc, binst & ~BKPTENB);
      break;
      } /* End dc_dis local scope */

   case dc_enb: {                /* Enable breakpoint */
      int bpc, binst, bnum = getPosInt(cmdwds[1]);
      if (bnum <= 0 || bnum > LastBkpt || !BkptAddr) {
         CmdError("***That breakpoint does not exist");
         break;
         }
      bpc = BkptAddr[bnum-1];
      binst = VM->q_pgm(bpc);
      if (bnum << BKPTSHFT != binst & BKPTNUM) {
         CmdError("***Internal breakpoint error, contact GNR");
         break;
         }
      VM->set_pgm(bpc, binst | BKPTENB);
      break;
      } /* End dc_enb local scope */

   case dc_help:                 /* Display help info */
      DisplayCommandSummary();
      break;

   case dc_info:                 /* Display list of breakpoints */
      if (LastBkpt <= 0) {
         CmdError("***There are currently no breakpoints");
         }
      else for (int ibp=1; ibp<=LastBkpt; ++ibp) {
         QString bpitem;
         int baddr = BkptAddr[ibp-1];
         bpitem.sprintf("Bkpt %d is at %#3x", ibp, baddr);
         if (PgmLocHash->contains(baddr))
            bpitem += " (" + PgmLocHash->value(baddr) + ")";
         bpitem += (VM->q_pgm(baddr) & BKPTENB) ?
            " (enabled)" : " (disabled)";
         dbgr->CommandWindow->appendPlainText(bpitem);
         }
      break;

   case dc_jump: {               /* Jump to address or label */
      int iaddr = getAddr(cmdwds[1], kad_pgm);
      if (iaddr < 0) break;
      VM->set_pc(iaddr);
      markInstruction(iaddr);
      break;
      } /* End dc_jump local scope */

   case dc_load:                 /* Load data memory from file */
      cmdwds[1] = getFile(Cmd);
      loadDataFile(cmdwds[1], 1);
      break;

   case dc_mdr: {                /* Load data memory from file */
      int irec = getPosInt(cmdwds[1]);
      if (irec <= 0) {
         CmdError("***A positive record number is required");
         break;
         }
      if (!MdatFile) {
         CmdError("***You must load a memory data file\n"
            "before you can use the mdr command");
         break;
         }
      loadDataFile(*MdatFnm, irec);
      break;
      } /* End dc_mdr local scope */

   case dc_next: {               /* Execute next instruction */
      int rc = VM->sim_next();
      spdState(rc);
      if (rc & ChMem) updateMemDisplay(VM->q_lmad());
      markInstruction(VM->q_pc());
      break;
      } /* End dc_next local scope */

   case dc_obj: {                /* Load object file to be debugged */
      *ObjFnm = getFile(Cmd);
      loadObjectFile(*ObjFnm, krel_new);
      break;
      } /* End dc_obj local scope */

   case dc_prt1:                 /* Print memory data */
   case dc_prt2: {
      int iaddr, ival;
      char rval[LRegDisp+1];
      iaddr = getAddr(cmdwds[1], kad_mem);
      if (iaddr < 0) break;
      ival = VM->q_mem(iaddr);
      if (DispBase == RK_IORF) {
         wbcdwt(&ival, rval, bd16|RK_IORF|RK_NINT|LRegDisp-1);
         rval[LRegDisp] = '\0';
         }
      else {
         wbcdwt(&ival, rval, RK_HEXF|RK_NZNW|RK_NINT|LRegHexd-1);
         rval[LRegHexd] = '\0';
         }
      dbgr->CommandWindow->appendPlainText(QString(rval));
      break;
      } /* End dc_prt local scope */

   case dc_prad: {               /* Push return address */
      int iaddr = getAddr(cmdwds[1], kad_pgm);
      if (iaddr < 0) break;
      VM->set_radd(2, VM->q_radd(1));
      VM->set_radd(1, iaddr);
      spdState(ChRadd);
      break;
      } /* End dc_radd local scope */

   case dc_pwd:                  /* Print working directory */
      dbgr->CommandWindow->appendPlainText(Cwd->absolutePath());
      break;

   case dc_quit:                 /* Terminate debugging */
      exit(0);
      break;

   case dc_set: {                /* Set register or memory value */
      /* Logic here is similar to getAddr() except the numeric value
      *  can be scaled decimal or hex or a defNum symbol */
      QString Qvalue;
      int c2sz, pos = 0, value = 0;
      bool qind;
      char lea[DFLT_MAXLEN+1];
      /* Parse the command line */
      cmdwds = Cmd.split(QRegExp("[ =]+"), QString::SkipEmptyParts);
      if (cmdwds.count() != 3) {
         CmdError("***'set' requires 'name=value'");
         break;
         }
      Qvalue = cmdwds[2];
      /* Check for indirect addressing */
      qind = Qvalue.startsWith("*");
      if (qind) Qvalue.remove(0,1);
      if (KindAddr->exactMatch(Qvalue)) {
         /* Numeric value was input directly */
         if ((c2sz = Qvalue.size()) > DFLT_MAXLEN) {
            CmdError("***Value is too long");
            break;
            }
         strncpy(lea, qPrintable(Qvalue), DFLT_MAXLEN);
         if (Qvalue.startsWith("0x", Qt::CaseInsensitive))
            wbcdin(lea+2, &value, RK_HEXF|RK_CTST|RK_NINT|c2sz-1);
         else
            wbcdin(lea, &value, bd16|RK_IORF|RK_CTST|RK_NINT|c2sz-1);
         }
      else {
         /* Get the numeric value from a symbol */
         if (defNumHash->contains(Qvalue))
            value = defNumHash->value(Qvalue);
         else {
            CmdError("***Symbol not recognized");
            break;
            }
         }
      /* Handle indirects */
      if (qind) {
         if (value > MAX_DADDR) {
            CmdError("***Address exceeds NPU capacity");
            break;
            }
         value = VM->q_mem(value);
         }
      if (Reg_ValName->validate(cmdwds[1], pos) ==
            QValidator::Acceptable) {
         /* Set a register */
         QString regno = cmdwds[1].right(1);
         bool Ok;
         int ireg = regno.toInt(&Ok, 16);
         if (!Ok) {
            CmdError("Invalid register number");
            break;
            }
         VM->set_reg(ireg, value);
         spdState(ChReg);
         }
      else {
         /* Set a memory location */
         int iaddr = getAddr(cmdwds[1], kad_mem);
         if (iaddr < 0) break;
         VM->set_mem(iaddr, value);
         updateMemDisplay(iaddr);
         }
      break;
      } /* End dc_set local scope */

   case dc_spi: {                /* Set program instruction */
      int c2sz, iaddr, instr;
      char cmdln[LCmdLine+1];
      char lea[DFLT_MAXLEN+1];
      cmdwds = Cmd.split(QRegExp("[ =]+"), QString::SkipEmptyParts);
      if (cmdwds.count() != 3) {
         CmdError("***'spi' requires 'addr=instr'");
         break;
         }
      /* Get the address */
      iaddr = getAddr(cmdwds[1], kad_pgm);
      if (iaddr < 0) break;
      /* Get the numeric instruction code */
      if ((c2sz = cmdwds[2].size()) > DFLT_MAXLEN) {
         CmdError("***Command is too long");
         break;
         }
      strncpy(lea, qPrintable(cmdwds[2]), DFLT_MAXLEN);
      wbcdin(lea, &instr, RK_HEXF|RK_CTST|RK_NINT|c2sz-1);
      if (instr > MAX_INSTR) {
         CmdError("***Invalid command");
         break;
         }
      QTextDocument *instrdoc = dbgr->InstrViewer->document();
      QTextBlock chgblk =
         instrdoc->findBlockByLineNumber(iaddrToTextLine(iaddr));
      QTextCursor chgblkcur = QTextCursor(chgblk);
      VM->set_pgm(iaddr, instr);
      wbcdwt(&iaddr, cmdln, RK_HEXF|RK_NZNW|RK_NINT|3);
      cmdln[4] = cmdln[5] = ' ';
      wbcdwt(&instr, cmdln+6, RK_HEXF|RK_NZNW|RK_NINT|5);
      cmdln[12] = '\0';
      chgblkcur.movePosition(QTextCursor::EndOfBlock,
         QTextCursor::KeepAnchor);
      chgblkcur.insertText(QString(cmdln));
      break;
      } /* End dc_spi local scope */

   case dc_sym:                  /* Read symbol table file */
      cmdwds[1] = getFile(Cmd);
      loadSymbolsFile(cmdwds[1]);
      break;

      } /* End debugger command switch */

   } /* End RunCommand() */
