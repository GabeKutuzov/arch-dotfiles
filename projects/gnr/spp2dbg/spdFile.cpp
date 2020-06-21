/* (c) Copyright 2009, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                             spdFile.cpp                              *
*                                                                      *
*  This file implements the actions triggered by the File menu.        *
*----------------------------------------------------------------------*
*  V1A, 08/07/09, New program by G.N. Reeke                            *
*  V1B, 11/29/09, GNR - Add symbol table and usage in debug commands   *
***********************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <QFileDialog>
#include <QIODevice>

#include "spp2dbg.h"

/*=====================================================================*
*                            okToContinue                              *
*                                                                      *
*  This routine asks user's permission to continue an operation.       *
*=====================================================================*/

int DbgMainWindow::okToContinue(const QString& OKtxt) {
   int rc = QMessageBox::warning(this, "SPP2 Debugger",
      OKtxt + "Do you want to continue?",
      QMessageBox::Yes | QMessageBox::Cancel);
   if (rc == QMessageBox::Yes) return YES;
   else return NO;
   } /* End okToContinue() */


/*=====================================================================*
*                             popupError                               *
*                                                                      *
*  This routine pops up a fatal error box and asks the user whether    *
*  to abort or cancel whatever caused the error.                       *
*=====================================================================*/

void DbgMainWindow::popupError(const QString& Errtxt, int abex) {
   int rc = QMessageBox::critical(this, "SPP2 Debugger",
      Errtxt, QMessageBox::Cancel | QMessageBox::Abort);
   if (rc == QMessageBox::Abort) exit(abex);
   } /* End popupError() */


/*=====================================================================*
*                            killMdatFile                              *
*       Switching to a new memory data file, close the old one         *
*=====================================================================*/

void DbgMainWindow::killMdatFile() {
   MdatQBin->setDevice(0);    /* Disconnect stream from file */
   if (MdatFile) {
      MdatFile->~QFile();
      MdatFile = NULL;
      }
   MdatFnm->clear();
   MdatRec = 0;
   FilesLoaded &= ~DataLoaded;
   } /* End killMdatFile() */


/*=====================================================================*
*                           killSymbolTable                            *
*         Switching to a new symbol table, close the old one           *
*=====================================================================*/

void DbgMainWindow::killSymbolTable() {
   LabelHash->clear();
   defNumHash->clear();
   PgmLocHash->clear();
   FilesLoaded &= ~SymbolsLoaded;
   } /* End killSymbolTable() */


/*=====================================================================*
*                           loadObjectFile                             *
*                                                                      *
*  Given a file name, this routine opens the file, assumed to be an    *
*  SPP2 object code file, cleans out any existing object code, symbol  *
*  table, and instruction line table, then reads the machine-language  *
*  commands into a binary buffer for the simulator and a text buffer   *
*  for the GUI.                                                        *
*                                                                      *
*  Arguments:                                                          *
*     objfnm         Name of the object code file to be read.          *
*     krel           If krel == 'krel_old', and a file has already     *
*  been read, it is just read again without asking the user--this is   *
*  for the case that the symbol table is read after the object file    *
*  and the code must be rewritten with symbols in place.               *
*                    If krel == 'krel_new', this is a request from     *
*  the GUI and the user must be asked before throwing out the          *
*  existing code list.                                                 *
*=====================================================================*/

void DbgMainWindow::loadObjectFile(const QString& objfnm, int krel) {

   QString canfnm = Cwd->absoluteFilePath(objfnm);
   int  ipc = 0;                 /* Current program counter */
   int  inst = 0;                /* Current program instruction */

// If this is a request for a new object file, and an object file was
// already loaded, ask permission to delete it and then also delete
// the symbol table and breakpoint table if these were constructed.

   if (krel == krel_new) {
      if (FilesLoaded & CodeLoaded) {
         if (!okToContinue("An object file has already been loaded:\n"
            "If you continue, this file and its breakpoints\n"
            "and symbols will be deleted.")) return;
         dbgr->InstrViewer->clear();
         for (int iaddr=0; iaddr<=MAX_IADDR; ++iaddr)
            VM->set_pgm(iaddr, 0);
         if (BkptAddr) {
            freev(BkptAddr, "Breakpoint table");
            BkptAddr = NULL; }
         if (FilesLoaded & SymbolsLoaded) killSymbolTable();
         ObjQTxt->setDevice(0);
         ObjFile->close();
         FilesLoaded &= ~CodeLoaded;
         /* Doc says don't call setFileName if file opened--not
         *  clear closing it is good enough--if this code works,
         *  do same w/data file, if fails, delete and reconstruct. */
         ObjFile->setFileName(canfnm);
         }
      else {
         /* First time through, create ObjFile */
         ObjFile = new QFile(canfnm);
         }
      }

// No, we are just reloading the same code file

   else {
      dbgr->InstrViewer->clear();
      for (int iaddr=0; iaddr<=MAX_IADDR; ++iaddr)
         VM->set_pgm(iaddr, 0);
      }

   if (!ObjFile->open(QIODevice::ReadOnly|QIODevice::Text)) {
      popupError("Unable to open specified SPP2 object code file.",
         531);
      ObjFile->~QFile();
      ObjFile = NULL;
      return;
      }
   ObjQTxt->setDevice(ObjFile);
   PrevMarked = -1;

// Read object code file one line at a time, store raw hex
// instructions in simulator's code space, and store formatted
// instructions in QTextEdit widget.

   while (!ObjQTxt->atEnd()) {
      char *focln;               /* First char in ocln to translate */
      QString Qistr;             /* Text to be passed to viewer */
      int  qnewaddr;             /* TRUE if this is a new address */
      char ocln[LOCLN+1];        /* One hex object code line */
      char teln[LCmdLine+1];     /* One formatted object code line */

      QString Qinstr = ObjQTxt->readLine(LOCLN);
      strncpy(ocln, qPrintable(Qinstr), LOCLN);
      qnewaddr = ocln[0] == '@';
      focln = ocln + qnewaddr;
      wbcdin(focln, &inst, RK_HEXF|RK_NINT|4);
      if (qnewaddr) {
         ipc = inst;
         Qistr = Qinstr;
         }
      else {
         VM->set_pgm(ipc, inst);
         if (PgmLocHash->contains(ipc)) {
            Qistr = PgmLocHash->value(ipc) + ":";
            dbgr->InstrViewer->appendPlainText(Qistr);
            }
         wbcdwt(&ipc, teln, RK_HEXF|RK_NZNW|RK_NINT|3);
         teln[4] = teln[5] = ' ';
         /* Breakpoints are not yet installed, so no need to
         *  remove breakpoint codes from inst words here */
         wbcdwt(&inst, teln+6, RK_HEXF|RK_NZNW|RK_NINT|5);
         teln[12] = '\0';
         Qistr = (QString)teln + Disassemble(inst);
         ipc += 1;
         }
      dbgr->InstrViewer->appendPlainText(Qistr);
      } /* End main read loop */

// Reached end of object code file.  Mark end-of-program.

   ObjFile->close();
   inst = ENDPGM << 12;
   VM->set_pgm(ipc, inst);
   markInstruction(0);
   FilesLoaded |= CodeLoaded;
   return;
   } /* End loadObjectFile() */


/*=====================================================================*
*                         menuLoadObjectFile                           *
*                                                                      *
*  This routine brings up a standard file selection menu, then calls   *
*  loadObjectFile to open and load the file selected by the user.      *
*=====================================================================*/

void DbgMainWindow::menuLoadObjectFile() {

   *ObjFnm = QFileDialog::getOpenFileName(this,
      "Open Object Code File", Cwd->canonicalPath(),
      "SPP2 Object Files (*.mem)");
   if (!ObjFnm->isEmpty()) loadObjectFile(*ObjFnm, krel_new);

   } /* End menuLoadObjectFile() */


/*=====================================================================*
*                           loadSymbolsFile                            *
*                                                                      *
*  Given a file name, this routine first checks whether a symbol table *
*  file has already been loaded, and if so, after getting permission   *
*  from user, deletes the old symbol table.  It then reads the speci-  *
*  fied file, does a minimal check to see if it is in the expected     *
*  format for an nas symbol table, and builds three hashes from the    *
*  data in the file:  one to look up the program addresses corres-     *
*  ponding to labels, one to look up the values of defNum symbols,     *
*  and one to convert program addresses back to labels for use by      *
*  the disassembler.                                                   *
*=====================================================================*/

void DbgMainWindow::loadSymbolsFile(const QString& stfnm) {

   QString canfnm = Cwd->absoluteFilePath(stfnm);
   QString Qsymline;             /* One .sym file line */

// Remove any symbol table that might have previously been loaded.
// (We don't bother checking whether the file name is the same as
//  before--this is very unlikely ever to happen, and if it does,
//  the only harm is the time wasted reading it in a second time.)

   if (FilesLoaded & SymbolsLoaded) {
      if (!okToContinue("A symbol table file has already been loaded:\n"
         "If you continue, the existing symbol table will be deleted."))
         return;
      killSymbolTable();
      }

// If an InstLine table exists, clear it.  If not, make one.

   if (InstLine)
      memset((char *)InstLine, 0, (MAX_IADDR+1)*sizeof(int));
   else
      InstLine = (int *)callocv(MAX_IADDR+1, sizeof(int),
         "Instruction line table");

// Open the symbol table file, read one line, and check format.

   SymbFile = new QFile(canfnm);
   if (!SymbFile->open(QIODevice::ReadOnly)) {
      popupError("Unable to open specified nas symbol table "
         "(.sym file).", 534);
      return;
      }
   SymbQTxt->setDevice(SymbFile);
   *SymbFnm = canfnm;
   Qsymline = SymbQTxt->readLine(MaxSymTabLine);
   if (Qsymline.isNull()) {
      popupError("Specified nas symbol table (.sym file) "
         "appears to be empty.", 534);
      return;
      }
   if (!(SymTabChk->exactMatch(Qsymline))) {
      popupError("Specified .sym file does not appear to "
         "be a nas symbol table.", 535);
      return;
      }
   SymbQTxt->seek(0);         /* Rewind */

// The file has been more or less validated.  Now read all the
// lines and build the hashes and InstLine table entries.

   while (1) {
      QString symbol;            /* One symbol name */
      int     sval;              /* Value of the symbol */
      int     qlbl;              /* 'true' if symbol is a label */
      bool    svok;              /* 'true' if sval conversion OK */
      Qsymline = SymbQTxt->readLine(MaxSymTabLine);
      if (Qsymline.isNull()) break;
      QStringList SymTokens = Qsymline.split(QRegExp(","));
      symbol = SymTokens[0];
      sval = SymTokens[1].toInt(&svok,10);
      if (!svok) {
         popupError("Nonnumeric address in symbol table.", 535);
         return;
         }
      qlbl = SymTokens[3] == "L";
      if (qlbl) {
         if (sval > MAX_IADDR) {
            popupError("Invalid instruction address in symbol table.",
               535);
            return;
            }
         InstLine[sval] = 1;
         LabelHash->insert(symbol, sval);
         PgmLocHash->insert(sval, symbol);
         }
      else {
         defNumHash->insert(symbol, sval);
         }
      } /* End main read loop */

// Indicate completion of symbol table load

   FilesLoaded |= SymbolsLoaded;

// Construct InstLine table from label information
// Then, if object code was already loaded, reload it so
//    the symbols can be displayed.

   InstLine[0] = 1;           /* Line for origin setting */
   for (int i=1; i<=MAX_IADDR; ++i)
      InstLine[i] += InstLine[i-1] + 1;
   if (FilesLoaded & CodeLoaded)
      loadObjectFile(*ObjFnm, krel_old);

   return;

   } /* End loadSymbolsFile() */


/*=====================================================================*
*                         menuLoadSymbolsFile                          *
*                                                                      *
*  This routine brings up a standard file selection menu, then calls   *
*  loadSymbolsFile to open and load the file selected by the user.     *
*=====================================================================*/

void DbgMainWindow::menuLoadSymbolsFile() {

   QString stfnm = QFileDialog::getOpenFileName(this,
      "Open Symbol Table File", Cwd->canonicalPath(),
      "SPP2 Symbol Table Files (*.sym)");
   if (!stfnm.isEmpty()) loadSymbolsFile(stfnm);

   } /* End menuLoadSymbolsFile() */


/*=====================================================================*
*                            loadDataFile                              *
*                                                                      *
*  Given a data file name and record number, this routine first checks *
*  whether a different data file is already open and if so closes it   *
*  and removes its data from memory, then opens the new file, assumed  *
*  to be an SPP2 memory data file, skips out to the requested record   *
*  number (corresponding to a particular cell in the simulation),      *
*  and reads the data into a binary buffer for the simulator and a     *
*  text buffer for the GUI.                                            *
*=====================================================================*/

void DbgMainWindow::loadDataFile(const QString& mdfnm, int mdrec) {

   QString canfnm = Cwd->absoluteFilePath(mdfnm);
   QString Qmdstr;               /* Memory data as a QString */
   int     imad;                 /* Memory address */
   int     lrec;                 /* Record length (bytes) */
   quint16 mdat;                 /* Memory data */
   quint16 rlen;                 /* Memory record length (words) */
   char mdln[LMDLine+1];         /* Memory data as a C string */

// Remove any memory data that might have previously been loaded.
// If a data file has already been specified, and it is different
//    from the file now requested, close it.

   if (FilesLoaded & DataLoaded) {
      if (!okToContinue("A data file has already been loaded:\n"
         "If you continue, all VM memory data will be modified."))
         return;
      for (int iaddr=0; iaddr<=MAX_DADDR; ++iaddr)
         VM->set_mem(iaddr, 0);
      if (canfnm != *MdatFnm) killMdatFile();
      }

// Open new data file.  (If this is an mdr call using same file
//    name, MdatFile will exist and this code will be skipped.)

   if (!MdatFile) {
      MdatFile = new QFile(canfnm);
      if (!MdatFile->open(QIODevice::ReadOnly)) {
         popupError("Unable to open specified SPP2 memory data "
            "(.bin) file.", 531);
         killMdatFile();
         return;
         }
      MdatQBin->setDevice(MdatFile);
      MdatRec = 1;
      *MdatFnm = canfnm;
      }

// If the requested data record is not equal to the next
// available data record, position file accordingly.
// End up positioned after the 16-byte header.

   if (mdrec < MdatRec) {     /* Lower number, rewind */
      if (!MdatFile->seek(0)) {
         popupError("Unable to rewind SPP2 memory data file.", 531);
         killMdatFile();
         return;
         }
      MdatRec = 1;
      }
   for ( ; MdatRec<=mdrec; ++MdatRec) {
      static const char ckrm[MdatLRecMark] =
         { 0x55, 0x55, 0xAA, 0xAA, 0xEF, 0xBE, 0xED, 0xFE };
      char inrm[MdatLRecMark];
      int  nskip;
      if (MdatQBin->readRawData(inrm, MdatLRecMark) < 0) {
         popupError("Unable to read memory data file record mark.",
            532);
         killMdatFile();
         return;
         }
      if (memcmp(inrm, ckrm, MdatLRecMark)) {
         popupError("File does not have expected record marker\n"
            "for an SPP2 memory data file.", 532);
         killMdatFile();
         return;
         }
      *MdatQBin >> rlen;
      if ((rlen % MdatRecSizeMult) != 0 || rlen > MdatMaxRecLen) {
         popupError("Invalid memory data file record length.", 532);
         killMdatFile();
         return;
         }
      lrec = BytesPerRecWord*rlen;
      nskip = (MdatRec == mdrec ? MdatLHeader : lrec) -
         (MdatLRecMark + sizeof(rlen));
      if (MdatQBin->skipRawData(nskip) < 0) {
         popupError("Error skipping over memory data file records.",
            532);
         killMdatFile();
         return;
         }
      }

// Read memory data file one 2-byte word at a time.
// Store raw binary data in simulator's memory data space space
// and store formatted data in QTextEdit widget.

   dbgr->DataViewer->clear(); /* Clear the memory data display */
   lrec -= MdatLHeader;       /* Length of header part skipped */
   for (imad=0; lrec > 0; ++imad,lrec-=SHSIZE) {
      if (MdatQBin->atEnd()) {
         popupError("Reached end-of-file while reading memory data.",
            532);
         }
      *MdatQBin >> mdat;

      VM->set_mem(imad, (int)mdat & UI16_MAX);

      wbcdwt(&imad, mdln, RK_HEXF|RK_NZNW|RK_NUNS|RK_NINT|3);
      mdln[4] = mdln[5] = ' ';
      wbcdwt(&mdat, mdln+6, RK_HEXF|RK_NZNW|RK_NUNS|RK_NHALF|4);
      mdln[11] = '\0';
      Qmdstr = (QString)mdln;
      dbgr->DataViewer->appendPlainText(Qmdstr);
      } /* End main read loop */

// Fill the rest of the VM mem viewer with "UNDEF"

   for ( ; imad <= MAX_DADDR; ++imad) {
      wbcdwt(&imad, mdln, RK_HEXF|RK_NZNW|RK_NUNS|RK_NINT|3);
      strcpy(mdln+4, "  UNDEF");
      Qmdstr = (QString)mdln;
      dbgr->DataViewer->appendPlainText(Qmdstr);
      }

   FilesLoaded |= DataLoaded;
   return;
   } /* End loadDataFile() */


/*=====================================================================*
*                          menuLoadDataFile                            *
*                                                                      *
*  This routine brings up a standard file selection menu, then calls   *
*  loadDataFile to open and load the data file selected by the user.   *
*=====================================================================*/

void DbgMainWindow::menuLoadDataFile() {

   QString mdfnm = QFileDialog::getOpenFileName(this,
      "Open Memory Data File", Cwd->canonicalPath(),
      "SPP2 Data Files (*.bin)");
   if (!mdfnm.isEmpty()) loadDataFile(mdfnm, 1);

   } /* End menuLoadDataFile() */
