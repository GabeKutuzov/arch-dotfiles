/* (c) Copyright 2009, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                              spp2dbg.h                               *
*                                                                      *
*  This is the header file for the spp2dbg project.                    *
*----------------------------------------------------------------------*
*  V1A, 07/30/09, New program by G.N. Reeke                            *
*  V1B, 11/29/09, GNR - Add symbol table and usage in debug commands   *
***********************************************************************/

#ifndef SPP2DBG_MAIN_H
#define SPP2DBG_MAIN_H

#include <QApplication>
#include <QMainWindow>
#include <QString>
#include <QDir>
#include <QFile>
#include <QDataStream>
#include <QMessageBox>
#include <QRegExp>
#include <QRegExpValidator>
#include <QTextBlock>
#include <QTextStream>
#include <QHash>
#include <QWidget>
#include "ui_spp2dbg.h"
#include "sysdef.h"
#include "spp2sim.h"
#include "rkxtra.h"
#include "rksubs.h"

/* Program configuration parameters */
#define BytesPerRecWord    4     /* Byte/word in Mdat files */
#define CmdDisplayTabs     4     /* Command display tab stop width */
/* Maximum chars in register displays allowing 2 extra decimals */
#define LACDisp           19     /* Num chars in AC display */
#define LCmdLine          12     /* Length of a line in instr list */
#define LMDLine           12     /* Length of a line in mdat widget */
#define LOCLN              8     /* Max length of object file line */
#define LRegDisp           9     /* Num chars in Reg display */
#define LRegHexd           5     /* Num chars in 16-bit hex */
#define LPCDisp            3     /* Num chars in PC display */
#define MaxBkpts         100     /* Maximum number of breakpoints */
#define MaxCmdDisplay    500     /* Max command lines */
#define MaxSymTabLine    120     /* !Arbitrary! */
#define MdatLHeader       16     /* Length of skipped .bin header */
#define MdatLRecMark       8     /* Length of .bin file record mark */
#define MdatRecSizeMult    8     /* Size constraint on .bin records */
#define MdatMaxRecLen   1024     /* Max .bin file rec size (words) */

/* Display base modes -- args to setDisplayBase */
enum DisplayMode { spdDecimal=0, spdHex=1 };

class DbgMainWindow : public QMainWindow {
   Q_OBJECT

public:
   DbgMainWindow();
   Ui_MainWindow *dbgr;
   void abexit(int code);
   void abexitm(int code, char *emsg);
   void abexitme(int code, char *emsg);

private slots:
   void DisplayAboutInfo();
   void DisplayCommandSummary();
   void menuLoadObjectFile();
   void menuLoadSymbolsFile();
   void menuLoadDataFile();
   void RunCommand();
   void setDisplayBaseDec();
   void setDisplayBaseHex();
   void setDisplayBscl(const QString&);
   void updVM_RAd0(const QString&);
   void updVM_RAd1(const QString&);
   void updVM_AC(const QString&);
   void updVM_Stk1(const QString&);
   void updVM_Stk2(const QString&);
   void updVM_Stk3(const QString&);
   void updVM_Stk4(const QString&);
   void updVM_Stk5(const QString&);
   void updVM_THI(const QString&);
   void updVM_CF(int);
   void updVM_NF(int);
   void updVM_ZF(int);
   void updVM_Reg0(const QString&);
   void updVM_Reg1(const QString&);
   void updVM_Reg2(const QString&);
   void updVM_Reg3(const QString&);
   void updVM_Reg4(const QString&);
   void updVM_Reg5(const QString&);
   void updVM_Reg6(const QString&);
   void updVM_Reg7(const QString&);
   void updVM_Reg8(const QString&);
   void updVM_Reg9(const QString&);
   void updVM_Rega(const QString&);
   void updVM_Regb(const QString&);
   void updVM_Regc(const QString&);
   void updVM_Regd(const QString&);
   void updVM_Rege(const QString&);
   void updVM_Regf(const QString&);

protected:
   void    keyPressEvent(QKeyEvent *event);

private:
   int     chkPgmLoaded(int kck);
#define kck_pgm   0     /* Just check program */
#define kck_pdat  1     /* Check program and data */
   void    createActions();
   void    configDisplay();
   QString Disassemble(int xcmd);
   void    getACVal(act *pac, const QString& ACtxt);
   int     getAddr(const QString& QSAddr, int kad);
#define kad_pgm   0     /* Requesting program address */
#define kad_mem   1     /* Requesting memory address */
   QString getFile(const QString& Cmd);
   int     getPosInt(const QString& QSInt);
   int     getRegVal(const QString& Regtxt);
   int     iaddrToTextLine(int iaddr);
   void    killMdatFile();
   void    killSymbolTable();
   void    loadDataFile(const QString& mdfnm, int mdrec);
   void    loadSymbolsFile(const QString& stfnm);
   void    loadObjectFile(const QString& objfnm, int krel);
#define krel_old  0     /* Reload existing object file */
#define krel_new  1     /* Load a new object file */
   void    markInstruction(int iaddr);
   int     okToContinue(const QString& OKtxt);
   void    popupError(const QString& Errtxt, int abex);
   void    setDisplayBase(enum DisplayMode dmode);
   void    spdState(int kregs);
   void    updateMemDisplay(int imad);

   spp2vm *VM;
   QDir             *Cwd;        /* Current working directory */
   QString          *ObjFnm;     /* Object code file name */
   QFile            *ObjFile;    /* Object code file */
   QTextStream      *ObjQTxt;    /* Data stream for ObjFile */
   QString          *SymbFnm;    /* Symbol table file name */
   QFile            *SymbFile;   /* Symbol table file */
   QTextStream      *SymbQTxt;   /* Data stream for SymbFile */
   QRegExp          *SymTabChk;  /* Symbol table line validator */
   QString          *MdatFnm;    /* Memory data file name */
   QFile            *MdatFile;   /* Memory data file */
   QDataStream      *MdatQBin;   /* Data stream for MdatFile */
   QRegExp          *KindAddr;   /* Address number/symbol checker */
   QRegExpValidator *All_ValDec; /* Decimal validator for AC+Regs */
   QRegExpValidator *AC_ValHex;  /* Accumulator hex validator */
   QRegExpValidator *Reg_ValName;/* Register name validator */
   QRegExpValidator *Reg_ValHex; /* Register hex validator */
   QRegExpValidator *Bscl_Val;   /* Bscl validator */
   QLineEdit *StkLE[STACK_DEPTH];/* Ptrs to stack displays */
   QLineEdit *RegLE[N_REGS];     /* Ptrs to register displays */
   QHash<QString, int> *LabelHash;  /* Label look-up */
   QHash<QString, int> *defNumHash; /* defNum look-up */
   QHash<int, QString> *PgmLocHash; /* Pgm location look-up */
   int    *BkptAddr;             /* Breakpoint address table */
   int    *InstLine;             /* Instruction line table */
   QTextBlock ObjLastMark;       /* Object block last marked */
   ui32   bscl,bd48,bd16;        /* Bin|Dec scale for displays */
   int    DispBase;              /* RK_IORF or RK_HEXF */
   int    FilesLoaded;           /* Flags for files loaded status */
#define CodeLoaded      1           /* TRUE if program code loaded */
#define DataLoaded      2           /* TRUE if memory data loaded */
#define SymbolsLoaded   4           /* TRUE if symbol table loaded */
   int    LastBkpt;              /* Number of last breakpoint */
   int    MdatRec;               /* Next record in MdatFile */
   int    PrevMarked;            /* Previously marked instruction */
   volatile int RunMode;            /* Flags for running simulator */
#define SimRunning   1              /* TRUE if simulation running */
#define SimStopRqst  2              /* TRUE if sim stop requested */

   }; /* End DbgMainWindow class declaration */

#endif
