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
    netsock.cpp \
    server.cpp \
    compression.cpp \
    filelocker.cpp \
    sighandlers.cpp \
    commands.cpp \
    pathhash.cpp \
    filetime.cpp

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
    netsock.h \
    server.h \
    compression.h \
    filelocker.h \
    sighandlers.h \
    commands.h \
    pathhash.h \
    filetime.h

LIBS += -lsodium -lz
