TEMPLATE = app
TARGET = testmain
DEPENDPATH += .
INCLUDEPATH += . ../plugin
QMAKE_CXXFLAGS += -DTETLIBRARY
CONFIG += release console
SOURCES += main.cpp
INSTALLS += target
target.path = .
