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

#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "technologies/technology.h"
#include "interfaces.h"
#include "style.h"

GtkWidget *list, *notebook;
struct technology *technologies[TECHNOLOGY_TYPE_COUNT];

void technology_selected(GtkListBox *box, GtkListBoxRow *row, gpointer data) {
	if(!G_IS_OBJECT(row))
		return;
	gint *id = g_object_get_data(G_OBJECT(row), "technology-id");
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), *id);
}

static void create_content(GtkWidget *window) {
	GtkWidget *frame, *box;

	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
	STYLE_ADD_MARGIN(box, 15);

	frame = gtk_frame_new(NULL);
	list = gtk_list_box_new();
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(list),
			GTK_SELECTION_BROWSE);
	g_signal_connect(list, "row-selected", G_CALLBACK(technology_selected),
			NULL);
	gtk_widget_set_size_request(list, 200, -1);

	notebook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);

	gtk_container_add(GTK_CONTAINER(frame), list);
	gtk_container_add(GTK_CONTAINER(box), frame);
	gtk_container_add_with_properties(GTK_CONTAINER(box), notebook,
			"expand", TRUE, "fill", TRUE, NULL);
	gtk_container_add(GTK_CONTAINER(window), box);
}

void destroy(GtkWidget *window, gpointer user_data) {
	int i;
	notebook = NULL;
	for(i = 0; i < TECHNOLOGY_TYPE_COUNT; i++)
		if(technologies[i])
			free_technology(technologies[i]);
}

void add_technology(GVariant *technology) {
	GVariant *path;
	GVariant *properties;
	struct technology *item;
	int pos;

	path = g_variant_get_child_value(technology, 0);
	properties = g_variant_get_child_value(technology, 1);
	item = create_technology(path, properties);
	technologies[item->type] = item;
	gtk_container_add(GTK_CONTAINER(list), item->list_item->item);
	pos = gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
			item->settings->box, NULL);
	technology_set_id(item, pos);
	g_variant_unref(path);
	g_variant_unref(properties);
}

void add_all_technologies(GVariant *technologies) {
	int i;
	int size = g_variant_n_children(technologies);
	for(i = 0; i < size; i++) {
		GVariant *child = g_variant_get_child_value(technologies, i);
		add_technology(child);
		g_variant_unref(child);
	}
}

void manager_connected(GObject *source, GAsyncResult *res, gpointer user_data) {
	(void)source;
	(void)res;
	(void)user_data;
	GError *error = NULL;
	GVariant *data, *child;
	GDBusProxy *proxy;
	proxy = g_dbus_proxy_new_finish(res, &error);
	if(error) {
		g_error("failed to connect to ConnMan: %s", error->message);
		g_error_free(error);
		return;
	}
	data = g_dbus_proxy_call_sync(proxy, "GetTechnologies", NULL,
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if(error) {
		g_error("failed to get technologies: %s", error->message);
		g_error_free(error);
		g_object_unref(proxy);
		return;
	}
	child = g_variant_get_child_value(data, 0);
	add_all_technologies(child);
	g_variant_unref(data);
	g_variant_unref(child);
}

void dbus_connected(GObject *source, GAsyncResult *res, gpointer user_data) {
	(void)source;
	(void)user_data;
	GDBusConnection *connection;
	GDBusNodeInfo *info;
	GError *error = NULL;
	connection = g_bus_get_finish(res, &error);
	if(error) {
		g_error("failed to connect to system dbus: %s",
				error->message);
		g_error_free(error);
		return;
	}
	info = g_dbus_node_info_new_for_xml(
			manager_interface, &error);
	if(error) {
		g_error("Failed to load manager interface: %s",
				error->message);
		g_error_free(error);
		return;
	}

	g_dbus_proxy_new(connection, G_DBUS_PROXY_FLAGS_NONE,
			g_dbus_node_info_lookup_interface(info,
				"net.connman.Manager"),
			"net.connman", "/", "net.connman.Manager", NULL,
			manager_connected, NULL);
	g_dbus_node_info_unref(info);
}

static void activate(GtkApplication *app, gpointer user_data) {
	GtkWidget *window;

	g_bus_get(G_BUS_TYPE_SYSTEM, NULL, dbus_connected, NULL);

	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), _("Network Settings"));
	gtk_window_set_default_size(GTK_WINDOW(window), 524, 324);

	create_content(window);

	gtk_widget_show_all(window);
}

int main(int argc, char *argv[]) {
	GtkApplication *app;
	int status;

	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, CONNMAN_GTK_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	style_init();

	app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	g_object_unref(css_provider);

	return status;
}
