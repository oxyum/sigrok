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

TARGET        = sigrok-gui
TEMPLATE      = app

SOURCES      += main.cpp \
	        mainwindow.cpp \
	        channelrenderarea.cpp \
	        configform.cpp

HEADERS      += mainwindow.h \
	        channelrenderarea.h \
	        configform.h

FORMS        += mainwindow.ui \
	        configform.ui

TRANSLATIONS  = locale/sigrok-gui_de_DE.ts

CONFIG       += link_pkgconfig
PKGCONFIG     = gmodule-2.0 glib-2.0 libusb-1.0 libzip

LIBS         += -L../../backend -lbackend 

INCLUDEPATH  += ../../include ../../backend

RESOURCES    += sigrok-gui.qrc

win32 {
	RC_FILE = sigrok-gui.rc
}

# TODO: This may need fixes.
macx {
	ICON = icons/sigrok-gui.icns
	#FILETYPES.files = ../lib/libsigrok.dylib
	#FILETYPES.path = Contents/Frameworks
	#QMAKE_BUNDLE_DATA += FILETYPES
} else {
	LIBS += -Wl,--export-dynamic
}
