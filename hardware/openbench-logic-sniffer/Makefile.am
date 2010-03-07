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

