/* This file is generated automatically by nxdr2.  It
*  contains data conversion tables for interprocessor
*  messaging.  Do not edit by hand!  */

/* When compiled with -DPAR, the application must
*  provide a definition of unicvtf, the interface
*  to union conversion routines, in some header
*  file that is included in the -f input file. */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

/* This is a test include file for use with nxdr2 */
/* Values of TX that should be tested are:
*  34201 - 34213, 34601 - 34604, 351, 34301 - 34302
*  When testing, add "struct tx" to testobj file.
*/

#include <stdlib.h>
#include "sysdef.h"
#include "testhdr.h"

long PQRSTT[] = {
   26,                               /* str_itm */
   ( 8 << 8 ) | 's',
   ( 2 << 8 ) | 'p',
   ( 1 << 8 ) | 'l',
   ( 2 << 8 ) | 'C',
   sizeof(item),                     /* item */
   ( 1 << 8 ) | 'X',
   ( 0 ),
   sizeof(typrec),                   /* typrec */
   ( 8 << 8 ) | 's',
   ( 3 << 8 ) | 'p',
   ( 1 << 8 ) | 'C',
   54,                               /* str_tb3 */
   ( 8 << 8 ) | 's',
   ( 5 << 8 ) | 'p',
   ( 1 << 8 ) | 'l',
   ( 2 << 8 ) | 'h',
   ( 2 << 8 ) | 'C',
   sizeof(tab3),                     /* tab3 */
   ( 1 << 8 ) | 'X',
   ( 12 ),
   sizeof(struct t1),                /* str_t1 */
   ( 8 << 8 ) | 's',
   ( 1 << 8 ) | 'i',
   ( 1 << 8 ) | 'p',
   ( 1 << 8 ) | 'i',
   ( 1 << 8 ) | 'e',
   ( 3 << 8 ) | 'p',
   ( 35 << 8 ) | 'I',
   54,                               /* uni_u1_1 */
   ( 1 << 8 ) | 'X',
   ( 12 ),
   26,                               /* uni_u1_2 */
   ( 1 << 8 ) | 'X',
   ( 0 ),
   8,                                /* uni_u1_3 */
   ( 1 << 8 ) | 'L',
   54,                               /* uni_u1 */
   ( 1 << 8 ) | 'J',
   ( 0 << 8) | 8,
   sizeof(uni1),                     /* uni1 */
   ( 1 << 8 ) | 'J',
   ( 0 << 8) | 8,
   };

#ifdef PAR
   extern unicvtf NXFuni_u1_u;

unicvtf *PQRSUT[] = {
   NXFuni_u1_u,
   };
#else
unicvtf *PQRSUT[] = { NULL };
#endif

