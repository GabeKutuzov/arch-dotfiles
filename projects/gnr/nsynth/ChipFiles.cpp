#include <algorithm>

#include "ChipFiles.h"
#include "OutputWin.h"
#include "TypeDefs.h"
#include "SimDefs.h"
#include "RndNums.h"
#include "Network.h"
#include "SPP2Globals.h"

/******* Special Defines *******/
// print debug files along with binary files
#define DEBUG_FILE
#define DEBUG_COMMENT
//#define VERBOSE

#define SPECIAL_INPUTS_DATA_TRAILER
#ifdef SPECIAL_INPUTS_DATA_TRAILER
        int Special_Inputs = 4;
#endif
//#define       ZERO_BURST

/* Initialize File Names */
QString ChipFiles::Prefix = "Chip";
QString ChipFiles::Tables::TanhFileName = "_TblTanh";
QString ChipFiles::Tables::OCICFileName = "_TblOCIC";
QString ChipFiles::SDRAM::FileName = "_MemData";

//to be moved appropriately*?
int shftB1 = 8;
int shftB2 = 16;
int shftB3 = 24;

ChipFiles::ChipFiles(Chip* ichip)
{
        m_chip = ichip;

        tables = new Tables(m_chip);
        sdram = new SDRAM(m_chip);
}

QString ChipFiles::FormatAsFullWord(uword data)
{
        QString result(QString::number(data,16));
        uword WordValue = data;
        uword NibbleMask = 0xF0000000;
        uword NibbleWidth = 4;

        if (WordValue == 0)
        {
                result = "00000000";
                return result;
        }
        else
        {
                int numNibbles = 8;
                for (int i = 0; i < numNibbles; ++i)
                {
                        if (((NibbleMask & WordValue) == 0))
                        {
                                result.prepend("0");
                        }
                        else
                        {
                                break;
                        }
                        NibbleMask = NibbleMask >> NibbleWidth;
                }
                return result;
        }
}

void ChipFiles::FileZeroBurst(QFile& iFile, int iZERO_BYTES)
{
        const int BYTE_BURST = 1000000;
        char data[BYTE_BURST];

        int BURSTS = iZERO_BYTES/BYTE_BURST;
        for (int i = 0; i < BURSTS; ++i)
        {
                memset(data, 0x00, BYTE_BURST);
                iFile.write(data, BYTE_BURST);
        }

        int LEFT_OVER = iZERO_BYTES%BYTE_BURST;
        memset(data, 0x00, LEFT_OVER);
        iFile.write(data, LEFT_OVER);
}

bool ChipFiles::SetAllData()
{
        OutputWin::message("Setting Chip-" + m_chip->GetalphaID() + " data. . .");
        if (!tables->SetData() || !sdram->SetSynData())
        {
                ClearAllData();
                return false;
        }
        else
        {
                OutputWin::message("\nFinished setting Chip-" + m_chip->GetalphaID() + " data!");
                return true;
        }
}

bool ChipFiles::WriteFiles()
{
        OutputWin::message("Writing Chip-" + m_chip->GetalphaID() + " files. . .");
        if (!tables->WriteFiles() || !sdram->WriteFiles())

        {
                ClearAllData();
                return false;
        }
        else
        {
                OutputWin::message("\nFinished writing all Chip-" + m_chip->GetalphaID() + " files! ");
                return true;
        }
}

void ChipFiles::ClearAllData()
{
        tables->ClearData();
        sdram->ClearData();
}

bool ChipFiles::SDRAM::WriteFiles()
{
        return WriteData();
}

bool ChipFiles::Tables::SetData()
{
        /* Preserve Ordering */
        if (!SetOCIC())
        {
                return false;
        }
        else{
                return true;
        }
}

bool ChipFiles::Tables::WriteFiles()
{
        if (!WriteOCIC())
        {
                return false;
        }
        else
        {
                return true;
        }
}

void ChipFiles::Tables::ClearData()
{
        OCICWDs.clear();
}

void ChipFiles::SDRAM::ClearData()
{
        SynWDs.clear();
}

/**
 * Sets up Synaptic params and associated ID's. Does
 * not ID Params that have already been ID'd, i.e doubles.
 */
bool ChipFiles::SDRAM::SetSynData()
{
#ifdef VERBOSE
                OutputWin::message("Setting Synaptic Coefficient Data. . .");
#endif

        bool found = false;

        int GrpID = 0;
        MacroLst UniqSynRecs;
        GrpLst grps = chip->GetOnGrps();
        for (GrpLstIter it1 = grps.begin(); it1 != grps.end(); ++it1)
        {
                //Set Grp ID's while we're here
                (*it1)->SetOnChipID(GrpID);
                ++GrpID;
                MacroLst InCxns = (*it1)->GetInCxns();
                for (MacroLstIter it2 = InCxns.begin(); it2 != InCxns.end(); ++it2)
                {
                        found = false;
                        for (MacroLstIter it3 = UniqSynRecs.begin(); it3 != UniqSynRecs.end(); ++it3)
                        {
                                /* Compare Plastic Synaptic Coefficients */
                                if ((*it2)->isPlastic())
                                {
                                        /* Format 1: Darwin 7 Plasticity(BCM terms) */
                                        if ((*it2)->isDarwin7_Plasticity() && (*it3)->isDarwin7_Plasticity())
                                        {
                                                if (((*it2)->GetPlastWmax().GetFxdVal() == (*it3)->GetPlastWmax().GetFxdVal())
                                                        && ((*it2)->GetLearningRate().GetFxdVal() == (*it3)->GetLearningRate().GetFxdVal())
                                                        && ((*it2)->GetDecay().GetFxdVal() == (*it3)->GetDecay().GetFxdVal())
                                                        && ((*it2)->GetTheta1().GetFxdVal() == (*it3)->GetTheta1().GetFxdVal())
                                                        && ((*it2)->GetTheta2().GetFxdVal() == (*it3)->GetTheta2().GetFxdVal())
                                                        && ((*it2)->GetK1().GetFxdVal() == (*it3)->GetK1().GetFxdVal())
                                                        && ((*it2)->GetK2().GetFxdVal() == (*it3)->GetK2().GetFxdVal())
                                                        && ((*it2)->GetOriginalWeight().GetFxdVal() == (*it3)->GetOriginalWeight().GetFxdVal()) //should I use OrigWeight for comparison here?
                                                        && ((*it2)->GetSaturation().GetIntVal() == (*it3)->GetSaturation().GetIntVal()))
                                                {
                                                        (*it2)->SetSynRecID((*it3)->GetSynRecID());
                                                        found = true;
                                                        break;
                                                }
                                        }
                                        /* Format 2: Cerebellum Trace Eligibility */
                                        else if ((*it2)->isCerebellar_ValueDependent() && (*it3)->isCerebellar_ValueDependent())
                                        {
                                                if (((*it2)->GetPlastWmax().GetFxdVal() == (*it3)->GetPlastWmax().GetFxdVal())
                                                        && ((*it2)->GetLearningRate().GetFxdVal() == (*it3)->GetLearningRate().GetFxdVal())
                                                        && ((*it2)->GetInternInit().GetIntVal() == (*it3)->GetInternInit().GetIntVal())
                                                        && ((*it2)->GetInternRate().GetIntVal() == (*it3)->GetInternRate().GetIntVal())
                                                        && ((*it2)->GetEligRate().GetFxdVal() == (*it3)->GetEligRate().GetFxdVal())
                                                        && ((*it2)->GetEligMin().GetFxdVal() == (*it3)->GetEligMin().GetFxdVal())
                                                        && ((*it2)->GetInternThresh().GetIntVal() == (*it3)->GetInternThresh().GetIntVal())
                                                        && ((*it2)->GetInputThresh().GetFxdVal() == (*it3)->GetInputThresh().GetFxdVal()))
                                                        //*** && ((*it2)->GetInfl().GetFxdVal() == (*it3)->GetInfl().GetFxdVal()))
                                                {
                                                        (*it2)->SetSynRecID((*it3)->GetSynRecID());
                                                        found = true;
                                                        break;
                                                }
                                        }
                                }// end IF Plastic

                                /* Compare Non-Plastic Synaptic Coefficients
                                else if ((*it1)->isCerebellum())
                                {
                                        if ((*it2)->GetInfl().GetFxdVal() == (*it3)->GetInfl().GetFxdVal())
                                        {
                                                (*it2)->SetSynRecID((*it3)->GetSynRecID());
                                                found = true;
                                                break;
                                        }
                                }*/
                                /* Special Test Case Scenario */
                                else if ((*it1)->isMemSpaceVerifier())
                                {
                                        if (((*it2)->GetPlastWmax().GetIntVal() == (*it3)->GetPlastWmax().GetIntVal())
                                                        && ((*it2)->GetLearningRate().GetIntVal() == (*it3)->GetLearningRate().GetIntVal())
                                                        && ((*it2)->GetDecay().GetIntVal() == (*it3)->GetDecay().GetIntVal())
                                                        && ((*it2)->GetTheta1().GetIntVal() == (*it3)->GetTheta1().GetIntVal())
                                                        && ((*it2)->GetTheta2().GetIntVal() == (*it3)->GetTheta2().GetIntVal())
                                                        && ((*it2)->GetK1().GetIntVal() == (*it3)->GetK1().GetIntVal())
                                                        && ((*it2)->GetK2().GetIntVal() == (*it3)->GetK2().GetIntVal())
                                                        && ((*it2)->GetOriginalWeight().GetIntVal() == (*it3)->GetOriginalWeight().GetIntVal())
                                                        && ((*it2)->GetSaturation().GetIntVal() == (*it3)->GetSaturation().GetIntVal()))
                                                {
                                                        (*it2)->SetSynRecID((*it3)->GetSynRecID());
                                                        found = true;
                                                        break;
                                                }
                                }
                        }// end Search Loop

                        if (!found)
                        {
                                if ((*it1)->isMemSpaceVerifier())
                                {
                                        quint32 dataWrd = 0;

                                        /* Preserve Ordering! */
                                        /* Word 0 */
                                        dataWrd = ((*it2)->GetDecay().GetIntVal() << shftB2) | (*it2)->GetLearningRate().GetIntVal();
                                        SynWDs.push_back(dataWrd);

                                        /* Word 1 */
                                        dataWrd =  ((*it2)->GetTheta2().GetIntVal() << shftB2) | (*it2)->GetTheta1().GetIntVal();
                                        SynWDs.push_back(dataWrd);

                                        /* Word 2 */
                                        dataWrd = ((*it2)->GetK2().GetIntVal() << shftB2) | (*it2)->GetK1().GetIntVal();
                                        SynWDs.push_back(dataWrd);

                                        /* Word 3 */
                                        dataWrd = ((*it2)->GetSaturation().GetIntVal() << shftB2) | (*it2)->GetOriginalWeight().GetIntVal();
                                        SynWDs.push_back(dataWrd);

                                        /* Word 4 */
                                        dataWrd = ((*it2)->GetK2SaturationQuotient().GetIntVal() << shftB2) | (*it2)->GetTheta12Avg().GetIntVal();
                                        SynWDs.push_back(dataWrd);

                                        /* Word 5 */
                                        dataWrd = (*it2)->GetPlastWmax().GetIntVal();
                                        SynWDs.push_back(dataWrd);

                                        /* Word 6, 7: Empty */
                                        /*dataWrd = 0;
                                        SynWDs.push_back(0);
                                        SynWDs.push_back(0);*/

                                        (*it2)->SetSynRecID(RecIDs);
                                        UniqSynRecs.push_back(*it2);
                                        ++RecIDs;
                                }
                                else if ((*it2)->isPlastic()) //see comment below || (*it1)->isCerebellum()))
                                {
                                        /* Format 1: Darwin 7 BCM Model */
                                        if ((*it2)->isDarwin7_Plasticity())
                                        {
                                                quint32 dataWrd = 0;

                                                /* Preserve Ordering! */
                                                /* Word 0 */
                                                dataWrd = ((*it2)->GetDecay().GetFxdVal() << shftB2) | (*it2)->GetLearningRate().GetFxdVal();
                                                SynWDs.push_back(dataWrd);

                                                /* Word 1 */
                                                dataWrd =  ((*it2)->GetTheta2().GetFxdVal() << shftB2) | (*it2)->GetTheta1().GetFxdVal();
                                                SynWDs.push_back(dataWrd);

                                                /* Word 2 */
                                                dataWrd = ((*it2)->GetK2().GetFxdVal() << shftB2) | (*it2)->GetK1().GetFxdVal();
                                                SynWDs.push_back(dataWrd);

                                                /* Word 3 */
                                                dataWrd = ((*it2)->GetSaturation().GetFxdVal() << shftB2) | (*it2)->GetOriginalWeight().GetFxdVal();
                                                SynWDs.push_back(dataWrd);

                                                /* Word 4 */
                                                dataWrd = ((*it2)->GetK2SaturationQuotient().GetFxdVal() << shftB2) | (*it2)->GetTheta12Avg().GetFxdVal();
                                                SynWDs.push_back(dataWrd);

                                                /* Word 5 */
                                                dataWrd = (*it2)->GetPlastWmax().GetFxdVal();
                                                SynWDs.push_back(dataWrd);

                                                /* Word 6, 7: Empty */
                                                /*dataWrd = 0;
                                                SynWDs.push_back(0);
                                                SynWDs.push_back(0);*/

                                                (*it2)->SetSynRecID(RecIDs);
                                                UniqSynRecs.push_back(*it2);
                                                ++RecIDs;
                                        }
                                        /* Format 2: Cerebellar Model */
                                        else if ((*it2)->isCerebellar_ValueDependent()) // we currently don't need non-plastic sets || (*it1)->isCerebellum())
                                        {
                                                uword dataWrd = 0;

                                                /* Preserve Ordering! */
                                                /* Word 0 */
                                                dataWrd =  ((uword)(*it2)->GetInternInit().GetIntVal() << shftB2) | (uword)(*it2)->GetLearningRate().GetFxdVal();
                                                SynWDs.push_back(dataWrd);

                                                /* Word 1 */
                                                dataWrd = ((uword)(*it2)->GetEligRate().GetFxdVal() << shftB2) | (uword)(*it2)->GetInternRate().GetIntVal();
                                                SynWDs.push_back(dataWrd);

                                                /* Word 2 */
                                                dataWrd = ((uword)(*it2)->GetInternThresh().GetIntVal() << shftB2) | (uword)(*it2)->GetEligMin().GetFxdVal();
                                                SynWDs.push_back(dataWrd);

                                                /* Word 3 */
                                                dataWrd = ((uword)(*it2)->GetInfl().GetFxdVal() << shftB2) | (uword)(*it2)->GetInputThresh().GetFxdVal();
                                                SynWDs.push_back(dataWrd);

                                                /* Word 4 */
                                                dataWrd = (uword)(*it2)->GetPlastWmax().GetFxdVal();
                                                SynWDs.push_back(dataWrd);

                                                /* Word 5, 6, 7: Empty */
                                                /*dataWrd = 0;
                                                SynWDs.push_back(dataWrd);
                                                SynWDs.push_back(dataWrd);
                                                SynWDs.push_back(dataWrd);*/

                                                (*it2)->SetSynRecID(RecIDs);
                                                UniqSynRecs.push_back(*it2);
                                                ++RecIDs;
                                        }
                                }//else if plastic
                        }// IF Not Found

                }//end In Connections Loop
        }//end Groups Loop

#ifdef VERBOSE
                OutputWin::message("Finished setting Synaptic Coefficient Data!");
#endif

        return true;
}

bool ChipFiles::Tables::SetTanh()
{
#ifdef VERBOSE
                OutputWin::message("Setting Tanh data. . .");
#endif
        /* tanh values calculated in spread-sheet */
        //format obsolete !!!
        {
                tanhWDs.push_back(0x07050300);
                tanhWDs.push_back(0x0F0D0B09);
                tanhWDs.push_back(0x17151311);
                tanhWDs.push_back(0x1F1D1B19);
                tanhWDs.push_back(0x27252321);
                tanhWDs.push_back(0x2F2D2B29);
                tanhWDs.push_back(0x37353331);
                tanhWDs.push_back(0x3E3C3A39);
                tanhWDs.push_back(0x46444240);
                tanhWDs.push_back(0x4D4B4948);
                tanhWDs.push_back(0x5452514F);
                tanhWDs.push_back(0x5B595856);
                tanhWDs.push_back(0x62605F5D);
                tanhWDs.push_back(0x69676664);
                tanhWDs.push_back(0x6F6E6C6B);
                tanhWDs.push_back(0x76747371);
                tanhWDs.push_back(0x7C7A7977);
                tanhWDs.push_back(0x82817F7E);
                tanhWDs.push_back(0x88868583);
                tanhWDs.push_back(0x8D8C8B89);
                tanhWDs.push_back(0x9392908F);
                tanhWDs.push_back(0x98979694);
                tanhWDs.push_back(0x9D9C9B99);
                tanhWDs.push_back(0xA2A1A09E);
                tanhWDs.push_back(0xA7A6A4A3);
                tanhWDs.push_back(0xABAAA9A8);
                tanhWDs.push_back(0xB0AFADAC);
                tanhWDs.push_back(0xB4B3B2B1);
                tanhWDs.push_back(0xB8B7B6B5);
                tanhWDs.push_back(0xBBBABAB9);
                tanhWDs.push_back(0xBFBEBDBC);
                tanhWDs.push_back(0xC2C2C1C0);
                tanhWDs.push_back(0xC6C5C4C3);
                tanhWDs.push_back(0xC9C8C7C7);
                tanhWDs.push_back(0xCCCBCACA);
                tanhWDs.push_back(0xCFCECDCD);
                tanhWDs.push_back(0xD1D1D0CF);
                tanhWDs.push_back(0xD4D3D3D2);
                tanhWDs.push_back(0xD6D6D5D5);
                tanhWDs.push_back(0xD9D8D8D7);
                tanhWDs.push_back(0xDBDADAD9);
                tanhWDs.push_back(0xDDDCDCDB);
                tanhWDs.push_back(0xDFDEDEDD);
                tanhWDs.push_back(0xE1E0E0DF);
                tanhWDs.push_back(0xE2E2E2E1);
                tanhWDs.push_back(0xE4E4E3E3);
                tanhWDs.push_back(0xE6E5E5E5);
                tanhWDs.push_back(0xE7E7E6E6);
                tanhWDs.push_back(0xE9E8E8E8);
                tanhWDs.push_back(0xEAEAE9E9);
                tanhWDs.push_back(0xEBEBEBEA);
                tanhWDs.push_back(0xECECECEC);
                tanhWDs.push_back(0xEEEDEDED);
                tanhWDs.push_back(0xEFEEEEEE);
                tanhWDs.push_back(0xF0EFEFEF);
                tanhWDs.push_back(0xF0F0F0F0);
                tanhWDs.push_back(0xF1F1F1F1);
                tanhWDs.push_back(0xF2F2F2F2);
                tanhWDs.push_back(0xF3F3F3F2);
                tanhWDs.push_back(0xF4F4F3F3);
                tanhWDs.push_back(0xF4F4F4F4);
                tanhWDs.push_back(0xF5F5F5F5);
                tanhWDs.push_back(0xF6F6F5F5);
                tanhWDs.push_back(0xF6F6F6F6);
        }
#ifdef VERBOSE
                OutputWin::message("Finished setting Tanh data.");
#endif

        return true;
}

bool ChipFiles::Tables::WriteTanh()
{
        QString binName = ChipFiles::Prefix + chip->GetalphaID() + TanhFileName +".bin";
#ifdef VERBOSE
                OutputWin::message("Writing " + binName + " . . .");
#endif

        QFile File(binName);
        if (!File.open(QIODevice::WriteOnly))
        {
                OutputWin::error(0, "Cannot open file for writing: "
                             + File.errorString());
                return false;
        }
        QDataStream out(&File);
        out.setVersion(QDataStream::Qt_4_1);
        out.setByteOrder(QDataStream::LittleEndian);/* ! LITTLE ENDIAN ! */

#ifdef DEBUG_FILE
                QString txtName = ChipFiles::Prefix + chip->GetaID() + TanhFileName + ".txt";
                QFile txtFile(txtName);
                if (!txtFile.open(QIODevice::WriteOnly))
                {
                        OutputWin::error(0, "Cannot open file for writing: "
                                        + txtFile.errorString());
                        return false;
                }
                QTextStream txtout(&txtFile);
        int cnt = 0;
#endif

        //write Group Coeffs First
        for (uWordVecIter it = tanhWDs.begin();it != tanhWDs.end(); ++ it)
        {
                out << *it;
#ifdef  DEBUG_FILE
                        txtout << FormatAsFullWord(*it) << endl;
#endif
        }

#ifdef ZERO_BURST
                int ZERO_BYTES = TANH_BYTES - (int)tanhWDs.size()*BYTEWIDTH;
#ifdef DEBUG_FILE
                        if (ZERO_BYTES > 0) {
                                txtout << "; <---- No more meaningful data. Rest of memory has been filled w/ Zero's ---->" ;
                        }
#endif

                FileZeroBurst(File, ZERO_BYTES);

#endif

#ifdef VERBOSE
                OutputWin::message("Finished writing " + binName + ".");
#endif

        return true;
}

bool ChipFiles::Tables::SetOCIC()
{
        GrpLst grps = chip->GetOffGrps();
        for (GrpLstIter it1 = grps.begin(); it1 != grps.end(); ++it1)
        {
                CellVec cells = (*it1)->GetCells();
                for (CellVecIter it2 = cells.begin(); it2 != cells.end(); ++it2)
                {
                        OCICWDs.push_back((*it2)->GetOCIC_SRAM_Addr());

                        /*pair<Chip*, quint16> AddrMap = (*it2)->GetAddr0Map();
                        if ((AddrMap.first == NULL) || (AddrMap.second < 0))
                        {
                                OutputWin::error(0, "OCIC address missing.");
                                return false;
                        }
                        if (AddrMap.first->GetID() == 0)
                        {
                                quint32 addr = (quint32)AddrMap.second | ACHIPMASK;
                                OCICWDs.push_back(addr);
                        }
                        else if (AddrMap.first->GetID() == 1)
                        {
                                quint32 addr = (quint32)AddrMap.second | BCHIPMASK;
                                OCICWDs.push_back(addr);
                        }
                        else if (AddrMap.first->GetID() == 2)
                        {
                                quint32 addr = (quint32)AddrMap.second | CCHIPMASK;
                                OCICWDs.push_back(addr);
                        }*/
                }
        }

        /* Sort addresses in ascending order */
        OCICWDs.sort();

        return true;
}

bool ChipFiles::Tables::WriteOCIC()
{
        QString binName = ChipFiles::Prefix + QString::number(chip->GetID()+1) + OCICFileName + ".bin";
#ifdef VERBOSE
                OutputWin::message("Writing " + binName + " . . .");
#endif
        QFile File(binName);
        if (!File.open(QIODevice::WriteOnly))
        {
                OutputWin::error(0, "Cannot open file for writing: "
                             + File.errorString());
                return false;
        }
        QDataStream out(&File);
        out.setVersion(QDataStream::Qt_4_1);
        out.setByteOrder(QDataStream::LittleEndian);// ! LITTLE ENDIAN BYTE ORDER !

#ifdef DEBUG_FILE
                QString txtName = ChipFiles::Prefix + QString::number(chip->GetID()+1) + OCICFileName + ".txt";
                QFile txtFile(txtName);
                if (!txtFile.open(QIODevice::WriteOnly))
                {
                        OutputWin::error(0, "Cannot open file for writing: "
                                        + txtFile.errorString());
                        return false;
                }
                QTextStream txtout(&txtFile);
                int SHIFT = 8;
#endif

        for (uWordLstIter it = OCICWDs.begin(); it != OCICWDs.end(); ++it)
        {
                out << *it;

#ifdef DEBUG_FILE
                        txtout << FormatAsFullWord(*it) << endl;
#endif
        }

        int ZERO_BYTES = ADDRS_BYTES*(MAX_OFF_ELEMS_PER_CHIP - (int)OCICWDs.size());
#ifdef DEBUG_FILE
                        if (ZERO_BYTES > 0) {
                                txtout << "; <---- Rest of data filled w/ Zero's ---->";
                        }
#endif

        FileZeroBurst(File, ZERO_BYTES);

#ifdef VERBOSE
                OutputWin::message("Finished writing " + binName + ".");
#endif

        return true;
}

bool ChipFiles::SDRAM::WriteData() {
#ifdef VERBOSE
   OutputWin::message("Setting SDRAM data. . .");
#endif

   /* Initialize Binary & Text Files */
   QString binName = ChipFiles::Prefix + QString::number(chip->GetID()+1) + FileName + ".bin";
#ifdef VERBOSE
   OutputWin::message("Writing " + binName + " . . .");
#endif

   QFile binFile(binName);
   if (!binFile.open(QIODevice::WriteOnly)) {
      OutputWin::error(0, "Cannot open file for writing: "
         + binFile.errorString());
      return false;
      }
   QDataStream binOut(&binFile);
   binOut.setVersion(QDataStream::Qt_4_3);
   binOut.setByteOrder(QDataStream::LittleEndian);/* Little Endian */

#ifdef DEBUG_FILE
   QString txtName = ChipFiles::Prefix + QString::number(chip->GetID()+1) + FileName + ".txt";
   QFile txtFile(txtName);
   if (!txtFile.open(QIODevice::WriteOnly)) {
      OutputWin::error(0, "Cannot open file for writing: "
         + txtFile.errorString());
      return false;
      }
   QTextStream txtOut(&txtFile);
#endif

   //#define PTR_DEBUG
#ifdef PTR_DEBUG
   QString ptrFileName = "Pointers_Debugger_Chip" + chip->GetalphaID() + ".txt";
   QFile ptrFile(ptrFileName);
   if (!ptrFile.open(QIODevice::WriteOnly)) {
      OutputWin::error(0, "Cannot open file for writing: " +
         ptrFile.errorString());
      return false;
      }
   QTextStream debugOut(&ptrFile);
#endif

   /* Local Parameters */
   int CurrentRecord_WordCnt = 0;
   int CurrentWordAddress = 0;
   int CurrentPos = 0;
   int CurrentFileSz = 0;
   int PreviousFileSz = 0;
   int BoundaryWords = 0;
   int SpecialPaddingWords = 4;
   uword dataWord = 0;
   int cxnCnt = 0;
   MicroLst MicroCxns, microSens, microPlainVI, microCoinc, microVD;
   MicroLstIter iterMicroCxn;
   MacroLstIter iterMacroCxn;
   MacroLst MacroCxns;

   // Element Header
   uword QtyInputs = 0, QtySens = 0, QtyCoinc = 0, QtyVI = 0;
   uword Param0 = 0, Param1 = 0, Param2 = 0, Param3 = 0;
   uword ElementCnt = 0, headerWord = 0;
   uword GrpID = 0;
   int WrdsNotSent = 4;
   uword RecordLength = 0;
   int posWord2 = 0;
   uword PointerWord = 0;
   uword PointerStart = 0;
   uword PointerEnd = 0;
   int posWord3 = 0;
   uword addrSDRAMWB = 0;
   int posWord4 = 0;
   uword WBRangeWord = 0;
   uword addrWBStart = 0;
   uword addrWBEnd = 0;
   int posWord5 = 0;
   uword wordAddr0 = 0;
   uword wordAddr1 = 0;
   uword BRAMWrd0 = 0;
   int posWord7 = 0;
   uword wordAddr2 = 0;
   uword wordAddr3 = 0;
   uword BRAMWrd1 = 0;
   int posWord8 = 0;
   uword wordAddr4 = 0;
   uword wordAddr5 = 0;
   uword BRAMWrd2 = 0;
   int posWord9 = 0;
   uword wordAddrThisCell = 0;
   uword ExtraWrd0 = 0;
   int posWord12 = 0;

#ifdef DEBUG_FILE
   int posWord2_TXT = 0;
   int posWord3_TXT = 0;
   int posWord4_TXT = 0;
   int posWord5_TXT = 0;
   int posWord6_TXT = 0;
   int posWord7_TXT = 0;
   int posWord8_TXT = 0;
   int posWord9_TXT = 0;
   int posWord12_TXT = 0;
   int CurrentPos_TXT = 0;
#endif

   // Synaptic Coefficent Sets
   int WORDS_PER_SET = 6; //changed from 8 to 6, last two words were nulls
   int RecID = 0;
   list<int> RecIDLst;
   int inx = 0;

   // Synaptic Descriptors
   int ID_RecInx = 0;
   uword sdtrWord = 0, tempDtr = 0;
   uword plastBitSet = 0x80;       // Set Plastic Bit
   uword vdBitSet    = 0x40;       // Set Value Dependent Bit
   uword coincBitSet = 0x20;       // Set Coincident Bit

   // Synaptic Weights
   uword weightTemp = 0;
   uword weightWord = 0;

   // Input Pointers
   CellLst InputCells, cellsSens, cellsPlainVI, cellsCoinc, cellsVD;
   CellLstIter iterCell;
   uword tempPointer = 0, pointerWord = 0;

   // Supplementals
   int NumInitBytes = 0;
   int NumInitWords = 0;
   int WordPadBurst = 0;

   /* Iterate through all On-Chip Elements */
   GrpLst grps = chip->GetOnGrps();
   for (GrpLstIter it1 = grps.begin(); it1 != grps.end(); ++it1) {
      CellVec cells = (*it1)->GetCells();
      for (CellVecIter it2 = cells.begin(); it2 != cells.end(); ++it2) {
         /* Start New Element Record */
         PreviousFileSz = binFile.size();

         /* Create Input Cells List */
         InputCells.clear();
         cellsSens.clear();
         cellsPlainVI.clear();
         cellsCoinc.clear();
         cellsVD.clear();
         cellsSens = (*it2)->GetSensIn();
         cellsPlainVI = (*it2)->GetPlainVIIn();
         cellsCoinc = (*it2)->GetCoincIn();
         cellsVD = (*it2)->GetVDIn();
         iterCell = InputCells.begin();
         /* ==================== Preserve Ordering ==================== */
         InputCells.insert(iterCell, cellsSens.begin(), cellsSens.end());
         InputCells.insert(iterCell, cellsPlainVI.begin(), cellsPlainVI.end());
         InputCells.insert(iterCell, cellsCoinc.begin(), cellsCoinc.end());
         InputCells.insert(iterCell, cellsVD.begin(), cellsVD.end());
         /* =========================================================== */

         /* Create Input-Connections List */
         MicroCxns.clear();
         microSens.clear();
         microPlainVI.clear();
         microCoinc.clear();
         microVD.clear();
         microSens = (*it2)->GetSens();
         microPlainVI = (*it2)->GetPlainVI();
         microCoinc = (*it2)->GetCoinc();
         microVD = (*it2)->GetVD();
         iterMicroCxn = MicroCxns.begin();
         /* ==================== Preserve Ordering ==================== */
         MicroCxns.insert(iterMicroCxn, microSens.begin(), microSens.end());
         MicroCxns.insert(iterMicroCxn, microPlainVI.begin(), microPlainVI.end());
         MicroCxns.insert(iterMicroCxn, microCoinc.begin(), microCoinc.end());
         MicroCxns.insert(iterMicroCxn, microVD.begin(), microVD.end());
         /* =========================================================== */

         /*-*-*-*-*-*-*-* Set Element Header *-*-*-*-*-*-*-*/
#ifdef DEBUG_FILE
         if (ElementCnt == 0) {
            txtOut << ";  Element Header Format: " << endl;
            txtOut << ";   Word#    B3\t\t\t\tB2\t\t\t\tB1\t\t\t\tB0 "                              << endl;
            txtOut << ";   0                recMark3                        recMark2                        recMark1                        recMark0 "                      << endl;
            txtOut << ";   1                recMark7                        recMark6                        recMark5                        recMark4 "                      << endl;
            txtOut << ";   2                0\t\t\t\t0\t\t\t\trecLgnth1                     regLgnth0 "                     << endl;
            txtOut << ";   3                ptrRgnEnd0                      ptrRgnEnd0                      ptrRgnStart1            ptrRgnStart0 "          << endl;
            txtOut << ";   4                wbAddr3\t\t\twbAddr2\t\t\twbAddr1\t\t\twbAddr0 "                        << endl;
            txtOut << ";   5                wbRgnEnd1                       wbRgnEnd0                       wbRgnStart1                     wbRgnStart0 "           << endl;
            txtOut << ";   6                0\t\t\t\tSi(t)\t\t\t\tElemNum1                  ElemNum0 "                      << endl;
            txtOut << ";   7                ptrWeights(MSB)         ptrWeights(LSB)         ptrSynDtrs(MSB)         ptrSynDtrs(LSB) "       << endl;
            txtOut << ";   8                ptrSupp1(MSB)           ptrSupp1(LSB)           ptrSupp0(MSB)           ptrSupp0(LSB)   "       << endl;
            txtOut << ";   9                ptrInputs(MSB)          ptrInputs(LSB)          ptrSupp2(MSB)           ptrSupp2(LSB)   "       << endl;
            txtOut << ";   10\tqtyVI                                qtySensor                       qtyCoinc                        qtyInputs "                     << endl;
            txtOut << ";   11\tPersistence                  VD Thresh                       Firing Thresh           Scaling-Factor "        <<  endl;
            txtOut << ";   12\t0                                    0                                       ptrSi(t-1)                      ptrSi(t-1)      "<< endl;
            txtOut << ";" << endl;
            }
         txtOut << "; <---------- Element " << (*it2)->GetID() << " of Group " << QString::fromStdString((*it1)->GetName()) << " ---------->" << endl;
         txtOut << ";\t |----- Header -----|" << endl;
#endif

         /*-*-*- Word 0, 1: Record Marker -*-*-*/
         binOut << RecMarkerWord0 << RecMarkerWord1;
#ifdef DEBUG_FILE
         txtOut << FormatAsFullWord(RecMarkerWord0) << endl;
         txtOut << FormatAsFullWord(RecMarkerWord1) << endl;
#endif

         /*-*-*- Word 2: Record Length -*-*-*/
         // calculate how many records are needed
         // figure out where the Input pointers will start
         posWord2 = binFile.pos(); // 1st byte position of word 2
         binOut << (uword)0;
#ifdef DEBUG_FILE
         posWord2_TXT = txtFile.pos();
         txtOut << FormatAsFullWord((uword)0) << endl;
#endif

         /*-*-*- Word 3: Input Pointers Start/End -*-*-*/
         posWord3 = binFile.pos(); // 1st byte position of word 3
         binOut << (uword)0;
#ifdef DEBUG_FILE
         posWord3_TXT = txtFile.pos();
         txtOut << FormatAsFullWord((uword)0) << endl;
#endif

         /*-*-*- Word 4: SDRAM Write back address -*-*-*/
         // this address the SDRAM - dependent upon where writeback data is
         // size of the record and the current record that I'm on. . .
         posWord4 = binFile.pos(); // 1st byte position of word 2
         binOut << (uword)0;
#ifdef DEBUG_FILE
         posWord4_TXT = txtFile.pos();
         txtOut << FormatAsFullWord((uword)0) << endl;
#endif

         /*-*-*- Word 5: W/B Start/End Word Address -*-*-*/
         posWord5 = binFile.pos(); // 1st byte position of word 2
         binOut << (uword)0;
#ifdef DEBUG_FILE
         posWord5_TXT = txtFile.pos();
         txtOut << FormatAsFullWord((uword)0) << endl;
#endif

         /*-*-*- Word 6: Element Number -*-*-*/
         GrpID = (uword)(*it1)->GetOnChipID(); // TBD!
         headerWord = (GrpID << shftB2) | ElementCnt;
         binOut << headerWord;
#ifdef DEBUG_FILE
         txtOut << FormatAsFullWord(ElementCnt) << endl;
#endif
         ++ElementCnt;

         /*-*-*- Word 7: NPU-BRAM Pointer 0, 1 -*-*-*/
         //0: Synaptic Descriptors
         //1: Weights
         posWord7 = binFile.pos();
         binOut << (uword)0;
#ifdef DEBUG_FILE
         posWord7_TXT = txtFile.pos();
         txtOut << FormatAsFullWord((uword)0) << endl;
#endif

         /*-*-*- Word 8: NPU-BRAM Pointer 2, 3 -*-*-*/
         //2: Supplemental 0
         //3: Supplemental 1
         posWord8 = binFile.pos();
         binOut << (uword)0;
#ifdef DEBUG_FILE
         posWord8_TXT = txtFile.pos();
         txtOut << FormatAsFullWord((uword)0) << endl;
#endif

         /*-*-*- Word 9: NPU-BRAM Pointer 3, 4 -*-*-*/
         //2: Supplemental 2
         //3: Inputs
         posWord9 = binFile.pos();
         binOut << (uword)0;
#ifdef DEBUG_FILE
         posWord9_TXT = txtFile.pos();
         txtOut << FormatAsFullWord((uword)0) << endl;
#endif

         /*-*-*- Word 10: Qty Inputs, Qty Coinc, Qty Sensor, Qty VI -*-*-*/
         QtyInputs = uword((*it2)->GetNumInCxns());

         // NOTE: Output Number of connections just in case we're greater than
         // the prescribed number!  What should I do if this occurs?
         if (QtyInputs > (quint32)(MAX_SYNAPSES_PER_CELL-1)) {
            OutputWin::error(0, "Special Qty Inputs is " +
               QString::number(QtyInputs) + ", which is greater than maximum: " +
               QString::number(MAX_SYNAPSES_PER_CELL_LIMIT));
            }

         /* Special Data Initialization to select special code test cases */
         if ((*it1)->isWeightReader() || (*it1)->isWeightReadWriter()
               || (*it1)->isSensorLoopBacker()
               || (*it1)->isEpochCounter()
               || (*it1)->isMemSpaceVerifier()) {
            QtyCoinc = uword((*it1)->GetTypeCode());
            }
         else {
            QtyCoinc = uword((*it2)->GetQtyCoinc());
            }

         QtySens = uword((*it2)->GetQtySens());
         QtyVI = uword((*it2)->GetQtyVI());
         headerWord = QtyInputs | (QtyCoinc << shftB1) |
            (QtySens << shftB2) | (QtyVI << shftB3);
         binOut << headerWord;
#ifdef DEBUG_FILE
         txtOut << FormatAsFullWord(headerWord) << endl;
#endif

         /*-*-*- Words 11-14: Izhikevich cell parameters -*-*-*/
         if ((*it1)->isIzhik()) {
            /* Word 11: nct and a */
            headerWord = Network::NumCT() |
               (*it1)->GetIzhi_a() << shftB2;
            binOut << headerWord;
#ifdef DEBUG_FILE
            txtOut << FormatAsFullWord(headerWord) << endl;
#endif
            /* Word 12: b and c */
            headerWord = (*it1)->GetIzhi_b() & 0xFFFF |
               (*it1)->GetIzhi_c() << shftB2;
            binOut << headerWord;
#ifdef DEBUG_FILE
            txtOut << FormatAsFullWord(headerWord) << endl;
#endif
            /* Word 13: d and k (stored in prst field) */
            headerWord = (*it1)->GetIzhi_d() & 0xFFFF |
               (*it1)->GetPrst() << shftB2;
            binOut << headerWord;
#ifdef DEBUG_FILE
            txtOut << FormatAsFullWord(headerWord) << endl;
#endif
            /* Word 14: vT (vd_thr) and vPeak (fr_thr) */
            headerWord = (*it1)->GetVd_Thr() & 0xFFFF |
               (*it1)->GetFr_Thr() << shftB2;
            binOut << headerWord;
#ifdef DEBUG_FILE
            txtOut << FormatAsFullWord(headerWord) << endl;
#endif
            /* Word 15: spares */
            binOut << (uword)0;
#ifdef DEBUG_FILE
            txtOut << FormatAsFullWord((uword)0) << endl;
#endif
            } /* End outputting Izhikevich cell type parms */

         else {   /* All the existing cases */
            /*-*-*- Word 11: Group Parameters -*-*-*/
            if ((*it1)->isMemSpaceVerifier()) {
               Param3 = (*it1)->GetPrst().GetIntVal();
               Param2 = (*it1)->GetVd_Thr().GetIntVal();
               Param1 = (*it1)->GetFr_Thr().GetIntVal();
               Param0 = (*it1)->GetScale().GetIntVal();
               }
            else {
               Param3 = (*it1)->GetPrst().GetFxdVal();
               Param2 = (*it1)->GetVd_Thr().GetFxdVal();
               Param1 = (*it1)->GetFr_Thr().GetFxdVal();
               Param0 = (*it1)->GetScale().GetFxdVal();
               }
            headerWord  = Param0 | (Param1 << shftB1) | (Param2 << shftB2) |
               (Param3 << shftB3);
            binOut << headerWord;
#ifdef DEBUG_FILE
            txtOut << FormatAsFullWord(headerWord) << endl;
#endif

            /*-*-*- Word 12: Byte0,1 = Si(t-1) npuBRAM pointer -*-*-*/
            posWord12 = binFile.pos();
            binOut << (uword)0;
#ifdef DEBUG_FILE
            posWord12_TXT = txtFile.pos();
            txtOut << FormatAsFullWord((uword)0) << endl;
#endif
            } /* End else not Izhikevich */
         /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

         /*-*-*-*-*-*-*-* Set Synaptic Coefficients *-*-*-*-*-*-*-*/
#ifdef DEBUG_FILE
         txtOut << ";\t |----- Synaptic Coefficients -----|" << endl;
#endif
         /* Collect all the MacroCxn Record IDs */
         MacroCxns = (*it1)->GetInCxns();
         for (iterMacroCxn = MacroCxns.begin();
               iterMacroCxn != MacroCxns.end(); ++iterMacroCxn) {
            RecID = (*iterMacroCxn)->GetSynRecID();
            RecIDLst.push_back(RecID);
            }

         /* Remove repeats */
         RecIDLst.sort();
         RecIDLst.unique();

         if ((*it1)->isCerebellum()) {
            WORDS_PER_SET = 5;
            }
         else {
            WORDS_PER_SET = 6;
            }

         /* Write Relevant Synaptic Coefficient Sets to file */
         for (list<int>::iterator RecIter = RecIDLst.begin();
               RecIter != RecIDLst.end(); ++RecIter) {
            /* Iterate through each Word of the set */
            for (int i = 0; i < WORDS_PER_SET; ++i) {
               inx = (*RecIter)*WORDS_PER_SET + i;

               //WATCH OUT for cases that don't use coeff. sets
               if (SynWDs.size() > 0) {
                  binOut << SynWDs[inx];
#ifdef DEBUG_FILE
                  txtOut << FormatAsFullWord(SynWDs[inx]) << endl;
#endif
                  }
               }
            }
         /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/


         /*-*-*-*-*-*-*-* Set Synaptic Descriptors *-*-*-*-*-*-*-*/

         /* Record Staring Word Address */
         CurrentFileSz = binFile.size();
         wordAddr0 = uword((CurrentFileSz - PreviousFileSz)/WORD_BYTES);

         /** Synaptic Descriptors -- 01/16/08
            *      Synaptic Descriptor Coefficient – bit fields
            *      Bit(7)          Bit(6)          Bit(5)  Bit(4:3)        Bit (2:0)
            *      Plastic         Value Dep.      Coinc.  IO Act. #       Syn. Type #
            */
         //Note: I only need 1 IO cell, so I no longer need to set IO Act. #
#ifdef DEBUG_FILE
         txtOut << ";\t |----- Synaptic Descriptors -----|" << endl;
#endif
         cxnCnt = 0, sdtrWord = 0;
         for (iterMicroCxn = MicroCxns.begin(); iterMicroCxn != MicroCxns.end(); ++iterMicroCxn) {
            ++cxnCnt;
            tempDtr = 0;
            if ((*iterMicroCxn)->GetMacroCxn()->isPlastic()) {
               tempDtr |= plastBitSet;
               }
            if ((*iterMicroCxn)->GetMacroCxn()->isVal()) {
               tempDtr |= vdBitSet;
               }
            if ((*iterMicroCxn)->GetMacroCxn()->isCoinc()) {
               tempDtr |= coincBitSet;
               }

            // Set Synaptic Record Number to determine which Synaptic . . .
            // Coefficient Set to use while on NPU Side
            RecID = (*iterMicroCxn)->GetMacroCxn()->GetSynRecID();
            ID_RecInx = distance(RecIDLst.begin(), std::find(RecIDLst.begin(), RecIDLst.end(), RecID));
            tempDtr |= uword(ID_RecInx);

            /* Byte 0 */
            if (cxnCnt%BYTEWIDTH == 1) {
               sdtrWord |= tempDtr;
               }
            /* Byte 1 */
            else if (cxnCnt%BYTEWIDTH == 2) {
               sdtrWord |= (tempDtr << shftB1);
               }
            /* Byte 2 */
            else if (cxnCnt%BYTEWIDTH == 3) {
               sdtrWord |= (tempDtr << shftB2);
               }
            /* Byte 3 */
            else if (cxnCnt%BYTEWIDTH == 0) {
               sdtrWord |= (tempDtr << shftB3);
               binOut << sdtrWord;
#ifdef DEBUG_FILE
               txtOut << FormatAsFullWord(sdtrWord) << endl;
#endif
               sdtrWord = 0;
               }
            } //end Micro-Cxns loop

         // If CxnCnt is not divisible by a word size then
         // write word of last data set.
         if (cxnCnt%BYTEWIDTH != 0) {
            binOut << sdtrWord;
#ifdef DEBUG_FILE
            txtOut << FormatAsFullWord(sdtrWord) << endl;
#endif
            sdtrWord = 0;
            }
         /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/


         /*-*-*-*-*-*-*-* Set Synaptic Weights *-*-*-*-*-*-*-*/
         /* Make Sure Weights Start on 8-Word boundary address */
         CurrentFileSz = binFile.size();
         CurrentWordAddress = (CurrentFileSz - PreviousFileSz)/WORD_BYTES;
         BoundaryWords = CurrentWordAddress%BOUNDARY_WORD_SIZE;
         /* Pad Bundary */
         if (BoundaryWords != 0) {
            WordPadBurst = BOUNDARY_WORD_SIZE - BoundaryWords;
            FileZeroBurst(binFile, WordPadBurst*WORD_BYTES);
#ifdef DEBUG_FILE
            for (int i = 0; i < WordPadBurst; ++i) {
               txtOut << FormatAsFullWord((uword)0) << endl;
               }
#endif
            }

#ifdef DEBUG_FILE
         txtOut << ";\t |----- Synaptic Weights -----|" << endl;
#endif

         CurrentFileSz = binFile.size();
         /* Record Starting Word Address for Write-Back Data */
         addrWBStart = uword((CurrentFileSz - PreviousFileSz)/WORD_BYTES);

         /* Record Staring Word Address */
         wordAddr1 = uword((CurrentFileSz - PreviousFileSz)/WORD_BYTES);

         /* Record SDRAM W/B Addresss */
         addrSDRAMWB = (uword)CurrentFileSz;

         cxnCnt = 0;
         for (iterMicroCxn = MicroCxns.begin(); iterMicroCxn != MicroCxns.end(); ++iterMicroCxn) {
            ++cxnCnt;

            /* NPU Test Cases */
            if ((*iterMicroCxn)->GetMacroCxn()->isHippocampus() || (*it1)->isMemSpaceVerifier() ||
                  (*it1)->isWeightReadWriter() || (*it1)->isWeightReader()) {
               // special case for testing non-fixed point weights
               //weightTemp = qint32((*iterMicroCxn)->GetMacroCxn()->GetOriginalWeight().GetIntVal());
               weightTemp = ElementCnt;
               }
            else {
#ifdef USE_ORIGINAL_WGT_FOR_ALL_CXNS
               weightTemp = qint32((*iterMicroCxn)->GetMacroCxn()->GetOriginalWeight().GetFxdVal());
               // weightTemp = (ElementCnt - 1);
               //weightTemp = 0;
#else
               weightTemp = qint32((*iterMicroCxn)->GetWeight().GetFxdVal());
#endif
               }

            /* First Weight */
            if (cxnCnt%WEIGHTS_PER_WORD == 1) {
               weightWord = weightTemp;
               }
            /* Second Weight */
            else if (cxnCnt%WEIGHTS_PER_WORD == 0) {
               weightWord |= (weightTemp << shftB2);//concatenate weight 0 & 1
               binOut << weightWord;
#ifdef DEBUG_FILE
               txtOut << FormatAsFullWord(weightWord) << endl;
#endif
               }
            } // end Micro-Cxns loop

         /* Write Word to file if there is an odd number of weights */
         if (cxnCnt%WEIGHTS_PER_WORD == 1) {
            binOut << weightWord;
#ifdef DEBUG_FILE
            txtOut << FormatAsFullWord(weightWord) << endl;
#endif
            }
         /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

         /*-*-*-*-*-*-*-*-*-*-* Supplementals *-*-*-*-*-*-*-*-*-*-*/
         if ((*it2)->GetQtyPlast() > 0 && (*it1)->isCerebellum()) {
            /*-*-*-*-*-*-*-* Supplemental 0 *-*-*-*-*-*-*-*/
            /* Initialize the Supps with Zeros, the Supps are byte-oriented */
            /* Record Staring Word Address */
            CurrentFileSz = binFile.size();
            wordAddr2 = uword((CurrentFileSz - PreviousFileSz)/WORD_BYTES);
#ifdef DEBUG_FILE
            txtOut << ";\t |----- Supplement 0 -----|" << endl;
#endif
            //make sure these bursts are rounded up to the nearest word
            NumInitBytes = (WORD_BYTES - (*it2)->GetNumInCxns()%WORD_BYTES) + (*it2)->GetNumInCxns();
            FileZeroBurst(binFile, NumInitBytes);
            NumInitWords = NumInitBytes/WORD_BYTES;
#ifdef DEBUG_FILE
            for (int i = 0; i < NumInitWords ; ++i) {
               txtOut << FormatAsFullWord((uword)0) << endl;
               }
#endif

            /*-*-*-*-*-*-*-* Supplemental 1 *-*-*-*-*-*-*-*/
            /* Initialize the Supps with Zeros, the Supps are byte-oriented */
            /* Record Staring Word Address */
            CurrentFileSz = binFile.size();
            wordAddr3 = uword((CurrentFileSz - PreviousFileSz)/WORD_BYTES);
#ifdef DEBUG_FILE
            txtOut << ";\t |----- Supplement 1 -----|" << endl;
#endif

            //make sure these bursts are rounded up to the nearest word
            NumInitBytes = (WORD_BYTES - (*it2)->GetNumInCxns()%WORD_BYTES) + (*it2)->GetNumInCxns();
            FileZeroBurst(binFile, NumInitBytes);
            NumInitWords = NumInitBytes/WORD_BYTES;
#ifdef DEBUG_FILE
            for (int i = 0; i < NumInitWords ; ++i) {
               txtOut << FormatAsFullWord((uword)0) << endl;
               }
#endif

            /*-*-*-*-*-*-*-* Supplemental 2 *-*-*-*-*-*-*-*/
            /* Initialize the Supps with Zeros, the Supps are byte-oriented */
            /* Record Staring Word Address */
            CurrentFileSz = binFile.size();
            wordAddr4 = uword((CurrentFileSz - PreviousFileSz)/WORD_BYTES);
#ifdef DEBUG_FILE
            txtOut << ";\t |----- Supplement 2 -----|" << endl;
#endif
            //make sure these bursts are rounded up to the nearest word
            NumInitBytes = (WORD_BYTES - (*it2)->GetNumInCxns()%WORD_BYTES) + (*it2)->GetNumInCxns();
            FileZeroBurst(binFile, NumInitBytes);
            NumInitWords = NumInitBytes/WORD_BYTES;
#ifdef DEBUG_FILE
            for (int i = 0; i < NumInitWords ; ++i) {
               txtOut << FormatAsFullWord((uword)0) << endl;
               }
#endif
            }
         else {
            wordAddr2 = 4, wordAddr3 = 4, wordAddr4 = 4;
            }
         /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

         /* Make Sure W/B region ends on 8-Word boundary address */
         CurrentFileSz = binFile.size();
         CurrentRecord_WordCnt = (CurrentFileSz - PreviousFileSz)/WORD_BYTES;
         BoundaryWords = CurrentRecord_WordCnt%BOUNDARY_WORD_SIZE;
         if (BoundaryWords != 0) {
            WordPadBurst = BOUNDARY_WORD_SIZE - BoundaryWords;
            FileZeroBurst(binFile, WordPadBurst*WORD_BYTES);
#ifdef DEBUG_FILE
            for (int i = 0; i < WordPadBurst; ++i) {
               txtOut << FormatAsFullWord((uword)0) << endl;
               }
#endif
            }

         /* Record W/B data Ending Word Address */
         CurrentFileSz = binFile.size();
         addrWBEnd = uword(((CurrentFileSz - PreviousFileSz)/WORD_BYTES) - 1);//subtract by 1 for zero-based addressing

         /*-*-*-*-*-*-*-* Set Input Pointers *-*-*-*-*-*-*-*/
#ifdef DEBUG_FILE
         txtOut << ";\t |----- Input Pointers -----|" << endl;
#endif

#ifdef PTR_DEBUG
         debugOut << "Group " << QString::fromStdString((*it1)->GetName()) << " - Cell ( " << (*it2)->GetID() << " ) w/ Chip Address: "
            << hex << (*it2)->GetAddr0() << endl;
         debugOut << "Quantity Value Dependent Connections = " << (*it2)->GetQtyVal() << endl;
#endif

         /* Record Pointers Starting Word Address */
         CurrentFileSz = binFile.size();
         PointerStart = uword((CurrentFileSz - PreviousFileSz)/WORD_BYTES);

         /* Record Starting Word Address */
         wordAddr5 = uword((CurrentFileSz - PreviousFileSz)/WORD_BYTES);

#define SPECIAL_NO_IO
#ifndef SPECIAL_NO_IO
         /* Write IO Pointers First */
         if ((*it1)->isCerebellum() && (*it2)->GetQtyVal() > 0) {
            /* Set IO Pointers */
            uword IOPtr = 0;
            MacroCxns = (*it1)->GetInCxns();
            for (iterMacroCxn = MacroCxns.begin(); iterMacroCxn != MacroCxns.end(); ++iterMacroCxn) {
               if ((*iterMacroCxn)->isVal() && ((*iterMacroCxn)->GetValGrp() != NULL)) {
                  /* Special Benchmark Test Set-up for plastcitiy,
                  *  just use single cell IO group */
                  if ((*iterMacroCxn)->GetCxnType() == 0x09) {
                     IOPtr = (*iterMacroCxn)->GetValGrp()->GetCell(0)->GetChipAddr(chip);
                     }
                  else {
                     IOPtr = (*iterMacroCxn)->GetValGrp()->GetCell((*it2)->GetID())->GetChipAddr(chip);
                     }
#ifdef PTR_DEBUG
                  debugOut << "IO Pointer " << " Address = " << hex << IOPtr << " of Group "
                     << QString::fromStdString((*iterMacroCxn)->GetValGrp()->GetName()) << endl;
#endif

                  // reserve whole word for IO-Pointer
                  if ((*iterMacroCxn)->GetCxnType() != 0x08) {
                     binOut << IOPtr;
#ifdef DEBUG_FILE
                     txtOut << FormatAsFullWord(IOPtr) << endl;
#endif
                     }
                  }
               } // end Macro-Cxn Loop
            } // end if Cerebellum Value-Dep.
#endif
         cxnCnt = 0;
         for (iterCell = InputCells.begin(); iterCell != InputCells.end(); ++iterCell) {
            ++cxnCnt;
#ifdef PTR_DEBUG
            debugOut << "Input Pointer " << (cxnCnt - 1) << " Address = " << hex << quint16((*iterCell)->GetChipAddr(chip)) << " of Group "
            << QString::fromStdString((*iterCell)->GetGrp()->GetName()) << endl;
#endif

            tempPointer = uword((*iterCell)->GetChipAddr(chip));
            if (cxnCnt%(BYTEWIDTH/PTRS_BYTES) == 1) {
               pointerWord = tempPointer;
               // *For Sensor Loopback testing . . .
               // align sensor Input with
               //if (cxnCnt == 1) {
               }
            else {
               pointerWord |= (tempPointer << shftB2);
               binOut << pointerWord;
#ifdef DEBUG_FILE
               txtOut << FormatAsFullWord(pointerWord) << endl;
#endif
               }
            } // end In-Cells Loop

         /* Finish-Up & Test for partial Words */
         if (cxnCnt%(BYTEWIDTH/PTRS_BYTES) == 1) {
            binOut << pointerWord;
#ifdef DEBUG_FILE
            txtOut << FormatAsFullWord(pointerWord) << endl;
#endif
            }

         /* Add Value Group Pointer for D7 Value Connections
         *  Starts on New Word */
         if (((*it2)->GetQtyVal() > 0) && (*it1)->isDarwin7()) {
            //kludge, I'm cheating since I already know what the value group is
            //instead of knowing it's a value connection and grabbing the value
            //group cells accordingly
            Group* ValueGrpS =  Network::GetGrp("S");

            //I'm cheating again since I know it's a 2X2 group
            uword S_Cell0_addr = ValueGrpS->GetCell(0)->GetChipAddr(chip);
            uword S_Cell1_addr = ValueGrpS->GetCell(1)->GetChipAddr(chip);
            uword S_Cell2_addr = ValueGrpS->GetCell(2)->GetChipAddr(chip);
            uword S_Cell3_addr = ValueGrpS->GetCell(3)->GetChipAddr(chip);

#ifdef PTRDEBUG
            debugOut << hex << "S Input Pointer Address = " << S_Cell0_addr << " of Group "
               << QString::fromStdString(ValueGrpS->GetName()) << endl;

            debugOut << hex << "S Input Pointer Address = " << S_Cell1_addr << " of Group "
               << QString::fromStdString(ValueGrpS->GetName()) << endl;

            debugOut << hex << "S Input Pointer Address = " << S_Cell2_addr << " of Group "
               << QString::fromStdString(ValueGrpS->GetName()) << endl;

            debugOut << hex << "S Input Pointer Address = " << S_Cell3_addr << " of Group "
               << QString::fromStdString(ValueGrpS->GetName()) << endl;
#endif

            /*-*-* Word 1 *-*-*/
            pointerWord = (S_Cell1_addr << shftB2) | S_Cell0_addr;//pointer 1, 2
            binOut << pointerWord;
#ifdef DEBUG_FILE
            txtOut << FormatAsFullWord(pointerWord) << endl;
#endif
            /*****************/

            /*-*-* Word 2 *-*-*/
            pointerWord =(S_Cell3_addr << shftB2) | S_Cell2_addr;//pointer 3, 4
            binOut << pointerWord;
#ifdef DEBUG_FILE
            txtOut << FormatAsFullWord(pointerWord) << endl;
#endif
            /*****************/
            }

         /* Record & Set Si(t-1) Pointer Address */
#ifdef PTRDEBUG
         debugOut << hex << "Si(t-1) Input Pointer Address = " << (*it2)->GetChipAddr(chip) << " of Group "
            << QString::fromStdString((*it1)->GetName()) << endl;
#endif

         // Set Si(t-1) Pointer
         pointerWord = (uword)(*it2)->GetChipAddr(chip);
         binOut << pointerWord;
#ifdef DEBUG_FILE
         txtOut << FormatAsFullWord(pointerWord) << endl;
#endif

         /* Record Pointers Ending Word Address */
         CurrentFileSz = binFile.size();
         PointerEnd = uword(((CurrentFileSz - PreviousFileSz)/WORD_BYTES) - 1);//subtract by 1 for zero-based addressing

         /* Record Cell Address */
         CurrentFileSz = binFile.size();
         wordAddrThisCell = PointerEnd;

#ifdef PTR_DEBUG
         debugOut << endl << endl;
#endif
         /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

         // **** TBD ******
         /* Special 4-Word Padding, requested by Rich S. on 10/31/08 to any funny
         *  cases at the end of the reading the record
         FileZeroBurst(binFile, SpecialPaddingWords*WORD_BYTES);
#ifdef DEBUG_FILE
         for (int i = 0; i < WordPadBurst; ++i) {
            txtOut << FormatAsFullWord((uword)0) << endl;
            }
#endif */

         /* Align Next Element Record for 8-Word Boundary */
         CurrentFileSz = binFile.size();
         CurrentWordAddress = (CurrentFileSz - PreviousFileSz)/WORD_BYTES;
         BoundaryWords = CurrentWordAddress%BOUNDARY_WORD_SIZE;
         if (BoundaryWords != 0) {
            WordPadBurst = BOUNDARY_WORD_SIZE - BoundaryWords;
            FileZeroBurst(binFile, WordPadBurst*WORD_BYTES);
#ifdef DEBUG_FILE
            for (int i = 0; i < WordPadBurst; ++i) {
               txtOut << FormatAsFullWord((uword)0) << endl;
               }
#endif
            }

         /*---------- Update Header Data in File(s) ----------*/
         /*-*-*- Word 2: Record Length  -*-*-*/
         //Save Current File Positions to come-back to at end
         CurrentPos = binFile.pos();// save to return to position
#ifdef DEBUG_FILE
         CurrentPos_TXT = txtFile.pos();
#endif
         CurrentFileSz = binFile.size();
         RecordLength = uword((CurrentFileSz - PreviousFileSz)/WORD_BYTES);
         binFile.seek(posWord2);
         binOut << RecordLength;
#ifdef DEBUG_FILE
         txtFile.seek(posWord2_TXT);
         txtOut << FormatAsFullWord(RecordLength) << endl;
#endif

         /*-*-*- Word 3: Pointer Start/End *-*-*/
         PointerWord = (PointerEnd << shftB2) | PointerStart;
         binFile.seek(posWord3);
         binOut << PointerWord;
#ifdef DEBUG_FILE
         txtFile.seek(posWord3_TXT);
         txtOut << FormatAsFullWord(PointerWord) << endl;
#endif

         /*-*-*- Word 4: SDRAM W/B Address *-*-*/
         if ((*it2)->GetQtyPlast() == 0) {
            addrSDRAMWB = 0;
            }
         binFile.seek(posWord4);
         binOut << addrSDRAMWB;
#ifdef DEBUG_FILE
         txtFile.seek(posWord4_TXT);
         txtOut << FormatAsFullWord(addrSDRAMWB) << endl;
#endif

         /*-*-*- Word 5: W/B Word Address Start/End *-*-*/
         if ((*it2)->GetQtyPlast() > 0) {
            WBRangeWord = ((addrWBEnd - WrdsNotSent) << shftB2) | (addrWBStart - WrdsNotSent);
            }
         else {
            WBRangeWord = 0;
            }
         binFile.seek(posWord5);
         binOut << WBRangeWord;
#ifdef DEBUG_FILE
         txtFile.seek(posWord5_TXT);
         txtOut << FormatAsFullWord(WBRangeWord) << endl;
#endif

         /*-*-*- Word 7, 8, 9: NPU BRAM Addresses *-*-*/
         //Word 7: synaptic descriptors, weights
         BRAMWrd0 = (((wordAddr1 - WrdsNotSent)*2) << shftB2) | ((wordAddr0 - WrdsNotSent)*2);
         binFile.seek(posWord7);
         binOut << BRAMWrd0;
#ifdef DEBUG_FILE
         txtFile.seek(posWord7_TXT);
         txtOut << FormatAsFullWord(BRAMWrd0) << endl;
#endif
         //Word 8: supp 0, 1
         BRAMWrd1 = (((wordAddr3 - WrdsNotSent)*2) << shftB2)  | ((wordAddr2 - WrdsNotSent)*2);
         binFile.seek(posWord8);
         binOut << BRAMWrd1;
#ifdef DEBUG_FILE
         txtFile.seek(posWord8_TXT);
         txtOut << FormatAsFullWord(BRAMWrd1) << endl;
#endif
         //Word 9: inputs, supp 2
         BRAMWrd2 = (((wordAddr5 - WrdsNotSent)*2) << shftB2)  | ((wordAddr4 - WrdsNotSent)*2);
         binFile.seek(posWord9);
         binOut << BRAMWrd2;
#ifdef DEBUG_FILE
         txtFile.seek(posWord9_TXT);
         txtOut << FormatAsFullWord(BRAMWrd2) << endl;
#endif

         //Word 12: Si(t-1)
         // Si(t-1) address = Start BRAM Ptr Addr + (Start Ptr Word Address - End Ptr Word Address)
         ExtraWrd0 = ((wordAddr5 - WrdsNotSent)*2) + (PointerEnd - PointerStart);
         //ExtraWrd0 = ((wordAddrThisCell - WrdsNotSent)*2);
         binFile.seek(posWord12);
         binOut << ExtraWrd0;
#ifdef DEBUG_FILE
         txtFile.seek(posWord12_TXT);
         txtOut << FormatAsFullWord(ExtraWrd0) << endl;
#endif
         /* Return file pointer to original last current */
         binFile.seek(CurrentPos);
#ifdef DEBUG_FILE
         txtFile.seek(CurrentPos_TXT);
#endif

         } // end Cell Loop
      } // end Group Super-Loop

   if (chip->OnCnt() > 0) {
      /* Pad File to fill MAX_WORD_BURST_SIZE */
      CurrentFileSz = binFile.size();
      WordPadBurst = MAX_WORD_BURST_SIZE - ((CurrentFileSz/WORD_BYTES)%MAX_WORD_BURST_SIZE);
      if (WordPadBurst != 0) {
         FileZeroBurst(binFile, WordPadBurst*WORD_BYTES);
#ifdef DEBUG_FILE
         for (int i = 0; i < WordPadBurst; ++i) {
            txtOut << FormatAsFullWord((uword)0) << endl;
            }
#endif
         }

      CurrentFileSz = binFile.size();
      Num_Word_Bursts = (CurrentFileSz/WORD_BYTES)/MAX_WORD_BURST_SIZE ;
      }
   else {
      Num_Word_Bursts = 0;
      }

#ifdef VERBOSE
   OutputWin::message("Finished setting SDRAM data! ");
#endif

   return true;
   }

ChipFiles::~ChipFiles() {
   delete tables;
   delete sdram;
   }
