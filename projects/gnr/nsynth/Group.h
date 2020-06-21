/**
 * @file Group.h
 *
 * Groups are clusters of Cells that
 * connect to other Groups. Groups are
 * sorted among SPP2 Chips accordingly.
 * Specific coefficients are set here.
 *
 * Rev, 11/25/09, GNR - Add parms for RockVis & Izhikevich cells
 */

#ifndef GROUP_H_
#define GROUP_H_

#include <list>
#include <vector>
#include <string>
#include <QStringList>
#include <algorithm>
#include "TypeDefs.h"
#include "MacroCxn.h"
#include "Coeff.h"
#include "Cell.h"


using namespace std;

/* Forward Declarations */
class Chip;
class Cell;
class MacroCxn;

class Group {
public:
   Group(QStringList tokens, int iID);
   ~Group();

   void AddCell(Cell* cell) { cells.push_back(cell); }

   int GetNumCells() { return cells.size(); }
   int GetSize() { return size; }

   vector<Cell*> GetCells() { return cells; }
   list<Cell*> GetInCells() { return InCells; }
   Cell* GetCell(int inx) { return cells[inx]; }

   int GetID() { return SysID; }
   string GetName() { return name; }

   void AddCxn(MacroCxn* NewCxn) { Cxns.push_back(NewCxn); }
   void AddInCxn(MacroCxn* NewInCxn) { InCxns.push_back(NewInCxn); }

   /*
   * Add a reference to another Group that input this Group.
   */
   void AddInGrp(Group* InGrp);
   bool isInGrp(Group* grp) { return binary_search(InGrps.begin(), InGrps.end(), grp); }
   GrpLst GetInGrps() { return InGrps; }

   int NumSensIn();
   int NumRegIn();

   MacroLst GetCxns() { return Cxns; }
   MacroLst GetInCxns() { return InCxns; }

   int GetNumInCxns();
   int GetNumCxns();

   /**
   * @return the number plastic input connections
   */
   int GetNumPlast();

   bool hasPlast();

   int GetNumInCells() { return NumInCells; }

   //int GetNumInElemsStored(){ return numInElems; }
   int GetHeight() { return height; }
   int GetWidth() { return width; }

   Coeff GetScale() { return scale; }
   Coeff GetFr_Thr() { return fr_thr; }
   Coeff GetVd_Thr() { return vd_thr; }
   Coeff GetPrst() { return prst; }
   Coeff GetIzhi_a() { return Izhi_a; }
   Coeff GetIzhi_b() { return Izhi_b; }
   Coeff GetIzhi_c() { return Izhi_c; }
   Coeff GetIzhi_d() { return Izhi_d; }

   void SetGrpRecID( int ID ) { GrpRecID = ID; }
   int GetGrpRecID() { return GrpRecID; }

   void SetOnChipID( int ID ) { OnChipID = ID; }
   int GetOnChipID() { return OnChipID; }

   vector<quint8> GetSynRecs() { return SynRecs; }
   void AddSynRec(quint8 rec) { SynRecs.push_back(rec); }
   void SetNumSynRecs(int num) { numSynRecs = num; }
   int GetNumSynRecs() { return numSynRecs; }

   int GetTypeCode() { return type; }

   bool isSens() { return type == GT_Sensor; }
   bool isVal() { return type == GT_Value; }
   bool isDarwin7() { return type == GT_D7; }
   bool isCerebellum() { return type == GT_Cereb; }
   bool isIzhik() { return type == GT_Izhik; }
   bool isRockVis() { return type == GT_RockVis; }

   bool isWeightReader() { return type == GT_WeightReader; }
   bool isWeightReadWriter() { return type == GT_WeightReadWriter; }
   bool isSensorLoopBacker() { return type == GT_SensorLoopBacker; }
   bool isEpochCounter() { return type == GT_EpochCounter; }
   bool isInstrSetVerifier() { return type == GT_InstrSetVerifier; }
   bool isMemSpaceVerifier() { return type == GT_MemSpaceVerifier; }

   void setHomeChip( Chip* ichip) { HomeChip = ichip; }
   Chip* GetHomechip() { return HomeChip; }

private:
   /* Group Network Parameters */
   string name;
   int type;   /* Actually an enum gptypes */
   int SysID;  /**< overall anatomy system ID */
   int height;
   int width;
   int size;

   /* Group Simulation Coefficients (from 'g' cards) */
   Coeff scale;
   Coeff fr_thr;
   Coeff vd_thr;
   Coeff prst;
   Coeff Izhi_a;
   Coeff Izhi_b;
   Coeff Izhi_c;
   Coeff Izhi_d;
   Coeff Izhi_k;
   Coeff Izhi_vT;
   Coeff Izhi_vPeak;
   Coeff RV_pt;
   Coeff RV_sdamp;
   Coeff RV_omega2;

   /* Params used for Izhik input or gen but not passed to SPP2 */
   double Izbin,Izdin,Izkin,IzCm,Izra,Izrb,Izrc,Izrd;

   int OnChipID; // On-Chip Group ID
   int GrpRecID; // Parameter Record ID
   vector<quint8> SynRecs; // On-Chip Synaptic Records

   CellVec cells;   // array of cells in the group
   CellLst InCells; // list of unique input elements to Group

   bool CxnsCalcd;
   GrpLst InGrps;
   int numSynRecs;
   int NumInCells;
   int numCxns;

   //Map Cells with index
   MacroLst Cxns;
   MacroLst InCxns;

   Chip* HomeChip;
   };
#endif