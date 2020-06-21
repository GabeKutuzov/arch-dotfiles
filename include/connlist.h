/* (c) Copyright 2010, The Rockefeller University *11115* */
/* $Id: connlist.h 59 2017-01-13 20:34:16Z  $ */
/*---------------------------------------------------------------------*
*                             connlist.h                               *
*               External connection list record format                 *
*                                                                      *
*  Note:  Because connection type numbers and Cij are both restricted  *
*  to not more than 16 bits, they are packed into one word of ECLREC.  *
*  When external input is KGNE2, Cij is read from this record.  If     *
*  external input is KGNEX, Cij is generated, ecij field is ignored.   *
*  By definition, all fields here are big-endian.  Hence the addition  *
*  of ecij in the high-order of jct does not affect jct in old files.  *
*---------------------------------------------------------------------*/

   struct ECLREC {
      si32 xjcell;            /* Source cell number */
      si32 xicell;            /* Target cell number */
      si16 ecij;              /* External Cij value */
      ui16 jct;               /* Relative connection type number */
      };

