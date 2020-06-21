//

#include <string.h>
#include "mfint.h"
#include "plots.h"

void symbol(float x, float y, float ht, const char *text,
      float angle, int n){

   ui32 tlen = strlen(text);
   if(!_RKG.pcw->MFActive) return;
   mfbchk(_RKG.s.mxlSymb);
   mfbitpk(OpSymb, Lop);
   x2mf(x);
   y2mf(y);
   h2mf(ht);
   a2mf(angle);
   s2mf(n);
   cs2mf(text, tlen);
} /* End symbol() */