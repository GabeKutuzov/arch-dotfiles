/**
 * @file Cell.h
 *
 * Each Cell belongs to a Group, in which, their
 * cellular and synaptic properties are implicity
 * defined. Each Cell also holds a specific SPP2
 * address that connects Cells to other Cells.
 */
#ifndef CELL_H_
#define CELL_H_

#include <list>
#include "TypeDefs.h"
#include "MicroCxn.h"
#include "Chip.h"
#include "Group.h"

using namespace std;

/* Forward Declarations */
class MicroCxn;
class Chip;
class Group;

class Cell {
public:
   Cell() { }
   Cell(int iID, Group* iGrp);

   /**
   * Adds a micro-connection to the Cell's connection list.
   * @param Cxn a Micro-Connection pointer
   */
   void AddCxn(MicroCxn* Cxn);

/**
   * Gets the list of micro-connections.
   * @return a Micro-Connection list
   */
   MicroLst GetCxns() { return Cxns; }

   /**
   * Returns the number micro-connections with the other cell.
   * @param other another Cell pointer
   * @return the number of connections
   */
   int NumCxnsWith(Cell* other);

   /**
   * Gets the total number of connections the Cell has.
   * @return the number of connections
   */
   int GetNumCxns() const { return (int)Cxns.size(); }

   /**
   * Gets the number of input connections to the Cell.
   * @return the number of input connections
   */
   int GetNumInCxns() const { return numInCxns; }

   /**
   * Gets the number of connections coming out of the Cell.
   * @return the number of output connections
   */
   int GetNumOutCxns() const { return (int)Cxns.size()-numInCxns; }

   /**
   * @return the number of input plastic connections
   */
   int GetQtyPlast() const { return (int)QtyPlast; }
   void IncPlast() { ++QtyPlast; }

   /**
   * @return the number of Voltage Dependent input connections
   */
   int GetQtyVD() const { return (int)VD.size(); }

   /**
   * @return the number of Sensory-Voltage Dependent input connections
   */
   int GetQtySensVD() { return QtySensVD; }

   /**
   * @return the number of Sensory input connections
   */
   int GetQtySens() const { return (int)Sens.size(); }

   /**
   * @return the number of Coincident input connections
   */
   int GetQtyCoinc() const { return (int)Coinc.size(); }

   /**
   * @return the number of Voltage Independent input connections ** LEFTOFF - must add
   */
   int GetQtyVI() const { return (int)PlainVI.size() +
      ((int)Sens.size() - QtySensVD) + Coinc.size(); }

   /**
   * @return the number of Value Dependent input connections
   */
   int GetQtyVal() const { return QtyVal; }
   void IncVal() { ++QtyVal; }

   /**
   * @return the number of Unique input cells
   */
   int GetNumInElems() const { return (int)InputCells.size(); }

   /**
   * @return the Group the Cell belongs to
   */
   Group* GetGrp() const { return Grp; }

   /**
   * @return the Cell ID that distiguishes it from other cells in it's group
   */
   int GetID() { return ID; }

   /**
   * @return the list of input cells to the Cell
   */
   CellLst GetInputCells() { return InputCells; }

   bool hasInputFrom(Cell* other);
   bool hasOutputTo(Cell* other);

   /**
   * Sets the Cell's address for it's home SPP2 Chip.
   * @param chip the Cell's resident Chip
   * @param addr the resident address to be set
   */
   void SetAddr0(Chip* chip, quint16 addr);
   quint16 GetAddr0() { return OSTAddr0.second; }
   pair<Chip*, quint16> GetAddr0Map() { return OSTAddr0; }

   /**
   * Sets the Cell's 1st address for it's Off-Chip location
   * @param chip a non-home Chip location
   * @param addr the address to be set
   */
   void SetAddr1(Chip* chip, quint16 addr);
   bool isAddr1Set() { return Addr1Set; }
   quint16 GetAddr1() { return OSTAddr1.second; }

   /**
   * Sets the Cell's 2nd address for it's Off-Chip location
   * @param chip a non-home Chip location
   * @param addr the address to be set
   */
   void SetAddr2(Chip* chip, quint16 addr);
   short unsigned int GetAddr2(){ return OSTAddr2.second; }

   quint16 GetChipAddr(Chip* chip);

   /**
   * Sets the Cell's OCIC SRAM address. This address
   * is used in the OCIC LUT during inter-chip activity
   * level swapping.
   */
   void SetOCIC_SRAM_Addr( uword addr ) { OcicSramAddr = addr; }

   uword GetOCIC_SRAM_Addr() { return OcicSramAddr; }

   MicroLst GetSens() { return Sens; }
   MicroLst GetCoinc() { return Coinc; }
   MicroLst GetPlainVI() { return PlainVI; }
   MicroLst GetVD() { return VD; }

   CellLst GetSensIn() { return SensIn; }
   CellLst GetCoincIn() { return CoincIn; }
   CellLst GetPlainVIIn() { return PlainVIIn; }
   CellLst GetVDIn() { return VDIn; }

   /**
   * Inserts Cells in sorted fashion according to memory address,
   * to improve look-up speed.
   */
   void INCellInsert(Cell* newCell);

   /**
   * @return true if the new connection has an input Cell or element
   * that has not made an input to this cell before.
   */
   bool isNewInElem(MicroCxn* NewCxn);

   /**
   * Inferior Olive(IO) value systems map to specifice Cells
   * according to Value Depdendent input connections. A Cell's
   * plastic connections depend on IO activity and thus need
   * it's address to access it's activity on SPP2.
   */
   void AddIOAddr(quint16 addr) { IOAddrs.push_back(addr); }
   int GetIOAddrNum(quint16 addr);

private:
   int ID;
   Group* Grp;/**< The Cell belongs to this group */
   MicroLst Cxns;
   CellLst InputCells;

   /* Input Connections by Type */
   MicroLst Sens;
   MicroLst Coinc;
   MicroLst PlainVI;
   MicroLst VD;

   /* Input Cells by Type for Config Files */
   CellLst SensIn;
   CellLst CoincIn;
   CellLst PlainVIIn;
   CellLst VDIn;

   /* Input Connection Characteristic Quantities */
   int QtyVal;
   int QtyPlast;
   int QtyVD;
   int QtyVI;
   int QtyCoinc;
   int QtySens;
   int QtySensVD;

   /* Connection Statistics */
   int numInCxns;
   int numOutCxns;
   int numOutElems;

   bool Addr1Set;
   pair<Chip*, quint16> OSTAddr0; /**< A Cell's home Chip and address on Chip */
   pair<Chip*, quint16> OSTAddr1; /**< A Cell's 1st Off-Chip and Off-Chip address */
   pair<Chip*, quint16> OSTAddr2; /**< A Cell's 2nd Off-Chip and Off-Chip address */

   uword OcicSramAddr;

   vector<quint16> IOAddrs; /**< List of IO value activity pointers for SPP2 */
   };

bool cmp ( Cell const* CellA, Cell const* CellB );
bool cmpMax ( Cell const* CellA, Cell const* CellB );

#endif