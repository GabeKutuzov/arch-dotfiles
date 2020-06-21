/*--------------------------------------------------------------------*/
/*                                                                    */
/*    EQSCAN and KWSCAN test                                          */
/*                                                                    */
/*--------------------------------------------------------------------*/

/*
   This test executes eqscan and kwscan with each possible calling code.
   Rev, 05/30/96, GNR - bcdin now returns 0 when QPOS test fails.
   Rev, 01/28/97, GNR - Add tests for unsigned integer types.
   Rev, 09/06/98, GNR - Add tests for N and O codes.
   Rev, 09/13/98, GNR - Change all printf to cryout for ordering.
   Rev, 07/27/00, GNR - Add test for R code.
   Rev, 04/27/02, GNR - First arg to kwscan is now a long.
   Rev, 12/31/07, GNR - Add new valck codes to kwscan
   Rev, 02/16/09, GNR - Add tests for '+=' and value adjustment
   Rev, 05/03/09, GNR - Add tests for J code.
   Rev, 08/16/09, GNR - Add tests for 64-bit input, renumber tests
   Rev, 12/18/09, GNR - Add cryout note about big-end, little-end ic
   Rev, 12/26/09, GNR - Add tests for IS code, count errors
   Rev, 06/15/12, GNR - Add tests for O code with on/off setting
   Rev, 09/02/13, GNR - Add tests for keys with prefixes
*/

#define MAIN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"
#include "rkarith.h"

void kwnfn(char *pn) {
   eqscan(pn,"A4",1);
   }

void kwjfn(char *pit, int *iv) {
   int it = pit[0] - '0';
   eqscan(iv,"I4",1);
   *iv -= it;
   }

int main() {

   char *card = "A=ABCDEFG,B=12,C=13,D=-1329,E=123456,F=1.25,G=2.4,H=189.75632 ";
   char *card2 = "AMB=18,XYZ=99,VAL1,X1,X2,NAME=GNR,OPTION,INC1=5,INC2=7";
   char *card3 = "KEY=A,KEY+=B,X=13.8mV+Vr,IVAL=12,BIG64=6.789E9,SET64";
   char *card4 = "R1=1009,R2=(34567,2106798003),R3=67890123456";
   char *card5 = "OPT1=TRUE,OPT2=FALSE,OPT3,OPT4=0,OPT5=1";
   char *card6 = "ITJ1=1,ITJ2=2,XY.ITJ3=3,IT.ITJ4=4";
   char lea[DFLT_MAXLEN+1];
   char item1[] = {"??????Z"};
   int  item2;
   schr item3;
   short item4;
   long item5;
   long item6;
   unsigned int item7;
   byte item8;
   unsigned short item9,iorh1,iorh2,iorh3;
   unsigned long item10;
   unsigned long item11;
   float item12;
   double item13;
   char item14[4];
   int  item15;
   int  item16,item17;
   si64 item18,item19,true1819;
   wseed item20,item21,item22;
   ui32 ic,icc;               /* kwscan items entered code */
   int  icrc;                 /* kwscan return code */
   int  iorv1,iorv2;          /* Values to be used in OR test */
   int  nerr = 0, nok = 0;

   cryout(RK_P1, " eqscan and kwscan test results:", RK_LN3,
      " Note:  ic values printed will be reversed for big-endian",
      RK_LN0, "     vs. little-endian systems.", RK_LN0, NULL);
   cdprnt(card);
   cdscan(card,0,DFLT_MAXLEN,RK_NOCONT);
   okmark(FALSE);

/* Initialize everybody */
   item2 = 0;
   item3 = 0;
   item4 = 0;
   item5 = 0;
   item6 = 0;
   item7 = 0;
   item8 = 0;
   item9 = 0;
   item10 = 0;
   item11 = 0;
   item12 = 0;
   item13 = 0;

/* Test 1: Alphanumeric field */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"A",1) == 0) abexit (1);
   if (eqscan(item1,"A6",1))
      cryout(RK_P1, " Test 1 eqscan nonzero rc", RK_LN1, NULL);
   if (strcmp(item1,"ABCDEFZ")) {
      cryout(RK_P1, " Test 1 fails, returning >", RK_LN1,
         item1, 7, "<", 1, NULL);
      ++nerr; }
   else ++nok;

/* Test 2: Integer conversion, default type */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"B",1) == 0) abexit (2);
   if (eqscan(&item2,"VI",1))
      cryout(RK_P1, " Test 2 eqscan nonzero rc", RK_LN1, NULL);
   if (item2 != 12) {
      convrt("P1,' Test 2 fails, returning ',J0I6)", &item2, NULL);
      ++nerr; }
   else ++nok;

/* Test 3: Integer conversion, character type */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"C",1) == 0) abexit (3);
   if (eqscan(&item3,"IC",1))
      cryout(RK_P1, " Test 3 eqscan nonzero rc", RK_LN1, NULL);
   if (item3 != 13) {
      cryout(RK_P1, " Test 3 fails, returning >", RK_LN1,
         &item3, 1, "<", 1, NULL);
      ++nerr; }
   else ++nok;

/* Test 4: Integer conversion, short type, deliberate sign error */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"D",1) == 0) abexit (4);
   if (eqscan(&item4,"VIH",1))
      cryout(RK_P1, " Test 4 eqscan nonzero rc", RK_LN1, NULL);
   if (item4 != 0) {
      convrt("(P1,' Test 4 fails, returning ',J0IH6)",&item4,NULL);
      ++nerr; }
   else ++nok;

/* Test 5: Integer conversion, long type */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"E",1) == 0) abexit (5);
   if (eqscan(&item5,"VIL",1))
      cryout(RK_P1, " Test 5 eqscan nonzero rc", RK_LN1, NULL);
   if (item5 != 123456) {
      convrt("(P1,' Test 5 fails, returning ',J0IL10)", &item5, NULL);
      ++nerr; }
   else ++nok;

/* Test 6: Integer conversion, long type with scale 14 */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"F",1) == 0) abexit (6);
   if (eqscan(&item6,"B14IL",1))
      cryout(RK_P1, " Test 6 eqscan nonzero rc", RK_LN1, NULL);
   if (item6 != 20480) {
      convrt("(P1,' Test 6 fails, returning ',J0IL10)", &item6, NULL);
      ++nerr; }
   else ++nok;

/* Test 7: Unsigned integer conversion, default type */
   cdscan(card,2,16,RK_WDSKIP+RK_NOCONT);
   okmark(FALSE);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"B",1) == 0) abexit (7);
   if (eqscan(&item7,"U",1))
      cryout(RK_P1, " Test 7 eqscan nonzero rc", RK_LN1, NULL);
   if (item7 != 12) {
      convrt("(P1,' Test 7 fails, returning ',J0IL10)", &item7, NULL);
      ++nerr; }
   else ++nok;

/* Test 8: Unsigned integer conversion, character type */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"C",1) == 0) abexit (8);
   if (eqscan(&item8,"UC",1))
      cryout(RK_P1, " Test 8 eqscan nonzero rc", RK_LN1, NULL);
   if (item8 != 13) {
      convrt("(P1,' Test 8 fails, returning ',J0IC6)", &item8, NULL);
      ++nerr; }
   else ++nok;

/* Test 9: Unsigned integer conversion, short type, deliberate sign error */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"D",1) == 0) abexit (9);
   if (eqscan(&item9,"V>UH",1))
      cryout(RK_P1, " Test 9 eqscan nonzero rc", RK_LN1, NULL);
   if (item9 != 1) {
      convrt("(P1,' Test 9 fails, returning ',J0UH6)", &item9,NULL);
      ++nerr; }
   else ++nok;

/* Test 10: Unsigned integer conversion, long type */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"E",1) == 0) abexit (10);
   if (eqscan(&item10,"V>UL",1))
      cryout(RK_P1, " Test 10 eqscan nonzero rc", RK_LN1, NULL);
   if (item10 != 123456UL) {
      convrt("(P1,' Test 10 fails, returning ',J0UL10)",
         &item10, NULL);
      ++nerr; }
   else ++nok;

/* Test 11: Unsigned integer conversion, long type with scale 14 */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"F",1) == 0) abexit (11);
   if (eqscan(&item11,"B14UL",1))
      cryout(RK_P1, " Test 11 eqscan nonzero rc", RK_LN1, NULL);
   if (item11 != 20480) {
      convrt("(P1,' Test 11 fails, returning ',J0UL10)",
         &item11, NULL);
      ++nerr; }
   else ++nok;

/* Test 12: Float conversion */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"G",1) == 0) abexit (12);
   if (eqscan(&item12,"F",1))
      cryout(RK_P1, " Test 12 eqscan nonzero rc", RK_LN1, NULL);
   if (fabs(item12 - 2.4) > 1.0E-5) {
      convrt("(P1,' Test 12 fails, returning ',J0E10.5)",
         &item12, NULL);
      ++nerr; }
   else ++nok;

/* Test 13: Float conversion */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"H",1) == 0) abexit (13);
   if (eqscan(&item13,"D",1))
      cryout(RK_P1, " Test 13 eqscan nonzero rc", RK_LN1, NULL);
   if (fabs(item13 - 189.75632) > 1.0E-12) {
      convrt("(P1,' Test 13 fails, returning ',J0D10.5)",
         &item13, NULL);
      ++nerr; }
   else ++nok;

/* Now proceed to first kwscan test--signed integers.
*  Note:  valck codes are tested thoroughly in eqsctest,
*     so here we just need to check the calls from kwscan  */

   cdprnt(card);
   cdscan(card,0,DFLT_MAXLEN,RK_NOCONT);
   okmark(FALSE);
   strcpy(item1,"??????Z");
   item2 = 0;
   item3 = 0;
   item4 = 0;
   item5 = 0;
   item6 = 0;
   item12 = 0;
   item13 = 0;
   ic = 0;
   icrc = kwscan(&ic,"A%A6",item1,"B%V<11I",&item2,"C%IC",&item3,
      "D%V>=-1300IH",&item4,"E%VIL",&item5,"F%B14IL",&item6,
      "G%F",&item12,"H%Q",&item13,NULL);
   convrt("(P1,' kwscan of card 1 gave ic = ',@Z8,', rc = ',J0I8)",
      &ic,&icrc,NULL);
#if BYTE_ORDRE > 0
   icc = 0xFF000000;
#else
   icc = 0x000000FF;
#endif
   if (ic == icc)
      cryout(RK_P1, " IC value is OK", RK_LN1, NULL);
   else
      convrt("(P1,' IC value should be ',@Z8)", &icc, NULL);
   if (strcmp(item1,"ABCDEFZ")) {
      cryout(RK_P1, " Test 14 fails, returning >", RK_LN1,
         item1, 7, "<", 1, NULL);
      ++nerr; }
   else ++nok;
   if (item2 != 10) {
      convrt("P1,' Test 15 fails, returning ',J0I6)", &item2, NULL);
      ++nerr; }
   else ++nok;
   if (item3 != 13) {
      cryout(RK_P1, " Test 16 fails, returning >", RK_LN1,
         &item3, 1, "<", 1, NULL);
      ++nerr; }
   else ++nok;
   if (item4 != -1300) {
      convrt("(P1,' Test 17 fails, returning ',J0IH6)", &item4,NULL);
      ++nerr; }
   else ++nok;
   if (item5 != 123456) {
      convrt("(P1,' Test 18 fails, returning ',J0IL10)", &item5, NULL);
      ++nerr; }
   else ++nok;
   if (item6 != 20480) {
      convrt("(P1,' Test 19 fails, returning ',J0IL10)", &item6, NULL);
      ++nerr; }
   else ++nok;
   if (fabs(item12 - 2.4) > 1.0E-5) {
      convrt("(P1,' Test 20 fails, returning ',J0E10.5)",
         &item12, NULL);
      ++nerr; }
   else ++nok;
   if (fabs(item13 - 189.75632) > 1.0E-12) {
      convrt("(P1,' Test 21 fails, returning ',J0D10.5)",
         &item13, NULL);
      ++nerr; }
   else ++nok;

/* Repeat all kwscan tests with unsigned integer types */

   cdprnt(card);
   cdscan(card,0,DFLT_MAXLEN,RK_NOCONT);
   okmark(FALSE);
   strcpy(item1,"??????Z");
   item7 = 0;
   item8 = 0;
   item9 = 0;
   item10 = 0;
   item11 = 0;
   item12 = 0;
   item13 = 0;
   ic = 0;
   icrc = kwscan(&ic,"A%A6",item1,"B%U",&item7,"C%UC",&item8,
      "D%V>UH",&item9,"E%V<=1000UL",&item10,"F%B14UL",&item11,
      "G%F",&item12,"H%Q",&item13,NULL);
   convrt("(P1,' kwscan of card 1 gave ic = ',@Z8,', rc = ',J0I8)",
      &ic,&icrc,NULL);
   if (ic == icc)
      cryout(RK_P1, " IC value is OK", RK_LN1, NULL);
   else
      convrt("(P1,' IC value should be ',@Z8)", &icc, NULL);
   if (strcmp(item1,"ABCDEFZ")) {
      cryout(RK_P1, " Test 22 fails, returning >", RK_LN1,
         item1, 7, "<", 1, NULL);
      ++nerr; }
   else ++nok;
   if (item7 != 12) {
      convrt("(P1,' Test 23 fails, returning ',J0IL10)", &item7, NULL);
      ++nerr; }
   else ++nok;
   if (item8 != 13) {
      convrt("(P1,' Test 24 fails, returning ',J0IC6)", &item8, NULL);
      ++nerr; }
   else ++nok;
   if (item9 != 1) {
      convrt("(P1,' Test 25 fails, returning ',J0UH6)", &item9,NULL);
      ++nerr; }
   else ++nok;
   if (item10 != 1000) {
      convrt("(P1,' Test 26 fails, returning ',J0UL10)",
         &item10, NULL);
      ++nerr; }
   else ++nok;
   if (item11 != 20480) {
      convrt("(P1,' Test 27 fails, returning ',J0UL10)",
         &item11, NULL);
      ++nerr; }
   else ++nok;
   if (fabs(item12 - 2.4) > 1.0E-5) {
      convrt("(P1,' Test 28 fails, returning ',J0E10.5)",
         &item12, NULL);
      ++nerr; }
   else ++nok;
   if (fabs(item13 - 189.75632) > 1.0E-12) {
      convrt("(P1,' Test 29 fails, returning ',J0D10.5)",
         &item13, NULL);
      ++nerr; }
   else ++nok;

/* Now test the unusual codes and errors */
   cryout(RK_P1,"0It is correct to get invalid numeric value"
      " and too long flags on each card above.",RK_LN2,NULL);
   cryout(RK_P1,"0Begin test of unusual codes and errors...",
      RK_LN2,NULL);
   cdprnt(card2);
   cdscan(card2,0,DFLT_MAXLEN,RK_NOCONT);
   okmark(FALSE);
   item2 = 0;
   item4 = 0;
   item5 = 0;
   strcpy(item14,"QQQ");
   item15 = 0x20a0;
   item16 = item17 = 0;
   iorv1 = 0x0f0f;
   ic = 0;
   kwsreg((void (*)(void *))kwnfn, 0);
   while ((icrc = kwscan(&ic,"AMBIG%I",&item2,"AMBUG%IH",&item4,
         "VAL1%S33L",&item5,"X1%X","X2%X","NAME%N",item14,"OPTION%O",
         &item15,&iorv1,"INC1%RI", TRUE, &item16, "INC2%RI", FALSE,
         &item17, NULL)) != 0) {
      convrt("(P1,' kwscan of card 2 gave ic = ',@Z8,', rc = ',J0I8)",
         &ic,&icrc,NULL);
      switch (icrc) {
      case 1 :
         cryout(RK_P1, " X1 exit reached", RK_LN1, NULL);
         break;
      case 2 :
         cryout(RK_P1, " X2 exit reached", RK_LN1, NULL);
         break;
         } /* End switch */
      } /* End while kwscan loop */
   convrt("(P1,' kwscan of card 2 gave ic = ',@Z8,', rc = ',J0I8)",
      &ic,&icrc,NULL);
#if BYTE_ORDRE > 0
   icc = 0x3F000000;
#else
   icc = 0x0000003F;
#endif
   if (ic == icc)
      cryout(RK_P1, " IC value is OK", RK_LN1, NULL);
   else
      convrt("(P1,' IC value should be ',@Z8)", &icc, NULL);
   if (item5 != 33) {
      convrt("(P1,' Set value test 30 fails, "
         "returning ',J0IL8)", &item5,NULL);
      ++nerr; }
   else ++nok;
   if (strcmp(item14,"GNR")) {
      convrt("(P1,' Name value test 31 "
         "fails, returning ',J0A4)",&item14,NULL);
      ++nerr; }
   else ++nok;
   if (item15 != 0x2faf) {
      convrt("(P1,' OR value test 32 fails,"
         " returning ',J0Z8)",&item15,NULL);
      ++nerr; }
   else ++nok;
   if (item16 != 5) {
      convrt("(P1,' Prereq test 33 fails,"
         " returning ',J0I8)",&item16,NULL);
      ++nerr; }
   else ++nok;
   if (item17 != 0) {
      convrt("(P1,' Prereq test 34 fails,"
         " returning ',J0I8)",&item17,NULL);
      ++nerr; }
   else ++nok;
   cryout(RK_P1,"0card2 test completed, should have given:",RK_LN2,
      "    Ambiguous key message for AMB",RK_LN1,
      "    Unrecognized key message for XYZ",RK_LN1,
      "    X1 and X2 exits reached",RK_LN1,
      "    Incompatible option for INC2.",RK_LN1+RK_FLUSH,NULL);

/* Test key and value arithmetic */
   cryout(RK_P1,"0Begin test of key updating, value adjustment, "
      "64-bit and wseed input.", RK_LN2, NULL);
   cdprnt(card3);
   cdscan(card3,0,DFLT_MAXLEN,RK_NOCONT);
   okmark(FALSE);
   item2 = 0;
   item4 = 0;
   item15 = 0;
   item18 = item19 = jesl(0);
   svvadj(0.015,"Vr");
   kwjreg((void (*)(char *, void *))kwjfn, 2);
   ic = 0;
   icrc = kwscan(&ic,
      "IVAL%J23", &item2,
      "KEY%KHAB", &item4,
      "X%B20I$-V", &item15,
      "SET64%SW65536", &item19,
      "BIG64X%B4IW", &item18,
      NULL);
   convrt("(P1,' kwscan of card 3 gave ic = ',@Z8,', rc = ',J0I8)",
      &ic,&icrc,NULL);
#if BYTE_ORDRE > 0
   icc = 0xF8000000;
#else
   icc = 0x000000F8;
#endif
   if (ic == icc)
      cryout(RK_P1, " IC value is OK", RK_LN1, NULL);
   else
      convrt("(P1,' IC value should be ',@Z8)", &icc, NULL);
   if (item4 != 3) {
      convrt("(P1,' Update key (+=) test 35 fails, "
         "returning ',J0IH8,', should be 3')", &item4, NULL);
      ++nerr; }
   else ++nok;
   if (item15 != 30199) {
      convrt("(P1,' Value adjustment test 36 "
         "fails, returning ',J0I8,', should be 30199')", &item15, NULL);
      ++nerr; }
   else ++nok;
   if (item2 != 9) {
      convrt("(P1,' J Regfn test 37 fails, "
         "returning ',J0I8,', should be 9')", &item2, NULL);
      ++nerr; }
   else ++nok;
   true1819 = jcsw(25,1249817600);
   if (qsw(jrsw(item18, true1819))) {
      convrt("(P1, ' 64-bit input test 38 fails, returning ',"
         "2@Z8,', should be ',2@Z8)", &item18, &true1819, NULL);
      ++nerr; }
   else ++nok;
   true1819 = jcsw(0,65536);
   if (qsw(jrsw(item19, true1819))) {
      convrt("(P1, ' 64-bit set test 39 fails, returning ',"
         "2@Z8,', should be ',2@Z8)", &item19, &true1819, NULL);
      ++nerr; }
   else ++nok;

/* Test wseed input.
*  (kwscan calls eqscan, no no separate eqscan test here.)  */
   cdprnt(card4);
   cdscan(card4,0,DFLT_MAXLEN,RK_NOCONT);
   okmark(FALSE);
   item20.seed27 = item20.seed31 = 1;
   item21.seed27 = item21.seed31 = 2;
   item22.seed27 = item22.seed31 = 3;
   ic = 0;
   icrc = kwscan(&ic,
      "R1%IS", &item20,
      "R2%IS", &item21,
      "R3%IS", &item22,
      NULL);
   convrt("(P1,' kwscan of card 4 gave ic = ',@Z8,', rc = ',J0I8)",
      &ic,&icrc,NULL);
#if BYTE_ORDRE > 0
   icc = 0xE0000000;
#else
   icc = 0x000000E0;
#endif
   if (ic == icc)
      cryout(RK_P1, " IC value is OK", RK_LN1, NULL);
   else
      convrt("(P1,' IC value should be ',@Z8)", &icc, NULL);
   if (item20.seed27 != -1 || item20.seed31 != 1009) {
      convrt("(P1,' wseed test 40 fails, "
      "returning ',IS24, ' should be (-1,1009)')", &item20, NULL);
      ++nerr; }
   else ++nok;
   if (item21.seed27 != 34567 || item21.seed31 != 2106798003) {
      convrt("(P1,' wseed test 41 fails, "
      "returning ',IS24, ' should be (34567,2106798003)')", &item21,
      NULL);
      ++nerr; }
   else ++nok;
   cryout(RK_P1, " R3 should give 'too big' error.",
      RK_LN1, NULL);
   if (item22.seed27 != -1 || item22.seed31 != 0x7ffffffe) {
      convrt("(P1,' wseed test 42 fails, "
      "returning ',IS24, ' should be (-1,2147483647)')", &item22,
      NULL);
      ++nerr; }
   else ++nok;

/* Test on/off option setting */
   cdprnt(card5);
   cdscan(card5,0,DFLT_MAXLEN,RK_NOCONT);
   okmark(FALSE);
   item9 = 0x1831, iorh1 = 0x2000, iorh2 = 0x0010, iorh3 = 0x0008;
   item15 = 0x2714, iorv1 = 0x0300, iorv2 = 0x0020;
   ic = 0;
   icrc = kwscan(&ic,
      "OPT2%O", &item15, &iorv1,
      "OPT3%OH", &item9, &iorh1,
      "OPT4%OH", &item9, &iorh2,
      "OPT1%OH", &item9, &iorh3,
      "OPT5%O", &item15, &iorv2,
      NULL);
   convrt("(P1,' kwscan of card 5 gave ic = ',@Z8,', rc = ',J0I8)",
      &ic,&icrc,NULL);
#if BYTE_ORDRE > 0
   icc = 0xF8000000;
#else
   icc = 0x000000F8;
#endif
   if (ic == icc)
      cryout(RK_P1, " IC value is OK", RK_LN1, NULL);
   else
      convrt("(P1,' IC value should be ',@Z8)", &icc, NULL);
   iorh1 = 0x3829;
   if (item9 != iorh1) {
      convrt("(P1,' flag test 43 fails, "
      "returning ',ZH4,' should be 3829')", &item9, NULL);
      ++nerr; }
   else ++nok;
   iorv1 = 0x2434;
   if (item15 != iorv1) {
      convrt("(P1,' flag test 44 fails, "
      "returning ',Z4,' should be 2434')", &item15, NULL);
      ++nerr; }
   else ++nok;

/* Repeat on/off option setting tests with reverse action */
   cdprnt(card5);
   cdscan(card5,0,DFLT_MAXLEN,RK_NOCONT);
   okmark(FALSE);
   item9 = 0x1831, iorh1 = 0x0800, iorh2 = 0x0004, iorh3 = 0x0020;
   item15 = 0x2714, iorv1 = 0x0060, iorv2 = 0x0600;
   ic = 0;
   icrc = kwscan(&ic,
      "OPT2%O~", &item15, &iorv1,
      "OPT3%OH~", &item9, &iorh1,
      "OPT4%OH~", &item9, &iorh2,
      "OPT1%OH~", &item9, &iorh3,
      "OPT5%O~", &item15, &iorv2,
      NULL);
   convrt("(P1,' kwscan of card 5 gave ic = ',@Z8,', rc = ',J0I8)",
      &ic,&icrc,NULL);
#if BYTE_ORDRE > 0
   icc = 0xF8000000;
#else
   icc = 0x000000F8;
#endif
   if (ic == icc)
      cryout(RK_P1, " IC value is OK", RK_LN1, NULL);
   else
      convrt("(P1,' IC value should be ',@Z8)", &icc, NULL);
   iorh1 = 0x1015;
   if (item9 != iorh1) {
      convrt("(P1,' flag test 45 fails, "
      "returning ',ZH4,' should be 1015')", &item9, NULL);
      ++nerr; }
   else ++nok;
   iorv1 = 0x2174;
   if (item15 != iorv1) {
      convrt("(P1,' flag test 46 fails, "
      "returning ',Z4,' should be 2174')", &item15, NULL);
      ++nerr; }
   else ++nok;

/* Test input with qualified keys */
   cdprnt(card6);
   cdscan(card6,0,DFLT_MAXLEN,RK_NOCONT);
   okmark(FALSE);
   item2 = item15 = item16 = item17 = -77;
   ic = 0;
   icrc = kwscan(&ic, "ZZ.ITJ1%I", &item2, "ITJ2%I", &item15,
      "IT.ITJ5%I", &item17, "XY.ITJ3%I", &item16, NULL);
   convrt("(P1,' kwscan of card 6 gave ic = ',@Z8,', cc = ',J0I8)",
      &ic,&icrc,NULL);
#if BYTE_ORDRE > 0
   icc = 0xD0000000;
#else
   icc = 0x000000D0;
#endif
   if (ic == icc)
      cryout(RK_P1, " IC value is OK", RK_LN1, NULL);
   else
      convrt("(P1,' IC value should be ',@Z8)", &icc, NULL);
   if (item2 != 1) {
      convrt("P1, ' Test 47 fails, returning ',J0I6)", &item2, NULL);
      ++nerr; }
   else ++nok;
   if (item15 != 2) {
      convrt("P1, ' Test 48 fails, returning ',J0I6)", &item15, NULL);
      ++nerr; }
   else ++nok;
   if (item16 != 3) {
      convrt("P1, ' Test 49 fails, returning ',J0I6)", &item16, NULL);
      ++nerr; }
   else ++nok;
   if (item17 != -77) {
      convrt("P1, ' Test 50 fails, returning ',J0I6)", &item17, NULL);
      ++nerr; }
   else ++nok;
   cryout(RK_P1,"0card6 test completed, should have given:",RK_LN2,
      "    Unrecognized key message for IT.ITJ4",RK_LN1,NULL);


   convrt("(P1,' End kwscan/eqscan tests, '"
      ",J0I6,' correct, ',J0I6,' failed')", &nok, &nerr, NULL);
   cryout(RK_P1," -->cryout flushed.",RK_LN1+RK_FLUSH,NULL);
   return 0;
   } /* End of program */
