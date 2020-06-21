#include "OutputWin.h"

void OutputWin::error(int code, QString msg)
{
        //could make red later
        // possible codes relating to synthesizer, downloads, updloads, board problems
        qDebug() << endl << "error: " << msg << endl;
}

void OutputWin::warning(int code, QString msg)
{
        //could make yellow or orange font
        qDebug() << endl << "warning: " << msg << endl;
}

void OutputWin::message(QString msg)
{
        qDebug() << msg << endl;
}