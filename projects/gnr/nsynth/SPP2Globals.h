#include <vector>
#include <QString>

#include "TypeDefs.h"
#include "MacroCxn.h"
#include "Group.h"

class Chip;

/* Global Variable Declarations */

extern int NUM_CHIPS;
extern int MAX_ELEMS_PER_CHIP;
extern int MAX_ON_ELEMS_PER_CHIP;
extern int MAX_OFF_ELEMS_PER_CHIP;
extern int MAX_SENS_ELEMS_PER_CHIP;
extern int MAX_SYNAPSES_PER_CELL;
extern int MAX_SYNAPSES_PER_CELL_LIMIT;

extern QString version;

extern int TotCxns;

extern MacroLst MacroCxns;
extern GrpLst Groups;
extern ChipVec Chips;

/* Miscellaneous Defines */
enum gptypes { GT_Sensor=0, GT_Value, GT_D7, GT_Hippo,
   GT_Cereb, GT_Izhik, GT_RockVis, GT_WeightReader=10,
   GT_WeightReadWriter, GT_SensorLoopBacker,
   GT_EpochCounter, GT_InstrSetVerifier,
   GT_MemSpaceVerifier };

#define BENCHMARK_SEED
//same wgts used for debugging purposes
//#define USE_ORIGINAL_WGT_FOR_ALL_CXNS
//dis-arms individual wgts from beeing initialized
//since we are using the original weight for all
//connections and we need to be syncrhonized with
//the BenchMark program
//#define MicroWgts_When_Using_Random_Wgts
//dis-arms Network Inspection module when running
//special Test-Case networks
//#define NPU_MEMORY_TESTCASE_NO_NETWORK_INSPECTION