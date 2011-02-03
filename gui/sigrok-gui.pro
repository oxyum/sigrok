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

# The sigrok-gui version number. Define APP_VERSION macro for use in the code.
VERSION       = 0.1
DEFINES      += APP_VERSION=\\\"$$VERSION\\\"

SOURCES      += main.cpp \
	        mainwindow.cpp \
	        configform.cpp \
	        sampleiodevice.cpp \
	        channelform.cpp

HEADERS      += mainwindow.h \
	        configform.h \
	        sampleiodevice.h \
	        channelform.h

FORMS        += mainwindow.ui \
	        configform.ui \
	        channelform.ui

TRANSLATIONS  = locale/sigrok-gui_de_DE.ts \
                locale/sigrok-gui_nl_NL.ts \
                locale/sigrok-gui_fr_FR.ts

# CONFIG       += link_pkgconfig debug_and_release build_all
CONFIG       += link_pkgconfig release

# One entry per line to avoid issues on some OSes with some qmake versions.
PKGCONFIG    += glib-2.0
PKGCONFIG    += libusb-1.0
PKGCONFIG    += libzip
PKGCONFIG    += libsigrok
PKGCONFIG    += libsigrokdecode

RESOURCES    += sigrok-gui.qrc

# Installation
target.path   = /usr/local/bin
locale.path   = /usr/local/share/sigrok/translations
locale.files  = locale/*.qm
locale.extra  = lrelease sigrok-gui.pro
INSTALLS     += target locale

# Python
win32 {
	# We currently hardcode the paths to the Python 2.6 default install
	# location as there's no 'python-config' script on Windows, it seems.
	LIBS        += -Lc:/Python26/libs -lpython26
	INCLUDEPATH += c:/Python26/include
} else {
	# Linux and Mac OS X have 'python-config', let's hope the rest too...
	LIBS        += $$system(python-config --ldflags)
	# Yuck. We have to use 'sed' magic here as 'python-config' returns
	# the include paths with prefixed '-I' but the qmake INCLUDEPATH
	# variable expects them _without_ the prefixed '-I' (and adds
	# those itself).
	INCLUDEPATH += $$system(python-config --includes | sed s/-I//g)
}

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
