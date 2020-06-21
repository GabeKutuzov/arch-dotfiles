/****************************************************************************
** Meta object code from reading C++ file 'spp2dbg.h'
**
** Created: Wed Dec 16 15:58:30 2009
**      by: The Qt Meta Object Compiler version 61 (Qt 4.5.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "spp2dbg.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'spp2dbg.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 61
#error "This file was generated using the moc from 4.5.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DbgMainWindow[] = {

 // content:
       2,       // revision
       0,       // classname
       0,    0, // classinfo
      37,   12, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors

 // slots: signature, parameters, type, tag, flags
      15,   14,   14,   14, 0x08,
      34,   14,   14,   14, 0x08,
      58,   14,   14,   14, 0x08,
      79,   14,   14,   14, 0x08,
     101,   14,   14,   14, 0x08,
     120,   14,   14,   14, 0x08,
     133,   14,   14,   14, 0x08,
     153,   14,   14,   14, 0x08,
     173,   14,   14,   14, 0x08,
     197,   14,   14,   14, 0x08,
     217,   14,   14,   14, 0x08,
     237,   14,   14,   14, 0x08,
     255,   14,   14,   14, 0x08,
     275,   14,   14,   14, 0x08,
     295,   14,   14,   14, 0x08,
     315,   14,   14,   14, 0x08,
     335,   14,   14,   14, 0x08,
     355,   14,   14,   14, 0x08,
     374,   14,   14,   14, 0x08,
     388,   14,   14,   14, 0x08,
     402,   14,   14,   14, 0x08,
     416,   14,   14,   14, 0x08,
     436,   14,   14,   14, 0x08,
     456,   14,   14,   14, 0x08,
     476,   14,   14,   14, 0x08,
     496,   14,   14,   14, 0x08,
     516,   14,   14,   14, 0x08,
     536,   14,   14,   14, 0x08,
     556,   14,   14,   14, 0x08,
     576,   14,   14,   14, 0x08,
     596,   14,   14,   14, 0x08,
     616,   14,   14,   14, 0x08,
     636,   14,   14,   14, 0x08,
     656,   14,   14,   14, 0x08,
     676,   14,   14,   14, 0x08,
     696,   14,   14,   14, 0x08,
     716,   14,   14,   14, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_DbgMainWindow[] = {
    "DbgMainWindow\0\0DisplayAboutInfo()\0"
    "DisplayCommandSummary()\0menuLoadObjectFile()\0"
    "menuLoadSymbolsFile()\0menuLoadDataFile()\0"
    "RunCommand()\0setDisplayBaseDec()\0"
    "setDisplayBaseHex()\0setDisplayBscl(QString)\0"
    "updVM_RAd0(QString)\0updVM_RAd1(QString)\0"
    "updVM_AC(QString)\0updVM_Stk1(QString)\0"
    "updVM_Stk2(QString)\0updVM_Stk3(QString)\0"
    "updVM_Stk4(QString)\0updVM_Stk5(QString)\0"
    "updVM_THI(QString)\0updVM_CF(int)\0"
    "updVM_NF(int)\0updVM_ZF(int)\0"
    "updVM_Reg0(QString)\0updVM_Reg1(QString)\0"
    "updVM_Reg2(QString)\0updVM_Reg3(QString)\0"
    "updVM_Reg4(QString)\0updVM_Reg5(QString)\0"
    "updVM_Reg6(QString)\0updVM_Reg7(QString)\0"
    "updVM_Reg8(QString)\0updVM_Reg9(QString)\0"
    "updVM_Rega(QString)\0updVM_Regb(QString)\0"
    "updVM_Regc(QString)\0updVM_Regd(QString)\0"
    "updVM_Rege(QString)\0updVM_Regf(QString)\0"
};

const QMetaObject DbgMainWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_DbgMainWindow,
      qt_meta_data_DbgMainWindow, 0 }
};

const QMetaObject *DbgMainWindow::metaObject() const
{
    return &staticMetaObject;
}

void *DbgMainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DbgMainWindow))
        return static_cast<void*>(const_cast< DbgMainWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int DbgMainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: DisplayAboutInfo(); break;
        case 1: DisplayCommandSummary(); break;
        case 2: menuLoadObjectFile(); break;
        case 3: menuLoadSymbolsFile(); break;
        case 4: menuLoadDataFile(); break;
        case 5: RunCommand(); break;
        case 6: setDisplayBaseDec(); break;
        case 7: setDisplayBaseHex(); break;
        case 8: setDisplayBscl((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 9: updVM_RAd0((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: updVM_RAd1((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 11: updVM_AC((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 12: updVM_Stk1((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 13: updVM_Stk2((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 14: updVM_Stk3((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 15: updVM_Stk4((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 16: updVM_Stk5((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 17: updVM_THI((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 18: updVM_CF((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 19: updVM_NF((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 20: updVM_ZF((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 21: updVM_Reg0((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 22: updVM_Reg1((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 23: updVM_Reg2((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 24: updVM_Reg3((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 25: updVM_Reg4((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 26: updVM_Reg5((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 27: updVM_Reg6((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 28: updVM_Reg7((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 29: updVM_Reg8((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 30: updVM_Reg9((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 31: updVM_Rega((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 32: updVM_Regb((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 33: updVM_Regc((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 34: updVM_Regd((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 35: updVM_Rege((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 36: updVM_Regf((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
        _id -= 37;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
