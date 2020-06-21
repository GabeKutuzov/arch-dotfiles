/**
 * @file MicroCxn.h
 *
 * A Micro-Connection is the synapse between two
 * neuronal cells. The MicroCxn class includes
 * references to Macro-Connection it belongs to
 * and references to both the post and pre synapse
 * cells.
 */
#ifndef MICROCXN_H_
#define MICROCXN_H_

#include "Cell.h"
#include "MacroCxn.h"
#include "Coeff.h"

/* Forward Declartions */
class Cell;
class MacroCxn;

class MicroCxn {
public:
   MicroCxn() { }
   MicroCxn(Cell* iPreCell, Cell* iPostCell, MacroCxn* imacrocxn);

   /* j(pre) ----> i(post) */
   bool isPreCell(Cell* cell) { return cell == PreCell; }
   bool isPostCell(Cell* cell) { return cell == PostCell; }

   Cell* GetPreCell() { return PreCell; }
   Cell* GetPostCell() { return PostCell; }

   MacroCxn* GetMacroCxn() { return macrocxn; }

   //*DEBUG* Coeff GetWeight() { return Weight; }

// the group id and cell id create a unique identifier
// for the connection
private:
   Cell* PreCell;
   Cell* PostCell;

   //Coeff Weight;
   MacroCxn* macrocxn; // defined in this macro-connection
   };
#endif