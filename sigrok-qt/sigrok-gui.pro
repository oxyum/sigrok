##
## This file is part of the sigrok project.
##
## Copyright (C) 2010-2011 Uwe Hermann <uwe@hermann-uwe.de>
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

# The sigrok-gui version number. Define APP_VERSION macro for use in the code.
VERSION       = 0.2
DEFINES      += APP_VERSION=\\\"$$VERSION\\\"

UNAME         = $$system(uname -s)

SOURCES      += main.cpp \
	        mainwindow.cpp \
	        configform.cpp \
	        sampleiodevice.cpp \
	        channelform.cpp \
	        decodersform.cpp

HEADERS      += mainwindow.h \
	        configform.h \
	        sampleiodevice.h \
	        channelform.h \
	        decodersform.h

FORMS        += mainwindow.ui \
	        configform.ui \
	        channelform.ui \
	        decodersform.ui

TRANSLATIONS  = locale/sigrok-gui_de_DE.ts \
                locale/sigrok-gui_nl_NL.ts \
                locale/sigrok-gui_fr_FR.ts

CONFIG       += release warn_on

RESOURCES    += sigrok-gui.qrc

# libsigrok and libsigrokdecode
win32 {
	# On Windows/MinGW we need to use '--libs --static'.
	# We also need to strip some stray '\n' characters here.
	QMAKE_CXXFLAGS += $$system(pkg-config --cflags libsigrokdecode \
			  libsigrok | sed s/\n//g)
	LIBS           += $$system(pkg-config --libs --static libsigrokdecode \
			  libsigrok | sed s/\n//g)
} else {
	QMAKE_CXXFLAGS += $$system(pkg-config --cflags libsigrokdecode)
	QMAKE_CXXFLAGS += $$system(pkg-config --cflags libsigrok)
	LIBS           += $$system(pkg-config --libs libsigrokdecode)
	LIBS           += $$system(pkg-config --libs libsigrok)
}

# Installation
target.path   = /usr/local/bin
locale.path   = /usr/local/share/sigrok/translations
locale.files  = locale/*.qm
locale.extra  = lrelease sigrok-gui.pro
INSTALLS     += target locale

win32 {
	RC_FILE = sigrok-gui.rc
}

# TODO: This may need fixes.
macx {
	ICON = icons/sigrok-gui.icns
	#FILETYPES.files = ../lib/libsigrok.dylib
	#FILETYPES.path = Contents/Frameworks
	#QMAKE_BUNDLE_DATA += FILETYPES
}
