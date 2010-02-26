AM_CPPFLAGS = -I $(top_srcdir)/include \
	-I $(top_srcdir)/backend

#	-I /home/bert/src/libusb-1.0.0/libusb


bin_PROGRAMS = sigrok-cli

sigrok_cli_SOURCES = sigrok-cli.c
sigrok_cli_LDADD = $(LIBOBJS) -L$(top_srcdir)/backend -lbackend

