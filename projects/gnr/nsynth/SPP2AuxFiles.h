/**
 * @file SPP2AuxFiles.h
 *
 * The SPP2 Auxillary files include I/O config. files contain
 * frame information for sensor SPP2-input data and activity
 * SPP2-output data. The files hold the location of each Groups
 * activity levels within both input and output frames. And other
 * information such as Group to Chip locations.
 */
#ifndef SPP2AUXFILES_H_
#define SPP2AUXFILES_H_

#include "SPP2Globals.h"

class SPP2AuxFiles {
   public:
      static void WriteActFile();
      static void WriteSensFile();
      static void WriteChipConfig();
      static void WriteNumElems();
      static void WriteNetStats();
      static void PrintFileBanner( QTextStream &outFile);

   private:
      static QString ActFileName;
      static QString SensFileName;
      static QString ConfigFileName;
      static QString NumElemsFileName;
      static QString NetStatsFileName;
   };
#endif