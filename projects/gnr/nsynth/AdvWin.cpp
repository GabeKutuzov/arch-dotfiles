#include <QtGui>
#include <QtCore>
#include <list>
#include <vector>

#include "AdvWin.h"
#include "TypeDefs.h"
#include "MacroCxn.h"
#include "Group.h"
#include "Network.h"
#include "Group.h"
#include "SPP2AuxFiles.h"
#include "OutputWin.h"
#include "ChipUtils.h"

using namespace std;

AdvWin::AdvWin(QWidget* parent)
        : QMainWindow(parent)
{
        setupUi(this);

        Chips[0] = new Chip(0, "A");
        Chips[1] = new Chip(1, "B");
        Chips[2] = new Chip(2, "C");

        //Set Widget Containers
        OnLeft.push_back(txtOn1);
        OnLeft.push_back(txtOn2);
        OnLeft.push_back(txtOn3);
        OffLeft << txtOff1 << txtOff2 << txtOff3;
        SensLeft.push_back(txtSens1);
        SensLeft.push_back(txtSens2);
        SensLeft.push_back(txtSens3);
        ChipTbls << tblChip1 << tblChip2 << tblChip3;
        ChipSel.addButton(rdoChip1, 0);
        ChipSel.addButton(rdoChip2, 1);
        ChipSel.addButton(rdoChip3, 2);
        StorSel.addButton(rdoOn, 0);
        StorSel.addButton(rdoOff, 1);
        StorSel.addButton(rdoSens, 2);

        ClearDisplays();

        FileLoaded = false;

        //txtFile->setText("D:/NSI/Work/Code_Source/SPP2_Network_Definition_Files_Repository/SPP2_Darwin12_BaseDemo_072508.txt");

        //set current directory 1-up from release/debug .exe dir
        SynthDir.cdUp();
        AppHomeDir = SynthDir.currentPath();

        // Debug **
        //#define BENCHMARK_SEED
        //#define NPU_MEMORY_TESTCASE_NO_NETWORK_INSPECTION
        //#define SINGLE_TEST
        //#define DISTRIB_TEST
}

void AdvWin::on_btnSynth_clicked()
{
        QString SimFilesDir, RawFileName;

        SynthDir.setCurrent(AppHomeDir + "/SPP2_Network_Configuration_Files_Repository");
        //qDebug() << "Current: " << SynthDir.currentPath() << endl;

        //** Need Anatomy File Error Check!*/
        // create new directory for config files
        RawFileName = AnatomyFile.fileName().section('/',-1);
        RawFileName.chop(4);
        SimFilesDir = QDir::currentPath() + '/' + QDateTime::currentDateTime().date().toString("MM.dd.yy") + "_NetworkConfigFiles_" + RawFileName;
        if (!SynthDir.mkdir(SimFilesDir)) { qDebug() << "Directory Not Created! " << endl; }
        SynthDir.setCurrent(SimFilesDir);

        //Add copy of anatomy file to the
        //new configuration files directory
        if (!AnatomyFile.copy(AnatomyFile.fileName().section('/',-1))) {
                OutputWin::warning(0, "Anatomy file has not been copied to new configuration files directory! \n");
        }

        OutputWin::message("\n\nSynthesizing Network . . . \n");
        if (rdoSingle->isChecked())
        {
                int ChipNum = txtChipNum->text().toInt();

                //int ChipNum = 2;// single Chip C
                if (!ChipUtils::SynthSingle(ChipNum))
                {
                        OutputWin::error(0, "Cannot place all Groups on a single Chip.");
                        return;
                }
        }
        else if (rdoFileConfig->isChecked())
        {
                /* !Need Browse Button For This! */
                SynthDir.setCurrent(AppHomeDir);
                SynthDir.setCurrent("SPP2_Chip_Configuration_Files_Repository");
                ChipUtils::CustomChipConfig("ChipConfig.txt");
        }

        SynthDir.setCurrent(SimFilesDir);
        /////////////////////////////////
        // set first, do in other class?
        ChipUtils::SetChipCellAddrs();
        ChipUtils::SetOCIC_SRAM_Addrs();
        //////////////////////////////////
        for (int i = 0; i < NUM_CHIPS; ++i)
        {
                if (!Chips[i]->InitSPP2Files())
                {
                        OutputWin::error(0, "The Neuro-Anatomy Synthesizer has exited.");
                        return;
                }

                if (!Chips[i]->WriteSPP2Files())
                {
                        OutputWin::error(0, "The Neuro-Anatomy Synthesizer has exited.");
                        return;
                }
                Chips[i]->ClearSPP2Files();
        }

        /* Write Auxilary Files */
        SPP2AuxFiles::WriteSensFile();
        SPP2AuxFiles::WriteActFile();
        SPP2AuxFiles::WriteChipConfig();
        SPP2AuxFiles::WriteNetStats();
        SPP2AuxFiles::WriteNumElems();
}

void AdvWin::on_btnClear_clicked()
{
        ChipUtils::ClearChips();

        for (MacroLstIter it = MacroCxns.begin();
                        it != MacroCxns.end(); ++it)
        {
                delete *it;
        }
        MacroCxns.clear();

        for (GrpLstIter it = Groups.begin();
                        it != Groups.end(); ++it)
        {
                delete *it;
        }
        Groups.clear();

        ClearDisplays();
        FileLoaded = false;
}

void AdvWin::ClearDisplays()
{
        //Chip Space Left Displays
        //Constraints Window
        txtNumChips->setText(QString::number(NUM_CHIPS));
        txtMaxOn->setText(QString::number(MAX_ON_ELEMS_PER_CHIP));
        txtMaxOff->setText(QString::number(MAX_OFF_ELEMS_PER_CHIP));
        txtMaxSens->setText(QString::number(MAX_SENS_ELEMS_PER_CHIP));

        //Chip Space Left Displays
        for (int i = 0; i < NUM_CHIPS; ++i)
        {
                OnLeft[i]->setText(QString::number(Chips[i]->GetOnLeft()));
                OffLeft[i]->setText(QString::number(Chips[i]->GetOffLeft()));
                SensLeft[i]->setText(QString::number(Chips[i]->GetSensLeft()));
        }

        //clear chip containers tables & reset num rows
        QStringList labels1;
        labels1 << "On-Group" << "Off-Group" << "Sensor-Group";
        QStringList labels2;
        labels2 << "Group" << "Type" << "# Cxns" << "# Plast";
        for (int i = 0; i < NUM_CHIPS; ++i)
        {
                ChipTbls[i]->clear();
                ChipTbls[i]->setHorizontalHeaderLabels(labels1);
                ChipTbls[i]->setRowCount(10);
        }

        tblGrps->clear();
        tblGrps->setHorizontalHeaderLabels(labels2);
        tblGrps->setRowCount(0);

        //Init Stats
        txtTotElems->setText("0");
        txtTotCxns->setText("0");
        txtTotPlast->setText("0");
        txtAvgCxns->setText("0");
        txtAvgPlast->setText("0");
}

void AdvWin::on_btnBrowse_clicked()
{
        QString filename = QFileDialog::getOpenFileName(this, "Select an Anatomy File", "SPP2_Network_Definition_Files_Repository");
        if (filename.size() > 0) {
                txtFile->setText(filename);
        }
}

void AdvWin::on_btnLoad_clicked()
{
        if (FileLoaded) {
                OutputWin::message("Network already loaded. Clear current network to load a new one.");
                return;
        }

        OutputWin::message("Loading Network. . .");
        AnatomyFile.setFileName(txtFile->text());
        if (!AnatomyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        OutputWin::error(0, "Cannot open file for writing: "
                              + AnatomyFile.errorString());
                        return;
        }
        Network::ProcessFile(&AnatomyFile);

        //Set Stats
        txtTotElems->setText(QString::number(Network::TotRegCells()));
        txtTotCxns->setText(QString::number(Network::TotalCxns()));
        txtTotPlast->setText(QString::number(Network::TotalPlast()));
        txtAvgCxns->setText(QString::number(Network::AvgCxns()));
        txtAvgPlast->setText(QString::number(Network::AvgPlast()));

        for (list<Group*>::iterator it = Groups.begin();
                        it != Groups.end(); ++it)
        {
                int row = tblGrps->rowCount();

                tblGrps->setRowCount(tblGrps->rowCount()+1);
                QTableWidgetItem *newItem = new QTableWidgetItem(tr("%1").arg(QString::fromStdString((*it)->GetName())));
                tblGrps->setItem(row,0,newItem);

                QString type = "regular";
                if ((*it)->isSens()) {
                        type = "sensor";
                }
                newItem = new QTableWidgetItem(tr("%1").arg(type));
                tblGrps->setItem(row, 1,newItem);

                newItem = new QTableWidgetItem(tr("%1").arg(QString::number((*it)->GetNumInCxns())));
                tblGrps->setItem(row,2,newItem);

                newItem = new QTableWidgetItem(tr("%1").arg(QString::number((*it)->GetNumPlast())));
                tblGrps->setItem(row,3,newItem);
        }
        FileLoaded = true;
        AnatomyFile.close();
}

void AdvWin::on_btnAdd_clicked()
{
        if (txtAddRem->text().isEmpty())
        {
                OutputWin::warning(0, "Please set Group Name");
                return;
        }
        else
        {
                int ChipNum = ChipSel.checkedId();
                int StorType = StorSel.checkedId();

                Group* currGrp = Network::GetGrp(txtAddRem->text());
                if (currGrp == NULL) {
                        OutputWin::warning(0, "Group does not exist!");
                        return;
                }

                switch(StorType)
                {
                        case 0://On-Group placement
                                if (!Chips[ChipNum]->AddOnGrp(currGrp))
                                {
                                        OutputWin::message("On-Group Not Added");
                                        return;
                                }
                                OnLeft[ChipNum]->setText(QString::number(Chips[ChipNum]->GetOnLeft()));
                                currGrp->setHomeChip(Chips[ChipNum]);
                                break;
                        case 1://Off-Group placement
                                if (!Chips[ChipNum]->AddOffGrp(currGrp))
                                {
                                        OutputWin::message("Off-Group Not Added");
                                        return;
                                }
                                OffLeft[ChipNum]->setText(QString::number(Chips[ChipNum]->GetOffLeft()));
                                break;
                        case 2://Sens-group
                                if (!Chips[ChipNum]->AddSensGrp(currGrp))
                                {
                                        OutputWin::message("Sensor Group Not Added");
                                        return;
                                }
                                SensLeft[ChipNum]->setText(QString::number(Chips[ChipNum]->GetSensLeft()));
                                currGrp->setHomeChip(Chips[ChipNum]);
                                break;
                        default:
                                break;
                }
                int found = false;
                int row = ChipTbls[ChipNum]->rowCount();
                for (int i = 0; i < row; ++i)
                {
                        if (ChipTbls[ChipNum]->item(i,StorType) == 0)
                        {
                                QTableWidgetItem *newItem = new QTableWidgetItem(tr("%1").arg(QString::fromStdString(currGrp->GetName())));
                                ChipTbls[ChipNum]->setItem(i,StorType,newItem);
                                found = true;
                                break;
                        }
                }
                if (!found)
                {
                        ChipTbls[ChipNum]->setRowCount(row+1);
                        QTableWidgetItem *newItem = new QTableWidgetItem(tr("%1").arg(QString::fromStdString(currGrp->GetName())));
                        ChipTbls[ChipNum]->setItem(row,StorType,newItem);
                }
                OutputWin::message("Group added to Chip " + Chips[ChipNum]->GetaID());
        }
}

void AdvWin::on_btnRem_clicked()
{
        if (txtAddRem->text().isEmpty())
        {
                OutputWin::warning(0, "Please set Group Name");
                return;
        }
        else
        {
                int ChipNum = ChipSel.checkedId();
                int StorType = StorSel.checkedId();

                Group* currGrp = Network::GetGrp(txtAddRem->text());
                if (currGrp == NULL) {
                        OutputWin::warning(0, "Group does not exist!");
                        return;
                }
                bool found = false;
                QTableWidgetItem* item;
                int cnt = ChipTbls[ChipNum]->rowCount();
                for (int i = (cnt-1); i >= 0; --i)
                {
                        item = ChipTbls[ChipNum]->item(i,StorType);
                        if (item != 0) //if item exists at location
                        {
                                if (item->text().toLower().compare(txtAddRem->text().toLower()) == 0)
                                {
                                        found = true;
                                        delete item;
                                        //Chips[ChipNum]->AutoRemGrp(Network::GetGrp(txtAddRem->text()));

                                        switch(StorType)
                                        {
                                                case 0://On-Chip Removal
                                                        Chips[ChipNum]->RemOnGrp(Network::GetGrp(txtAddRem->text()));
                                                        OnLeft[ChipNum]->setText(QString::number(Chips[ChipNum]->GetOnLeft()));
                                                        break;
                                                case 1:// Off-Chip Removal
                                                        Chips[ChipNum]->RemOffGrp(Network::GetGrp(txtAddRem->text()));
                                                        OffLeft[ChipNum]->setText(QString::number(Chips[ChipNum]->GetOffLeft()));
                                                        break;
                                                case 2://Sensor Removal
                                                        Chips[ChipNum]->RemSensGrp(Network::GetGrp(txtAddRem->text()));
                                                        SensLeft[ChipNum]->setText(QString::number(Chips[ChipNum]->GetSensLeft()));
                                                        break;
                                                default:
                                                        break;
                                        }
                                        break;//leave loop
                                }//end if == 0
                        }//end if1 != 0
                }//end for
                if (!found) {
                        OutputWin::message("Group does not exist");
                }
                else{
                        OutputWin::message("Group has been removed");
                }
        }
}