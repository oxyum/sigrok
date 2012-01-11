##
## This file is part of the sigrok project.
##
## Copyright (C) 2012 Uwe Hermann <uwe@hermann-uwe.de>
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

ACLOCAL_AMFLAGS = -I autostuff

AM_CPPFLAGS = -I$(top_srcdir)

# SUBDIRS = 

lib_LTLIBRARIES = libsigrokfile.la

libsigrokfile_la_SOURCES = \
	main.c \
	log.c

libsigrokfile_la_LIBADD = \
	$(LIBOBJS)

libsigrokfile_la_LDFLAGS = $(LIBSIGROKFILE_LT_LDFLAGS)

include_HEADERS = sigrokfile.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libsigrokfile.pc

EXTRA_DIST = Doxyfile

