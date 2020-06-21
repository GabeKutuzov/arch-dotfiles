/* (c) Copyright 2009, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                             spp2dbg.cpp                              *
*                                                                      *
*  This file and the accompanying Qt .ui file implement an offline     *
*  debugger for the NRF SPP2 special-purpose-processor NPU.  It can    *
*  load an assembled output file, the symbol table made from the same  *
*  source, and a data file used to initialize NPU memory.              *
*  It provides a GUI with windows for the source and object code,      *
*  displays of all programmer-visible machine state, and a command     *
*  window in which gdb-like instructions can be entered.               *
*                                                                      *
*  The supported instructions are documented in the implementing       *
*  routine DbgMainWindow::RunCommand in file spdCmd.cpp.               *
*                                                                      *
*  In addition, the PC, AC, stack, registers, and flags may be         *
*  changed at any time by entering new values into the display.        *
*                                                                      *
*  Abend error codes 530-539 are assigned to this module.              *
*                                                                      *
*  Availabilityu of crk header files and library routines written by   *
*  GNR is assumed                                                      *
*----------------------------------------------------------------------*
*  V1A, 07/30/09, New program by G.N. Reeke                            *
*  V1B, 11/29/09, GNR - Add symbol table and usage in debug commands   *
***********************************************************************/

#include <stdlib.h>
#include <errno.h>
#include <QApplication>
#include <QMainWindow>
#include <QLineEdit>

#define MAIN
#include "spp2dbg.h"
#include "rocks.h"

/***********************************************************************
*                       ABNORMAL EXIT FUNCTIONS                        *
*  These functions have the same names and arguments as the corres-    *
*  ponding routines in the crk library, but here bring up a critical   *
*  Qt message box instead of writing to stdout/stderr.                 *
***********************************************************************/

/*---------------------------------------------------------------------*
*                              abexit()                                *
*                                                                      *
*  This routine is used to handle an abnormal termination produced     *
*  by a program in the ROCKS library when called from outside the      *
*  normal ROCKS environment--in this case, from a Qt application.      *
*  It generates a Qt 'critical' message box, then terminates with      *
*  the requested exit code.                                            *
*                                                                      *
*  Usage: void abexit(int code)                                        *
*                                                                      *
*  Argument: 'code' is a unique abnormal termination code which        *
*     must be documented in the ~/src/crk/errtab file.                 *
*                                                                      *
*  Return:  This routine does not return.  It currently calls the      *
*     standard C library exit routine, but may be implemented on       *
*     IBM systems to produce a genuine abend.                          *
*---------------------------------------------------------------------*/

void abexit(int code) {

   QString abmsg;
   QTextStream(&abmsg) << "***Program terminated with abend code " <<
      code;
   QMessageBox::critical(0, "SPP2 Debugger", abmsg,
      QMessageBox::Ok);
   exit(code > 255 ? 100 : code);
   } /* End abexit() */


/*---------------------------------------------------------------------*
*                              abexitm()                               *
*                                                                      *
*  Same as abexit() except provides and additional message component.  *
*---------------------------------------------------------------------*/

void abexitm(int code, char *emsg) {

   QString abmsg;
   QTextStream(&abmsg) << "***" << emsg <<
      "\nProgram terminated with abend code " << code;
   QMessageBox::critical(0, "SPP2 Debugger", abmsg,
      QMessageBox::Ok);
   exit(code > 255 ? 100 : code);
   } /* End abexitm() */


/*---------------------------------------------------------------------*
*                             abexitme()                               *
*                                                                      *
*  Same as abexitm() except also displays the system errno code.       *
*---------------------------------------------------------------------*/

void abexitme(int code, char *emsg) {

   int sverrno = errno;
   QString abmsg;
   QTextStream(&abmsg) << "***" << emsg <<
      "\nThe system error code was " << sverrno <<
      "\nProgram terminated with abend code " << code;
   QMessageBox::critical(0, "SPP2 Debugger", abmsg,
      QMessageBox::Ok);
   exit(code > 255 ? 100 : code);
   } /* End abexitme() */


/***********************************************************************
*                      IMPLEMENTATION FUNCTIONS                        *
***********************************************************************/

/*---------------------------------------------------------------------*
*                 Construct the DbgMainWindow object                   *
*---------------------------------------------------------------------*/

DbgMainWindow::DbgMainWindow() {
   dbgr = new Ui_MainWindow;
   dbgr->setupUi(this);
   Cwd = new QDir();
   ObjFnm   = new QString();
   ObjFile  = NULL;
   ObjQTxt  = new QTextStream();
   SymbFnm  = new QString();
   SymbFile = NULL;
   SymbQTxt = new QTextStream();
   MdatFnm  = new QString();
   MdatFile = NULL;
   MdatQBin = new QDataStream();
   MdatQBin->setByteOrder(QDataStream::LittleEndian);
   KindAddr = new QRegExp("\\*?(0[xX])?[0-9A-Fa-f]+");
   LabelHash  = new QHash<QString, int>();
   defNumHash = new QHash<QString, int>();
   PgmLocHash = new QHash<int, QString>();
   MdatRec  = 0;
   BkptAddr = NULL;
   InstLine = NULL;
   FilesLoaded = 0;
   LastBkpt = 0;
   PrevMarked = -1;
   RunMode  = 0;
   VM = new spp2vm(spp2vm::q_max_iaddr(), spp2vm::q_max_daddr());
   /* Allow access to AC stack and registers by index number */
   StkLE[0] = dbgr->Stk1_View;
   StkLE[1] = dbgr->Stk2_View;
   StkLE[2] = dbgr->Stk3_View;
   StkLE[3] = dbgr->Stk4_View;
   StkLE[4] = dbgr->Stk5_View;
   RegLE[0] = dbgr->Reg0_View;
   RegLE[1] = dbgr->Reg1_View;
   RegLE[2] = dbgr->Reg2_View;
   RegLE[3] = dbgr->Reg3_View;
   RegLE[4] = dbgr->Reg4_View;
   RegLE[5] = dbgr->Reg5_View;
   RegLE[6] = dbgr->Reg6_View;
   RegLE[7] = dbgr->Reg7_View;
   RegLE[8] = dbgr->Reg8_View;
   RegLE[9] = dbgr->Reg9_View;
   RegLE[10] = dbgr->Rega_View;
   RegLE[11] = dbgr->Regb_View;
   RegLE[12] = dbgr->Regc_View;
   RegLE[13] = dbgr->Regd_View;
   RegLE[14] = dbgr->Rege_View;
   RegLE[15] = dbgr->Regf_View;
   createActions();
   configDisplay();
   } /* End DbgMainWindow constructor */

/*---------------------------------------------------------------------*
*            Construct actions for menu and toolbar items              *
*---------------------------------------------------------------------*/

void DbgMainWindow::createActions() {

   /* Set up Load Object action */
   dbgr->actionLoad_Object->setShortcut(QKeySequence("Ctrl+O"));
   dbgr->actionLoad_Object->setStatusTip("Open object code file");
   connect(dbgr->actionLoad_Object, SIGNAL(triggered()),
      this, SLOT(menuLoadObjectFile()));
   dbgr->InstrViewer->setReadOnly(TRUE);
   dbgr->InstrViewer->ensureCursorVisible();

   /* Set up Load Symbols action */
   SymTabChk = new QRegExp("[A-Za-z0-9_]+,[0-9]+,[UN],[LD]");
   dbgr->actionLoad_Symbols->setShortcut(QKeySequence("Ctrl+S"));
   dbgr->actionLoad_Symbols->setStatusTip("Open symbol table file");
   connect(dbgr->actionLoad_Symbols, SIGNAL(triggered()),
      this, SLOT(menuLoadSymbolsFile()));

   /* Set up Load Data action */
   dbgr->actionLoad_Data->setShortcut(QKeySequence("Ctrl+D"));
   dbgr->actionLoad_Data->setStatusTip("Open memory data file");
   connect(dbgr->actionLoad_Data, SIGNAL(triggered()),
      this, SLOT(menuLoadDataFile()));
   dbgr->DataViewer->setReadOnly(TRUE);

   /* Set up Quit action */
   dbgr->actionQuit->setShortcut(QKeySequence("Alt+Q"));
   dbgr->actionQuit->setStatusTip("Terminate Debugger");
   connect(dbgr->actionQuit, SIGNAL(triggered()),
      this, SLOT(close()));

   /* Set up Command Summary action */
   connect(dbgr->actionCommand_Summary, SIGNAL(triggered()),
      this, SLOT(DisplayCommandSummary()));

   /* Set up About action */
   connect(dbgr->actionAbout, SIGNAL(triggered()),
      this, SLOT(DisplayAboutInfo()));

   } /* End createActions() */


/*---------------------------------------------------------------------*
*     Configure Numeric Displays for Decimal/Hex and Binary Scale      *
*---------------------------------------------------------------------*/

/* Set the display mode to decimal or hexadecimal.  No error test
*  is considered necessary, since this routine will only be called
*  with controlled argument.  */

/* Slot fillers -- no args */
void DbgMainWindow::setDisplayBaseDec() {
   setDisplayBase(spdDecimal);
   }
void DbgMainWindow::setDisplayBaseHex() {
   setDisplayBase(spdHex);
   }
void DbgMainWindow::setDisplayBase(enum DisplayMode dmode) {

   int i;
   switch (dmode) {
   case spdDecimal:
      DispBase = RK_IORF;
      dbgr->AC_View->setValidator(All_ValDec);
      for (i=0; i<STACK_DEPTH; ++i)
         StkLE[i]->setValidator(All_ValDec);
      for (i=0; i<N_REGS; ++i)
         RegLE[i]->setValidator(All_ValDec);
      dbgr->THI_View->setValidator(All_ValDec);
      break;
   case spdHex:
      DispBase = RK_HEXF;
      dbgr->AC_View->setValidator(AC_ValHex);
      for (i=0; i<STACK_DEPTH; ++i)
         StkLE[i]->setValidator(AC_ValHex);
      for (i=0; i<N_REGS; ++i)
         RegLE[i]->setValidator(Reg_ValHex);
      dbgr->THI_View->setValidator(Reg_ValHex);
      break;
      } /* End dmode switch */

   spdState(-1);  /* And rewrite the display */

   } /* End setDisplayBase() */

/* Set the binary and decimal scales of the numeric displays
*  according to the value in the Bscl QLineEdit widget.  */

void DbgMainWindow::setDisplayBscl(const QString& bstxt) {
   /* Decimal parameters for 48-bit accumulators as fn of bscl */
   static const byte dec48[48] = { 0,2,3,4,4,5,5,5,5,6,6,6,7,7,7,8,8,
      8,8,9,9,9,10,10,10,11,11,11,11,12,12,12,
      13,13,13,14,14,14,14,15,15,15,16,16,16,17,17,17 };
   /* Decimal parameters for 16-bit registers as fn of bscl */
   static const byte dec16[16] = { 0,2,3,4,4,4,4,5,5,5,6,6,6,7,7,7 };

   /* Validator was placed on the widget, so value here should
   *  be constrained to interval 0-63 without need to check.  */
   int iscl = bstxt.toInt();
   bscl = bd16 = bd48 = (ui32)iscl << RK_BS;
   bd16 |= (iscl >= 16 ?  7 : (int)dec16[iscl]) << RK_DS;
   bd48 |= (iscl >= 48 ? 17 : (int)dec48[iscl]) << RK_DS;

   spdState(-1);  /* And rewrite the display */
   } /* End setDisplayBscl() */


/*---------------------------------------------------------------------*
*           Initial Configuration of Machine State Display             *
*---------------------------------------------------------------------*/

void DbgMainWindow::configDisplay() {

   /* Regular expressions for checking register inputs */
   static const QRegExp *ACReg_Dec = new QRegExp(" ?-?[0-9.]+");
   static const QRegExp *AC_Hex = new QRegExp(
      " ?[0-9A-Fa-f]{,4} +[0-9A-Fa-f]{,4} +[0-9A-Fa-f]{1,4}");
   static const QRegExp *Reg_Hex = new QRegExp(" ?[0-9A-Fa-f]{1,4}");
   static const QRegExp *Bscl_Dec = new QRegExp(" ?[0-9]{1,2}");
   static const QRegExp *Reg_Name = new QRegExp("(R|reg)[0-9a-f]");

   /* Various widget properties etc. */
   bscl = bd48 = bd16 = 0;
   dbgr->AC_View->setMaxLength(LACDisp);
   dbgr->PC_View->setMaxLength(LPCDisp);
   dbgr->PC_View->setReadOnly(TRUE);
   dbgr->RAd0_View->setMaxLength(LPCDisp);
   dbgr->RAd1_View->setMaxLength(LPCDisp);
   dbgr->Dec_OP->setChecked(TRUE);
   dbgr->CommandWindow->setMaximumBlockCount(MaxCmdDisplay);
   dbgr->CommandWindow->setReadOnly(TRUE);
   dbgr->CommandInput->setText(">");
   connect(dbgr->CommandInput, SIGNAL(returnPressed()),
      this, SLOT(RunCommand()));

   /* Establish decimal, hex, and name validators for the inputs */
   All_ValDec = new QRegExpValidator(*ACReg_Dec, this);
   AC_ValHex  = new QRegExpValidator(*AC_Hex, this);
   Reg_ValHex = new QRegExpValidator(*Reg_Hex, this);
   Reg_ValName= new QRegExpValidator(*Reg_Name, this);
   connect(dbgr->Dec_OP, SIGNAL(clicked()),
      this, SLOT(setDisplayBaseDec()));
   connect(dbgr->Hex_OP, SIGNAL(clicked()),
      this, SLOT(setDisplayBaseHex()));

   /* Set changes in bscl widget to change scale accordingly */
   Bscl_Val = new QRegExpValidator(*Bscl_Dec, this);
   dbgr->Bscl_Set->setValidator(Bscl_Val);
   connect(dbgr->Bscl_Set, SIGNAL(textEdited(const QString&)),
      this, SLOT(setDisplayBscl(const QString&)));

   /* Connect user changes in registers to implementing routines */
   connect(dbgr->RAd0_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_RAd0(const QString&)));
   connect(dbgr->RAd1_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_RAd1(const QString&)));
   connect(dbgr->AC_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_AC(const QString&)));
   connect(dbgr->Stk1_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Stk1(const QString&)));
   connect(dbgr->Stk2_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Stk2(const QString&)));
   connect(dbgr->Stk3_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Stk3(const QString&)));
   connect(dbgr->Stk4_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Stk4(const QString&)));
   connect(dbgr->Stk5_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Stk5(const QString&)));
   connect(dbgr->THI_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_THI(const QString&)));
   connect(dbgr->CARRY_View, SIGNAL(stateChanged(int)),
      this, SLOT(updVM_CF(int)));
   connect(dbgr->NEG_View, SIGNAL(stateChanged(int)),
      this, SLOT(updVM_NF(int)));
   connect(dbgr->ZERO_View, SIGNAL(stateChanged(int)),
      this, SLOT(updVM_ZF(int)));
   connect(dbgr->Reg0_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Reg0(const QString&)));
   connect(dbgr->Reg1_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Reg1(const QString&)));
   connect(dbgr->Reg2_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Reg2(const QString&)));
   connect(dbgr->Reg3_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Reg3(const QString&)));
   connect(dbgr->Reg4_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Reg4(const QString&)));
   connect(dbgr->Reg5_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Reg5(const QString&)));
   connect(dbgr->Reg6_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Reg6(const QString&)));
   connect(dbgr->Reg7_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Reg7(const QString&)));
   connect(dbgr->Reg8_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Reg8(const QString&)));
   connect(dbgr->Reg9_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Reg9(const QString&)));
   connect(dbgr->Rega_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Rega(const QString&)));
   connect(dbgr->Regb_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Regb(const QString&)));
   connect(dbgr->Regc_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Regc(const QString&)));
   connect(dbgr->Regd_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Regd(const QString&)));
   connect(dbgr->Rege_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Rege(const QString&)));
   connect(dbgr->Regf_View, SIGNAL(textEdited(const QString&)),
      this, SLOT(updVM_Regf(const QString&)));

   setDisplayBase(spdDecimal);
   spdState(-1);                 /* Put text in all displays */
   } /* End configDisplay() */


/*---------------------------------------------------------------------*
*                       Key Press Event Handler                        *
*  When a simulation is running ("run" or "continue" command), any     *
*  key press sets a flag to stop the simulation and return to normal   *
*  keyboard input to the command line.                                 *
*---------------------------------------------------------------------*/

void DbgMainWindow::keyPressEvent(QKeyEvent *event) {

   if (RunMode & SimRunning)
      RunMode |= SimStopRqst;
   else
      QWidget::keyPressEvent(event);

   } /* End keyPressEvent() */


/***********************************************************************
*                            MAIN PROGRAM                              *
***********************************************************************/

int main(int argc, char *argv[]) {

   QApplication app(argc, argv);
   DbgMainWindow *MainWindow = new DbgMainWindow;
   MainWindow->show();

   return app.exec();            /* It all happens here */

   } /* End spp2dbg main program */
