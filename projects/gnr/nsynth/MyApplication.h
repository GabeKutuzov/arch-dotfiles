/**
 * @file MyApplication.h
 *
 * Re-Implements the Qt QApplication class
 * so I can catch exceptions that run
 * the GUI thread.
 */
#ifndef MYAPPLICATION_H_
#define MYAPPLICATION_H_

#include <QApplication>
#include <QtCore>
#include <exception>
#include <iostream>

class MyApplication : public QApplication {
   Q_OBJECT

public:
   MyApplication( int &c, char **v) : QApplication(c, v){ }
   bool notify( QObject *receiver, QEvent *event ) {
      //qDebug() << "In Notify!" << endl;
      try {
         return QApplication::notify( receiver, event );
         }
      catch (std::exception& ex) {
         std::cout << "EXCEPTION: " << ex.what() << endl;
         abort();
         }
      catch (...) {
         std::cout << "Uknown exception!" << endl;
         abort();
         }
      }
   };
#endif