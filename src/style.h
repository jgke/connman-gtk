#ifndef _CONNMAN_GTK_STYLE_H
#define _CONNMAN_GTK_STYLE_H

#include <gtk/gtk.h>

#define STYLE_ADD_CONTEXT(widget) \
	do { \
		gtk_style_context_add_provider( \
				gtk_widget_get_style_context(widget), \
				GTK_STYLE_PROVIDER(css_provider), \
				GTK_STYLE_PROVIDER_PRIORITY_APPLICATION); \
	} while(0)


extern GtkCssProvider *css_provider;

void style_init();

#endif /* _CONNMAN_GTK_STYLE_H */
