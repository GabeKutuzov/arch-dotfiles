#include <vector>
#include <math.h>

#include "MacroCxn.h"
#include "Group.h"

/* Forward Declaration */
class Chip;

/**
 * Global SPP2 Constraints 
 */
int NUM_CHIPS = 3;
int MAX_ELEMS_PER_CHIP = int(pow(2.0,10.0)*64);//64K
int MAX_ON_ELEMS_PER_CHIP = 50000;//this and the next are flexible, so I can have up to 64k for both off & on chip
int MAX_OFF_ELEMS_PER_CHIP = (MAX_ELEMS_PER_CHIP - MAX_ON_ELEMS_PER_CHIP);
int MAX_SENS_ELEMS_PER_CHIP = 65536;
int MAX_SYNAPSES_PER_CELL = 256;
int MAX_SYNAPSES_PER_CELL_LIMIT = MAX_SYNAPSES_PER_CELL-5;// *special case since Edtrs only have 1 byte for number of input cxns
														 // & is not zero-based, &  I need 4 bytes for special data

QString version = "v2.1";//must put somewhere else eventually

/**
 * GlobaNetwork & SPP2 Chip Variables 
 */
GrpLst Groups;
MacroLst MacroCxns;
ChipVec Chips(NUM_CHIPS);

int TotCxns = 0;