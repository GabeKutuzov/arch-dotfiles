/**
 * @file OutputWin.h
 *
 * Handles basic error & warning messages then
 * outputs them to selected displays.
 */
#ifndef OutputWin_H_
#define OutputWin_H_

#include <QtCore>

class OutputWin {
   public:
      static void error( int code, QString msg );
      static void warning( int code, QString msg );
      static void message( QString msg );
   };
#endif