/**
 * @file Network.h
 *
 * Includes neural network initialization methods for synaptic
 * and neuronal group properties all stemming from the NSI
 * Neuro-Anatomy file.
 */
#ifndef NETWORK_H_
#define NETWORK_H_

#include <string>
#include <QFile>
#include <QStringList>
#include <QString>

#include "Group.h"
#include "SPP2Globals.h"

class Network {
public:

   /**
   * @return true if a connection is made between two Cells according
   * to specified rules.
   */
   static bool Synapse( int proj_height, int proj_width,
      int proj_xoffset, int proj_yoffset, int proj_angle,
      int from_height, int from_width, int to_height, int to_width,
      int from_inx, int to_inx, QString proj_type, double prob );

   /* Initialize the Connection. */
   static void Cxn(QStringList Params);

   /* Initialize the Group. */
   static Group* Grp(QStringList Params);

   /* Processes & Inspects Neuro-Anatomy File. */
   static bool CheckIntTok(QString TokName, QStringList Tokens,
      int line, int itok, int low, int high);
   static bool CheckDblTok(QString TokName, QStringList Tokens,
      int line, int itok, double dlow, double dhigh)
   static void ProcessFile( QFile* AnatomyFile );
   static bool InspectLine( QString FileName, QStringList Tokens,
      int lineNum, QStringList GrpCheck );
   static Group* GetGrp(QString GrpName);
   static int TotalCells();
   static int TotSensCells();
   static int TotValCells();
   static int TotRegCells();
   /* Return total number of connections in network */
   static int TotalCxns();
   /* Return total number plastic connections */
   static int TotalPlast();
   /* Return average number of plastic connections per cell */
   static double AvgPlast();
   /* Return average number of connections per cell */
   static double AvgCxns();
   /* Return number of connection types.
   *  N.B.  This is a bit of a kludge for the added Izhikevich
   *  neuron types, whose Assembler code wants to know the
   *  number of connection types coming into each cell.  In
   *  the R.Martin scheme of things, this could in principle
   *  be different for each cell according to which connections
   *  actually get generated.  Since this is only a test, we
   *  will do it the CNS way for this one cell type.  --GNR  */
   int NumCT() { return nct; };
private:
   int nct;             /* Number of connection types */
   };
#endif
