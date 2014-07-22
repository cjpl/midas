######################################################################
#
# qmake file for the Qt hvEdit project
#
# Andreas Suter, 2007/01/08
#
# $Id$
#
######################################################################

###########################################################################
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
###########################################################################

MidasRoot = $$(MIDASSYS)
myHome    = $${MidasRoot}/gui/hvedit/qt-3.3
BaseInc   = $${myHome}/base/include
BaseSrc   = $${myHome}/base/src
BaseForms = $${myHome}/base/forms

TARGET       = hvEdit
target.path  = $${myHome}/hvEdit/bin
INSTALLS    += target

# install path for the XML configuration file
unix:  xml.path = $${myHome}/hvEdit/bin/xmls
xml.files = hvEdit-example.xml
INSTALLS += xml

SOURCES = hvEdit.cpp \
          PHvEdit.cpp \
          PHvAdmin.cpp \
          $${BaseSrc}/PPwd.cpp \
          $${BaseSrc}/PGlobals.cpp \
          $${BaseSrc}/PExperiment.cpp \
          $${BaseSrc}/PKey.cpp \
          help/PHelpWindow.cpp 

HEADERS = PHvEdit.h \
          PHvAdmin.h \
          $${BaseInc}/PPwd.h \
          $${BaseInc}/PGlobals.h \
          $${BaseInc}/PExperiment.h \
          $${BaseInc}/PKey.h \
          help/PHelpWindow.h 

FORMS   = forms/PHvEditBase.ui \
          forms/PHvEditAbout.ui \
          forms/PFileList.ui \
          $${BaseForms}/PPwdBase.ui

CONFIG += qt console warn_on debug

DEFINES += QT_MIDAS_MSG

# Attention! the variable HOME needs to be set to the
#            needed path.
INCLUDEPATH += ./forms
INCLUDEPATH += $$(MIDASSYS)/include
INCLUDEPATH += $${BaseInc}
INCLUDEPATH += $${BaseForms}

#
# linux standard libs
#
unix:LIBS += -lbsd -lm -lutil -lmidas -lrt
#
