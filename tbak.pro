TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    nodedb.cpp \
    folderdb.cpp \
    serialize.cpp \
    settings.cpp \
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
    filetime.cpp \
    servercommands.cpp \
    source.cpp \
    archive.cpp \
    sourcefile.cpp \
    humanreadable.cpp \
    archivefile.cpp \
    pathtools.cpp

CONFIG += c++11

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    nodedb.h \
    folderdb.h \
    serialize.h \
    settings.h \
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
    filetime.h \
    source.h \
    archive.h \
    sourcefile.h \
    humanreadable.h \
    archivefile.h \
    pathtools.h \
    vt100.h

LIBS += -lsodium -lz
