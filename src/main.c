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

#include "technology.h"
#include "interfaces.h"
#include "style.h"

GtkCssProvider *css_provider;
GtkWidget *window, *box, *list;
struct technology *item;

static GtkWidget *create_technology_list(GtkWidget *box) {
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

	gtk_container_add(GTK_CONTAINER(frame), list);
	gtk_container_add_with_properties(GTK_CONTAINER(inner_box), frame,
			"expand", TRUE, "fill", TRUE, NULL);
	gtk_container_add(GTK_CONTAINER(box), inner_box);

	return list;
}

static void create_content() {
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
	gtk_widget_set_margin_start(box, 15);
	gtk_widget_set_margin_end(box, 15);
	gtk_widget_set_margin_top(box, 15);
	gtk_widget_set_margin_bottom(box, 15);

	list = create_technology_list(box);
}

static void activate(GtkApplication *app, gpointer user_data) {
	create_content();
	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), _("Network Settings"));
	gtk_window_set_default_size(GTK_WINDOW(window), 524, 324);

	gtk_container_add(GTK_CONTAINER(window), box);

	gtk_widget_show_all(window);
}

void destroy(GtkWidget *window, gpointer user_data) {
	free_technology(item);
}

void add_technology(GVariant *technology) {
	GVariant *path_v;
	gchar *path;
	path_v = g_variant_get_child_value(technology, 0);
	path = g_variant_dup_string(path_v, NULL);
	item = create_technology(path);
	gtk_widget_show(item->list_item->item);
	gtk_container_add(GTK_CONTAINER(list), item->list_item->item);
	gtk_container_add(GTK_CONTAINER(box), item->settings->box);
	gtk_widget_show(item->settings->box);
	g_variant_unref(path_v);
	g_free(path);
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

	g_bus_get(G_BUS_TYPE_SYSTEM, NULL, dbus_connected, NULL);

	app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	g_object_unref(css_provider);

	return status;
}
