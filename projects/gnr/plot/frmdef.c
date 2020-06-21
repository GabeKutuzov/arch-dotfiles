//

#include "mfint.h"
#include "plots.h"

void frmdef(unsigned int *pfrm, unsigned int mode,
   float width, float height, float vv[LVxx]){

   int i = 0;
   mfbchk(_RKG.s.mxlFrmWVxx);
   mfbitpk(OpFrame, Lop);
   mfbitpk(mode, 10);
   k2mf(*pfrm);
   w2mf(width);
   h2mf(height);
   for(i;i<LVxx;i++){
      g2mf(vv[i]);
   }
} /* End frmdef() */
