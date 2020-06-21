/********************************************************************************
** Form generated from reading ui file 'spp2dbg.ui'
**
** Created: Wed Dec 9 17:27:21 2009
**      by: Qt User Interface Compiler version 4.5.2
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_SPP2DBG_H
#define UI_SPP2DBG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QRadioButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QStatusBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionLoad_Object;
    QAction *actionLoad_Symbols;
    QAction *actionLoad_Data;
    QAction *actionQuit;
    QAction *actionCommand_Summary;
    QAction *actionAbout;
    QWidget *centralwidget;
    QPlainTextEdit *DataViewer;
    QPlainTextEdit *InstrViewer;
    QWidget *layoutWidget;
    QVBoxLayout *verticalLayout;
    QLabel *State_Label;
    QHBoxLayout *horizontalLayout_2;
    QRadioButton *Dec_OP;
    QRadioButton *Hex_OP;
    QSpacerItem *horizontalSpacer;
    QLabel *Bscl_Label;
    QLineEdit *Bscl_Set;
    QGridLayout *gridLayout;
    QLabel *PC_Label;
    QLineEdit *PC_View;
    QLabel *THI_Label;
    QLineEdit *THI_View;
    QLabel *RAd0_Label;
    QLineEdit *RAd0_View;
    QLabel *RAd1_Label;
    QLineEdit *RAd1_View;
    QGridLayout *gridLayout_3;
    QLabel *AC_Label;
    QLineEdit *AC_View;
    QLabel *Stk1_Label;
    QLineEdit *Stk1_View;
    QLabel *Stk2_Label;
    QLineEdit *Stk2_View;
    QLabel *Stk3_Label;
    QLineEdit *Stk3_View;
    QLabel *Stk4_Label;
    QLabel *Stk5_Label;
    QLineEdit *Stk5_View;
    QLineEdit *Stk4_View;
    QHBoxLayout *horizontalLayout;
    QCheckBox *CARRY_View;
    QCheckBox *NEG_View;
    QCheckBox *ZERO_View;
    QGridLayout *gridLayout_2;
    QLabel *Reg0_Label;
    QLabel *Reg8_Label;
    QLabel *Reg1_Label;
    QLabel *Reg9_Label;
    QLabel *Reg2_Label;
    QLabel *Rega_Label;
    QLabel *Reg3_Label;
    QLabel *Regb_Label;
    QLabel *Reg4_Label;
    QLabel *Regc_Label;
    QLabel *Reg5_Label;
    QLabel *Reg6_Label;
    QLabel *Rege_Label;
    QLabel *Reg7_Label;
    QLabel *Regf_Label;
    QLineEdit *Reg8_View;
    QLineEdit *Reg1_View;
    QLineEdit *Reg9_View;
    QLineEdit *Reg2_View;
    QLineEdit *Rega_View;
    QLineEdit *Reg3_View;
    QLineEdit *Regb_View;
    QLineEdit *Reg4_View;
    QLineEdit *Reg5_View;
    QLineEdit *Regd_View;
    QLineEdit *Reg6_View;
    QLineEdit *Rege_View;
    QLineEdit *Reg7_View;
    QLineEdit *Regf_View;
    QLineEdit *Reg0_View;
    QLineEdit *Regc_View;
    QLabel *Regd_Label;
    QPlainTextEdit *CommandWindow;
    QLineEdit *CommandInput;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuHelp;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(748, 888);
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);
        QFont font;
        font.setFamily(QString::fromUtf8("Bitstream Vera Sans Mono"));
        font.setKerning(false);
        MainWindow->setFont(font);
        actionLoad_Object = new QAction(MainWindow);
        actionLoad_Object->setObjectName(QString::fromUtf8("actionLoad_Object"));
        actionLoad_Symbols = new QAction(MainWindow);
        actionLoad_Symbols->setObjectName(QString::fromUtf8("actionLoad_Symbols"));
        actionLoad_Data = new QAction(MainWindow);
        actionLoad_Data->setObjectName(QString::fromUtf8("actionLoad_Data"));
        actionQuit = new QAction(MainWindow);
        actionQuit->setObjectName(QString::fromUtf8("actionQuit"));
        actionCommand_Summary = new QAction(MainWindow);
        actionCommand_Summary->setObjectName(QString::fromUtf8("actionCommand_Summary"));
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName(QString::fromUtf8("actionAbout"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        DataViewer = new QPlainTextEdit(centralwidget);
        DataViewer->setObjectName(QString::fromUtf8("DataViewer"));
        DataViewer->setGeometry(QRect(11, 11, 112, 641));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(DataViewer->sizePolicy().hasHeightForWidth());
        DataViewer->setSizePolicy(sizePolicy1);
        DataViewer->setMaximumSize(QSize(112, 16777215));
        QFont font1;
        font1.setFamily(QString::fromUtf8("LucidaTypewriter"));
        font1.setKerning(false);
        DataViewer->setFont(font1);
        InstrViewer = new QPlainTextEdit(centralwidget);
        InstrViewer->setObjectName(QString::fromUtf8("InstrViewer"));
        InstrViewer->setGeometry(QRect(130, 10, 280, 641));
        InstrViewer->setMinimumSize(QSize(280, 0));
        InstrViewer->setMaximumSize(QSize(196, 16777215));
        QFont font2;
        font2.setFamily(QString::fromUtf8("LucidaTypewriter"));
        InstrViewer->setFont(font2);
        layoutWidget = new QWidget(centralwidget);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        layoutWidget->setGeometry(QRect(420, 10, 311, 643));
        verticalLayout = new QVBoxLayout(layoutWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        State_Label = new QLabel(layoutWidget);
        State_Label->setObjectName(QString::fromUtf8("State_Label"));
        State_Label->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(State_Label);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        Dec_OP = new QRadioButton(layoutWidget);
        Dec_OP->setObjectName(QString::fromUtf8("Dec_OP"));
        QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(Dec_OP->sizePolicy().hasHeightForWidth());
        Dec_OP->setSizePolicy(sizePolicy2);

        horizontalLayout_2->addWidget(Dec_OP);

        Hex_OP = new QRadioButton(layoutWidget);
        Hex_OP->setObjectName(QString::fromUtf8("Hex_OP"));
        sizePolicy2.setHeightForWidth(Hex_OP->sizePolicy().hasHeightForWidth());
        Hex_OP->setSizePolicy(sizePolicy2);

        horizontalLayout_2->addWidget(Hex_OP);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        Bscl_Label = new QLabel(layoutWidget);
        Bscl_Label->setObjectName(QString::fromUtf8("Bscl_Label"));
        QSizePolicy sizePolicy3(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(Bscl_Label->sizePolicy().hasHeightForWidth());
        Bscl_Label->setSizePolicy(sizePolicy3);

        horizontalLayout_2->addWidget(Bscl_Label);

        Bscl_Set = new QLineEdit(layoutWidget);
        Bscl_Set->setObjectName(QString::fromUtf8("Bscl_Set"));
        QSizePolicy sizePolicy4(QSizePolicy::Fixed, QSizePolicy::Minimum);
        sizePolicy4.setHorizontalStretch(24);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(Bscl_Set->sizePolicy().hasHeightForWidth());
        Bscl_Set->setSizePolicy(sizePolicy4);
        Bscl_Set->setMaximumSize(QSize(24, 16777215));
        Bscl_Set->setMaxLength(2);

        horizontalLayout_2->addWidget(Bscl_Set);


        verticalLayout->addLayout(horizontalLayout_2);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        PC_Label = new QLabel(layoutWidget);
        PC_Label->setObjectName(QString::fromUtf8("PC_Label"));
        QSizePolicy sizePolicy5(QSizePolicy::Preferred, QSizePolicy::Minimum);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(PC_Label->sizePolicy().hasHeightForWidth());
        PC_Label->setSizePolicy(sizePolicy5);
        PC_Label->setMaximumSize(QSize(96, 26));

        gridLayout->addWidget(PC_Label, 0, 0, 1, 1);

        PC_View = new QLineEdit(layoutWidget);
        PC_View->setObjectName(QString::fromUtf8("PC_View"));
        sizePolicy5.setHeightForWidth(PC_View->sizePolicy().hasHeightForWidth());
        PC_View->setSizePolicy(sizePolicy5);
        PC_View->setMaximumSize(QSize(108, 16777215));

        gridLayout->addWidget(PC_View, 0, 1, 1, 1);

        THI_Label = new QLabel(layoutWidget);
        THI_Label->setObjectName(QString::fromUtf8("THI_Label"));
        sizePolicy5.setHeightForWidth(THI_Label->sizePolicy().hasHeightForWidth());
        THI_Label->setSizePolicy(sizePolicy5);
        THI_Label->setMaximumSize(QSize(96, 16777215));

        gridLayout->addWidget(THI_Label, 0, 2, 1, 1);

        THI_View = new QLineEdit(layoutWidget);
        THI_View->setObjectName(QString::fromUtf8("THI_View"));
        sizePolicy5.setHeightForWidth(THI_View->sizePolicy().hasHeightForWidth());
        THI_View->setSizePolicy(sizePolicy5);
        THI_View->setMaximumSize(QSize(108, 16777215));

        gridLayout->addWidget(THI_View, 0, 3, 1, 1);

        RAd0_Label = new QLabel(layoutWidget);
        RAd0_Label->setObjectName(QString::fromUtf8("RAd0_Label"));
        sizePolicy5.setHeightForWidth(RAd0_Label->sizePolicy().hasHeightForWidth());
        RAd0_Label->setSizePolicy(sizePolicy5);
        RAd0_Label->setMinimumSize(QSize(0, 0));
        RAd0_Label->setMaximumSize(QSize(16777215, 26));

        gridLayout->addWidget(RAd0_Label, 1, 0, 1, 1);

        RAd0_View = new QLineEdit(layoutWidget);
        RAd0_View->setObjectName(QString::fromUtf8("RAd0_View"));
        sizePolicy5.setHeightForWidth(RAd0_View->sizePolicy().hasHeightForWidth());
        RAd0_View->setSizePolicy(sizePolicy5);
        RAd0_View->setMinimumSize(QSize(0, 0));
        RAd0_View->setMaximumSize(QSize(108, 26));

        gridLayout->addWidget(RAd0_View, 1, 1, 1, 1);

        RAd1_Label = new QLabel(layoutWidget);
        RAd1_Label->setObjectName(QString::fromUtf8("RAd1_Label"));
        sizePolicy5.setHeightForWidth(RAd1_Label->sizePolicy().hasHeightForWidth());
        RAd1_Label->setSizePolicy(sizePolicy5);
        RAd1_Label->setMinimumSize(QSize(0, 0));
        RAd1_Label->setMaximumSize(QSize(16777215, 26));

        gridLayout->addWidget(RAd1_Label, 1, 2, 1, 1);

        RAd1_View = new QLineEdit(layoutWidget);
        RAd1_View->setObjectName(QString::fromUtf8("RAd1_View"));
        sizePolicy5.setHeightForWidth(RAd1_View->sizePolicy().hasHeightForWidth());
        RAd1_View->setSizePolicy(sizePolicy5);
        RAd1_View->setMinimumSize(QSize(0, 0));
        RAd1_View->setMaximumSize(QSize(108, 26));

        gridLayout->addWidget(RAd1_View, 1, 3, 1, 1);


        verticalLayout->addLayout(gridLayout);

        gridLayout_3 = new QGridLayout();
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        AC_Label = new QLabel(layoutWidget);
        AC_Label->setObjectName(QString::fromUtf8("AC_Label"));
        AC_Label->setMaximumSize(QSize(96, 16777215));

        gridLayout_3->addWidget(AC_Label, 0, 0, 1, 1);

        AC_View = new QLineEdit(layoutWidget);
        AC_View->setObjectName(QString::fromUtf8("AC_View"));
        sizePolicy5.setHeightForWidth(AC_View->sizePolicy().hasHeightForWidth());
        AC_View->setSizePolicy(sizePolicy5);
        AC_View->setMaximumSize(QSize(240, 16777215));

        gridLayout_3->addWidget(AC_View, 0, 1, 1, 1);

        Stk1_Label = new QLabel(layoutWidget);
        Stk1_Label->setObjectName(QString::fromUtf8("Stk1_Label"));
        Stk1_Label->setMaximumSize(QSize(96, 16777215));

        gridLayout_3->addWidget(Stk1_Label, 1, 0, 1, 1);

        Stk1_View = new QLineEdit(layoutWidget);
        Stk1_View->setObjectName(QString::fromUtf8("Stk1_View"));
        sizePolicy5.setHeightForWidth(Stk1_View->sizePolicy().hasHeightForWidth());
        Stk1_View->setSizePolicy(sizePolicy5);
        Stk1_View->setMaximumSize(QSize(240, 16777215));

        gridLayout_3->addWidget(Stk1_View, 1, 1, 1, 1);

        Stk2_Label = new QLabel(layoutWidget);
        Stk2_Label->setObjectName(QString::fromUtf8("Stk2_Label"));
        Stk2_Label->setMaximumSize(QSize(96, 16777215));

        gridLayout_3->addWidget(Stk2_Label, 2, 0, 1, 1);

        Stk2_View = new QLineEdit(layoutWidget);
        Stk2_View->setObjectName(QString::fromUtf8("Stk2_View"));
        sizePolicy5.setHeightForWidth(Stk2_View->sizePolicy().hasHeightForWidth());
        Stk2_View->setSizePolicy(sizePolicy5);
        Stk2_View->setMaximumSize(QSize(240, 16777215));

        gridLayout_3->addWidget(Stk2_View, 2, 1, 1, 1);

        Stk3_Label = new QLabel(layoutWidget);
        Stk3_Label->setObjectName(QString::fromUtf8("Stk3_Label"));
        Stk3_Label->setMaximumSize(QSize(96, 16777215));

        gridLayout_3->addWidget(Stk3_Label, 3, 0, 1, 1);

        Stk3_View = new QLineEdit(layoutWidget);
        Stk3_View->setObjectName(QString::fromUtf8("Stk3_View"));
        sizePolicy5.setHeightForWidth(Stk3_View->sizePolicy().hasHeightForWidth());
        Stk3_View->setSizePolicy(sizePolicy5);
        Stk3_View->setMaximumSize(QSize(240, 16777215));

        gridLayout_3->addWidget(Stk3_View, 3, 1, 1, 1);

        Stk4_Label = new QLabel(layoutWidget);
        Stk4_Label->setObjectName(QString::fromUtf8("Stk4_Label"));
        Stk4_Label->setMaximumSize(QSize(96, 16777215));

        gridLayout_3->addWidget(Stk4_Label, 4, 0, 1, 1);

        Stk5_Label = new QLabel(layoutWidget);
        Stk5_Label->setObjectName(QString::fromUtf8("Stk5_Label"));
        Stk5_Label->setMaximumSize(QSize(96, 16777215));

        gridLayout_3->addWidget(Stk5_Label, 5, 0, 1, 1);

        Stk5_View = new QLineEdit(layoutWidget);
        Stk5_View->setObjectName(QString::fromUtf8("Stk5_View"));
        sizePolicy5.setHeightForWidth(Stk5_View->sizePolicy().hasHeightForWidth());
        Stk5_View->setSizePolicy(sizePolicy5);
        Stk5_View->setMaximumSize(QSize(240, 16777215));

        gridLayout_3->addWidget(Stk5_View, 5, 1, 1, 1);

        Stk4_View = new QLineEdit(layoutWidget);
        Stk4_View->setObjectName(QString::fromUtf8("Stk4_View"));
        sizePolicy5.setHeightForWidth(Stk4_View->sizePolicy().hasHeightForWidth());
        Stk4_View->setSizePolicy(sizePolicy5);
        Stk4_View->setMaximumSize(QSize(240, 16777215));

        gridLayout_3->addWidget(Stk4_View, 4, 1, 1, 1);


        verticalLayout->addLayout(gridLayout_3);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        CARRY_View = new QCheckBox(layoutWidget);
        CARRY_View->setObjectName(QString::fromUtf8("CARRY_View"));
        QSizePolicy sizePolicy6(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy6.setHorizontalStretch(0);
        sizePolicy6.setVerticalStretch(0);
        sizePolicy6.setHeightForWidth(CARRY_View->sizePolicy().hasHeightForWidth());
        CARRY_View->setSizePolicy(sizePolicy6);

        horizontalLayout->addWidget(CARRY_View);

        NEG_View = new QCheckBox(layoutWidget);
        NEG_View->setObjectName(QString::fromUtf8("NEG_View"));
        sizePolicy6.setHeightForWidth(NEG_View->sizePolicy().hasHeightForWidth());
        NEG_View->setSizePolicy(sizePolicy6);

        horizontalLayout->addWidget(NEG_View);

        ZERO_View = new QCheckBox(layoutWidget);
        ZERO_View->setObjectName(QString::fromUtf8("ZERO_View"));
        sizePolicy6.setHeightForWidth(ZERO_View->sizePolicy().hasHeightForWidth());
        ZERO_View->setSizePolicy(sizePolicy6);

        horizontalLayout->addWidget(ZERO_View);


        verticalLayout->addLayout(horizontalLayout);

        gridLayout_2 = new QGridLayout();
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        Reg0_Label = new QLabel(layoutWidget);
        Reg0_Label->setObjectName(QString::fromUtf8("Reg0_Label"));
        sizePolicy3.setHeightForWidth(Reg0_Label->sizePolicy().hasHeightForWidth());
        Reg0_Label->setSizePolicy(sizePolicy3);

        gridLayout_2->addWidget(Reg0_Label, 0, 0, 1, 1);

        Reg8_Label = new QLabel(layoutWidget);
        Reg8_Label->setObjectName(QString::fromUtf8("Reg8_Label"));

        gridLayout_2->addWidget(Reg8_Label, 0, 2, 1, 1);

        Reg1_Label = new QLabel(layoutWidget);
        Reg1_Label->setObjectName(QString::fromUtf8("Reg1_Label"));

        gridLayout_2->addWidget(Reg1_Label, 1, 0, 1, 1);

        Reg9_Label = new QLabel(layoutWidget);
        Reg9_Label->setObjectName(QString::fromUtf8("Reg9_Label"));

        gridLayout_2->addWidget(Reg9_Label, 1, 2, 1, 1);

        Reg2_Label = new QLabel(layoutWidget);
        Reg2_Label->setObjectName(QString::fromUtf8("Reg2_Label"));

        gridLayout_2->addWidget(Reg2_Label, 2, 0, 1, 1);

        Rega_Label = new QLabel(layoutWidget);
        Rega_Label->setObjectName(QString::fromUtf8("Rega_Label"));

        gridLayout_2->addWidget(Rega_Label, 2, 2, 1, 1);

        Reg3_Label = new QLabel(layoutWidget);
        Reg3_Label->setObjectName(QString::fromUtf8("Reg3_Label"));

        gridLayout_2->addWidget(Reg3_Label, 3, 0, 1, 1);

        Regb_Label = new QLabel(layoutWidget);
        Regb_Label->setObjectName(QString::fromUtf8("Regb_Label"));

        gridLayout_2->addWidget(Regb_Label, 3, 2, 1, 1);

        Reg4_Label = new QLabel(layoutWidget);
        Reg4_Label->setObjectName(QString::fromUtf8("Reg4_Label"));

        gridLayout_2->addWidget(Reg4_Label, 4, 0, 1, 1);

        Regc_Label = new QLabel(layoutWidget);
        Regc_Label->setObjectName(QString::fromUtf8("Regc_Label"));

        gridLayout_2->addWidget(Regc_Label, 4, 2, 1, 1);

        Reg5_Label = new QLabel(layoutWidget);
        Reg5_Label->setObjectName(QString::fromUtf8("Reg5_Label"));

        gridLayout_2->addWidget(Reg5_Label, 5, 0, 1, 1);

        Reg6_Label = new QLabel(layoutWidget);
        Reg6_Label->setObjectName(QString::fromUtf8("Reg6_Label"));

        gridLayout_2->addWidget(Reg6_Label, 6, 0, 1, 1);

        Rege_Label = new QLabel(layoutWidget);
        Rege_Label->setObjectName(QString::fromUtf8("Rege_Label"));

        gridLayout_2->addWidget(Rege_Label, 6, 2, 1, 1);

        Reg7_Label = new QLabel(layoutWidget);
        Reg7_Label->setObjectName(QString::fromUtf8("Reg7_Label"));

        gridLayout_2->addWidget(Reg7_Label, 7, 0, 1, 1);

        Regf_Label = new QLabel(layoutWidget);
        Regf_Label->setObjectName(QString::fromUtf8("Regf_Label"));

        gridLayout_2->addWidget(Regf_Label, 7, 2, 1, 1);

        Reg8_View = new QLineEdit(layoutWidget);
        Reg8_View->setObjectName(QString::fromUtf8("Reg8_View"));
        sizePolicy5.setHeightForWidth(Reg8_View->sizePolicy().hasHeightForWidth());
        Reg8_View->setSizePolicy(sizePolicy5);
        Reg8_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Reg8_View, 0, 3, 1, 1);

        Reg1_View = new QLineEdit(layoutWidget);
        Reg1_View->setObjectName(QString::fromUtf8("Reg1_View"));
        sizePolicy5.setHeightForWidth(Reg1_View->sizePolicy().hasHeightForWidth());
        Reg1_View->setSizePolicy(sizePolicy5);
        Reg1_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Reg1_View, 1, 1, 1, 1);

        Reg9_View = new QLineEdit(layoutWidget);
        Reg9_View->setObjectName(QString::fromUtf8("Reg9_View"));
        sizePolicy5.setHeightForWidth(Reg9_View->sizePolicy().hasHeightForWidth());
        Reg9_View->setSizePolicy(sizePolicy5);
        Reg9_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Reg9_View, 1, 3, 1, 1);

        Reg2_View = new QLineEdit(layoutWidget);
        Reg2_View->setObjectName(QString::fromUtf8("Reg2_View"));
        sizePolicy5.setHeightForWidth(Reg2_View->sizePolicy().hasHeightForWidth());
        Reg2_View->setSizePolicy(sizePolicy5);
        Reg2_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Reg2_View, 2, 1, 1, 1);

        Rega_View = new QLineEdit(layoutWidget);
        Rega_View->setObjectName(QString::fromUtf8("Rega_View"));
        sizePolicy5.setHeightForWidth(Rega_View->sizePolicy().hasHeightForWidth());
        Rega_View->setSizePolicy(sizePolicy5);
        Rega_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Rega_View, 2, 3, 1, 1);

        Reg3_View = new QLineEdit(layoutWidget);
        Reg3_View->setObjectName(QString::fromUtf8("Reg3_View"));
        sizePolicy5.setHeightForWidth(Reg3_View->sizePolicy().hasHeightForWidth());
        Reg3_View->setSizePolicy(sizePolicy5);
        Reg3_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Reg3_View, 3, 1, 1, 1);

        Regb_View = new QLineEdit(layoutWidget);
        Regb_View->setObjectName(QString::fromUtf8("Regb_View"));
        sizePolicy5.setHeightForWidth(Regb_View->sizePolicy().hasHeightForWidth());
        Regb_View->setSizePolicy(sizePolicy5);
        Regb_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Regb_View, 3, 3, 1, 1);

        Reg4_View = new QLineEdit(layoutWidget);
        Reg4_View->setObjectName(QString::fromUtf8("Reg4_View"));
        sizePolicy5.setHeightForWidth(Reg4_View->sizePolicy().hasHeightForWidth());
        Reg4_View->setSizePolicy(sizePolicy5);
        Reg4_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Reg4_View, 4, 1, 1, 1);

        Reg5_View = new QLineEdit(layoutWidget);
        Reg5_View->setObjectName(QString::fromUtf8("Reg5_View"));
        sizePolicy5.setHeightForWidth(Reg5_View->sizePolicy().hasHeightForWidth());
        Reg5_View->setSizePolicy(sizePolicy5);
        Reg5_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Reg5_View, 5, 1, 1, 1);

        Regd_View = new QLineEdit(layoutWidget);
        Regd_View->setObjectName(QString::fromUtf8("Regd_View"));
        sizePolicy5.setHeightForWidth(Regd_View->sizePolicy().hasHeightForWidth());
        Regd_View->setSizePolicy(sizePolicy5);
        Regd_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Regd_View, 5, 3, 1, 1);

        Reg6_View = new QLineEdit(layoutWidget);
        Reg6_View->setObjectName(QString::fromUtf8("Reg6_View"));
        sizePolicy5.setHeightForWidth(Reg6_View->sizePolicy().hasHeightForWidth());
        Reg6_View->setSizePolicy(sizePolicy5);
        Reg6_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Reg6_View, 6, 1, 1, 1);

        Rege_View = new QLineEdit(layoutWidget);
        Rege_View->setObjectName(QString::fromUtf8("Rege_View"));
        sizePolicy5.setHeightForWidth(Rege_View->sizePolicy().hasHeightForWidth());
        Rege_View->setSizePolicy(sizePolicy5);
        Rege_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Rege_View, 6, 3, 1, 1);

        Reg7_View = new QLineEdit(layoutWidget);
        Reg7_View->setObjectName(QString::fromUtf8("Reg7_View"));
        sizePolicy5.setHeightForWidth(Reg7_View->sizePolicy().hasHeightForWidth());
        Reg7_View->setSizePolicy(sizePolicy5);
        Reg7_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Reg7_View, 7, 1, 1, 1);

        Regf_View = new QLineEdit(layoutWidget);
        Regf_View->setObjectName(QString::fromUtf8("Regf_View"));
        sizePolicy5.setHeightForWidth(Regf_View->sizePolicy().hasHeightForWidth());
        Regf_View->setSizePolicy(sizePolicy5);
        Regf_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Regf_View, 7, 3, 1, 1);

        Reg0_View = new QLineEdit(layoutWidget);
        Reg0_View->setObjectName(QString::fromUtf8("Reg0_View"));
        QSizePolicy sizePolicy7(QSizePolicy::Preferred, QSizePolicy::Minimum);
        sizePolicy7.setHorizontalStretch(108);
        sizePolicy7.setVerticalStretch(0);
        sizePolicy7.setHeightForWidth(Reg0_View->sizePolicy().hasHeightForWidth());
        Reg0_View->setSizePolicy(sizePolicy7);
        Reg0_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Reg0_View, 0, 1, 1, 1);

        Regc_View = new QLineEdit(layoutWidget);
        Regc_View->setObjectName(QString::fromUtf8("Regc_View"));
        sizePolicy5.setHeightForWidth(Regc_View->sizePolicy().hasHeightForWidth());
        Regc_View->setSizePolicy(sizePolicy5);
        Regc_View->setMaximumSize(QSize(108, 16777215));

        gridLayout_2->addWidget(Regc_View, 4, 3, 1, 1);

        Regd_Label = new QLabel(layoutWidget);
        Regd_Label->setObjectName(QString::fromUtf8("Regd_Label"));

        gridLayout_2->addWidget(Regd_Label, 5, 2, 1, 1);


        verticalLayout->addLayout(gridLayout_2);

        CommandWindow = new QPlainTextEdit(centralwidget);
        CommandWindow->setObjectName(QString::fromUtf8("CommandWindow"));
        CommandWindow->setGeometry(QRect(10, 660, 720, 144));
        QSizePolicy sizePolicy8(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
        sizePolicy8.setHorizontalStretch(0);
        sizePolicy8.setVerticalStretch(144);
        sizePolicy8.setHeightForWidth(CommandWindow->sizePolicy().hasHeightForWidth());
        CommandWindow->setSizePolicy(sizePolicy8);
        CommandWindow->setMinimumSize(QSize(600, 144));
        CommandWindow->setMaximumSize(QSize(16777215, 144));
        CommandInput = new QLineEdit(centralwidget);
        CommandInput->setObjectName(QString::fromUtf8("CommandInput"));
        CommandInput->setGeometry(QRect(10, 810, 721, 30));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 748, 24));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName(QString::fromUtf8("menuHelp"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);
        QWidget::setTabOrder(Dec_OP, Hex_OP);
        QWidget::setTabOrder(Hex_OP, Bscl_Set);
        QWidget::setTabOrder(Bscl_Set, PC_View);
        QWidget::setTabOrder(PC_View, AC_View);
        QWidget::setTabOrder(AC_View, Stk1_View);
        QWidget::setTabOrder(Stk1_View, Stk2_View);
        QWidget::setTabOrder(Stk2_View, Stk3_View);
        QWidget::setTabOrder(Stk3_View, Stk4_View);
        QWidget::setTabOrder(Stk4_View, Stk5_View);
        QWidget::setTabOrder(Stk5_View, THI_View);
        QWidget::setTabOrder(THI_View, Reg0_View);
        QWidget::setTabOrder(Reg0_View, Reg1_View);
        QWidget::setTabOrder(Reg1_View, Reg2_View);
        QWidget::setTabOrder(Reg2_View, Reg3_View);
        QWidget::setTabOrder(Reg3_View, Reg4_View);
        QWidget::setTabOrder(Reg4_View, Reg5_View);
        QWidget::setTabOrder(Reg5_View, Reg6_View);
        QWidget::setTabOrder(Reg6_View, Reg7_View);
        QWidget::setTabOrder(Reg7_View, Reg8_View);
        QWidget::setTabOrder(Reg8_View, Reg9_View);
        QWidget::setTabOrder(Reg9_View, Rega_View);
        QWidget::setTabOrder(Rega_View, Regb_View);
        QWidget::setTabOrder(Regb_View, Regc_View);
        QWidget::setTabOrder(Regc_View, Regd_View);
        QWidget::setTabOrder(Regd_View, Rege_View);
        QWidget::setTabOrder(Rege_View, Regf_View);
        QWidget::setTabOrder(Regf_View, InstrViewer);
        QWidget::setTabOrder(InstrViewer, CARRY_View);
        QWidget::setTabOrder(CARRY_View, NEG_View);
        QWidget::setTabOrder(NEG_View, ZERO_View);
        QWidget::setTabOrder(ZERO_View, DataViewer);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionLoad_Object);
        menuFile->addAction(actionLoad_Symbols);
        menuFile->addAction(actionLoad_Data);
        menuFile->addAction(actionQuit);
        menuFile->addSeparator();
        menuHelp->addAction(actionCommand_Summary);
        menuHelp->addSeparator();
        menuHelp->addAction(actionAbout);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "SSP2 Interpreter/Debugger", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        MainWindow->setToolTip(QApplication::translate("MainWindow", "Enter debugger commands here", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        actionLoad_Object->setText(QApplication::translate("MainWindow", "Load &Object", 0, QApplication::UnicodeUTF8));
        actionLoad_Symbols->setText(QApplication::translate("MainWindow", "Load &Symbols", 0, QApplication::UnicodeUTF8));
        actionLoad_Data->setText(QApplication::translate("MainWindow", "Load &Data", 0, QApplication::UnicodeUTF8));
        actionQuit->setText(QApplication::translate("MainWindow", "&Quit", 0, QApplication::UnicodeUTF8));
        actionCommand_Summary->setText(QApplication::translate("MainWindow", "Command Summary", 0, QApplication::UnicodeUTF8));
        actionAbout->setText(QApplication::translate("MainWindow", "About", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        DataViewer->setToolTip(QApplication::translate("MainWindow", "Memory data viewer", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        InstrViewer->setToolTip(QApplication::translate("MainWindow", "Program instruction viewer", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        State_Label->setText(QApplication::translate("MainWindow", "Machine State", 0, QApplication::UnicodeUTF8));
        Dec_OP->setText(QApplication::translate("MainWindow", "Dec", 0, QApplication::UnicodeUTF8));
        Hex_OP->setText(QApplication::translate("MainWindow", "Hex", 0, QApplication::UnicodeUTF8));
        Bscl_Label->setText(QApplication::translate("MainWindow", "Bscl", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        Bscl_Set->setToolTip(QApplication::translate("MainWindow", "Set binary scale for register displays", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        Bscl_Set->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        PC_Label->setText(QApplication::translate("MainWindow", "PC", 0, QApplication::UnicodeUTF8));
        PC_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        THI_Label->setText(QApplication::translate("MainWindow", "THI", 0, QApplication::UnicodeUTF8));
        THI_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        RAd0_Label->setText(QApplication::translate("MainWindow", "RAd0", 0, QApplication::UnicodeUTF8));
        RAd0_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        RAd1_Label->setText(QApplication::translate("MainWindow", "RAd1", 0, QApplication::UnicodeUTF8));
        RAd1_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        AC_Label->setText(QApplication::translate("MainWindow", "AC", 0, QApplication::UnicodeUTF8));
        AC_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Stk1_Label->setText(QApplication::translate("MainWindow", "AC(1)", 0, QApplication::UnicodeUTF8));
        Stk1_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Stk2_Label->setText(QApplication::translate("MainWindow", "AC(2)", 0, QApplication::UnicodeUTF8));
        Stk2_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Stk3_Label->setText(QApplication::translate("MainWindow", "AC(3)", 0, QApplication::UnicodeUTF8));
        Stk3_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Stk4_Label->setText(QApplication::translate("MainWindow", "AC(4)", 0, QApplication::UnicodeUTF8));
        Stk5_Label->setText(QApplication::translate("MainWindow", "AC(5)", 0, QApplication::UnicodeUTF8));
        Stk5_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Stk4_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        CARRY_View->setText(QApplication::translate("MainWindow", "CARRY", 0, QApplication::UnicodeUTF8));
        NEG_View->setText(QApplication::translate("MainWindow", "NEG", 0, QApplication::UnicodeUTF8));
        ZERO_View->setText(QApplication::translate("MainWindow", "ZERO", 0, QApplication::UnicodeUTF8));
        Reg0_Label->setText(QApplication::translate("MainWindow", "reg0", 0, QApplication::UnicodeUTF8));
        Reg8_Label->setText(QApplication::translate("MainWindow", "reg8", 0, QApplication::UnicodeUTF8));
        Reg1_Label->setText(QApplication::translate("MainWindow", "reg1", 0, QApplication::UnicodeUTF8));
        Reg9_Label->setText(QApplication::translate("MainWindow", "reg9", 0, QApplication::UnicodeUTF8));
        Reg2_Label->setText(QApplication::translate("MainWindow", "reg2", 0, QApplication::UnicodeUTF8));
        Rega_Label->setText(QApplication::translate("MainWindow", "rega", 0, QApplication::UnicodeUTF8));
        Reg3_Label->setText(QApplication::translate("MainWindow", "reg3", 0, QApplication::UnicodeUTF8));
        Regb_Label->setText(QApplication::translate("MainWindow", "regb", 0, QApplication::UnicodeUTF8));
        Reg4_Label->setText(QApplication::translate("MainWindow", "reg4", 0, QApplication::UnicodeUTF8));
        Regc_Label->setText(QApplication::translate("MainWindow", "regc", 0, QApplication::UnicodeUTF8));
        Reg5_Label->setText(QApplication::translate("MainWindow", "reg5", 0, QApplication::UnicodeUTF8));
        Reg6_Label->setText(QApplication::translate("MainWindow", "reg6", 0, QApplication::UnicodeUTF8));
        Rege_Label->setText(QApplication::translate("MainWindow", "rege", 0, QApplication::UnicodeUTF8));
        Reg7_Label->setText(QApplication::translate("MainWindow", "reg7", 0, QApplication::UnicodeUTF8));
        Regf_Label->setText(QApplication::translate("MainWindow", "regf", 0, QApplication::UnicodeUTF8));
        Reg8_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Reg1_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Reg9_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Reg2_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Rega_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Reg3_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Regb_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Reg4_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Reg5_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Regd_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Reg6_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Rege_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Reg7_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Regf_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Reg0_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Regc_View->setText(QApplication::translate("MainWindow", "0", 0, QApplication::UnicodeUTF8));
        Regd_Label->setText(QApplication::translate("MainWindow", "regd", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        CommandWindow->setToolTip(QApplication::translate("MainWindow", "Output of debugger commands appears here", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        CommandInput->setToolTip(QApplication::translate("MainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Bitstream Vera Sans Mono'; font-size:13pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Type debugger commands here</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        menuFile->setTitle(QApplication::translate("MainWindow", "File", 0, QApplication::UnicodeUTF8));
        menuHelp->setTitle(QApplication::translate("MainWindow", "Help", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SPP2DBG_H
