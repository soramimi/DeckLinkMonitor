
TARGET = DeckLinkMonitor
TEMPLATE = app
QT += core gui widgets


CONFIG += c++11

DESTDIR = $$PWD/_bin

linux:LIBS += -ldl
win32:LIBS += -lole32 -loleaut32
macx:LIBS += -framework CoreFoundation
LIBS += -lavdevice -lavformat -lavfilter -lavcodec -lswresample -lswscale -lavutil

SOURCES += \
	DeckLinkMonitor.cpp \
	Image.cpp \
	ImageUtil.cpp \
        MainWindow.cpp \
	VideoDecoder.cpp \
        main.cpp \
    DeckLinkAPI.cpp

HEADERS += \
        DeckLinkAPI.h \
        DeckLinkMonitor.h \
        Image.h \
        ImageUtil.h \
        MainWindow.h \
        VideoDecoder.h

FORMS += \
        MainWindow.ui

win32:SOURCES += sdk/Win/DeckLinkAPI_i.c
win32:HEADERS += sdk/Win/DeckLinkAPI_h.h
linux:SOURCES += sdk/Linux/include/DeckLinkAPIDispatch.cpp
macx:SOURCES += sdk/Mac/include/DeckLinkAPIDispatch.cpp
