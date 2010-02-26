AM_CPPFLAGS = -I $(top_srcdir)/include \
	-DHWPLUGIN_DIR='"$(HWPLUGIN_DIR)"'

noinst_LIBRARIES = libbackend.a

libbackend_a_SOURCES = \
	backend.c backend.h \
	datastore.c datastore.h \
	device.c device.h \
	session.c session.h \
	hwplugin.c hwplugin.h
libbackend_a_LIBADD = $(LIBOBJS)
