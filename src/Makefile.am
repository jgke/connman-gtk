uidir = $(datadir)/connman-gtk/
ui_DATA = stylesheet.css
bin_PROGRAMS = connman-gtk
connman_gtk_SOURCES = main.c agent.c style.c technology.c service.c settings.c \
		      connection.c \
			connections/bluetooth.c \
			connections/cellular.c \
			connections/ethernet.c \
			connections/p2p.c \
			connections/vpn.c \
			connections/wireless.c
connman_gtk_LDADD = @GTK_LIBS@ @GLIB_LIBS@
connman_gtk_CPPFLAGS = @GTK_CFLAGS@ @GLIB_CFLAGS@ \
		       -DCONNMAN_GTK_LOCALEDIR=\"${CONNMAN_GTK_LOCALEDIR}\" \
		       -DCONNMAN_GTK_UIDIR=\""$(uidir)"\"
