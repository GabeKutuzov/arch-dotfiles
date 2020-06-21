/**
 * @file
 *
 * A set of Typedef's for containers, iterators, and
 * data types.
 */
#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

#include <list>
#include <vector>
#include <QtCore>

#include "Coeff.h"

/* Forward Declarations */
class MicroCxn;
class MacroCxn;
class Cell;
class Group;
class Chip;

typedef std::list<Group*>::iterator GrpLstIter;
typedef std::list<Group*> GrpLst;

typedef std::vector<Cell*>::iterator CellVecIter;
typedef std::list<Cell*>::iterator CellLstIter;
typedef std::vector<Cell*> CellVec;
typedef std::list<Cell*> CellLst;

typedef std::list<MicroCxn*> MicroLst;
typedef std::list<MicroCxn*>::iterator MicroLstIter;
typedef std::vector<MicroCxn*>::iterator MicroVecIter;
typedef std::vector<MicroCxn*> MicroVec;

typedef std::list<MacroCxn*> MacroLst;
typedef std::list<MacroCxn*>::iterator MacroLstIter;

typedef std::vector<Chip*>::iterator ChipVecIter;
typedef std::vector<Chip*> ChipVec;

typedef std::vector<Coeff> CoeffVec;

typedef std::vector<quint8> UINT8Vec;
typedef std::vector<quint8>::iterator UINT8VecIter;
typedef std::vector<quint16> UINT16Vec;
typedef std::vector<quint16>::iterator UINT16VecIter;

typedef std::list<quint16> UINT16Lst;
typedef std::list<quint16>::iterator UINT16LstIter;

typedef unsigned short int uint16;
typedef signed short int int16;
typedef const uint16 conuint16;
typedef const int conint;
typedef const quint32 conquint;

/*  Data width of main address access */
typedef qint64 longword;// 8 bytes
typedef qint32 word;// 4 bytes
typedef quint32 uword;
typedef quint16 ushortword;

typedef	std::vector<word> WordVec;
typedef std::vector<uword> uWordVec;
typedef WordVec::iterator WordVecIter;
typedef	uWordVec::iterator uWordVecIter;

typedef	std::list<word> WordLst;
typedef std::list<uword> uWordLst;
typedef WordLst::iterator WordLstIter;
typedef	uWordLst::iterator uWordLstIter;

#endif