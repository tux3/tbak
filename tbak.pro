TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    nodedb.cpp \
    folderdb.cpp \
    folder.cpp \
    file.cpp \
    serialize.cpp \
    settings.cpp \
    humanReadable.cpp \
    node.cpp \
    net.cpp \
    crypto.cpp \
    netpacket.cpp \
    netaddr.cpp \
    netstream.cpp \
    netsock.cpp \
    server.cpp \
    compression.cpp \
    filelocker.cpp

CONFIG += c++11

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    nodedb.h \
    folderdb.h \
    folder.h \
    file.h \
    serialize.h \
    settings.h \
    humanReadable.h \
    node.h \
    net.h \
    crypto.h \
    netpacket.h \
    netaddr.h \
    netstream.h \
    netsock.h \
    server.h \
    compression.h \
    filelocker.h

LIBS += -lsodium -lz
