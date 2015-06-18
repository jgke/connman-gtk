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
GHashTable *technology_types;
GHashTable *services;
struct technology *technologies[TECHNOLOGY_TYPE_COUNT];

/* sort smallest enum value first */
gint technology_list_sort_cb(GtkListBoxRow *row1, GtkListBoxRow *row2,
		gpointer user_data) {
	enum technology_type type1 = *(enum technology_type *)g_object_get_data(
			G_OBJECT(row1), "technology-type");
	enum technology_type type2 = *(enum technology_type *)g_object_get_data(
			G_OBJECT(row2), "technology-type");
	return type1 - type2;
}

void technology_selected(GtkListBox *box, GtkListBoxRow *row, gpointer data) {
	if(!G_IS_OBJECT(row))
		return;
	struct technology *tech = g_object_get_data(G_OBJECT(row), "technology");
	gint num = gtk_notebook_page_num(GTK_NOTEBOOK(notebook),
			tech->settings->grid);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), num);
}

static void create_content(GtkWidget *window) {
	GtkWidget *frame, *grid;

	grid = gtk_grid_new();
	STYLE_ADD_MARGIN(grid, MARGIN_LARGE);
	gtk_widget_set_hexpand(grid, TRUE);
	gtk_widget_set_vexpand(grid, TRUE);

	frame = gtk_frame_new(NULL);
	list = gtk_list_box_new();
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(list),
			GTK_SELECTION_BROWSE);
	gtk_list_box_set_sort_func(GTK_LIST_BOX(list), technology_list_sort_cb,
			NULL, NULL);
	g_signal_connect(list, "row-selected", G_CALLBACK(technology_selected),
			NULL);
	gtk_widget_set_size_request(list, LIST_WIDTH, -1);

	notebook = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
	gtk_widget_set_hexpand(notebook, TRUE);
	gtk_widget_set_vexpand(notebook, TRUE);

	gtk_container_add(GTK_CONTAINER(frame), list);
	gtk_grid_attach(GTK_GRID(grid), frame, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), notebook, 1, 0, 1, 1);
	gtk_container_add(GTK_CONTAINER(window), grid);
}

void destroy(GtkWidget *window, gpointer user_data) {
	int i;
	notebook = NULL;
	for(i = 0; i < TECHNOLOGY_TYPE_COUNT; i++)
		if(technologies[i])
			technology_free(technologies[i]);
	g_hash_table_remove_all(services);
}

void add_technology(GDBusConnection *connection, GVariant *technology) {
	GVariant *path;
	const gchar *object_path;
	GVariant *properties;
	GDBusProxy *proxy;
	GDBusNodeInfo *info;
	GError *error = NULL;
	struct technology *item;
	info = g_dbus_node_info_new_for_xml(technology_interface, &error);
	if(error) {
		g_warning("Failed to load technology interface: %s",
				error->message);
		g_error_free(error);
		return;
	}

	path = g_variant_get_child_value(technology, 0);
	properties = g_variant_get_child_value(technology, 1);

	object_path = g_variant_get_string(path, NULL);

	proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE,
			g_dbus_node_info_lookup_interface(info,
				"net.connman.Technology"),
			"net.connman", object_path, "net.connman.Technology",
			NULL, &error);
	if(error) {
		g_warning("failed to connect ConnMan technology proxy: %s",
				error->message);
		g_error_free(error);
		goto out;
	}

	item = create_technology(proxy, path, properties);

	g_hash_table_insert(technology_types, g_strdup(object_path), &item->type);
	technologies[item->type] = item;

	gtk_container_add(GTK_CONTAINER(list), item->list_item->item);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
			item->settings->grid, NULL);

out:
	g_dbus_node_info_unref(info);
	g_variant_unref(path);
	g_variant_unref(properties);
}

void remove_technology(GVariant *parameters) {
	GVariant *path_v;
	const gchar *path;
	enum technology_type *type_p;
	enum technology_type type;

	path_v = g_variant_get_child_value(parameters, 0);
	path = g_variant_get_string(path_v, NULL);
	type_p = g_hash_table_lookup(technology_types, path);
	if(!type_p) {
		g_warning("Tried to remove unknown technology");
		goto out;
	}
	type = *type_p;
	g_hash_table_remove(technology_types, path);
	technology_free(technologies[type]);
	technologies[type] = NULL;
out:
	g_variant_unref(path_v);
}

void add_service(GDBusConnection *connection, const gchar *path,
		GVariant *properties) {
	struct service *serv;
	GDBusProxy *proxy;
	GDBusNodeInfo *info;
	GError *error = NULL;
	info = g_dbus_node_info_new_for_xml(service_interface, &error);
	if(error) {
		g_warning("Failed to load service interface: %s",
				error->message);
		g_error_free(error);
		return;
	}

	proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE,
			g_dbus_node_info_lookup_interface(info,
				"net.connman.Service"),
			"net.connman", path, "net.connman.Service",
			NULL, &error);
	if(error) {
		g_warning("failed to connect ConnMan service proxy: %s",
				error->message);
		g_error_free(error);
		goto out;
	}

	serv = service_create(proxy, path, properties);
	g_hash_table_insert(services, g_strdup(path), serv);
out:
	g_dbus_node_info_unref(info);
}

void services_changed(GDBusConnection *connection, GVariant *parameters) {
	GVariant *modified, *deleted;
	GVariantIter *iter;
	gchar *path;
	GVariant *value;

	modified = g_variant_get_child_value(parameters, 0);
	deleted = g_variant_get_child_value(parameters, 1);

	iter = g_variant_iter_new(modified);
	while(g_variant_iter_loop(iter, "(o@*)", &path, &value)) {
		enum technology_type type = technology_type_from_path(path);
		if(type != TECHNOLOGY_TYPE_UNKNOWN) {
			if(!g_hash_table_contains(services, path)) {
				add_service(connection, path, value);
				continue;
			}
			struct service *serv = g_hash_table_lookup(services, path);
			technologies[type]->update_service(serv, value);
		}
	}

	iter = g_variant_iter_new(deleted);
	while(g_variant_iter_loop(iter, "o", &path)) {
		enum technology_type type = technology_type_from_path(path);
		if(type != TECHNOLOGY_TYPE_UNKNOWN)
			if(g_hash_table_contains(services, path))
				technologies[type]->remove_service(path);
	}

	g_variant_unref(modified);
	g_variant_unref(deleted);
}

void manager_signal(GDBusProxy *proxy, gchar *sender, gchar *signal,
		GVariant *parameters, gpointer user_data) {
	GDBusConnection *connection = g_dbus_proxy_get_connection(proxy);
	if(!strcmp(signal, "TechnologyAdded")) {
		add_technology(connection, parameters);
	} else if(!strcmp(signal, "TechnologyRemoved")) {
		remove_technology(parameters);
	} else if(!strcmp(signal, "ServicesChanged")) {
		services_changed(connection, parameters);
	}
}

void add_all_technologies(GDBusConnection *connection, GVariant *technologies_v) {
	int i;
	int size = g_variant_n_children(technologies_v);
	for(i = 0; i < size; i++) {
		GVariant *child = g_variant_get_child_value(technologies_v, i);
		add_technology(connection, child);
		g_variant_unref(child);
	}
	for(i = TECHNOLOGY_TYPE_ETHERNET; i < TECHNOLOGY_TYPE_COUNT; i++) {
		if(technologies[i]) {
			GtkWidget *row = technologies[i]->list_item->item;
			gtk_list_box_select_row(GTK_LIST_BOX(list),
					GTK_LIST_BOX_ROW(row));
			break;
		}
	}
}

void dbus_connected(GObject *source, GAsyncResult *res, gpointer user_data) {
	(void)source;
	(void)user_data;
	GDBusConnection *connection;
	GDBusNodeInfo *info;
	GError *error = NULL;
	GVariant *data, *child;
	GDBusProxy *proxy;

	connection = g_bus_get_finish(res, &error);
	if(error) {
		g_error("failed to connect to system dbus: %s",
				error->message);
		g_error_free(error);
		return;
	}

	info = g_dbus_node_info_new_for_xml(manager_interface, &error);
	if(error) {
		/* TODO: show user error message */
		g_critical("Failed to load manager interface: %s",
				error->message);
		g_error_free(error);
		return;
	}

	proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE,
			g_dbus_node_info_lookup_interface(info,
				"net.connman.Manager"),
			"net.connman", "/", "net.connman.Manager", NULL,
			&error);
	g_dbus_node_info_unref(info);
	if(error) {
		g_warning("failed to connect to ConnMan: %s", error->message);
		g_error_free(error);
		return;
	}

	data = g_dbus_proxy_call_sync(proxy, "GetTechnologies", NULL,
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if(error) {
		g_warning("failed to get technologies: %s", error->message);
		g_error_free(error);
		g_object_unref(proxy);
		return;
	}

	g_signal_connect(proxy, "g-signal", G_CALLBACK(manager_signal), NULL);

	child = g_variant_get_child_value(data, 0);
	add_all_technologies(connection, child);
	g_variant_unref(data);
	g_variant_unref(child);
}

static void activate(GtkApplication *app, gpointer user_data) {
	GtkWidget *window;

	g_bus_get(G_BUS_TYPE_SYSTEM, NULL, dbus_connected, NULL);

	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), _("Network Settings"));
	gtk_window_set_default_size(GTK_WINDOW(window), DEFAULT_WIDTH,
			DEFAULT_HEIGHT);

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

	technology_types = g_hash_table_new_full(g_str_hash, g_str_equal,
			g_free, NULL);
	services = g_hash_table_new_full(g_str_hash, g_str_equal,
			g_free, (GDestroyNotify)service_free);

	app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	g_object_unref(css_provider);

	return status;
}
