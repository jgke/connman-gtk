uidir = $(datadir)/connman-gtk/
ui_DATA = stylesheet.css
bin_PROGRAMS = connman-gtk
connman_gtk_SOURCES = main.c style.c connection.c \
		      technologies/technology.c \
			technologies/bluetooth.c \
			technologies/cellular.c \
			technologies/ethernet.c \
			technologies/p2p.c \
			technologies/vpn.c \
			technologies/wireless.c \
		      services/service.c \
		        services/ethernet.c
connman_gtk_LDADD = @GTK_LIBS@ @GLIB_LIBS@
connman_gtk_CPPFLAGS = @GTK_CFLAGS@ @GLIB_CFLAGS@ \
		       -DCONNMAN_GTK_LOCALEDIR=\"${CONNMAN_GTK_LOCALEDIR}\" \
		       -DCONNMAN_GTK_UIDIR=\""$(uidir)"\"
