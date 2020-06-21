/* (c) Copyright 1995 George N. Reeke, Jr.  *** CONFIDENTIAL ***   */
/* For use only in testing performance of computers on a simplified   */
/* version of a Darwin III simulation.  Please do not distribute      */
/* to any third parties without permission.                           */
/*--------------------------------------------------------------------*/
/*                                                                    */
/*    Darwin III "Benchmark" Program                                  */
/*                                                                    */
/*    This program is designed to be a "miniature" version of         */
/*    Darwin III, incorporating the most time-consuming inner-        */
/*    loop calculations.  It should be a suitable basis for           */
/*    developing specialized versions for various parallel-           */
/*    architecture machines.                                          */
/*                                                                    */
/*    V1A, 05/05/88, G. N. Reeke                                      */
/*    V1B, 05/26/88, GNR - Fix bug allocating CONNDATA structures     */
/*                                                                    */
/*--------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
*              THIS IS THE SPARC HOST (CLIENT) COMPONENT               *
*               OF THE TI MVP VERSION OF THE BENCHMARK                 *
*                                                                      *
*  This program simply loads the actual benchmark program onto the     *
*  MVP, and then operates as a server to transfer text messages        *
*  emanating from the MVP to the users stdout.  The actual benchmark   *
*  is contained in the file bserv_mp.c, where comments explaining      *
*  its operation may be found.  Note that in this first version of     *
*  the host program, there is no provision for passing stdin to the    *
*  MVP.  That also may be done at some point.                          *
*                                                                      *
*  Rev, 11/21/95, GNR - Add debug print task & host communications     *
*---------------------------------------------------------------------*/

/* Include standard library functions */

#include <stdlib.h>           /* MVP Standard Definitions */
#include <stdarg.h>
#include <stdio.h>
#include <sys/error.h>
#include <fcntl.h>

#include "sip80ioctl.h"
#include "sip80.h"
#include "host.h"             /* GIC host communication defs */
#include "bhost.h"            /* GNR host communication defs */

#define Sip80DeviceName "/dev/sip80:0"
#define Sip80ServerName "bserv.out"

/*---------------------------------------------------------------------*
*                          fatal error exits                           *
*                                                                      *
*  N.B.  All functions in this program are expected to report          *
*  terminal errors by calling one of these routines.                   *
*                                                                      *
*  (1) Generate fatal error message and terminate                      *
*  void fatal(char *txt, int num)                                      *
*  Arguments:                                                          *
*     txt      text string to be printed                               *
*     num      error code to be returned                               *
*                                                                      *
*  (2) Generate fatal error message including a system error number,   *
*        and terminate with error code (-1)                            *
*  void fatalpe(char *s1)                                              *
*  Arguments:                                                          *
*     s1       initial part of text string to be printed               *
*---------------------------------------------------------------------*/

/* Fatal error with descriptive text */

void fatal(char *txt, int num) {
 
   printf("\n***%s\n",txt);
   exit(num);
   } /* End fatal */

/* Fatal error with system error number */

void fatalpe(char *s1)  {
 
   printf("\n%s--Error number %d\n", s1, errno);
   exit(-1);
   }
 
/*---------------------------------------------------------------------*
*                         MAIN CLIENT PROGRAM                          *
*---------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
   
   long myLen;                /* Length of incoming */
   int myOp;                  /* Opcode of incoming */
   int sip80dev;              /* Sip80 device descriptor */
   COMM_s *myMsg,*myMsg1;     /* Ptrs to message headers */
   char *msgtext;             /* Space for text from MVP */
   static char devname[] = Sip80DeviceName
   static char mvpname[] = Sip80ServerName

   myMsg1 = malloc(sizeof(COMM_s));
   if (!myMsg1)
      fatal("Unable to allocate for incoming message hdrs", 1);
   msgtext = malloc(HMsgSize);
   if (!msgtext)
      fatal("Unable to allocate for incoming message text", 2);

/* Establish communications with the Sip80 */

   printf("Opening %s...\n", devname);
   fflush(stdout);
   sip80dev = open(devname, O_RDWR, 0);
   if (sip80dev < 0)
      fatalpe("Unable to open sip80 device");
   commNodeControl(sip80dev, CNC_REGISTER);
   commNodeControl(sip80dev, CNC_NOTIFY, NOTIFY_WAIT);

/* Load the server application onto the Sip80 */

   printf("Loading benchmark program onto Sip80...\n");
   fflush(stdout);
   if (commNodeControl(sip80dev, CNC_LOAD, mvpname)
      fatal("Unable to load MVP server code", 3);

/* Exchange handshake with MVP server */

   printf("Exchanging handshake messages with Sip80...\n");
   fflush(stdout);             
   if (mvpRouteMsg(sip80dev, CLIENT_INIT, sizeof(COMM_s), myMsg1) < 0)
      fatal("Unable to send CLIENT_INIT message"), 4);

   myOp = 0;
   myLen = 0;
   myMsg = NULL;
   if (mvpReceiveMsg(sip80dev, &myOp, &myLen, &myMsg) < 0)
      fatal("Unable to receive SERVER_INIT message", 5);
   if (myOp != SERVER_INIT)
      fatal("Initial message from MVP was not SERVER_INIT", 6);
   free(myMsg);

/* Now enter server loop, printing messages received from MVP */

   for (;;) {

      }







   


   } /* End main */
