/* (c) Copyright 1999-2016, The Rockefeller University *11115* */
/* $Id: swap.h 59 2017-01-13 20:34:16Z  $ */
/***********************************************************************
*                               swap.h                                 *
*      Prototypes for message and file data conversion routines        *
*                                                                      *
*  The new standard scheme is that all binary interprocessor message   *
*  contents are little-endian ("lem"), but binary file data are big-   *
*  endian ("bem").  Separate routines are provided for conversions in  *
*  each direction, so that routines for non-IEEE floating point can be *
*  added if necessary.  Accesses to file or message data are via ptrs, *
*  in case the converted item would be damaged if treated as register  *
*  data of the original type.  This also allows alignment and even     *
*  size to differ on source and target systems.  Where conversion is   *
*  trivial, routines are implemented as macros.                        *
*                                                                      *
*  Note that there are no separate ---u2 or ---u4 routines because     *
*  these transfers are the same as the signed ---i2 and ---i4 ones.    *
*                                                                      *
*  In some cases, a memcpy definition could be used, but this would    *
*  not work with arguments that are expressions, so this was not done. *
*                                                                      *
*  V2A, 05/08/99, GNR - New swapping scheme defined                    *
*  V2B, 03/09/06, GNR - Allow for case where long is 64 bits           *
*  ==>, 08/18/07, GNR - Last date before committing to svn repository  *
*  V2C, 10/26/13, GNR - Add FMJSIZE, FMLSIZE --> 8 for 64-bit systems  *
*  Rev, 02/06/16, GNR - Add FMPSIZE, remove comment "no pointers"      *
*  V2D, 03/01/16, GNR - Add 'il' versions for longs, 32 or 64 bits     *
***********************************************************************/


#ifndef SWAP_INCLUDED
#define SWAP_INCLUDED

/* Define lengths that types will have in messages and files--not
*  necessarily same as lengths returned by sizeof() on this machine.
*  Longs will be transferred as 8-byte, 'il' transfer routines will
*  expand/shorten where actual size is 4 bytes.  */
#define FMZSIZE   8     /* Size of size_t (max) */
#define FMPSIZE   8     /* Size of pointer (max) */
#define FMDSIZE   8     /* Size of double */
#define FMESIZE   4     /* Size of float */
#define FMWSIZE   8     /* Size of long long, si64, ui64 */
#define FMLSIZE   8     /* Size of long (max) */
#define FMJSIZE   4     /* Size of ui32 or si32 */
#define FMISIZE   4     /* Size of integer */
#define FMSSIZE   2     /* Size of short */
#define FMBSIZE   1     /* Size of char or byte */

/* Big-endian message data from short integer */
#define bemfmi2(m,i2) { register ui16 ui2 = (i2); \
   m[1] = (char)ui2, m[0] = (char)(ui2 >> BITSPERBYTE); }

/* Big-endian message data from ui32 or si32 integer */
#define bemfmi4(m,i4) { register ui32 ui4 = (i4); \
   m[3] = (char)ui4, m[2] = (char)(ui4 >>= BITSPERBYTE), m[1] = \
   (char)(ui4 >>= BITSPERBYTE), m[0] = (char)(ui4 >> BITSPERBYTE); }

/* Big-endian message data from long (32 or 64 per system) integer */
void bemfmil(char *m, long il);
void bemfmul(char *m, ulng il);

/* Big-endian message data from long long integer */
void bemfmi8(char *m, si64 i8);
void bemfmu8(char *m, ui64 u8);

/* Big-endian message data from short floating point */
void bemfmr4(char *m, float r4);

/* Big-endian message data from double-precision floating point */
void bemfmr8(char *m, double r8);

/* Big-endian message data to short integer */
#define bemtoi2(m) ((ui16)(byte)m[1] | \
   ((ui16)(byte)m[0] << BITSPERBYTE))

/* Big-endian message data to ui32 or si32 integer */
#define bemtoi4(m) (((((((ui32)(byte)m[0] << BITSPERBYTE) | \
   (ui32)(byte)m[1]) << BITSPERBYTE) | (ui32)\
   (byte)m[2]) << BITSPERBYTE) | (ui32)(byte)m[3])

/* Big-endian message data to long integer */
long bemtoild(char *m);
long bemtoile(char *m, int ec);
ulng bemtould(char *m);
ulng bemtoule(char *m, int ec);

/* Big-endian message data to long long integer */
si64 bemtoi8(char *m);
ui64 bemtou8(char *m);

/* Big-endian message data to short floating point */
float bemtor4(char *m);

/* Big-endian message data to double-precision floating point */
double bemtor8(char *m);

/* Little-endian message data from short integer */
#define lemfmi2(m,i2) { register ui16 ui2 = (i2); \
   m[0] = (char)ui2, m[1] = (char)(ui2 >> BITSPERBYTE); }

/* Little-endian message data from ui32 or si32 integer */
#define lemfmi4(m,i4) { register ui32 ui4 = (i4); \
   m[0] = (char)ui4, m[1] = (char)(ui4 >>= BITSPERBYTE), m[2] = \
   (char)(ui4 >>= BITSPERBYTE), m[3] = (char)(ui4 >> BITSPERBYTE); }

/* Little-endian message data from long (32 or 64 per system) integer */
void lemfmil(char *m, long il);
void lemfmul(char *m, ulng il);

/* Little-endian message data from long long integer */
void lemfmi8(char *m, si64 i8);
void lemfmu8(char *m, ui64 u8);

/* Little-endian message data from short floating point */
void lemfmr4(char *m, float r4);

/* Little-endian message data from double-precision floating point */
void lemfmr8(char *m, double r8);

/* Little-endian message data to short integer */
#define lemtoi2(m) ((ui16)(byte)m[0] | \
   ((ui16)(byte)m[1] << BITSPERBYTE))

/* Little-endian message data to ui32 or si32 integer */
#define lemtoi4(m) (((((((ui32)(byte)m[3] << BITSPERBYTE) | \
   (ui32)(byte)m[2]) << BITSPERBYTE) | (ui32)\
   (byte)m[1]) << BITSPERBYTE) | (ui32)(byte)m[0])

/* Little-endian message data to long integer */
long lemtoild(char *m);
long lemtoile(char *m, int ec);
ulng lemtould(char *m);
ulng lemtoule(char *m, int ec);

/* Little-endian message data to long long integer */
si64 lemtoi8(char *m);
ui64 lemtou8(char *m);

/* Little-endian message data to short floating point */
float lemtor4(char *m);

/* Little-endian message data to double-precision floating point */
double lemtor8(char *m);

#endif                        /* End ifndef SWAP_INCLUDED */
