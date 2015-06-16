uidir = $(datadir)/connman-gtk/
ui_DATA = stylesheet.css
bin_PROGRAMS = connman-gtk
connman_gtk_SOURCES = main.c technologies/technology.c technologies/ethernet.c
connman_gtk_LDADD = @DEPS_LIBS@
connman_gtk_CPPFLAGS = @DEPS_CFLAGS@ \
		       -DCONNMAN_GTK_LOCALEDIR=\"${CONNMAN_GTK_LOCALEDIR}\" \
		       -DCONNMAN_GTK_UIDIR=\""$(uidir)"\"
