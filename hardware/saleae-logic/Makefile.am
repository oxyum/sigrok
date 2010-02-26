AM_CPPFLAGS = -I $(top_srcdir) \
	-I $(top_srcdir)/include \
	-I ../common \
	-I $(top_srcdir)/backend \
	-DFIRMWARE_DIR='"$(FIRMWARE_DIR)"'
LIBOBJS_EXTRA = $(LIBOBJS) \
	-L$(top_srcdir)/hardware/common -lhw-common
	-L$(top_srcdir)/backend -lbackend
hwplugindir = $(HWPLUGIN_DIR)
firmwaredir = $(FIRMWARE_DIR)

hwplugin_LTLIBRARIES = saleae-logic.la
saleae_logic_la_SOURCES = saleae-logic.c
saleae_logic_la_LIBADD = $(LIBOBJS_EXTRA)
saleae_logic_la_LDFLAGS = -module -avoid-version

dist_firmware_DATA = saleae-logic.firmware

