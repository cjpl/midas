#--------------------------------------------------------------------------
#
#     PLed_plugin.pro
#
#--------------------------------------------------------------------------
#     author               : Andreas Suter, 2004/09/27
#     copyright            : (C) 2004 by
#     email                : andreas.suter@psi.ch
#
#  $Id$
#--------------------------------------------------------------------------
#
#    This library is free software; you can redistribute it and/or
#    modify it under the terms of the GNU Library General Public
#    License as published by the Free Software Foundation; either
#    version 2 of the License, or (at your option) any later version.
#
#    This library is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#    Library General Public License for more details.
#
#    You should have received a copy of the GNU Library General Public License
#    along with this library; see the file COPYING.LIB.  If not, write to
#    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
#    Boston, MA 02111-1307, USA.
#
#--------------------------------------------------------------------------

BaseDir   = /home/nemu/midas/midas-1.9.5/gui/hvedit/qt-3.3
SOURCES  += PLed_plugin.cpp \
            $${BaseDir}/base/src/PLed.cpp
HEADERS  += PLed_plugin.h \
            $${BaseDir}/base/include/PLed.h
DESTDIR   = $$(QTDIR)/plugins/designer
TARGET    = PLed

target.path=$${DESTDIR}

INSTALLS    += target
TEMPLATE     = lib
CONFIG      += qt warn_on release plugin
INCLUDEPATH += $${BaseDir}/base/include
DBFILE       = plugin.db
LANGUAGE     = C++
CONFIG      += thread
