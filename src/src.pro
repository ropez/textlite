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
    bundlemanager.cpp \
    regex.cpp \
    theme.cpp \
    scopeselector.cpp \
    grammar.cpp

HEADERS  += mainwindow.h \
    navigator.h \
    window.h \
    editor.h \
    highlighter.h \
    plistreader.h \
    bundlemanager.h \
    regex.h \
    theme.h \
    scopeselector.h \
    grammar.h \
    ruledata.h

FORMS += \
    navigator.ui

unix|win32: LIBS += -lonig



win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libqgit2/release/ -lqgit2
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libqgit2/debug/ -lqgit2
else:symbian: LIBS += -lqgit2
else:unix: LIBS += -L$$OUT_PWD/../libqgit2/ -lqgit2

INCLUDEPATH += $$PWD/../libqgit2/libgit2/include $$PWD/../libqgit2 $$PWD/../libqgit2/src
DEPENDPATH += $$PWD/../libqgit2

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libqgit2/release/libqgit2.lib
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libqgit2/debug/libqgit2.lib
else:unix:!symbian: PRE_TARGETDEPS += $$OUT_PWD/../libqgit2/libqgit2.a
