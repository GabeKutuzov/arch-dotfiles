#include "Coeff.h"
#include <math.h>

void Coeff::Set(double value, int iradix, int ibits, bool isigned)
{
        FxdPt = true;
        intVal = 0;
        intLo = 0;
        intHi = 0;

        radix = iradix;
        numBits = ibits;
        Signed = isigned;
        fltVal = value;

        if (Signed) {
                fltLo = -(pow(2.0, ibits-1)-1)*pow(2.0, -radix);
                fltHi = (pow(2.0, ibits-1)-1)*pow(2.0, -radix);
        }
        else{
                fltLo = 0.0;
                fltHi = (pow(2.0, ibits)-1)*pow(2.0, -radix);
        }

        if (!inRange()) {
                Normalize();
        }
        else{
                fxdVal = qint16(pow(2.0, radix)*fltVal);
        }
}

void Coeff::Set(int value, int ibits, bool isigned)
{
        FxdPt = false;
        fltVal = 0.0;
        radix = 0;
        fxdVal = 0;

        numBits = ibits;
        Signed = isigned;
        intVal32 = value;

        if (Signed)
        {
                intLo = qint16(-(pow(2.0, ibits-1)-1));
                intHi = qint16(pow(2.0, ibits-1)-1);
        }
        else
        {
                intLo = 0;
                intHi = qint16(pow(2.0, ibits)-1);
        }

        if (!inRange()) {
                Normalize();
        }
        else{
                intVal = value;
        }
}

bool Coeff::inRange()
{
        if (FxdPt)
        {
                if ((fltVal < fltLo)  || (fltVal > fltHi)) {
                        return false;
                }
                else{
                        return true;
                }
        }
        else
        {
                if ((intVal32 < (qint32)intLo) || (intVal32 > (qint32)intHi)) {
                        return false;
                }
                else{
                        return true;
                }
        }
}

void Coeff::Normalize()
{
        if (FxdPt)
        {
                if (fltVal < fltLo)
                {
                        fltVal = fltLo;
                }
                else{
                        fltVal = fltHi;
                }
                fxdVal = qint16(fltVal*pow(2.0, radix));
        }
        else
        {
                if (intVal32 < intLo) {
                        intVal = intLo;
                }
                else{
                        intVal = intHi;
                }
        }
}