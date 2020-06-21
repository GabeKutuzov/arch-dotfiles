#include "ChipUtils.h"
#include "SPP2AuxFiles.h"
#include "SPP2Globals.h"
#include "Network.h"
#include "OutputWin.h"

void ChipUtils::ClearChips()
{
        for (ChipVecIter it = Chips.begin();
                        it != Chips.end(); ++it)
        {
                (*it)->ClearAllGrps();
        }
}

Chip* ChipUtils::MinInCxns()
{
        Chip* min = Chips[0];
        for (int i = 1; i < NUM_CHIPS; ++i)
        {
                if (Chips[i]->GetNumInCxns() < min->GetNumInCxns()) {
                        min = Chips[i];
                }
        }
        return min;
}

Chip* ChipUtils::MinElems()
{
        Chip* min = Chips[0];
        for (int i = 1; i < NUM_CHIPS; ++i)
        {
                if (Chips[i]->OnCnt() < min->OnCnt()) {
                        min = Chips[i];
                }
        }
        return min;
}

// could also distribute plastic connections as well if this works,
// plastic connections are crunchy!
// i could sort groups with plastic connections first on chips
// then sort for regular connections

// with this algorithm I must make sure that it's OK to duplicate
// sensor activity or that there's enough room !
bool ChipUtils::DistribCxns()
{
        Chip* minIn;
        Chip* minElem;
        for (GrpLstIter it = Groups.begin();
                        it != Groups.end(); ++it)
        {
                if (!(*it)->isSens()) //only Simulated Groups added as On-Grps
                {
                        minIn = MinInCxns();
                        if (!minIn->AddOnGrp(*it))
                        {//does not fit, add to other chip
                                minElem = MinElems();
                                if (!minElem->AddOnGrp(*it))
                                {
                                        ClearChips();
                                        return false;//unable to set distribution -- need another algorithm!
                                }
                        }
                }
        }
        //in connections should now be evenly distributed & fit on every chip
        // SO,

        /**
         * Add new In-Grps needed by New Group that the Chip does not yet provide
         */
        for (ChipVecIter it1 = Chips.begin(); it1 != Chips.end(); ++it1)
        {
                GrpLst ChipGrps = (*it1)->GetOnGrps();
                for (GrpLstIter it2 = ChipGrps.begin(); it2 != ChipGrps.end(); ++it2)
                {
                        GrpLst InGrps = (*it2)->GetInGrps();
                        for (GrpLstIter it3 = InGrps.begin(); it3 != InGrps.end(); ++it3)
                        {
                                if ((*it1)->isOffGrp(*it3)|| (*it1)->isOnGrp(*it3) || (*it1)->isSensGrp(*it3)) {
                                        continue;
                                }
                                else
                                {
                                        if ((*it3)->isSens()) //must try to add sensor grp first
                                        {
                                                if (!(*it1)->AddSensGrp(*it3))
                                                {
                                                        ClearChips();
                                                        return false;//unable to set distribution -- need another algorithm!
                                                }
                                        }
                                        else
                                        {
                                                if (!(*it1)->AddOffGrp(*it3))
                                                {
                                                        ClearChips();
                                                        return false;//unable to set distribution -- need another algorithm!
                                                }
                                        }
                                }
                        }
                }
        }//could be consolidated -- both main loops?

#define TESTALG
#ifdef TESTALG
        for (int i = 0; i < NUM_CHIPS; ++i)
        {
                qDebug() << "Chip : " << Chips[i]->GetalphaID()
                        << " Num On: " << Chips[i]->OnCnt() << " Num Off " << Chips[i]->OffCnt()
                        << " Num Sens: " << Chips[i]->SensCnt() << "Num In-Cxns "
                        << Chips[i]->GetNumInCxns() << endl;
                GrpLst ChipGrps = Chips[i]->GetOnGrps();
                for (GrpLstIter it = ChipGrps.begin(); it != ChipGrps.end(); ++it)
                {
                        qDebug() << "Contents: " << endl;
                        qDebug() << "Group " << QString::fromStdString((*it)->GetName())
                                     << " -- Num In Cxns: " << (*it)->GetNumInCxns() << endl;
                }

        }
        SPP2AuxFiles::WriteChipConfig();
#endif TESTALG
        return true;
}

//later change to least # Chips possible routine
// sort groups by number of input connections
//start with Chip A placing as many Groups on as possible
//starting with the Group with the Lowest # of Inputs
bool ChipUtils::SynthSingle(int id)
{
        if ((Network::TotSensCells() <= Chips[id]->GetSensLeft()) && (Network::TotRegCells() <= Chips[id]->GetOnLeft()))
        {
                for (GrpLstIter it = Groups.begin();
                        it != Groups.end(); ++it)
                {
                        if ((*it)->isSens()) {
                                Chips[id]->AddSensGrp(*it);
                        }
                        else{
                                Chips[id]->AddOnGrp(*it);
                                (*it)->setHomeChip(Chips[id]);
                        }
                }
                return true;
        }
        else{
                return false;
        }
}

void ChipUtils::CustomChipConfig(QString ConfigFileName)
{
        QFile configFile(ConfigFileName);
        if (!configFile.open(QIODevice::ReadOnly))
        {
                OutputWin::error(0, "Cannot open file for writing: "
                             + configFile.errorString());
                return;
        }
        QTextStream input(&configFile);

        QStringList Tokens;
        Group* Grp;
        int ChipInx = 0;
        // 0: On-Group, 1: Off-Group, 2: Sensor-Group
        int command = 0;

        while (!input.atEnd())
        {
                Tokens = input.readLine().split(QRegExp("\\s+"), QString::SkipEmptyParts);
                if (Tokens.size())
                {
                        if (Tokens[0].at(0) != '#')
                        {
                                if (Tokens[0].toUpper() == "CHIP")
                                {
                                        ChipInx = Tokens[1].toInt()-1;
                                }
                                else if (Tokens[0].toUpper() == "ON")
                                {
                                        command = 0;
                                }
                                else if (Tokens[0].toUpper() == "OFF")
                                {
                                        command = 1;
                                }
                                else if (Tokens[0].toUpper() == "SENSOR")
                                {
                                        command = 2;
                                }
                                // else do the work according to the last command
                                else
                                {
                                        Grp = Network::GetGrp(Tokens[0]);
                                        if (Grp == NULL)
                                        {
                                                OutputWin::warning(0, "Group " + Tokens[0] + " does not exist!");
                                                break;
                                        }
                                        switch(command)
                                        {
                                                case 0:
                                                        Chips[ChipInx]->AddOnGrp(Grp);
                                                        Grp->setHomeChip(Chips[ChipInx]);
                                                        break;
                                                case 1:
                                                        Chips[ChipInx]->AddOffGrp(Grp);
                                                        break;
                                                case 2:
                                                        Chips[ChipInx]->AddSensGrp(Grp);
                                                        Grp->setHomeChip(Chips[ChipInx]);
                                                        break;
                                                default:
                                                        break;
                                        }
                                }
                        }
                }
        }//end while
}

void ChipUtils::SetOCIC_SRAM_Addrs()
{
        // address Grps starting @ Chip A then to Chip C
        //
        uword addr = 0;
        for (int i = 0; i < NUM_CHIPS; ++i)
        {
                GrpLst OnGrps = Chips[i]->GetOnGrps();
                for (GrpLstIter it1 = OnGrps.begin(); it1 != OnGrps.end(); ++it1)
                {
                        CellVec GrpCells = (*it1)->GetCells();
                        for (CellVecIter it2 = GrpCells.begin(); it2 != GrpCells.end(); ++it2)
                        {
                                (*it2)->SetOCIC_SRAM_Addr(addr);
                                ++addr;
                        }
                }
        }
}

void ChipUtils::SetChipCellAddrs()
{
        for (int i = 0; i < NUM_CHIPS; ++i) {
                Chips[i]->SetCellAddrs();
        }
}