/**
 * @file Coeff.h
 *
 * Coefficients are values used
 * in the SPP2 neural models. They
 * are converted into fixed point
 * formats for use in the SPP2
 * NPUs.
 */
#ifndef COEFF_H_
#define COEFF_H_

#include <QtCore>

class Coeff {
public:
   Coeff() {}
   ~Coeff() {}

   void Set( double value, int radix, int ibits, bool isigned ); //for fixed point numbers
   void Set( int value, int ibits, bool isigned ); //for plain integers
   int GetRadix() { return radix; }
   double GetFltVal() { return fltVal; }
   qint16 GetFxdVal() { return fxdVal; }
   qint16 GetIntVal() { return intVal; }
   bool isFxdPt() { return FxdPt; }
   bool inRange();
   void Normalize();

private:
   int     radix;
   int     numBits;
   bool    Signed;
   bool    FxdPt;

   qint16  intLo;//currently only supporting 16 bit ints
   qint16  intHi;

   double  fltLo;
   double  fltHi;

   double  fltVal;
   qint16  fxdVal;
   qint16  intVal;
   qint32  intVal32;
   };
#endif