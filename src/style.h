#ifndef _CONNMAN_GTK_STYLE_H
#define _CONNMAN_GTK_STYLE_H

#include <gtk/gtk.h>

#define DEFAULT_WIDTH 785
#define DEFAULT_HEIGHT 485
#define LIST_WIDTH 200

#define MARGIN_SMALL 5
#define MARGIN_MEDIUM 10
#define MARGIN_LARGE 15

#define STYLE_ADD_CONTEXT(widget) \
	do { \
		gtk_style_context_add_provider( \
				gtk_widget_get_style_context(widget), \
				GTK_STYLE_PROVIDER(css_provider), \
				GTK_STYLE_PROVIDER_PRIORITY_APPLICATION); \
	} while(0)

#define STYLE_ADD_MARGIN(widget, margin) \
	do { \
		gtk_widget_set_margin_start(widget, margin); \
		gtk_widget_set_margin_end(widget, margin); \
		gtk_widget_set_margin_top(widget, margin); \
		gtk_widget_set_margin_bottom(widget, margin); \
	} while(0)


extern GtkCssProvider *css_provider;

void style_init();

#endif /* _CONNMAN_GTK_STYLE_H */
