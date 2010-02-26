AC_PREREQ(2.61)
AC_INIT(sigrok, 0.1)
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
CFLAGS="-g -Wall"
AC_PROG_CC
AM_PROG_CC_C_O
LT_INIT

# Checks for libraries.
AM_PATH_GLIB_2_0(2.0.0,
	CFLAGS="$CFLAGS $GLIB_CFLAGS"; LIBS="$LIBS $GLIB_LIBS")

PKG_CHECK_MODULES([GMODULE], [gmodule-2.0 >= 2.0],
	[CFLAGS="$CFLAGS $GMODULE_CFLAGS";
	LIBS="$LIBS $GMODULE_LIBS"])

PKG_CHECK_MODULES([LIBUSB], [libusb-1.0 >= 1.0],
	[CFLAGS="$CFLAGS $LIBUSB_CFLAGS";
	LIBS="$LIBS $LIBUSB_LIBS"])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
				 backend/Makefile
				 hardware/Makefile
				 hardware/common/Makefile
				 hardware/saleae-logic/Makefile
				 frontend/Makefile
				 frontend/cli/Makefile
                 ])

AC_SUBST(HWPLUGIN_DIR,'$(libdir)/sigrok')
AC_SUBST(FIRMWARE_DIR,'$(datadir)/sigrok')

AC_OUTPUT

