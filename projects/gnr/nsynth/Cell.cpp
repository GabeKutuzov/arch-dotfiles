#include "Cell.h"

#include <algorithm>

Cell::Cell(int iID, Group *iGrp)
{
        OSTAddr0.first = NULL;
        OSTAddr0.second = -1;
        OSTAddr1.first = NULL;
        OSTAddr1.second = -1;
        OSTAddr2.first = NULL;
        OSTAddr2.second = -1;

        ID = iID;
        Grp = iGrp;
        numInCxns = 0;
        numOutCxns = 0;
        QtyPlast = 0;
        QtyVD = 0;
        QtyVI = 0;
        QtyCoinc = 0;
    QtyVal = 0;
        QtySens = 0;
        QtySensVD = 0;
        numOutElems = 0;
        Addr1Set = false;
        OcicSramAddr = 0xFFFFFFFF;
}

//if equal take one with most cxns!!!
bool cmp (Cell const* CellA, Cell const* CellB)
{
        return CellA->GetNumInElems() < CellB->GetNumInElems();
}

bool cmpMax (Cell const* CellA, Cell const* CellB)
{
        return CellA->GetNumInElems() < CellB->GetNumInElems();
}

bool Cell::hasInputFrom(Cell* other)
{
        for (list<MicroCxn*>::iterator it = Cxns.begin();
                        it != Cxns.end(); ++it)
        {
                if ((*it)->isPreCell(other)) {
                        return true;
                }
        }
        return false;
}

bool Cell::hasOutputTo(Cell* other)
{
        for (list<MicroCxn*>::iterator it = Cxns.begin();
                        it != Cxns.end(); ++it)
        {
                if ((*it)->isPostCell(other)) {
                        return true;
                }
        }
        return false;
}
/**
 *If it has not connected to this element before
*/
bool Cell::isNewInElem(MicroCxn* NewCxn)
{
        Cell* other;
        if (NewCxn->isPostCell(this) && !NewCxn->isPreCell(this)) {
                other = NewCxn->GetPreCell();
        }
        else{
                return false;
        }

        for (list<MicroCxn*>::iterator it = Cxns.begin();
                        it != Cxns.end(); ++it)
        {
                if ((*it)->isPreCell(other)) {
                        return false;
                }
                else{
                        continue;
                }
        }
        return true;
}

void Cell::AddCxn(MicroCxn* NewCxn)
{
        if (NewCxn->isPostCell(this))
        {
                /* Add Input Element to InputCells list?*/
                INCellInsert(NewCxn->GetPreCell());
                //make sure no duplicate input cells are in list
                InputCells.unique();
                ++numInCxns;

                // Will need to check if only Micro Connection is plast?
                if (NewCxn->GetMacroCxn()->isPlastic()) {
                        IncPlast();
                }
                if (NewCxn->GetMacroCxn()->isVal()) {
                        IncVal();
                }
                /* Add Input Connections to their associated Type container */
                if (NewCxn->GetMacroCxn()->isSens())
                {
                        //add VD inputs to the front
                        if (NewCxn->GetMacroCxn()->isVD())
                        {
                                Sens.push_front(NewCxn);
                                SensIn.push_front(NewCxn->GetPreCell());
                                ++QtySensVD;
                        }
                        else
                        {
                                Sens.push_back(NewCxn);
                                SensIn.push_back(NewCxn->GetPreCell());
                        }
                }
                else if (NewCxn->GetMacroCxn()->isCoinc())
                {
                        Coinc.push_back(NewCxn);
                        CoincIn.push_back(NewCxn->GetPreCell());
                }
                else if (NewCxn->GetMacroCxn()->isVD())
                {
                        VD.push_back(NewCxn);
                        VDIn.push_back(NewCxn->GetPreCell());
                }
                else
                {
                        PlainVI.push_back(NewCxn);
                        PlainVIIn.push_back(NewCxn->GetPreCell());
                }
        }
        Cxns.push_back(NewCxn);
}

void Cell::INCellInsert(Cell* newCell)
{
        list<Cell*>::iterator iBegin  = InputCells.begin();
        list<Cell*>::iterator iEnd    = InputCells.end();
        list<Cell*>::iterator iSorter = iBegin;
        int                                     steps   = InputCells.size();

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
          if (*iSorter <= newCell)
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
        InputCells.insert(iSorter, newCell);
}

int Cell::NumCxnsWith(Cell* other)
{
        int numCxns = 0;
        for (list<MicroCxn*>::iterator it = Cxns.begin();
                                it != Cxns.end(); ++it)
        {
                if ((*it)->isPreCell(other) || (*it)->isPostCell(other)) {
                        ++numCxns;
                }
                else{
                        continue;
                }
        }
        return numCxns;
}

void Cell::SetAddr1(Chip* chip, quint16 addr)
{
        OSTAddr1.first = chip;
        OSTAddr1.second = addr;
        Addr1Set = true;
}

void Cell::SetAddr0(Chip* chip, quint16 addr)
{
        OSTAddr0.first = chip;
        OSTAddr0.second = addr;
}

void Cell::SetAddr2(Chip* chip, quint16 addr)
{
        OSTAddr2.first = chip;
        OSTAddr2.second = addr;
}

unsigned short Cell::GetChipAddr(Chip* chip)
{
        if (chip == OSTAddr0.first) {
                return OSTAddr0.second;
        }
        else if (chip == OSTAddr1.first) {
                return OSTAddr1.second;
        }
        else{
                return OSTAddr2.second;
        }
}

int Cell::GetIOAddrNum(quint16 addr)
{
        for (int i = 0; i < (int)IOAddrs.size(); ++i)
        {
                if (addr == IOAddrs[i]) {
                        return i;
                }
                else{
                        continue;
                }
        }
        return -1;//does not have a IO Address pointer
}