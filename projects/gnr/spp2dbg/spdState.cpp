/* (c) Copyright 2009, Neurosciences Research Foundation, Inc. */
/***********************************************************************
*                            spdState.cpp                              *
*                                                                      *
*  The routines in this file are responsible for transferring updates  *
*  of the virtual machine state from the simulator to the GUI display  *
*  and from the GUI display to the virtual machine when edited by the  *
*  user.                                                               *
*----------------------------------------------------------------------*
*  V1A, 08/17/09, New program by G.N. Reeke                            *
***********************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <QString>
#include <QRadioButton>
#include <QCheckBox>
#include <QLineEdit>

#include "spp2dbg.h"
#include "rkarith.h"

#define HEXWTCC (RK_HEXF|RK_NZNW|RK_NINT|LRegHexd-1)

/* Convert accumulator three shorts to si64 */
si64 dAC64(int h, int m, int l) {
   ui32 low = m << BITSPERSHORT | l;
   return jcsw(h, low);
   }

/*=====================================================================*
*                          Display VM state                            *
*  This routine updates the widgets that display the state of the      *
*  simulated SPP2 machine.  The calling argument contains bit flags    *
*  set by the instruction simulator indicating which registers must    *
*  be polled for possible changes.  If this arg is passed as -1, all   *
*  registers will be polled.                                           *
*=====================================================================*/

void DbgMainWindow::spdState(int kregs) {

   char rval[LACDisp+1];         /* Reg values as char strings */

   /* Always display PC in hex */
   {  int dPC = VM->q_pc();
      wbcdwt(&dPC, rval, RK_HEXF|RK_NZNW|RK_NINT|LPCDisp-1);
      dbgr->PC_View->setText(QString(rval));
      }

   if (kregs & ChRadd) {         /* CALL return stack changed */
      int dRAd0 = VM->q_radd(1), dRAd1 = VM->q_radd(2);
      wbcdwt(&dRAd0, rval, RK_HEXF|RK_NZNW|RK_NINT|LPCDisp-1);
      dbgr->RAd0_View->setText(QString(rval));
      wbcdwt(&dRAd1, rval, RK_HEXF|RK_NZNW|RK_NINT|LPCDisp-1);
      dbgr->RAd1_View->setText(QString(rval));
      }

   if (kregs & ChFlg) {          /* Flag bits changed */
      dbgr->CARRY_View->setChecked(VM->q_CF());
      dbgr->NEG_View->setChecked(VM->q_NF());
      dbgr->ZERO_View->setChecked(VM->q_ZF());
      }

   if (DispBase == RK_IORF) {

/*---------------------------------------------------------------------*
*                        Everything in decimal                         *
*---------------------------------------------------------------------*/

      if (kregs & ChAC) {        /* Accumulator changed */
         si64 dAC = dAC64(VM->q_achi(), VM->q_acmd(), VM->q_aclo());
         wbcdwt(&dAC, rval, bd48|RK_IORF|RK_NI64|LACDisp-1);
         rval[LACDisp] = '\0';
         dbgr->AC_View->setText(QString(rval));
         }

      if (kregs & ChStk) {       /* AC Stack changed */
         int i;
         for (i=1; i<=STACK_DEPTH; ++i) {
            si64 dAC = dAC64(VM->q_stkhi(i), VM->q_stkmd(i),
               VM->q_stklo(i));
            wbcdwt(&dAC, rval, bd48|RK_IORF|RK_NI64|LACDisp-1);
            rval[LACDisp] = '\0';
            StkLE[i-1]->setText(QString(rval));
            }
         }

      if (kregs & ChReg) {       /* One or more GPRs changed */
         int regx,i;
         for (i=0; i<N_REGS; ++i) {
            regx = VM->q_reg(i);
            wbcdwt(&regx, rval, bd16|RK_IORF|RK_NINT|LRegDisp-1);
            rval[LRegDisp] = '\0';
            RegLE[i]->setText(QString(rval));
            }
         }

      if (kregs & ChThi) {       /* Temp hi register changed */
         int thi = VM->q_thi();
         wbcdwt(&thi, rval, bd16|RK_IORF|RK_NINT|LRegDisp-1);
         rval[LRegDisp] = '\0';
         dbgr->THI_View->setText(QString(rval));
         }

      } /* End decimal displays */

   else {

/*---------------------------------------------------------------------*
*                      Everything in hexadecimal                       *
*---------------------------------------------------------------------*/

      if (kregs & ChAC) {        /* Accumulator changed */
         int hAC = VM->q_achi();
         wbcdwt(&hAC, rval, HEXWTCC);
         hAC = VM->q_acmd();
         wbcdwt(&hAC, rval+LRegHexd, HEXWTCC);
         hAC = VM->q_aclo();
         wbcdwt(&hAC, rval+2*LRegHexd, HEXWTCC);
         rval[3*LRegHexd] = '\0';
         dbgr->AC_View->setText(QString(rval));
         }

      if (kregs & ChStk) {       /* AC Stack changed */
         int hAC,i;
         for (i=1; i<=STACK_DEPTH; ++i) {
            hAC = VM->q_stkhi(i);
            wbcdwt(&hAC, rval, HEXWTCC);
            hAC = VM->q_stkmd(i);
            wbcdwt(&hAC, rval+LRegHexd, HEXWTCC);
            hAC = VM->q_stklo(i);
            wbcdwt(&hAC, rval+2*LRegHexd, HEXWTCC);
            rval[3*LRegHexd] = '\0';
            StkLE[i-1]->setText(QString(rval));
            }
         }

      if (kregs & ChReg) {       /* One or more GPRs changed */
         int regx,i;
         for (i=0; i<N_REGS; ++i) {
            regx = VM->q_reg(i);
            wbcdwt(&regx, rval, HEXWTCC);
            rval[LRegHexd] = '\0';
            RegLE[i]->setText(QString(rval));
            }
         }

      if (kregs & ChThi) {       /* Temp hi register changed */
         int thi = VM->q_thi();
         wbcdwt(&thi, rval, HEXWTCC);
         rval[LRegHexd] = '\0';
         dbgr->THI_View->setText(QString(rval));
         }

      } /* End hex displays */

   } /* End spdState() */

/*=====================================================================*
*                    Update VM state from display                      *
*                                                                      *
*  These routines are connected by the configDisplay() setup routine   *
*  to the textEdited signal emitted by the various QLineEdit widgets   *
*  in the main GUI.  The texts will have already been validated, so no *
*  further checking is needed here (except what wbcdin does anyway).   *
*=====================================================================*/

/* Interpret text as a 64-bit AC or 16-bit register */
void DbgMainWindow::getACVal(act *pac, const QString& ACtxt) {
   si64 acval;
   ui32 aclow, ccode = RK_NI64|LACDisp;
   std::string stdactxt = ACtxt.toStdString();
   const char *actxt = stdactxt.c_str();
   ccode |= (DispBase == RK_IORF) ? (bscl|RK_IORF) : RK_HEXF;
   wbcdin(actxt, &acval, ccode);
   aclow = swlo(acval);
   pac->h = (si16)swhi(acval);
   pac->m = (ui16)(aclow >> BITSPERSHORT);
   pac->l = (ui16)(aclow & UI16_MAX);
   } /* End getACVal() */

int DbgMainWindow::getRegVal(const QString& Regtxt) {
   int regval;
   ui32 ccode = RK_NINT|LRegDisp;
   std::string stdregtxt = Regtxt.toStdString();
   const char *regtxt = stdregtxt.c_str();
   ccode |= (DispBase == RK_IORF) ? (bscl|RK_IORF) : RK_HEXF;
   wbcdin(regtxt, &regval, ccode);
   return regval;
   } /* End getRegVal() */

void DbgMainWindow::updVM_RAd0(const QString& RAd0txt) {
   bool bok = 0;
   int rad0 = RAd0txt.toInt(&bok, 16);
   if (!bok) VM->set_radd(1, rad0);
   }

void DbgMainWindow::updVM_RAd1(const QString& RAd1txt) {
   bool bok = 0;
   int rad1 = RAd1txt.toInt(&bok, 16);
   if (!bok) VM->set_radd(2, rad1);
   }

void DbgMainWindow::updVM_AC(const QString& ACtxt) {
   act newac;
   getACVal(&newac, ACtxt);
   VM->set_ac(newac.h, newac.m, newac.l);
   }

void DbgMainWindow::updVM_Stk1(const QString& Stk1txt) {
   act newstk1;
   getACVal(&newstk1, Stk1txt);
   VM->set_stk(1, newstk1.h, newstk1.m, newstk1.l);
   }

void DbgMainWindow::updVM_Stk2(const QString& Stk2txt) {
   act newstk2;
   getACVal(&newstk2, Stk2txt);
   VM->set_stk(2, newstk2.h, newstk2.m, newstk2.l);
   }

void DbgMainWindow::updVM_Stk3(const QString& Stk3txt) {
   act newstk3;
   getACVal(&newstk3, Stk3txt);
   VM->set_stk(3, newstk3.h, newstk3.m, newstk3.l);
   }

void DbgMainWindow::updVM_Stk4(const QString& Stk4txt) {
   act newstk4;
   getACVal(&newstk4, Stk4txt);
   VM->set_stk(4, newstk4.h, newstk4.m, newstk4.l);
   }

void DbgMainWindow::updVM_Stk5(const QString& Stk5txt) {
   act newstk5;
   getACVal(&newstk5, Stk5txt);
   VM->set_stk(5, newstk5.h, newstk5.m, newstk5.l);
   }

void DbgMainWindow::updVM_THI(const QString& THItxt) {
   VM->set_thi(getRegVal(THItxt));
   }

void DbgMainWindow::updVM_CF(int jcf) {
   VM->set_CF(jcf);
   }

void DbgMainWindow::updVM_NF(int jnf) {
   VM->set_NF(jnf);
   }

void DbgMainWindow::updVM_ZF(int jzf) {
   VM->set_ZF(jzf);
   }

void DbgMainWindow::updVM_Reg0(const QString& Reg0txt) {
   VM->set_reg(0, getRegVal(Reg0txt));
   }

void DbgMainWindow::updVM_Reg1(const QString& Reg1txt) {
   VM->set_reg(1, getRegVal(Reg1txt));
   }

void DbgMainWindow::updVM_Reg2(const QString& Reg2txt) {
   VM->set_reg(2, getRegVal(Reg2txt));
   }

void DbgMainWindow::updVM_Reg3(const QString& Reg3txt) {
   VM->set_reg(3, getRegVal(Reg3txt));
   }

void DbgMainWindow::updVM_Reg4(const QString& Reg4txt) {
   VM->set_reg(4, getRegVal(Reg4txt));
   }

void DbgMainWindow::updVM_Reg5(const QString& Reg5txt) {
   VM->set_reg(5, getRegVal(Reg5txt));
   }

void DbgMainWindow::updVM_Reg6(const QString& Reg6txt) {
   VM->set_reg(6, getRegVal(Reg6txt));
   }

void DbgMainWindow::updVM_Reg7(const QString& Reg7txt) {
   VM->set_reg(7, getRegVal(Reg7txt));
   }

void DbgMainWindow::updVM_Reg8(const QString& Reg8txt) {
   VM->set_reg(8, getRegVal(Reg8txt));
   }

void DbgMainWindow::updVM_Reg9(const QString& Reg9txt) {
   VM->set_reg(9, getRegVal(Reg9txt));
   }

void DbgMainWindow::updVM_Rega(const QString& Regatxt) {
   VM->set_reg(10, getRegVal(Regatxt));
   }

void DbgMainWindow::updVM_Regb(const QString& Regbtxt) {
   VM->set_reg(11, getRegVal(Regbtxt));
   }

void DbgMainWindow::updVM_Regc(const QString& Regctxt) {
   VM->set_reg(12, getRegVal(Regctxt));
   }

void DbgMainWindow::updVM_Regd(const QString& Regdtxt) {
   VM->set_reg(13, getRegVal(Regdtxt));
   }

void DbgMainWindow::updVM_Rege(const QString& Regetxt) {
   VM->set_reg(14, getRegVal(Regetxt));
   }

void DbgMainWindow::updVM_Regf(const QString& Regftxt) {
   VM->set_reg(15, getRegVal(Regftxt));
   }
