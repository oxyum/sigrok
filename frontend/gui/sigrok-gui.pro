##
## This file is part of the sigrok project.
##
## Copyright (C) 2010 Uwe Hermann <uwe@hermann-uwe.de>
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
##

TARGET     = sigrok-gui
TEMPLATE   = app

SOURCES   += main.cpp \
	     mainwindow.cpp \
	     channelrenderarea.cpp \
	     configform.cpp

HEADERS   += mainwindow.h \
	     channelrenderarea.h \
	     configform.h

FORMS     += mainwindow.ui \
	     configform.ui

LIBS      += -L../../backend -lbackend -lgmodule-2.0 -lglib-2.0 -lusb-1.0 -lzip

INCLUDEPATH += /usr/include/glib-2.0 /usr/lib/glib-2.0/include \
	       /usr/include/libusb-1.0 ../../include ../../backend

RESOURCES += sigrok-gui.qrc

# TODO: This may need fixes.
macx {
	LIBS += -L/opt/local/lib -L/opt/local/lib/glib-2.0 -lgmodule-2.0 -lglib-2.0
	INCLUDEPATH += /opt/local/lib/glib-2.0/include /opt/local/include/glib-2.0
	#FILETYPES.files = ../lib/libsigrok.dylib
	#FILETYPES.path = Contents/Frameworks
	#QMAKE_BUNDLE_DATA += FILETYPES
}else{
	LIBS += -Wl,--export-dynamic
}
