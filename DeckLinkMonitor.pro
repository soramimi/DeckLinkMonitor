
TARGET = DeckLinkMonitor
TEMPLATE = app
QT += core gui widgets


CONFIG += c++11

linux:LIBS += -ldl

SOURCES += \
        MainWindow.cpp \
        main.cpp \
        platform.cpp

HEADERS += \
        DeckLinkAPI.h \
        MainWindow.h \
        platform.h

FORMS += \
        MainWindow.ui

win32:SOURCES += sdk/Win/DeckLinkAPI_i.c
win32:HEADERS += sdk/Win/DeckLinkAPI_h.h
linux:SOURCES += sdk/Linux/include/DeckLinkAPIDispatch.cpp
macx:SOURCES += sdk/Mac/include/DeckLinkAPIDispatch.cpp
