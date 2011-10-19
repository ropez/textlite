#-------------------------------------------------
#
# Project created by QtCreator 2011-10-09T18:29:03
#
#-------------------------------------------------

QT       += core gui

TARGET = textlite
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    navigator.cpp \
    window.cpp \
    editor.cpp \
    highlighter.cpp \
    plistreader.cpp \
    bundlemanager.cpp

HEADERS  += mainwindow.h \
    navigator.h \
    window.h \
    editor.h \
    highlighter.h \
    plistreader.h \
    bundlemanager.h

FORMS += \
    navigator.ui

unix|win32: LIBS += -lboost_regex
