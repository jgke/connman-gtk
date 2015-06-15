/*
 * ConnMan GTK GUI
 *
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 * Author: Jaakko Hannikainen <jaakko.hannikainen@intel.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "connection_item.h"
#include "style.h"

GtkCssProvider *css_provider;

static GtkWidget *create_connection_item_list(GtkWidget *box) {
	GtkWidget *frame, *list, *inner_box;

	frame = gtk_frame_new(NULL);
	inner_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	list = gtk_list_box_new();
	gtk_widget_set_size_request(list, 200, -1);
	gtk_style_context_add_provider(gtk_widget_get_style_context(inner_box),
			GTK_STYLE_PROVIDER(css_provider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_class(gtk_widget_get_style_context(inner_box),
			"cm-list-box");

	GtkWidget *buttons = gtk_toolbar_new();
	gtk_style_context_add_class(gtk_widget_get_style_context(buttons),
			"inline-toolbar");
	GtkToolItem *add_button = gtk_tool_button_new(NULL, NULL);
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(add_button),
			"list-add-symbolic");
	GtkToolItem *remove_button = gtk_tool_button_new(NULL, NULL);
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(remove_button),
			"list-remove-symbolic");
	gtk_toolbar_insert(GTK_TOOLBAR(buttons), add_button, -1);
	gtk_toolbar_insert(GTK_TOOLBAR(buttons), remove_button, -1);
	gtk_container_add(GTK_CONTAINER(frame), list);
	gtk_container_add_with_properties(GTK_CONTAINER(inner_box), frame,
			"expand", TRUE, "fill", TRUE, NULL);
	gtk_container_add(GTK_CONTAINER(inner_box), buttons);
	gtk_container_add(GTK_CONTAINER(box), inner_box);

	return list;
}

static void activate(GtkApplication *app, gpointer user_data) {
	GtkWidget *window, *box, *list;
	struct connection_item item;

	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), _("Network Settings"));
	gtk_window_set_default_size(GTK_WINDOW(window), 524, 324);

	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
	gtk_widget_set_margin_start(box, 15);
	gtk_widget_set_margin_end(box, 15);
	gtk_widget_set_margin_top(box, 15);
	gtk_widget_set_margin_bottom(box, 15);

	item = create_connection_item();

	list = create_connection_item_list(box);
	gtk_list_box_insert(GTK_LIST_BOX(list), item.list_item->item, 0);
	gtk_container_add(GTK_CONTAINER(box), item.settings->box);
	gtk_container_add(GTK_CONTAINER(window), box);

	gtk_widget_show_all(window);
}

int main(int argc, char *argv[])
{
	GtkApplication *app;
	GError *error = NULL;
	int status;

	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, CONNMAN_GTK_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_path(css_provider,
				CONNMAN_GTK_UIDIR "stylesheet.css", &error);
	if(error != NULL) {
		g_warning("couldn't load stylesheet %s: %s",
				CONNMAN_GTK_UIDIR "stylesheet.css",
				error->message);
		g_error_free(error);
	}

	app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
