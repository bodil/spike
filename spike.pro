
QT += core widgets x11extras
CONFIG += debug link_pkgconfig

unix:INCLUDEPATH += $(HOME)/.nix-profile/include
unix:LIBS += -L$(HOME)/.nix-profile/lib -lX11

DESTDIR = build
TARGET = spike
HEADERS += spike.h x11.h history.h entry.h
SOURCES += spike.cpp x11.cpp history.cpp entry.cpp
RESOURCES += spike.qrc
#PKGCONFIG += dbus-1
QMAKE_CXXFLAGS += -std=gnu++11
