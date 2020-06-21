#include "Group.h"
#include "TypeDefs.h"
#include "Network.h"
#include "MacroCxn.h"
#include "OutputWin.h"

#include <vector>
#include <list>
#include <QString>
#include <QStringList>
#include <QDebug>


Group::Group(QStringList tokens, int iID) {
   name =  tokens.at(1).toStdString();
   SysID = iID;
   type = tokens.at(2).toInt();
   height = tokens.at(3).toInt();
   width = tokens.at(4).toInt();
   size = height*width;

   if (this->isCerebellum()) {
      scale.Set(tokens.at(5).toDouble(), 8, 8, false);
      fr_thr.Set(tokens.at(6).toDouble(), 8, 8, false);
      vd_thr.Set(tokens.at(7).toDouble(), 8, 8, false);
      prst.Set(tokens.at(8).toDouble(), 8, 8, false);
      }
   else if (this->isDarwin7()) {
      scale.Set(tokens.at(5).toDouble(), 6, 8, false);
      fr_thr.Set(tokens.at(6).toDouble(), 8, 8, false);
      vd_thr.Set(tokens.at(7).toDouble(), 8, 8, false);
      prst.Set(tokens.at(8).toDouble(), 8, 8, false);
      }
   else if (this->isIzhik()) {
      Izhi_a.Set(tokens.at(5).toDouble(), 14, 2, true);
      Izbin = tokens.at(6).toDouble();
      Izhi_c.Set(tokens.at(7).toDouble(), 8, 8, true);
      Izdin = tokens.at(8).toDouble();
      Izkin = tokens.at(9).toDouble();
      Izhi_vT.Set(tokens.at(10).toDouble(), 8, 8, true);
      Izhi_vPeak.Set(tokens.at(11).toDouble(), 8, 8, true);
      IzCm = tokens.at(12).toDouble();
      Izra = tokens.at(13).toDouble();
      Izrb = tokens.at(14).toDouble();
      Izrc = tokens.at(15).toDouble();
      Izrd = tokens.at(16).toDouble();
      }
   else if (this->isRockVis()) {
      RV_pt.Set(tokens.at(6).toDouble(), 15, 1, true);
      RV_sdamp.Set(tokens.at(7).toDouble(), 15, 1, false);
      RV_omega2.Set(tokens.at(8).toDouble(), 15, 1, false);
      }
   else {
      //integer values for special test cases*
      scale.Set(tokens.at(5).toInt(), 8, false);
      fr_thr.Set(tokens.at(6).toInt(), 8, false);
      vd_thr.Set(tokens.at(7).toInt(), 8, false);
      prst.Set(tokens.at(8).toInt(), 8, false);
      }

   HomeChip = 0;
   numSynRecs = 0;
   NumInCells = 0;
   OnChipID = -1;
   GrpRecID = -1;
   CxnsCalcd = false;
   } /* End Group constructor */

void Group::AddInGrp(Group* InGrp)
{
        GrpLstIter iBegin  = InGrps.begin();
        GrpLstIter iEnd    = InGrps.end();
        GrpLstIter iSorter = iBegin;
        int                     steps  = InGrps.size();

        while (iBegin != iEnd)
        {
          // start with the iterator at the current beginning of the list
          iSorter = iBegin;
          // find the middle
          steps = (int)(steps / 2);
          // move the iterator to the middle
          advance(iSorter, steps);
          // if the date of the file being inserted is earlier or equal
          // to the date of the ciurrent iterator position
          if (*iSorter <= InGrp)
          {
                 // change the beginning of the list to the current
                 // iterator position
                 iBegin = iSorter;
                 // if we didn't move at all, and if we aren't at the
                 // end of the list,  move the beginning one more step.
                 if (steps == 0 && iBegin != iEnd) {
                        ++iBegin;
                 }
                 // we need to do it this way because eventually, you just
                 // run out of "steps" (it's equal to 0), and the routine
                 // would just sit there on the same iterator forever.  If
                 // it gets to this point, it's safe to assume that simply
                 // moving the iterator one more step in the appropriate
                 // direction will locate the correct insertion point.
          }
          else
          {
                 iEnd = iSorter;
                 if (steps == 0 && iEnd != iBegin) {
                        --iEnd;
                 }
          }
        }
        iSorter = iEnd;

        InGrps.insert(iSorter, InGrp);
        NumInCells += InGrp->GetSize();
}

int Group::NumSensIn()
{
        int num = 0;
        for (GrpLstIter it = InGrps.begin(); it != InGrps.end(); ++it)
        {
                if ((*it)->isSens()) {
                        num += (*it)->GetSize();
                }
        }

        return num;
}

int Group::NumRegIn()
{
        int num = 0;
        for (GrpLstIter it = InGrps.begin(); it != InGrps.end(); ++it)
        {
                if (!(*it)->isSens()) {
                        num += (*it)->GetSize();
                }
        }

        return num;
}

int Group::GetNumInCxns()
{
        int qty = 0;
        for (MacroLstIter it = InCxns.begin();
                        it != InCxns.end(); ++it)
        {
                qty += (*it)->GetNumCxns();
        }
        return qty;
}

int Group::GetNumPlast()
{
        int plast = 0;
        for (MacroLstIter it = InCxns.begin();
                                it != InCxns.end(); ++it)
        {
                if ((*it)->isPlastic()) {
                        plast += (*it)->GetNumCxns();
                }
        }
        return plast;
}

bool Group::hasPlast()
{
        int plast = 0;
        for (MacroLstIter it = InCxns.begin();
                                it != InCxns.end(); ++it)
        {
                if ((*it)->isPlastic()) {
                        return true;
                }
        }
        return false;
}

int Group::GetNumCxns()
{
        if (!CxnsCalcd)
        {
                int TotCxns = 0;
                for (MacroLstIter it = Cxns.begin();
                                it != Cxns.end(); ++it)
                {
                        TotCxns += (*it)->GetNumCxns();
                }
                numCxns = TotCxns;
                CxnsCalcd = true;
        }
        return numCxns;
}

Group::~Group()
{
        for (CellVecIter it = cells.begin();
                it != cells.end(); ++it)
        {
                delete *it;
        }
}