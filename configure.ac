AC_INIT([connman-gtk],[0.1],[https://github.com/jgke/connman-gtk])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])

AM_INIT_AUTOMAKE([1.10 dist-bzip2 no-dist-gzip foreign tar-ustar subdir-objects])
AM_SILENT_RULES([yes])

IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=connman-gtk
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"],
		   [The domain to use with gettext])
AM_GLIB_GNU_GETTEXT

CONNMAN_GTK_LOCALEDIR=[${datadir}/locale]
AC_SUBST(CONNMAN_GTK_LOCALEDIR)

AC_PROG_CC

AX_CFLAGS_WARN_ALL

PKG_PROG_PKG_CONFIG([0.22])
PKG_CHECK_MODULES([DEPS], [gtk+-3.0 >= 3.0], [], [AC_MSG_ERROR(gtk+ >= 3.0 is required)])
PKG_CHECK_MODULES([DEPS], [glib-2.0 >= 2.0], [], [AC_MSG_ERROR(glib >= 2.0 is required)])

AC_CONFIG_FILES([
  Makefile
  po/Makefile.in
  src/Makefile
])
AC_OUTPUT
