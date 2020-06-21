/* (c) Copyright 1998-2017, The Rockefeller University *11114* */
/* $Id: symbc.c 34 2017-09-15 18:13:10Z  $ */
/***********************************************************************
*                               symbc()                                *
*                                                                      *
*  SYNOPSIS:                                                           *
*     void symbc(float ht, const char *text, float theta, short ns)    *
*                                                                      *
*  DESCRIPTION:                                                        *
*     Same as symbol() when ht < 0.0.  Otherwise, plots the text       *
*     string at the current plotting position.  This position is       *
*     passed on in the metafile so the interpreter does not need       *
*     to maintain separate current positions for each node in a        *
*     parallel computer.                                               *
*                                                                      *
*  ARGUMENTS:                                                          *
*     ht      Height of the letters, in inches.  If negative or        *
*             zero, use height and angle from previous call.           *
*     text    The actual label to be plotted, normally encoded as      *
*             a quoted string in C.  See discussion in symbol.c        *
*             regarding availability of special (marker) characters.   *
*     theta   The angle of the base line of the label above the        *
*             horizontal, in degrees.                                  *
*     ns      The length of the text string.  If ns is zero or         *
*             negative, a single centered symbol is drawn.  If         *
*             ns is 0 or -1, the pen is up when moving to the          *
*             location of the symbol, otherwise the pen is down.       *
*                                                                      *
*  CURRENT PLOT POSITION ON RETURN:                                    *
*     Undefined, but stored in the rendering program such that a       *
*     following symbol(), symbc(), number(), or numbc() call with      *
*     ht < 0 will concatenate the strings properly.                    *
*                                                                      *
*  RETURN VALUE:                                                       *
*     None.                                                            *
*                                                                      *
*  NOTE:                                                               *
*     This routine assumes the character string to be plotted          *
*     is expressed in the native character set of the host system.     *
*     At present, this is EBCDIC for IBM370 and ASCII for all others.  *
************************************************************************
*  V2A, 12/31/98, GNR - New routine, make binary metafiles             *
*  Rev, 08/29/14, GNR - Add 'const' to pointer args                    *
*  ==>, 01/03/17, GNR - Last mod before committing to svn repository   *
***********************************************************************/

#include "glu.h"
#include "rkxtra.h"
#include "rksubs.h"
#ifdef IBM370
#error Must add EBCDIC->ASCII conversion to symbol() for IBM370
#endif
#include "plots.h"

#define SYMNSL    0     /* gks ns locator */
#define SYMLEN    1     /* gks text length locator */

void symbc(float ht, const char *text, float theta, int ns) {

   static char MCsymbol[] = { OpSymb, Txc, NEW, Tyc, NEW, Tht,
      Tang, Ts, SYMNSL, Tchr, SYMLEN, Tend };
   static char MCcsymbl[] = { OpSymc, Tht, Tang,
      Ts, SYMNSL, Tchr, SYMLEN, Tend };

   if (_RKG.s.MFmode) {

      /* If necessary, restore current font */
      if (_RKG.MFReset & ResetF) {
         /* Retain State bits in MFReset across font() call */
         byte SaveMFReset = _RKG.MFReset;
         font(getrktxt(_RKG.hfontname));
         _RKG.MFReset = SaveMFReset & ~ResetF;
         }

      _RKG.gks[SYMNSL] = ns;
      if (ns <= 0) ns = 1;
      _RKG.gks[SYMLEN] = strnlen(text, ns);
      _RKG.psymbol = text;

      if (ht <= 0.0) {
         /* Use old height and angle--error if not set */
         if (_RKG.lht <= 0) abexit(265); }
      else {
         /* Save and use new height and angle */
         _RKG.lht  = (long)fixxy(ht);
         _RKG.lang = (long)fixaa(theta); }
      _RKG.h[NEW] = _RKG.lht;
      _RKG.aa     = _RKG.lang;

      if (ht < 0.0) {
         /* Concatenation option, coordinates are irrelevant */
         CMDPACK(MCcsymbl, _RKG.s.mxlSymc + _RKG.gks[SYMLEN]); }
      else {
         /* Use current coordinates--this will generate a two-
         *  bit same-coord code unless starts a new buffer.  */
         _RKG.x[NEW] = _RKG.x[CUR];
         _RKG.y[NEW] = _RKG.y[CUR];
         CMDPACK(MCsymbol, _RKG.s.mxlSymb + _RKG.gks[SYMLEN]); }
      } /* End if MFmode */

   } /* End symbc() */

