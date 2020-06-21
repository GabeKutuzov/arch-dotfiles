/**
 * @file ChipUtils.h
 *
 * Contains utilities used for
 * SPP2 Chip component manipulations.
 * i.e. sorting and placing neuronal
 * groups on select Chips.
 */
#ifndef CHIPUTILS_H_
#define CHIPUTILS_H_

#include "Chip.h"

class ChipUtils {
public:
   /**
   * Places all neuronal Groups
   * on Single SPP2 Chip.
   * @param id for the chip
   * @return true if succeeds
   */
   static bool SynthSingle( int id );
   static bool DistribCxns();
   static void SetChipCellAddrs();
   static void SetOCIC_SRAM_Addrs();
   static void ClearChips();
   static void CustomChipConfig( QString ConfigFileName);

   /**
   * @return the Chip with the least amount
   * of connections
   */
   static Chip* MinInCxns();
   static Chip* MinElems();
   };
#endif