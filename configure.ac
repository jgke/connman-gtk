AC_INIT([connman-gtk],[1.0],[https://github.com/jgke/connman-gtk])

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
PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.10], [], [AC_MSG_ERROR(gtk+ >= 3.10 is required)])
PKG_CHECK_MODULES([GLib], [glib-2.0 >= 2.0], [], [AC_MSG_ERROR(glib >= 2.0 is required)])

AC_ARG_WITH([openconnect],
            [AS_HELP_STRING([--with-openconnect],
                            [support easier authentication to AnyConnect VPNs @<:@default=check@:>@])],
            [],
            [with_openconnect=check])
AC_ARG_ENABLE([status-icon],
              [AS_HELP_STRING([--disable-status-icon],
                              [Disable the status icon @<:@default=false@:>@])])

AS_CASE(["$with_openconnect"],
        [yes], [PKG_CHECK_MODULES([openconnect], [openconnect >= 5.99], [HAVE_OPENCONNECT=1])],
        [no], [HAVE_OPENCONNECT=0],
        [dynamic], [PKG_CHECK_MODULES([openconnect], [openconnect >= 5.99], [HAVE_OPENCONNECT=2])],
        [PKG_CHECK_MODULES([openconnect], [openconnect >= 5.99], [HAVE_OPENCONNECT=1], [HAVE_OPENCONNECT=0])])
AM_CONDITIONAL([USE_OPENCONNECT], [test "$with_openconnect" != no -a "$HAVE_OPENCONNECT" -ne 0])
AM_CONDITIONAL([USE_OPENCONNECT_DYNAMIC], [test "$HAVE_OPENCONNECT" -eq 2])

AS_IF([test "x$enable_status_icon" != "xno"],
        [STATUS_ICON=1],
        [STATUS_ICON=0])
AM_CONDITIONAL([USE_STATUS_ICON], [test "$STATUS_ICON" -eq 1])

AC_CONFIG_FILES([
  Makefile
  po/Makefile.in
  connman-gtk.desktop
])
AC_OUTPUT
