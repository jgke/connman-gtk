uidir = $(datadir)/connman-gtk/
ui_DATA = stylesheet.css
bin_PROGRAMS = connman-gtk
connman_gtk_SOURCES = main.c agent.c style.c technology.c service.c util.c \
		      settings.c settings_content.c settings_content_callback.c \
		      vpn.c \
		      connection.c \
			connections/bluetooth.c \
			connections/cellular.c \
			connections/ethernet.c \
			connections/p2p.c \
			connections/wireless.c
connman_gtk_LDADD = @GTK_LIBS@ @GLIB_LIBS@
connman_gtk_CPPFLAGS = @GTK_CFLAGS@ @GLIB_CFLAGS@ \
		       -DCONNMAN_GTK_LOCALEDIR=\"${CONNMAN_GTK_LOCALEDIR}\" \
		       -DCONNMAN_GTK_UIDIR=\""$(uidir)"\"

EXTRA_DIST =  agent.h interfaces.h main.h technology.h style.h service.h \
	      util.h vpn.h \
	      settings.h settings_content.h settings_content_callback.h \
	      connection.h \
		connections/bluetooth.h \
		connections/cellular.h \
		connections/ethernet.h \
		connections/p2p.h \
		connections/wireless.h \
	      stylesheet.css
