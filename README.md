connman-gtk
===========

GTK GUI for ConnMan.

![screenshot-1](https://raw.githubusercontent.com/jgke/jgke.github.io/connman-gtk/connman-gtk-1.png)
![screenshot-2](https://raw.githubusercontent.com/jgke/jgke.github.io/connman-gtk/connman-gtk-2.png)

Dependencies
------------

 * intltool
 * GLib
 * GTK >= 3.14

Installation
------------

	./autogen.sh
	./configure
	make
	make install

configure options
-----------------

	--enable-status-icon=[yes,no]
	--disable-status-icon

Enable or disable the status icon.

	--with-openconnect=[yes,no,check,dynamic]

Enables use of [openconnect](http://infradead.org/openconnect/) for easier
authentication for AnyConnect VPNs. Using 'dynamic' will load the library at
runtime, if present. Default argument is 'check' which checks for the library
at configure time.

License
-------

GPLv2, see COPYING
