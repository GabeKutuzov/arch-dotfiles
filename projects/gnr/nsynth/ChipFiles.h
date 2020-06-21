/**
 * @file ChipFiles.h
 *
 * All 3 Chips on SPP2 need configuration files
 * to initialize their respected memory spaces and
 * configure memory address pointers. There were 8
 * files per Chip that set specific properties,
 * they are: Group-Synaptic Ceoff. Table, Element Descriptors,
 * Synaptic Descriptors, Synaptic Weights, Input Pointers,
 * Group Assoc. Table, OCIC Internal Address Table, and tanh LUT.
 */
#ifndef SPP2CHIPFILES_H_
#define SPP2CHIPFILES_H_

#include <QtCore>
#include <vector>
#include "TypeDefs.h"
#include "SPP2Globals.h"
#include "Group.h"

using namespace std;

/* Forward Declarations */
class Chip;

/**
 * Contains the routines necessary to create configuration files
 * used to configure specific SPP2 Chip's.
 */
class ChipFiles {
public:
   ChipFiles(Chip* chip);
   ~ChipFiles();

   /**
   * Calls all memory initializion routines in a specified order.
   */
   bool SetAllData();

   void ClearAllData();

   /**
   * Calls write file routines for all memory spaces initialized.
   */
   bool WriteFiles();

   /**
   * Adds zero's to end of files that are not full.
   */
   static void FileZeroBurst(QFile& iFile, int iZERO_BYTES);

   /**
   * Formats string-word representations into
   * full words by prepending the appropriate
   * number of zeros.
   */
   static QString FormatAsFullWord( unsigned int data );

   uword GetWordBursts() { return sdram->GetWordBursts(); }

   /**
   * This class is responsible for creating the Memory
   * Table files. The specifics described below.
   */
   class Tables {
   public:
      Tables() { }
      Tables(Chip* ichip) : chip(ichip), RecIDs(0) { }

      bool SetData();
      bool WriteFiles();
      void ClearData();

      bool SetTanh();
      bool WriteTanh();
      bool SetOCIC();
      bool WriteOCIC();

   private:
      Chip* chip;

      static QString TanhFileName;
      static QString AssocFileName;
      static QString OCICFileName;
      static QString GrpSynFileName;

      static conint BYTEWIDTH = 4;
      static conint TANH_BYTES = 256;

      static conint REGS_PER_SET = 8; /**< Num. registers per Group; 1st one for Group */

      static conint ADDRS_BYTES = 4;

      static conint BYTES_REC = 32; /**< Number of bytes per coeff. set */

      quint8 RecIDs; /**< Total number of Record IDs assigned to Synaptic and Group Records */

      uWordVec tanhWDs; /**<tanh look-up table data */

      /**
      * The Off-Chip Inter-Connect class creates the address look-up table for
      * each Off-Chip Group's activity levels. These activity levels are transferred at
      * the end of each epoch. This occurs when an On-Chip's Group's input Cell is computed on another
      * SPP2 Chip. Cell activity levels are accessed in an intermediate memory space(SRAM) addressed
      * from 0 to 5FFFF - partitioned between different Chip activity levels and Chip sensor activity levels.
      */
      uWordLst OCICWDs;/**< Holds the list of look-up addresses in order for this Chip */
      };

   /**
   * The SDRAM class creates the configuration files
   * for each defined memory page in SdRAM. Each file
   * is formatted accordingly.
   */
   class SDRAM {
   public:
      SDRAM() { }
      SDRAM(Chip* ichip) : chip(ichip), RecIDs(0), Num_Word_Bursts(0) { }

      bool SetSynData();
      bool WriteFiles();
      bool WriteData();
      void ClearData();

      void WriteTanh(); //tanh is currently being loading directly to the NPU BRAM

      uword GetWordBursts() { return Num_Word_Bursts; }

   private:
      Chip* chip;

      static QString FileName;

      uword Num_Word_Bursts;

      static conint RecMarkerWord0 = 0xAAAA5555;
      static conint RecMarkerWord1 = 0xFEEDBEEF;

      static conint BYTEWIDTH = 4;
      static conint WORD_BYTES = 4;
      static conint BOUNDARY_WORD_SIZE = 8;
      static conint MAX_WORD_BURST_SIZE = 128;

      static conint MAX_IOPTRS = 1; //Only need one for now. . .
      static conint IOPTR_BYTES = 2;

      uWordVec EdtrsWds; /**< element descriptors data in wrd length*/
      static conint EDTRS_BYTES = 16;

      uWordVec SdtrsWds; /**< synaptic descriptors data in wrd length*/
      static conint SDTRS_BYTES = 1;

      uWordVec WgtsWds; /**< Synaptic Wgts data in wrd length */
      static conint WGTS_BYTES = 2;
      static conint WEIGHTS_PER_WORD = 2;

      uWordVec InPtrsWds; /**< input cxn. pointer addresses in wrd length*/
      static conint PTRS_BYTES = 2;

      uWordVec SynWDs; /**< Holds list of Synaptic Coeffs */
      uword RecIDs; /**< Holds number of Synaptic Record IDs */
   };

private:
   Chip* m_chip;

   static QString Prefix;/**< Chip name Prefix for files names*/

   Tables* tables;
   SDRAM* sdram;
   };

#endif