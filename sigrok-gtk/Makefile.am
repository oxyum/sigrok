##
## This file is part of the sigrok project.
##
## Copyright (C) 2010 Bert Vermeulen <bert@biot.com>
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

SUBDIRS = contrib doc

ACLOCAL_AMFLAGS = -I autostuff

bin_PROGRAMS = sigrok-gtk

sigrok_gtk_SOURCES = main.c log.c toolbar.c gtkcellrenderersignal.c \
		sigview.c help.c devselect.c icons.c \
		sigrok-gtk.h gtkcellrenderersignal.h

sigrok_gtk_CPPFLAGS = -DPKG_DATA_DIR=\"$(pkgdatadir)\"

sigrok_gtk_LDADD = $(LIBOBJS)

iconsdir = $(pkgdatadir)/icons

dist_icons_DATA = icons/sigrok_logo.png

EXTRA_DIST = icons/sigrok-logo-notext.ico

if WINDOWS
sigrok_gtk_SOURCES += sigrok-gtk.rc
sigrok_gtk_LDFLAGS = -mwindows
endif

.rc.o:
	$(WINDRES) -o $@ $^

MAINTAINERCLEANFILES = ChangeLog

.PHONY: ChangeLog
ChangeLog:
	git --git-dir $(top_srcdir)/../.git log > ChangeLog || touch ChangeLog

dist-hook: ChangeLog

