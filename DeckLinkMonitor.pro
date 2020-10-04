
TARGET = DeckLinkMonitor
TEMPLATE = app
QT += core gui widgets


CONFIG += c++11

DESTDIR = $$PWD/_bin

linux:LIBS += -ldl
win32:LIBS += -lole32 -loleaut32
macx:LIBS += -framework CoreFoundation
unix:LIBS += -lavdevice -lavformat -lavfilter -lavcodec -lswresample -lswscale -lavutil

win32:INCLUDEPATH += "C:/Program Files/ffmpeg-4.2-25-lgpl21-x64-windows/installed/x64-windows/include"
win32:LIBS += "-LC:/Program Files/ffmpeg-4.2-25-lgpl21-x64-windows/installed/x64-windows/lib"
win32:LIBS += -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lbz2 -llibcharset -llibiconv -llzma -lopus -lsnappy -lsoxr -lswresample -lswscale -lvpxmd -lwavpackdll -lzlib

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
