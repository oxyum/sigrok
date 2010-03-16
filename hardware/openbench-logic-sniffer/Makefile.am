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

AM_CPPFLAGS = -I $(top_srcdir) \
	-I $(top_srcdir)/include \
	-I ../common \
	-I $(top_srcdir)/backend
LIBOBJS_EXTRA = $(LIBOBJS) \
	-L$(top_srcdir)/hardware/common -lhw-common \
	-L$(top_srcdir)/backend -lbackend
hwplugindir = $(HWPLUGIN_DIR)

hwplugin_LTLIBRARIES = ols.la
ols_la_SOURCES = ols.c
ols_la_LIBADD = $(LIBOBJS_EXTRA)
ols_la_LDFLAGS = -module -avoid-version

