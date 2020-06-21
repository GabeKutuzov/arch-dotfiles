//

#include <string.h>
#include "mfint.h"
#include "plots.h"

void pentyp(const char *pentype){

   if(!(_RKG.pcw->MFActive)) return;
   ui32 n = strlen(pentype);
   mfbchk(mxlPenTyp);
   mfbitpk(OpPTyp, Lop);
   k2mf(0);
   mfbitpk(n,4);
   mfalign();
   cs2mf(pentyp,n);
} /* End pentyp() */
