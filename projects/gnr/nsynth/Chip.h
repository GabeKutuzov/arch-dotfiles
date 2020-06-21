/**
 * @file Chip.h
 *
 * Defines the properties of one SPP2 Chip.
 * Properties include resident Group lists
 * and references to Chip specific configuration files.
 */
#ifndef CHIP_H_
#define CHIP_H_

#include <list>
#include "SPP2Globals.h"
#include "TypeDefs.h"
#include "Group.h"
#include "ChipFiles.h"
#include "Cell.h"

using namespace std;

/* Forward Declaration */
class ChipFiles;
class Cell;
class Group;

class Chip {
public:
   Chip( int iID, QString ialphaID );
   ~Chip();

/**
   * @return the number of cells that input grp
   * that are an On-Chip and/or Off-Chip Group.
   */
   int NumOnOffTo(Group* grp);

   /**
   * @return the number of cells that input grp
   * that are a Sensory Group found on this Chip.
   */
   int NumSensTo(Group* grp);

   /**
   * @return the number of Off-Chip cells from grp.
   */
   int NumOffFrom(Group* grp);

   bool isInputToChip(Group* grp);

   bool AutoAddGrp(Group* grp);
   void AutoRemGrp(Group* grp);

   bool AddOnGrp(Group* grp);
   bool AddOffGrp(Group* grp);
   bool AddSensGrp(Group* grp);

   void RemOffGrp(Group* grp);
   void RemOnGrp(Group* grp);
   void RemSensGrp(Group* grp);

   void InsertGrp(Group* newGrp, GrpLst& Lst);
   void OnGrpInsert(Group* grp);
   void OffGrpInsert(Group* grp);
   void SensGrpInsert(Group* grp);

   bool isOnGrp(Group* grp) { return binary_search(OnGrps.begin(),
      OnGrps.end(), grp); }
   bool isSensGrp(Group* grp) { return binary_search(SensGrps.begin(),
      SensGrps.end(), grp); }
   bool isOffGrp(Group* grp) { return ( binary_search(
      OffGrps_FromOtherChip1.begin(), OffGrps_FromOtherChip1.end(), grp) ||
      binary_search(OffGrps_FromOtherChip2.begin(), OffGrps_FromOtherChip2.end(),
      grp) ); }


   GrpLst GetOnGrps() { return OnGrps; }
   GrpLst GetOffGrps();
   GrpLst GetSensGrps() { return SensGrps; }

   int GetOnLeft() { return OnLeft; }
   int GetOffLeft() { return OffLeft; }
   int GetSensLeft() { return SensLeft; }

   int OnCnt() { return MAX_ON_ELEMS_PER_CHIP - OnLeft; }
   int OffCnt() { return MAX_OFF_ELEMS_PER_CHIP - OffLeft; }
   int SensCnt() { return MAX_SENS_ELEMS_PER_CHIP - SensLeft; }

   void ClearAllGrps();

   /**
   * @return the number of in-connections to On-Groups
   */
   int GetNumInCxns();

   /**
   * @return the number of plastic in-connections to On-Groups
   */
   int GetNumPlastCxns();

   /**
   * Sets all of the addresses of the Chip's Cells that are
   * On-Chip( Cell activities calculated on this Chip ),
   * Off-Chip( Cell activities calculated on another Chip, but
   * referenced by On-Chip Cells), or Sensory( activities
   * calculated outside SPP2 and used by On-Chip Cells ).
   */
   void SetCellAddrs();

   bool InitSPP2Files();
   bool WriteSPP2Files();
   void ClearSPP2Files();

   void SetIDs( int iID , QString aID ) { myID = iID; myAlphaID = aID; }
   int GetID() { return myID; }
   QString GetaID() { return QString::number(myID+1); }
   QString GetalphaID() { return myAlphaID; }

   uword GetWordBursts();

private:
   int OnLeft;
   int OffLeft;
   int SensLeft;

   int myID;          /**< ID as 0, 1, or 2 */
   QString myAlphaID; /**< ID as A, B, or C */

   GrpLst OnGrps;
   //GrpLst OffGrps;
   GrpLst OffGrps_FromOtherChip1;
   GrpLst OffGrps_FromOtherChip2;
   GrpLst SensGrps;

   ChipFiles* m_chipfiles; /**< Pointer to Chip specific SPP2 configuration files */
   };
#endif
