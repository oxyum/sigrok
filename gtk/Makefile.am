AM_CFLAGS = $(CPPFLAGS_PYTHON)
LDADD = $(LDFLAGS_PYTHON)

if WINDOWS
sigrok_gtk_LDFLAGS = -mwindows
endif

bin_PROGRAMS = sigrok-gtk

sigrok_gtk_SOURCES = main.c log.c toolbar.c gtkcellrenderersignal.c \
		sigview.c help.c devselect.c sigrok-gtk.h gtkcellrenderersignal.h

