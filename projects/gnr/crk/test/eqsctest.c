/*--------------------------------------------------------------------*/
/*                                                                    */
/*    EQSCAN test                                                     */
/*                                                                    */
/*--------------------------------------------------------------------*/

/*
   This test executes eqscan with each possible calling code.
   Rev, 05/30/96, GNR - bcdin now returns 0 when QPOS test fails.
   Rev, 06/03/97, GNR - Test 9, expect return 1 when V> test fails.
   Rev, 08/23/01, GNR - Add test for text type code (T), errors.
   Rev, 10/18/01, GNR - Add test for four flavors of keycodes.
   Rev, 02/01/03, GNR - Add tests for metric unit specs.
   Rev, 02/16/03, GNR - Add test for alternative scales.
   Rev, 12/31/07, GNR - Add various flavors of valck tests.
   Rev, 06/13/09, GNR - Add test 21j
*/

#define MAIN
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

int main() {

   char card[] = {"A=ABCDEFG,B=12,C=13,D=-1329,E=123456,F=1.25,"
      "G=2.4,H=189.75632 "};
   char keycd[] = {"K1=AC,K2=+B,K3=-C,K4=0"};
   char metcd[] = {"M1=30mV,M2=30,M3=250jV,M4=250mQ,M5=30mm2,"
      "M6=8.2cm/sec2,M7=30V,M8=0.125/mV,M9=0.125,M10=20mV"};
   char lowcd[] = {"ALOW=abcde12Z"};
   char lea[17];
   char item1[10] = {"??????Z"};
   int item2;
   schr item3;
   short item4;
   long item5;
   long item6;
   unsigned int item7;
   byte item8;
   unsigned short item9;
   unsigned long item10;
   unsigned long item11;
   float item12;
   double item13;

   cdscan(card,0,16,0);
   okmark(FALSE);

/* Test 1: Alphanumeric field--make sure code length not exceeded */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"A",1) == 0) exit (1);
   if (eqscan(item1,"A6",RK_EQCHK))
      cryout(RK_P1, " Test 1 eqscan nonzero rc", RK_LN1, NULL);
   if (strcmp(item1,"ABCDEFZ"))
      cryout(RK_P1, "0***Test 1 fails, item1 == ", RK_LN2,
         item1, RK_CCL+8, NULL);
   else cryout(RK_P1, " Test 1 OK", RK_LN1, NULL);

/* Test 2: Integer conversion, default type */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"B",1) == 0) exit (2);
   if (eqscan(&item2,"VI",RK_EQCHK))
      cryout(RK_P1, " Test 2 eqscan nonzero rc", RK_LN1, NULL);
   if (item2 != 12)
      convrt("(P1,'0***Test 2 fails, item2 == ',J0I8)",
         &item2, NULL);
   else cryout(RK_P1, " Test 2 OK", RK_LN1, NULL);
   /* Test 2a:  Warn & replace against minimum */
   scanagn();
   if (eqscan(&item2,"W>18I",RK_EQCHK))
      cryout(RK_P1, " Test 2a eqscan nonzero rc", RK_LN1, NULL);
   if (item2 != 19)
      convrt("(P1,'0***Test 2a fails, item2 == ',J0I8)",
         &item2, NULL);
   else cryout(RK_P1, " Test 2a OK", RK_LN1, NULL);
   /* Test 2b:  Warn & replace against maximum */
   scanagn();
   if (eqscan(&item2,"V<=6I",RK_EQCHK))
      cryout(RK_P1, " Test 2b eqscan nonzero rc", RK_LN1, NULL);
   if (item2 != 6)
      convrt("(P1,'0***Test 2b fails, item2 == ',J0I8)",
         &item2, NULL);
   else cryout(RK_P1, " Test 2b OK", RK_LN1, NULL);

/* Test 3: Integer conversion, character type */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"C",1) == 0) exit (3);
   if (eqscan(&item3,"IC",RK_EQCHK))
      cryout(RK_P1, " Test 3 eqscan nonzero rc", RK_LN1, NULL);
   if (item3 != 13)
      convrt("(P1,'0***Test 3 fails, item3 == ',J0IC8)",
         &item3, NULL);
   else cryout(RK_P1, " Test 3 OK", RK_LN1, NULL);
   /* Test 3a:  Warn & replace against minimum */
   scanagn();
   if (eqscan(&item3,"V>=18IC",RK_EQCHK))
      cryout(RK_P1, " Test 3a eqscan nonzero rc", RK_LN1, NULL);
   if (item3 != 18)
      convrt("(P1,'0***Test 3a fails, item3 == ',J0IC8)",
         &item3, NULL);
   else cryout(RK_P1, " Test 3a OK", RK_LN1, NULL);
   /* Test 3b:  Warn & replace against maximum */
   scanagn();
   if (eqscan(&item3,"V<6IC",RK_EQCHK))
      cryout(RK_P1, " Test 3b eqscan nonzero rc", RK_LN1, NULL);
   if (item3 != 5)
      convrt("(P1,'0***Test 3b fails, item3 == ',J0IC8)",
         &item3, NULL);
   else cryout(RK_P1, " Test 3b OK", RK_LN1, NULL);

/* Test 4: Integer conversion, short type, deliberate sign error */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"D",1) == 0) exit (4);
   if (eqscan(&item4,"VIH",RK_EQCHK))
      cryout(RK_P1, " Test 4 eqscan nonzero rc", RK_LN1, NULL);
   if (item4 != 0)
      convrt("(P1,'0***Test 4 fails, item4 == ',J0IH8)",
         &item4, NULL);
   else cryout(RK_P1, " Test 4 OK", RK_LN1, NULL);

/* Test 5: Integer conversion, long type */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"E",1) == 0) exit (5);
   if (eqscan(&item5,"VIL",RK_EQCHK))
      cryout(RK_P1, " Test 5 eqscan nonzero rc", RK_LN1, NULL);
   if (item5 != 123456)
      convrt("(P1,'0***Test 5 fails, item5 == ',J0IL8)",
         &item5, NULL);
   else cryout(RK_P1, " Test 5 OK", RK_LN1, NULL);

/* Test 6: Integer conversion, long type with scale 14 */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"F",1) == 0) exit (6);
   if (eqscan(&item6,"B14IL",RK_EQCHK))
      cryout(RK_P1, " Test 6 eqscan nonzero rc", RK_LN1, NULL);
   if (item6 != 20480)
      convrt("(P1,'0***Test 6 fails, item6 == ',J0B14IL10.5)",
         &item6, NULL);
   else cryout(RK_P1, " Test 6 OK", RK_LN1, NULL);
   /* Test 6a:  Warn & replace against minimum */
   scanagn();
   if (eqscan(&item6,"V>=2.2B14IL",RK_EQCHK))
      cryout(RK_P1, " Test 6a eqscan nonzero rc", RK_LN1, NULL);
   if (item6 != 36045)
      convrt("(P1,'0***Test 6a fails, item6 == ',J0B14IL10.5)",
         &item6, NULL);
   else cryout(RK_P1, " Test 6a OK", RK_LN1, NULL);
   /* Test 6b:  Warn & replace against maximum */
   scanagn();
   if (eqscan(&item6,"V<6B14IL",RK_EQCHK))
      cryout(RK_P1, " Test 6b eqscan nonzero rc", RK_LN1, NULL);
   if (item6 != 20480)
      convrt("(P1,'0***Test 6b fails, item6 == ',J0B14IL10.5)",
         &item6, NULL);
   else cryout(RK_P1, " Test 6b OK", RK_LN1, NULL);

/* Test 7: Unsigned integer conversion, default type */
   cdscan(card,2,16,RK_WDSKIP);
   okmark(FALSE);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"B",1) == 0) exit (7);
   if (eqscan(&item7,"V>1U",RK_EQCHK))
      cryout(RK_P1, " Test 7 eqscan nonzero rc", RK_LN1, NULL);
   if (item7 != 12)
      convrt("(P1,'0***Test 7 fails, item7 == ',J0UL10)",
         &item7, NULL);
   else cryout(RK_P1, " Test 7 OK", RK_LN1, NULL);

/* Test 8: Unsigned integer conversion, character type */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"C",1) == 0) exit (8);
   if (eqscan(&item8,"UC",RK_EQCHK))
      cryout(RK_P1, " Test 8 eqscan nonzero rc", RK_LN1, NULL);
   if (item8 != 13)
      convrt("(P1,'0***Test 8 fails, item8 == ',J0UC4)",
         &item8, NULL);
   else cryout(RK_P1, " Test 8 OK", RK_LN1, NULL);

/* Test 9: Unsigned integer conversion, short type */
   /* Deliberate sign error */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"D",1) == 0) exit (9);
   if (eqscan(&item9,"V>UH",RK_EQCHK))
      cryout(RK_P1, " Test 9 eqscan nonzero rc", RK_LN1, NULL);
   if (item9 != 1)
      convrt("(P1,'0***Test 9 fails, item9 == ',J0UH10)",
         &item9, NULL);
   else cryout(RK_P1, " Test 9 OK", RK_LN1, NULL);
   /* Test 9a:  Warn & replace against maximum */
   scanagn();
   if (eqscan(&item9,"W<6IH",RK_EQCHK))
      cryout(RK_P1, " Test 9a eqscan nonzero rc", RK_LN1, NULL);
   if (item9 != 0)
      convrt("(P1,'0***Test 9a fails, item9 == ',J0UH10)",
         &item9, NULL);
   else cryout(RK_P1, " Test 9a OK", RK_LN1, NULL);

/* Test 10: Unsigned integer conversion, long type */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"E",1) == 0) exit (10);
   if (eqscan(&item10,"UL",RK_EQCHK))
      cryout(RK_P1, " Test 10 eqscan nonzero rc", RK_LN1, NULL);
   if (item10 != 123456UL)
      convrt("(P1,'0***Test 10 fails, item10 == ',J0UL10)",
         &item10, NULL);
   else cryout(RK_P1, " Test 10 OK", RK_LN1, NULL);

/* Test 11: Unsigned integer conversion, long type with scale 14 */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"F",1) == 0) exit (11);
   if (eqscan(&item11,"B14UL",RK_EQCHK))
      cryout(RK_P1, " Test 11 eqscan nonzero rc", RK_LN1, NULL);
   if (item11 != 20480)
      convrt("(P1,'0***Test 11 fails, item11 == ',J0B14UL10)",
         &item11, NULL);
   else cryout(RK_P1, " Test 11 OK", RK_LN1, NULL);

/* Test 12: Float conversion */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"G",1) == 0) exit (12);
   if (eqscan(&item12,"F",RK_EQCHK))
      cryout(RK_P1, " Test 12 eqscan nonzero rc", RK_LN1, NULL);
   if (fabs(item12 - 2.4) > 1.0E-5)
      convrt("(P1,'0***Test 12 fails, item12 == ',J0F10.A)",
         &item12, NULL);
   else cryout(RK_P1, " Test 12 OK", RK_LN1, NULL);
   /* Test 12a:  Try a range */
   scanagn();
   if (eqscan(&item12,"W>2.5678<3F",RK_EQCHK))
      cryout(RK_P1, " Test 12a eqscan nonzero rc", RK_LN1, NULL);
   if (fabs(item12 - 2.5678) > 0.0001)
      convrt("(P1,'0***Test 12a fails, item12 == ',J0F10.A)",
         &item12, NULL);
   else cryout(RK_P1, " Test 12a OK", RK_LN1, NULL);

/* Test 13: Double float conversion */
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"H",1) == 0) exit (13);
   if (eqscan(&item13,"D",RK_EQCHK))
      cryout(RK_P1, " Test 13 eqscan nonzero rc", RK_LN1, NULL);
   if (fabs(item13 - 189.75632) > 1.0E-12)
      convrt("(P1,'0***Test 13 fails, item13 == ',J0D10.A)",
         &item13, NULL);
   else cryout(RK_P1, " Test 13 OK", RK_LN1, NULL);
   /* Test 13a:  Check out the V~ test */
   scanagn();
   if (eqscan(&item13,"W~Q",RK_EQCHK))
      cryout(RK_P1, " Test 13a eqscan nonzero rc", RK_LN1, NULL);
   if (fabs(item13 - 189.75632) > 1.0E-12)
      convrt("(P1,'0***Test 13a fails, item13 == ',J0D10.A)",
         &item13, NULL);
   else cryout(RK_P1, " Test 13a OK", RK_LN1, NULL);

/* Test 14: Text conversion */
   cdscan(card,1,16,RK_WDSKIP);
   okmark(FALSE);
   if (eqscan(&item7,"T40",RK_EQCHK))
      cryout(RK_P1, " Test 14 eqscan nonzero rc", RK_LN1, NULL);
   if (findtxt("ABCDEFG") != item7)
      cryout(RK_P1, " Test 14 Fails", RK_LN1, NULL);
   else cryout(RK_P1, " Test 14 OK", RK_LN1, NULL);

/* Test 15: Byte keycode conversion */
   item2 = 0;     /* Just used as an error flag here */
   cdscan(keycd,0,16,RK_WDSKIP);
   okmark(FALSE);
   item8 = 0;
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K1",2) == 0) exit(15);
   if (eqscan(&item8,"KCABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 15a eqscan nonzero rc", RK_LN1, NULL);
   if (item8 != 10) item2=1,
      convrt("(P1,'0***Test 15a fails, returning ',J0UC4)",
         &item8, NULL);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K2",2) == 0) exit(15);
   if (eqscan(&item8,"KCABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 15b eqscan nonzero rc", RK_LN1, NULL);
   if (item8 != 14) item2=1,
      convrt("(P1,'0***Test 15b fails, returning ',J0UC4)",
         &item8, NULL);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K3",2) == 0) exit(15);
   if (eqscan(&item8,"KCABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 15c eqscan nonzero rc", RK_LN1, NULL);
   if (item8 != 12) item2=1,
      convrt("(P1,'0***Test 15c fails, returning ',J0UC4)",
         &item8, NULL);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K4",2) == 0) exit(15);
   if (eqscan(&item8,"KCABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 15d eqscan nonzero rc", RK_LN1, NULL);
   if (item8 != 0) item2=1,
      convrt("(P1,'0***Test 15d fails, returning ',J0UC4)",
         &item8, NULL);
   if (!item2)
      cryout(RK_P1, " Test 15 OK", RK_LN1, NULL);

/* Test 16: Unsigned short keycode conversion */
   item2 = 0;     /* Just used as an error flag here */
   cdscan(keycd,0,16,RK_WDSKIP);
   okmark(FALSE);
   item9 = 0;
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K1",2) == 0) exit(16);
   if (eqscan(&item9,"KHABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 16a eqscan nonzero rc", RK_LN1, NULL);
   if (item9 != 10) item2=1,
      convrt("(P1,'0***Test 16a fails, returning ',J0UH8)",
         &item9, NULL);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K2",2) == 0) exit(16);
   if (eqscan(&item9,"KHABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 16b eqscan nonzero rc", RK_LN1, NULL);
   if (item9 != 14) item2=1,
      convrt("(P1,'0***Test 16b fails, returning ',J0UH8)",
         &item9, NULL);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K3",2) == 0) exit(16);
   if (eqscan(&item9,"KHABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 16c eqscan nonzero rc", RK_LN1, NULL);
   if (item9 != 12) item2=1,
      convrt("(P1,'0***Test 16c fails, returning ',J0UH8)",
         &item9, NULL);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K4",2) == 0) exit(16);
   if (eqscan(&item9,"KHABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 16d eqscan nonzero rc", RK_LN1, NULL);
   if (item9 != 0) item2=1,
      convrt("(P1,'0***Test 16d fails, returning ',J0UH8)",
         &item9, NULL);
   if (!item2)
      cryout(RK_P1, " Test 16 OK", RK_LN1, NULL);

/* Test 17: Unsigned int keycode conversion */
   item2 = 0;     /* Just used as an error flag here */
   cdscan(keycd,0,16,RK_WDSKIP);
   okmark(FALSE);
   item7 = 0;
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K1",2) == 0) exit(17);
   if (eqscan(&item7,"KIABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 17a eqscan nonzero rc", RK_LN1, NULL);
   if (item7 != 10) item2=1,
      convrt("(P1,'0***Test 17a fails, returning ',J0U8)",
         &item7, NULL);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K2",2) == 0) exit(17);
   if (eqscan(&item7,"KIABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 17b eqscan nonzero rc", RK_LN1, NULL);
   if (item7 != 14) item2=1,
      convrt("(P1,'0***Test 17b fails, returning ',J0U8)",
         &item7, NULL);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K3",2) == 0) exit(17);
   if (eqscan(&item7,"KIABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 17c eqscan nonzero rc", RK_LN1, NULL);
   if (item7 != 12) item2=1,
      convrt("(P1,'0***Test 17c fails, returning ',J0U8)",
         &item7, NULL);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K4",2) == 0) exit(17);
   if (eqscan(&item7,"KIABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 17d eqscan nonzero rc", RK_LN1, NULL);
   if (item7 != 0) item2=1,
      convrt("(P1,'0***Test 17d fails, returning ',J0U8)",
         &item7, NULL);
   if (!item2)
      cryout(RK_P1, " Test 17 OK", RK_LN1, NULL);

/* Test 18: Unsigned long keycode conversion */
   item2 = 0;     /* Just used as an error flag here */
   cdscan(keycd,0,16,RK_WDSKIP);
   okmark(FALSE);
   item10 = 0;
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K1",2) == 0) exit(18);
   if (eqscan(&item10,"KLABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 18a eqscan nonzero rc", RK_LN1, NULL);
   if (item10 != 10) item2=1,
      convrt("(P1,'0***Test 18a fails, returning ',J0UL10)",
         &item10, NULL);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K2",2) == 0) exit(18);
   if (eqscan(&item10,"KABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 18b eqscan nonzero rc", RK_LN1, NULL);
   if (item10 != 14) item2=1,
      convrt("(P1,'0***Test 18b fails, returning ',J0UL10)",
         &item10, NULL);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K3",2) == 0) exit(18);
   if (eqscan(&item10,"KABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 18c eqscan nonzero rc", RK_LN1, NULL);
   if (item10 != 12) item2=1,
      convrt("(P1,'0***Test 18c fails, returning ',J0UL10)",
         &item10, NULL);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"K4",2) == 0) exit(18);
   if (eqscan(&item10,"KLABCD",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 18d eqscan nonzero rc", RK_LN1, NULL);
   if (item10 != 0) item2=1,
      convrt("(P1,'0***Test 18d fails, returning ',J0UL10)",
         &item10, NULL);
   if (!item2)
      cryout(RK_P1, " Test 18 OK", RK_LN1, NULL);

/* Test 19: Deliberate error, checking for equals and not finding */
   lea[0] = 1;
   if (eqscan(lea,"A4",RK_EQCHK) == 0)
      cryout(RK_P1, "0***Test 19 fails--didn't detect equals "
         "sign missing", RK_LN2, NULL);
   else if (lea[0] != 1)
      cryout(RK_P1, "0***Test 19 fails--on = error, wrote "
         "into user data", RK_LN2, NULL);
   else cryout(RK_P1, " Test 19 OK", RK_LN1, NULL);

/* Test 20: Deliberate error: checking for in parens and isn't */
   if (eqscan(lea,"A4",RK_PNCHK) == 0)
      cryout(RK_P1, "0***Test 20 fails--didn't detect "
         "parens missing", RK_LN2, NULL);
   else if (lea[0] != 1)
      cryout(RK_P1, "0***Test 20 fails--on () error, wrote "
         "into user data", RK_LN2, NULL);
   else cryout(RK_P1, " Test 20 OK", RK_LN1, NULL);

/* Test 21: Metric units tests */
   item2 = 0;     /* Just used as an error flag here */
   cdscan(metcd,0,16,RK_WDSKIP);
   okmark(FALSE);
   item13 = 0.0;
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"M1",2) == 0) exit (21);
   if (eqscan(&item13,"D$-V",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 21a eqscan nonzero rc", RK_LN1, NULL);
   if (fabs(item13 - 0.03) > 1E-12) item2=1,
      convrt("(P1,'0***Test 21a fails, returning ',J0D10.A)",
         &item13, NULL);

   item12 = 0.0;
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"M2",2) == 0) exit (21);
   if (eqscan(&item12,"F.1$-V",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 21b eqscan nonzero rc", RK_LN1, NULL);
   if (fabs(item12 - 3.0) > 1E-12) item2=1,
      convrt("(P1,'0***Test 21b fails, returning ',J0F10.A)",
         &item12, NULL);

   item12 = 0.0;
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"M3",2) == 0) exit (21);
   eqscan(&item12,"F$-V",RK_EQCHK);
   if (!(RK.erscan & RK_MULTERR)) item2=1,
      cryout(RK_P1, "0***Test 21c fails; MULTERR not set.",
         RK_LN2, NULL);
   RK.erscan = 0;

   item12 = 0.0;
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"M4",2) == 0) exit (21);
   eqscan(&item12,"F$-V",RK_EQCHK);
   if (!(RK.erscan & RK_UNITERR)) item2=1,
      cryout(RK_P1, "0***Test 21d fails; UNITERR not set.",
         RK_LN2, NULL);
   RK.erscan = 0;

   item13 = 0.0;
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"M5",2) == 0) exit (21);
   if (eqscan(&item13,"D$cm2",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 21e eqscan nonzero rc", RK_LN1, NULL);
   if (fabs(item13 - 0.3) > 1E-12) item2=1,
      convrt("(P1,'0***Test 21e fails; returning ',J0D10.A)",
         &item13, NULL);

   item13 = 0.0;
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"M6",2) == 0) exit (21);
   if (eqscan(&item13,"D$-m/-sec2",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 21f eqscan nonzero rc", RK_LN1, NULL);
   if (fabs(item13 - 0.082) > 1E-12) item2=1,
      convrt("(P1,'0***Test 21f fails; item == ',J0D10.A)",
         &item13, NULL);

   item13 = 0.0;
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"M7",2) == 0) exit (21);
   if (eqscan(&item13,"D$mV",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 21g eqscan nonzero rc", RK_LN1, NULL);
   if (fabs(item13 - 30000.0) > 1E-12) item2=1,
      convrt("(P1,'0***Test 21g fails; item == ',J0D10.A)",
         &item13, NULL);

   item13 = 0.0;
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"M8",2) == 0) exit (21);
   if (eqscan(&item13,"D$-/mV",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 21h eqscan nonzero rc", RK_LN1, NULL);
   if (fabs(item13 - 0.125) > 1E-12) item2=1,
      convrt("(P1,'0***Test 21h fails; item == ',J0D10.A)",
         &item13, NULL);

   item13 = 0.0;
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"M9",2) == 0) exit (21);
   if (eqscan(&item13,"D$-/mV",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 21i eqscan nonzero rc", RK_LN1, NULL);
   if (fabs(item13 - 0.125) > 1E-12) item2=1,
      convrt("(P1,'0***Test 21i fails; item == ',J0D10.A)",
         &item13, NULL);

   item13 = 0.0;
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"M10",3) == 0) exit (21);
   if (eqscan(&item13,"D$mV",RK_EQCHK)) item2=1,
      cryout(RK_P1, " Test 21j eqscan nonzero rc", RK_LN1, NULL);
   if (fabs(item13 - 20.0) > 1E-12) item2=1,
      convrt("(P1,'0***Test 21j fails; item == ',J0D10.A)",
         &item13, NULL);

   if (!item2)
      cryout(RK_P1, " Test 21 OK", RK_LN1, NULL);

/* Test 22: Integer conversion, alternative scales */
   item2 = 0;     /* Just used as an error flag here */
   /* Test 22a--use normal scale */
   cdscan(card,10,16,RK_WDSKIP);
   okmark(FALSE);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"F",1) == 0) exit (22);
   if (eqscan(&item6,"B14/7IL",RK_EQCHK))
      cryout(RK_P1, " Test 22a eqscan nonzero rc", RK_LN1, NULL);
   if (item6 != 20480) item2=1,
      convrt("(P1,'0***Test 22a fails; item == ',J0B14/7IL10.A)",
         &item6, NULL);
   /* Test 22b--use compatibility scale */
   bscompat(RK_BSSLASH);
   cdscan(card,10,16,RK_WDSKIP);
   okmark(FALSE);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"F",1) == 0) exit (22);
   if (eqscan(&item6,"B14/7IL",RK_EQCHK))
      cryout(RK_P1, " Test 22b eqscan nonzero rc", RK_LN1, NULL);
   if (item6 != 160) item2=1,
      convrt("(P1,'0***Test 22b fails; item == ',J0B14/7IL10.A)",
         &item6, NULL);

   if (!item2)
      cryout(RK_P1, " Test 22 OK", RK_LN1, NULL);

/* Test 23: Alphanumeric field--converted to upper case */
   cdscan(lowcd,0,16,0);
   okmark(FALSE);
   scan(lea,RK_NEWKEY+RK_REQFLD);
   if (ssmatch(lea,"ALOW",1) == 0) exit (23);
   if (eqscan(item1,"VA8",RK_EQCHK))
      cryout(RK_P1, " Test 23 eqscan nonzero rc", RK_LN1, NULL);
   if (strcmp(item1,"ABCDE12Z"))
      cryout(RK_P1, "0***Test 23 fails; item1 == ", RK_LN2,
         item1, RK_CCL+10, NULL);
   else cryout(RK_P1, " Test 23 OK", RK_LN1, NULL);

   cryout(RK_P1,"\0",RK_LN0+RK_FLUSH+1,NULL);
   return 0;
   } /* End of program */

