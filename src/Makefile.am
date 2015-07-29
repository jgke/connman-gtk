uidir = $(datadir)/connman-gtk/
ui_DATA = stylesheet.css
bin_PROGRAMS = connman-gtk
connman_gtk_SOURCES = main.c agent.c style.c technology.c service.c util.c \
		      settings.c settings_content.c \
		      connection.c vpn.c wireless.c
connman_gtk_LDADD = @GTK_LIBS@ @GLIB_LIBS@
connman_gtk_CPPFLAGS = @GTK_CFLAGS@ @GLIB_CFLAGS@ \
		       -DCONNMAN_GTK_LOCALEDIR=\"${CONNMAN_GTK_LOCALEDIR}\" \
		       -DCONNMAN_GTK_UIDIR=\""$(uidir)"\"

EXTRA_DIST =  agent.h interfaces.h main.h technology.h style.h service.h \
	      util.h settings.h settings_content.h \
	      connection.h vpn.h wireless.h \
	      stylesheet.css
