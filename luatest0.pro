CONFIG += c++14
include( $$PWD/lua/lua.pri )

QT += core
QT += gui
QT += widgets
CONFIG += console
HEADERS += \
    MainTest.hpp

SOURCES += \
    MainTest.cpp


