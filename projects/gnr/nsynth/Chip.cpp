#include "ChipFiles.h"
#include "Chip.h"
#include "Group.h"
#include "Cell.h"
#include "TypeDefs.h"
#include "OutputWin.h"

#include <vector>
#include <QDebug>

Chip::Chip(int iID, QString ialphaID)
{
        myID = iID;
        myAlphaID = ialphaID;
        OnLeft = MAX_ON_ELEMS_PER_CHIP;
        OffLeft = MAX_OFF_ELEMS_PER_CHIP;
        SensLeft = MAX_SENS_ELEMS_PER_CHIP;

        m_chipfiles = new ChipFiles(this);
}

bool Chip::InitSPP2Files() { return m_chipfiles->SetAllData(); }
bool Chip::WriteSPP2Files() { return m_chipfiles->WriteFiles(); }
void Chip::ClearSPP2Files() { m_chipfiles->ClearAllData(); }

uword Chip::GetWordBursts() { return m_chipfiles->GetWordBursts(); }

int Chip::GetNumInCxns()
{
        int qty = 0;
        for (GrpLstIter it = OnGrps.begin();
                        it != OnGrps.end(); ++it)
        {
                qty += (*it)->GetNumInCxns();
        }
        return qty;
}

int Chip::GetNumPlastCxns()
{
        int plast = 0;
        for (GrpLstIter it = OnGrps.begin();
                        it != OnGrps.end(); ++it)
        {
                plast += (*it)->GetNumPlast();
        }
        return plast;
}

void Chip::ClearAllGrps()
{
        OnGrps.clear();
        OffGrps_FromOtherChip1.clear();
        OffGrps_FromOtherChip2.clear();
        SensGrps.clear();

        OnLeft = MAX_ON_ELEMS_PER_CHIP;
        OffLeft = MAX_OFF_ELEMS_PER_CHIP;
        SensLeft = MAX_SENS_ELEMS_PER_CHIP;
}

/**
 * Insert Groups In Ascending Order
 *
 */
void Chip::InsertGrp(Group* newGrp, GrpLst& Lst)
{
        GrpLstIter iBegin  = Lst.begin();
        GrpLstIter iEnd    = Lst.end();
        GrpLstIter iSorter = iBegin;
        int                     steps  = Lst.size();

        while (iBegin != iEnd)
        {
          iSorter = iBegin;
          steps = (int)(steps / 2);
          advance(iSorter, steps);
          if (*iSorter <= newGrp)
          {
                 iBegin = iSorter;
                 if (steps == 0 && iBegin != iEnd) {
                        ++iBegin;
                 }
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
        Lst.insert(iSorter, newGrp);
}

bool Chip::AddOnGrp(Group* grp)
{
        if (grp->GetSize() <= OnLeft) {
                OnGrpInsert(grp);
                return true;
        }
        else{
                return false;
        }
}

bool Chip::AddOffGrp(Group* grp)
{
        if (grp->GetSize() <= OffLeft) {
                OffGrpInsert(grp);
                return true;
        }
        else{
                return false;
        }
}

bool Chip::AddSensGrp(Group* grp)
{
        if (grp->GetSize() <= SensLeft) {
                SensGrpInsert(grp);
                return true;
        }
        else{
                return false;
        }
}

void Chip::OnGrpInsert(Group* grp)
{
        InsertGrp(grp, OnGrps);
        OnLeft -= grp->GetSize();
}

/**
 * This function will insert the Off-Groups
 * such that they preserve the same ordering
 * between Groups that have the same home chip and
 * the A-Chip Groups are listed first then the
 * B-Chip and finally C-Chip.
 */
void Chip::OffGrpInsert(Group* grp)
{
        if (grp->GetHomechip() == NULL)
        {
                qDebug() << "Group " << QString::fromStdString(grp->GetName()) << "'s Home-Chip location "
                                 << "must be set first before setting it's Off-Chip locations." << endl;

                return;
        }

        if (myID == 0)
        {
                if (grp->GetHomechip()->GetID() == 1)
                {
                        InsertGrp(grp, OffGrps_FromOtherChip1);
                }
                else if (grp->GetHomechip()->GetID() == 2)
                {
                        InsertGrp(grp, OffGrps_FromOtherChip2);
                }
        }
        else if (myID == 1)
        {
                if (grp->GetHomechip()->GetID() == 0)
                {
                        InsertGrp(grp, OffGrps_FromOtherChip1);
                }
                else if (grp->GetHomechip()->GetID() == 2)
                {
                        InsertGrp(grp, OffGrps_FromOtherChip2);
                }
        }
        else if (myID = 2)
        {
                if (grp->GetHomechip()->GetID() == 0)
                {
                        InsertGrp(grp, OffGrps_FromOtherChip1);
                }
                else if (grp->GetHomechip()->GetID() == 1)
                {
                        InsertGrp(grp, OffGrps_FromOtherChip2);
                }
        }

        OffLeft -= grp->GetSize();
}

void Chip::SensGrpInsert(Group* grp)
{
        InsertGrp(grp, SensGrps);
        SensLeft -= grp->GetSize();
}

bool Chip::isInputToChip(Group* grp)
{
        for (GrpLstIter it = OnGrps.begin();
                        it != OnGrps.end(); ++it)
        {
                if ((*it)->isInGrp(grp)) {
                        return true;
                }
                else{
                        continue;
                }
        }
        return false;
}

int Chip::NumOffFrom(Group* grp)
{
        if (isOffGrp(grp)) {
                return grp->GetSize();
        }
        else{
                return 0;
        }
}

int Chip::NumOnOffTo(Group* grp)
{
        int num = 0;
        GrpLst OtherInGrps = grp->GetInGrps();
        for (GrpLstIter it = OtherInGrps.begin(); it != OtherInGrps.end(); ++it)
        {
                if (isOffGrp(*it) || isOnGrp(*it)) {
                        num += (*it)->GetSize();
                }
        }

        return num;
}

void Chip::RemOffGrp(Group* grp)
{
        if (isOffGrp(grp))
        {
                // remove group from either list
                OffGrps_FromOtherChip1.remove(grp);
                OffGrps_FromOtherChip2.remove(grp);
                OffLeft += grp->GetSize();
        }
}

void Chip::RemOnGrp(Group* grp)
{
        if (isOnGrp(grp))
        {
                OnGrps.remove(grp);
                OnLeft += grp->GetSize();
        }
}

void Chip::RemSensGrp(Group* grp)
{
        if (isSensGrp(grp))
        {
                SensGrps.remove(grp);
                SensLeft += grp->GetSize();
        }
}

int Chip::NumSensTo(Group* grp)
{
        int num = 0;
        GrpLst OtherInGrps = grp->GetInGrps();
        for (GrpLstIter it = OtherInGrps.begin(); it != OtherInGrps.end(); ++it)
        {
                if (isSensGrp(*it)) {
                        num += (*it)->GetSize();
                }
        }
        return num;
}

bool Chip::AutoAddGrp(Group* grp)
{
        OutputWin::message("Checking Chip constraints. . .");

        if (grp->isSens())
        {
                if (isSensGrp(grp))
                {
                        OutputWin::message("Group has already been added to Chip!");
                        return false;
                }
                else if (grp->GetSize() <= SensLeft)
                {
                        SensGrpInsert(grp);
                        OutputWin::message("Sensor Group Added to Chip!");
                        return true;
                }
                else
                {
                        OutputWin::message("Sensor Group does not fit on Chip!");
                        return false;
                }
        }
        else//regular group
        {
                if ((grp->GetSize() <= OnLeft) && !isOnGrp(grp))
                {
                        //make sure has enough room for sensor groups & off-chip groups -- may have to have eventually automate adding value groups
                        int spaceSens = grp->NumSensIn() - NumSensTo(grp);
                        if (spaceSens > SensLeft)
                        {
                                OutputWin::warning(0, "Sensor space left " + QString::number(SensLeft)
                                                                        + " is less than required " + QString::number(spaceSens));
                                return false;
                        }
                        int spaceOff = (grp->NumRegIn() - NumOnOffTo(grp)) - NumOffFrom(grp);
                        //special temporary test
                        if (spaceOff > OffLeft)
                        {
                                OutputWin::warning(0, "Off-Group Space left " + QString::number(OffLeft)
                                        + " is less than the amount needed " + QString::number(spaceOff));
                                //false;//may not want to return false because there may be enough room on the OnChip side for this Off-Chip ?
                        }
                        OutputWin::message("Adding Group to Chip. . .");
                }
                else
                {
                        OutputWin::warning(0, "Group does not fit or is already on Chip!");
                        return false;
                }

                /* Add New Group to Chip */
                OnGrpInsert(grp);

                /**
                 * Add new In-Grps needed by New Group that the Chip does not yet provide
                 */
                GrpLst OtherInGrps = grp->GetInGrps();
                for (GrpLstIter it = OtherInGrps.begin(); it != OtherInGrps.end(); ++it)
                {
                        if (isOffGrp(*it)|| isOnGrp(*it) || isSensGrp(*it)) {
                                continue;
                        }
                        else
                        {
                                if ((*it)->isSens()) {//for now, make sure sensors are added First!
                                        SensGrpInsert(*it);
                                }
                                else{
                                        OffGrpInsert(*it);
                                }
                        }
                }

                /* Remove Grp if it was an Off-Chip Group */
                RemOffGrp(grp);
                OutputWin::message("Finished Adding Group to Chip!");

                return true;
        }
}

void Chip::AutoRemGrp(Group* grp)
{
        /* Remove Group from Chip */
        RemOnGrp(grp);

        /* If Group is an Input to other Groups add it as Sens or Off */
        OutputWin::message("Adding Off-Chip elements Old Group was supplying. . .");
        if (isInputToChip(grp))
        {
                if (grp->isSens()) {
                        SensGrpInsert(grp);
                }
                else{
                        OffGrpInsert(grp);
                }
        }

        //remove new InGrps as was added by Ol' Group that the Chip does not need
        OutputWin::message("Removing Off-Chip elements that was needed by Old Group. . .");
        GrpLst OtherInGrps = grp->GetInGrps();
        for (GrpLstIter it = OtherInGrps.begin(); it != OtherInGrps.end(); ++it)
        {
                if (!isInputToChip(*it))
                {
                        if ((*it)->isSens()) {
                                RemSensGrp(*it);
                        }
                        else{
                                RemOffGrp(*it);
                        }
                }
        }
}

void Chip::SetCellAddrs()
{
        quint16 addr1 = 0;
        quint16 addr2 = 0;

        /* (1) Address Sensor Group Cells */
        for (GrpLstIter it1 = SensGrps.begin(); it1 != SensGrps.end(); ++it1)
        {
                CellVec SensCells = (*it1)->GetCells();
                for (CellVecIter it2 = SensCells.begin(); it2 != SensCells.end(); ++it2)
                {
                        (*it2)->SetAddr0(this, addr1);
                        ++addr1;
                }
        }

        /* (2) Address On-Chip Group Cells */
        for (GrpLstIter it1 = OnGrps.begin(); it1 != OnGrps.end(); ++it1) // must be aligned with element numbers!
        {
                CellVec OnCells = (*it1)->GetCells();
                for (CellVecIter it2 = OnCells.begin(); it2 != OnCells.end(); ++it2)
                {
                        (*it2)->SetAddr0(this, addr2);
                        ++addr2;
                }
        }

        /* (3) Address Off-Chip Group Cells */
        if (OffCnt() > MAX_OFF_ELEMS_PER_CHIP) {
                OutputWin::warning(0, "Off-Chip Input Cells for this Chip exceeds " + QString::number(MAX_OFF_ELEMS_PER_CHIP) + " limit!");
        }//proceed or Quit!-> This should be tested earlier!

        //quint16 addr3 = (quint16)MAX_ON_ELEMS_PER_CHIP;// OFF-Chip Address partition start
        quint16 addr3 = addr2;// OFF-Chip Address partition continues tightly packed after On-Chip Groups
        GrpLst AllOffGrps = GetOffGrps();
        for (GrpLstIter it1 = AllOffGrps.begin(); it1 != AllOffGrps.end(); ++it1)
        {
                CellVec OffCells = (*it1)->GetCells();
                for (CellVecIter it2 = OffCells.begin(); it2 != OffCells.end(); ++it2)
                {
                        /* Set Cell Off-Chip Addresses; each cell can have up to three Chip addresses */
                        if (!(*it2)->isAddr1Set()) {
                                (*it2)->SetAddr1(this, addr3);
                        }
                        else{
                                (*it2)->SetAddr2(this, addr3);
                        }
                        ++addr3;
                }
        }
}

GrpLst Chip::GetOffGrps()
{
        GrpLst AllOffGrps;
        GrpLstIter InsertIter;
        InsertIter = AllOffGrps.begin();

        /* ==================== Preserve Ordering ==================== */
        AllOffGrps.insert(InsertIter, OffGrps_FromOtherChip1.begin(), OffGrps_FromOtherChip1.end());
        AllOffGrps.insert(InsertIter, OffGrps_FromOtherChip2.begin(), OffGrps_FromOtherChip2.end());
        /* =========================================================== */

        return AllOffGrps;
}

Chip::~Chip()
{
        delete m_chipfiles;
}
