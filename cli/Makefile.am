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

bin_PROGRAMS = sigrok-cli

sigrok_cli_SOURCES = sigrok-cli.c parsers.c anykey.c

sigrok_cli_CPPFLAGS = -I$(top_srcdir)/libsigrok \
		      -I$(top_srcdir)/libsigrokdecode \
		      $(CPPFLAGS_PYTHON)

sigrok_cli_LDADD = $(LIBOBJS) \
		   -L$(top_builddir)/libsigrok -lsigrok \
		   -L$(top_builddir)/libsigrokdecode -lsigrokdecode \
		   $(LDFLAGS_PYTHON)

