/**
 * @file MainWindow.h
 *
 * The MainWindow contains GUI controls for manipulating
 * a SPP2's Chip contents. There are 3 Chips labeled
 * A, B, and C on SPP2.
 */
#ifndef ADVWIN_H_
#define ADVWIN_H_

#include <QMainWindow>
#include <QFile>
#include <QDir>
#include <list>

#include  "ui_AdvWin.h"
#include "SPP2Globals.h"
#include "Chip.h"


class AdvWin : public QMainWindow, public Ui::AdvWin {
   Q_OBJECT

   public:
      AdvWin( QWidget* parent = 0 );
      void ClearDisplays();

   public slots:
      void on_btnLoad_clicked();
      void on_btnClear_clicked();

      void on_btnBrowse_clicked();
      void on_btnSynth_clicked();
      void on_btnAdd_clicked();
      void on_btnRem_clicked();

   private:
      QVector<QLineEdit*> OnLeft;
      QVector<QLineEdit*> OffLeft;
      QVector<QLineEdit*> SensLeft;
      QVector<QTableWidget*> ChipTbls;
      QButtonGroup ChipSel;
      QButtonGroup StorSel;

      bool FileLoaded;
      QString AppHomeDir;
      QFile AnatomyFile;
      QDir SynthDir;
   };
#endif