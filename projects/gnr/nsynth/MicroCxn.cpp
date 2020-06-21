#include "MicroCxn.h"
#include "RndNums.h"
#include "SPP2Globals.h"

MicroCxn::MicroCxn(Cell* iPreCell, Cell* iPostCell, MacroCxn* imacrocxn)
{
        macrocxn = imacrocxn;

        PreCell = iPreCell;
        PostCell = iPostCell;

        PreCell->AddCxn(this);
        PostCell->AddCxn(this);

        // radix = 10, range = (-32, 32), minimal increment => 2^-10 = 0.0009765625 or ~0.001
        //Weight.Set(rnduniform(imacrocxn->GetWmin(), imacrocxn->GetWmax()), 10, 16, true);

#ifdef MicroWgts_When_Using_Random_Wgts
                double wgtInRange = 0;
                if (iPostCell->GetGrp()->isDarwin7())
                {
                        wgtInRange = rnduniform(imacrocxn->GetWmin(), imacrocxn->GetWmax());
                        Weight.Set(wgtInRange, 15, 16, true);
                }
                else if (iPostCell->GetGrp()->isCerebellum())
                {
                        wgtInRange = rnduniform(imacrocxn->GetWmin(), imacrocxn->GetWmax());
                        Weight.Set(wgtInRange, 10, 16, true);
                }
#endif
}