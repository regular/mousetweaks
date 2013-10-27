AC_INIT([mousetweaks],
        [3.10.0],
        [https://bugzilla.gnome.org/enter_bug.cgi?product=mousetweaks],
        [mousetweaks],
        [https://live.gnome.org/Mousetweaks/Home])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_PREFIX_DEFAULT([/usr])

AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])

AC_PROG_CC
AC_PROG_INSTALL
AC_HEADER_STDC
AC_C_CONST

GNOME_COMPILE_WARNINGS([maximum])

IT_PROG_INTLTOOL([0.41.0])

AC_SUBST([GETTEXT_PACKAGE], [mousetweaks])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], "$GETTEXT_PACKAGE", [Gettext Package])

AC_PATH_PROG([GLIB_COMPILE_RESOURCES], [glib-compile-resources])

GLIB_GSETTINGS

GIO_REQUIRED=2.38
GTK_REQUIRED=3.10
GDS_REQUIRED=0.1

PKG_CHECK_MODULES(DAEMON,
    glib-2.0
    gio-2.0 >= $GIO_REQUIRED
    gtk+-3.0 >= $GTK_REQUIRED
    gsettings-desktop-schemas >= $GDS_REQUIRED
    atspi-2)

PKG_CHECK_MODULES(WINDOW, gtk+-3.0 >= $GTK_REQUIRED)

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    data/Makefile
    po/Makefile.in
])

AC_OUTPUT
