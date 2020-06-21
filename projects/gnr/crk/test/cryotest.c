/*--------------------------------------------------------------------*/
/*                                                                    */
/*    CRYOUT, SPOUT, SETTIT, GETTIT, SETPID, GETDAT tests             */
/*                                                                    */
/* Rev, 06/04/94, GNR - Add spoutm() invocation from command line     */
/* Rev, 11/07/09, GNR - Remove spoutm(), more logical spout calls     */
/* Rev, 08/19/11, GNR - Add RK_PF calls                               */
/*--------------------------------------------------------------------*/

#define MAIN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdef.h"
#include "rocks.h"
#include "rkxtra.h"

int main(int argc, char *argv[]) {

   char result[80];
   static char *parms[2] = { "SPOUT", "NOSPOUT" };

   if (argc > 1) switch (smatch(0, argv[1], parms, 2)) {
      case 1: spout(SPTM_LCLON);
         break;
      case 2: spout(SPTM_GBLOFF);
         break;
      } /* End parameter processing */

/* Traditional cryout tests -- ansicc mode */

   setpid("CRYOTEST - cryout test");
   cryout(RK_P1,"0This is a line with a zero",RK_LN2,
      "+This is a line with an overstrike",RK_LN0,
      " This is a line with a blank",RK_LN1,
      "-This is a line with a minus",RK_LN3,NULL);
   RKC.dmsg = (char *)malloc(81);
   memset(RKC.dmsg,' ',80);
   *(RKC.dmsg+80) = '\0';
   *(RKC.dmsg+10) = '$';
   RK.erscan = 7; RKC.accpt |= ACCPT_ERP;
   okmark(ON);
   cryout(RK_P1,"0This should give a $ in col. 10",RK_LN2,NULL);
   cryout(RK_P1+1,"0This is a spouted line with a zero",RK_LN2,
      "+This is a spouted line with an overstrike",RK_LN0,
      " This is a spouted line with a blank",RK_LN1,
      "-This is a spouted line with a minus",RK_LN3,NULL);
   cryout(RK_P1,"Tack onto previous",RK_CCL,NULL);
   cryout(RK_P1,"0Line followed by flush",RK_LN2+RK_FLUSH,NULL);
   cryout(RK_P1,"Tack on previous now should fail",RK_CCL,NULL);
   cryout(RK_P1,"0Here is a line+",RK_LN2,NULL);
   cryout(RK_P1," followed by a correct continuation.",RK_CCL,NULL);
   cryout(RK_P1,"0This line should be skipped",RK_LN2+RK_SKIP,NULL);
   cryout(RK_P1,"0Only 10 characters should appear",RK_LN2+19,NULL);
   cryout(RK_P1,"0***Error message should spout.",RK_LN2,NULL);
   cryout(RK_P1,"0-->Warning should also spout.",RK_LN2,NULL);
   settit("TITLE THIS TITLE SHOULD BE TRANSMITTED TO THE OUTPUT");
   memcpy(result,getdat(),12);
   cryout(RK_P1,"0Date returned by getdat: ",RK_LN2,result,RK_CCL+12,
      NULL);
   memcpy(result,gettit(),60);
   cryout(RK_P1,"0Title returned by gettit: ",RK_LN2,result,
      RK_CCL+60,NULL);
   cryout(RK_P1,"0This is a subtitle",RK_SUBTTL+RK_LN2,
      " continued thus",RK_CCL,NULL);
   cryout(RK_P1," This should trigger a new page",RK_LN1,NULL);
   cryout(RK_P1+1," and this should trigger subtitle spout",RK_LN1,
      NULL);
   cryout(RK_P1,"This is an orphan subtitle",RK_SUBTTL+RK_CCL,NULL);
   cryout(RK_P1," This should trigger the orphan subtitle",
      RK_LN2,NULL);
   cryout(RK_P1," This is a filler",RK_LN1,NULL);
   cryout(RK_P1,"0This is a no-trigger subtitle",RK_NFSUBTTL+RK_LN2,
      " continued thus",RK_CCL,NULL);
   cryout(RK_P1," This should not trigger a new page",RK_LN1,NULL);
   cryout(RK_P1+1," and this should trigger subtitle spout",RK_LN1,
      NULL);
   cryout(RK_P1," This is a filler",RK_LN1,NULL);
   cryout(RK_P1," This should trigger a new page",RK_NEWPG,
      NULL);
   cryout(RK_P1," This is the last line, with flush",RK_LN1+RK_FLUSH,
      NULL);

/* Test intermixed traditional and RK_PF (printf controls) calls */

   cryout(RK_P1|RK_PF,"This is a line with two final LF\n\n",RK_LN2,
      "This is a line with no final LF",RK_LN1,
      "\rThis is a line with an overstrike\n",RK_LN0,
      "This is a line with three final LFs\n\n\n",RK_LN3,NULL);
   cryout(RK_P1," This is an ansicc line",RK_LN1,NULL);
   cryout(RK_P1|1|RK_PF,"This is a spouted line with two LF\n\n",RK_LN2,
      "This is a spouted line with no final LF",RK_LN1,
      "\rThis is a spouted line with an overstrike\n",RK_LN0,
      "This is a spouted line with three LFs, then xx\n\n\nxx",
      RK_LN3,NULL);
   cryout(RK_P1|RK_PF,"Tack onto previous\n",RK_CCL,NULL);
   cryout(RK_P1|RK_PF,"Line followed by flush\n",RK_LN2+RK_FLUSH,NULL);
   cryout(RK_P1|RK_PF,"Tack on previous now should fail",RK_CCL,NULL);
   cryout(RK_P1|RK_PF,"\n\nHere is a line+",RK_LN3,NULL);
   cryout(RK_P1|RK_PF,"followed by a correct continuation.",RK_CCL,
      NULL);
   cryout(RK_P1|RK_PF,"This line should be skipped\n",RK_LN2+RK_SKIP,
      NULL);
   cryout(RK_P1|RK_PF,"\n\nOnly 10 characters should appear",RK_LN2+20,
      NULL);
   cryout(RK_P1|RK_PF,"\nThis is a subtitle",RK_SUBTTL+RK_LN2,
      " continued thus (not ended)",RK_CCL,NULL);
   cryout(RK_P1|RK_PF,"This should trigger a new page\n",RK_LN1,NULL);
   cryout(RK_P1|RK_PF|1,"and this should trigger subtitle spout\n",
      RK_LN1, NULL);
   cryout(RK_P1|RK_PF,"This is an orphan subtitle",RK_SUBTTL+RK_CCL,NULL);
   cryout(RK_P1|RK_PF,"This should trigger the orphan subtitle\n",
      RK_LN2,NULL);
   cryout(RK_P1|RK_PF,"This is a filler\n",RK_LN1,NULL);
   cryout(RK_P1|RK_PF,"\nThis is a no-trigger subtitle",
      RK_NFSUBTTL+RK_LN2, " continued thus",RK_CCL,NULL);
   cryout(RK_P1|RK_PF,"This should not trigger a new page\n",RK_LN1,NULL);
   cryout(RK_P1|RK_PF|1,"and this should trigger subtitle spout\n",RK_LN1,
      NULL);
   cryout(RK_P1|RK_PF,"This is a filler\n",RK_LN1,NULL);
   cryout(RK_P1|RK_PF,"This should trigger a new page\n",
      RK_LN1+RK_NEWPG, NULL);
   cryout(RK_P1|RK_PF,"This is the last line, with flush\n",
      RK_LN1+RK_FLUSH, NULL);

   return 0;
   } /* End cryotest */


