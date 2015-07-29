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

#include "agent.h"
#include "connection.h"
#include "technology.h"
#include "interfaces.h"
#include "style.h"
#include "util.h"
#include "vpn.h"

GtkWidget *list, *notebook, *main_window;
GHashTable *technology_types, *services;
GDBusProxy *manager_proxy, *vpn_manager_proxy;
struct technology *technologies[CONNECTION_TYPE_COUNT];
gboolean shutting_down = FALSE;
const gchar *default_page;

/* sort smallest enum value first */
gint technology_list_sort_cb(GtkListBoxRow *row1, GtkListBoxRow *row2,
                             gpointer user_data)
{
	enum connection_type type1 = *(enum connection_type *)g_object_get_data(
	                                     G_OBJECT(row1), "technology-type");
	enum connection_type type2 = *(enum connection_type *)g_object_get_data(
	                                     G_OBJECT(row2), "technology-type");
	return type1 - type2;
}

static void create_content(void)
{
	GtkWidget *frame, *grid;

	grid = gtk_grid_new();
	style_add_margin(grid, MARGIN_LARGE);
	gtk_widget_set_hexpand(grid, TRUE);
	gtk_widget_set_vexpand(grid, TRUE);

	frame = gtk_frame_new(NULL);
	list = gtk_list_box_new();
	notebook = gtk_notebook_new();
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(list),
	                                GTK_SELECTION_BROWSE);
	gtk_list_box_set_sort_func(GTK_LIST_BOX(list), technology_list_sort_cb,
	                           NULL, NULL);
	g_signal_connect(list, "row-selected", G_CALLBACK(list_item_selected),
	                 notebook);
	gtk_widget_set_size_request(list, LIST_WIDTH, -1);

	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
	gtk_widget_set_hexpand(notebook, TRUE);
	gtk_widget_set_vexpand(notebook, TRUE);

	gtk_container_add(GTK_CONTAINER(frame), list);
	gtk_grid_attach(GTK_GRID(grid), frame, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), notebook, 1, 0, 1, 1);
	gtk_container_add(GTK_CONTAINER(main_window), grid);
}

static void add_technology(GDBusConnection *connection, GVariant *technology)
{
	GVariant *path;
	const gchar *object_path;
	GVariant *properties;
	GDBusProxy *proxy;
	GDBusNodeInfo *info;
	GError *error = NULL;
	struct technology *item;
	info = g_dbus_node_info_new_for_xml(TECHNOLOGY_INTERFACE, &error);
	if(error) {
		g_warning("Failed to load technology interface: %s",
		          error->message);
		g_error_free(error);
		return;
	}

	path = g_variant_get_child_value(technology, 0);
	properties = g_variant_get_child_value(technology, 1);

	object_path = g_variant_get_string(path, NULL);

	if(g_hash_table_contains(technology_types, object_path))
		goto out;

	proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE,
	                              g_dbus_node_info_lookup_interface(info,
	                                              "net.connman.Technology"),
	                              "net.connman", object_path,
	                              "net.connman.Technology", NULL, &error);
	if(error) {
		g_warning("failed to connect ConnMan technology proxy: %s",
		          error->message);
		g_error_free(error);
		goto out;
	}

	item = technology_create(proxy, object_path, properties);

	g_hash_table_insert(technology_types, g_strdup(object_path),
	                    &item->type);
	technologies[item->type] = item;

	gtk_container_add(GTK_CONTAINER(list), item->list_item->item);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
	                         item->settings->grid, NULL);

out:
	g_dbus_node_info_unref(info);
	g_variant_unref(path);
	g_variant_unref(properties);
}

static void remove_technology_by_path(const gchar *path)
{
	enum connection_type *type_p;
	enum connection_type type;

	type_p = g_hash_table_lookup(technology_types, path);

	if(!type_p)
		return;
	type = *type_p;
	g_hash_table_remove(technology_types, path);
	technology_free(technologies[type]);
	technologies[type] = NULL;
}

static void remove_technology(GVariant *parameters)
{
	GVariant *path_v;
	const gchar *path;

	path_v = g_variant_get_child_value(parameters, 0);
	path = g_variant_get_string(path_v, NULL);
	remove_technology_by_path(path);
	g_variant_unref(path_v);
}

static void add_service(GDBusConnection *connection, const gchar *path,
                        GVariant *properties)
{
	struct service *serv;
	GDBusProxy *proxy;
	GDBusNodeInfo *info;
	GDBusInterfaceInfo *interface;
	GError *error = NULL;
	enum connection_type type;
	const char *interface_xml;
	const char *connman_name;
	const char *interface_name;

	type = connection_type_from_properties(properties);
	if(type == CONNECTION_TYPE_VPN) {
		interface_xml = VPN_CONNECTION_INTERFACE;
		connman_name = CONNMAN_VPN_PATH;
		interface_name = VPN_CONNECTION_NAME;
	}
	else {
		interface_xml = SERVICE_INTERFACE;
		connman_name = CONNMAN_PATH;
		interface_name = SERVICE_NAME;
	}
	info = g_dbus_node_info_new_for_xml(interface_xml, &error);
	if(error) {
		g_warning("Failed to load service interface: %s",
			  error->message);
		g_error_free(error);
		return;
	}

	interface = g_dbus_node_info_lookup_interface(info, interface_name);
	proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE,
				      interface, connman_name, path,
	                              interface_name, NULL, &error);
	if(error) {
		g_warning("failed to connect ConnMan service proxy: %s",
		          error->message);
		g_error_free(error);
		goto out;
	}

	serv = service_create(technologies[type], proxy, path, properties);
	g_hash_table_insert(services, g_strdup(path), serv);
	if(technologies[type])
		technology_add_service(technologies[type], serv);

out:
	g_dbus_node_info_unref(info);
}

void modify_service(GDBusConnection *connection, const gchar *path,
                           GVariant *properties)
{
	enum connection_type type;
	struct service *serv;

	serv = g_hash_table_lookup(services, path);

	if(serv)
		type = serv->type;
	else
		type = connection_type_from_properties(properties);

	if(type != CONNECTION_TYPE_UNKNOWN) {
		if(!serv) {
			add_service(connection, path, properties);
			return;
		}
		service_update(serv, properties);
	}
}

void remove_service(const gchar *path)
{
	enum connection_type type;
	struct service *serv;
	void *spath;

	serv = g_hash_table_lookup(services, path);
	if(!serv)
		return;

	type = serv->type;
	if(type != CONNECTION_TYPE_UNKNOWN && technologies[type])
		technology_remove_service(technologies[type], path);

	service_free(g_hash_table_lookup(services, path));

	if(!g_hash_table_lookup_extended(services, path, &spath, NULL))
		return;

	g_hash_table_steal(services, path);
	g_free(spath);
}

static void remove_service_struct(void *serv)
{
	remove_service(((struct service *)serv)->path);
}

static void services_changed(GDBusConnection *connection, GVariant *parameters)
{
	GVariant *modified, *deleted;
	GVariantIter *iter;
	gchar *path;
	GVariant *value;

	modified = g_variant_get_child_value(parameters, 0);
	deleted = g_variant_get_child_value(parameters, 1);

	iter = g_variant_iter_new(modified);
	while(g_variant_iter_loop(iter, "(o@*)", &path, &value))
		if(!strstr(path, "service/vpn"))
			modify_service(connection, path, value);
	g_variant_iter_free(iter);

	iter = g_variant_iter_new(deleted);
	while(g_variant_iter_loop(iter, "o", &path))
		if(!strstr(path, "service/vpn"))
			remove_service(path);
	g_variant_iter_free(iter);

	g_variant_unref(modified);
	g_variant_unref(deleted);
}

static void manager_signal(GDBusProxy *proxy, gchar *sender, gchar *signal,
                           GVariant *parameters, gpointer user_data)
{
	GDBusConnection *connection = g_dbus_proxy_get_connection(proxy);
	if(!strcmp(signal, "TechnologyAdded")) {
		add_technology(connection, parameters);
	} else if(!strcmp(signal, "TechnologyRemoved")) {
		remove_technology(parameters);
	} else if(!strcmp(signal, "ServicesChanged")) {
		services_changed(connection, parameters);
	}
}

static void add_all_technologies(GDBusConnection *connection,
                                 GVariant *technologies_v)
{
	int i;
	int size = g_variant_n_children(technologies_v);
	enum connection_type default_type = CONNECTION_TYPE_UNKNOWN;
	for(i = 0; i < size; i++) {
		GVariant *child = g_variant_get_child_value(technologies_v, i);
		add_technology(connection, child);
		g_variant_unref(child);
	}
	if(default_page)
		default_type = connection_type_from_string(default_page);
	for(i = CONNECTION_TYPE_ETHERNET; i < CONNECTION_TYPE_COUNT; i++) {
		if(default_type && technologies[i]) {
			if(default_type != technologies[i]->type)
				continue;
			GtkWidget *row = technologies[i]->list_item->item;
			gtk_list_box_select_row(GTK_LIST_BOX(list),
			                        GTK_LIST_BOX_ROW(row));
			return;
		}
	}
	for(i = CONNECTION_TYPE_ETHERNET; i < CONNECTION_TYPE_COUNT; i++) {
		if(technologies[i]) {
			GtkWidget *row = technologies[i]->list_item->item;
			gtk_list_box_select_row(GTK_LIST_BOX(list),
			                        GTK_LIST_BOX_ROW(row));
			break;
		}
	}
}

static void add_all_services(GDBusConnection *connection, GVariant *services_v)
{
	int i;
	int size = g_variant_n_children(services_v);
	for(i = 0; i < size; i++) {
		GVariant *path_v, *properties, *child;;
		const gchar *path;

		child = g_variant_get_child_value(services_v, i);
		path_v = g_variant_get_child_value(child, 0);
		properties = g_variant_get_child_value(child, 1);
		path = g_variant_get_string(path_v, NULL);
		if(!strstr(path, "service/vpn"))
			add_service(connection, path, properties);

		g_variant_unref(child);
		g_variant_unref(path_v);
		g_variant_unref(properties);
	}
}

static GDBusProxy *manager_create(GDBusConnection *connection,
				  const gchar *interface_data,
				  const gchar *path, const gchar *connman_path)
{
	GDBusNodeInfo *info;
	GDBusInterfaceInfo *interface;
	GError *error = NULL;
	GDBusProxy *proxy;

	info = g_dbus_node_info_new_for_xml(interface_data, &error);
	if(error)
		goto out;

	interface = g_dbus_node_info_lookup_interface(info, path);
	proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE,
				      interface, connman_path, "/", path, NULL,
				      &error);
	g_dbus_node_info_unref(info);
	if(error)
		goto out;

	return proxy;

out:
	g_critical("Failed to connect to connman: %s", error->message);
	g_error_free(error);
	return NULL;
}

static GDBusProxy *manager_register(GDBusConnection *connection)
{
	GError *error = NULL;
	GDBusProxy *proxy;
	GVariant *data, *child;

	proxy = manager_create(connection, MANAGER_INTERFACE, MANAGER_NAME,
			       CONNMAN_PATH);
	if(!proxy)
		return NULL;

	data = g_dbus_proxy_call_sync(proxy, "GetTechnologies", NULL,
	                              G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if(error) {
		g_object_unref(proxy);
		goto out;
	}

	g_signal_connect(proxy, "g-signal", G_CALLBACK(manager_signal), NULL);

	child = g_variant_get_child_value(data, 0);
	add_all_technologies(connection, child);

	g_variant_unref(data);
	g_variant_unref(child);

	data = g_dbus_proxy_call_sync(proxy, "GetServices", NULL,
	                              G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if(error) {
		g_object_unref(proxy);
		goto out;
	}

	child = g_variant_get_child_value(data, 0);
	add_all_services(connection, child);

	g_variant_unref(data);
	g_variant_unref(child);

	return proxy;

out:
	g_critical("Failed to connect to connman: %s", error->message);
	g_error_free(error);
	return NULL;
}

static GDBusProxy *vpn_manager_register(GDBusConnection *connection)
{
	GDBusProxy *proxy;

	proxy = manager_create(connection, VPN_MANAGER_INTERFACE,
			       VPN_MANAGER_NAME, CONNMAN_VPN_PATH);
	if(!proxy)
		return NULL;

	vpn_get_connections(proxy);
	return proxy;
}

static void connman_appeared(GDBusConnection *connection, const gchar *name,
                             const gchar *name_owner, gpointer user_data)
{
	manager_proxy = manager_register(connection);
	register_agent(connection, manager_proxy);
}

static gboolean is_service(gpointer key, gpointer value, gpointer user_data)
{
	return key && !strstr(key, "/vpn");
}

static void connman_disappeared(GDBusConnection *connection, const gchar *name,
                                gpointer user_data)
{
	int i;
	g_hash_table_foreach_remove(services, is_service, NULL);
	for(i = CONNECTION_TYPE_ETHERNET; i < CONNECTION_TYPE_COUNT; i++) {
		if(i == CONNECTION_TYPE_VPN)
			continue;

		if(technologies[i] && technologies[i]->path)
			remove_technology_by_path(technologies[i]->path);

		technologies[i] = NULL;
	}

	agent_release();
	g_object_unref(manager_proxy);
	manager_proxy = NULL;
}

static void connman_vpn_appeared(GDBusConnection *connection, const gchar *name,
				 const gchar *name_owner, gpointer user_data)
{
	if(technologies[CONNECTION_TYPE_VPN])
		return;
	struct technology *vpn = vpn_register(connection, list, notebook);
	technologies[CONNECTION_TYPE_VPN] = vpn;
	gtk_container_add(GTK_CONTAINER(list), vpn->list_item->item);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
	                         vpn->settings->grid, NULL);
	vpn_manager_proxy = vpn_manager_register(connection);
	register_vpn_agent(connection, vpn_manager_proxy);
}

static gboolean is_connection(gpointer key, gpointer value, gpointer user_data)
{
	return key && strstr(key, "/vpn");
}

static void connman_vpn_disappeared(GDBusConnection *connection,
				    const gchar *name, gpointer user_data)
{
	enum connection_type type = CONNECTION_TYPE_VPN;

	g_hash_table_foreach_remove(services, is_connection, NULL);

	if(technologies[type])
		technology_free(technologies[type]);

	technologies[type] = NULL;
	vpn_agent_release();
	g_object_unref(vpn_manager_proxy);
	vpn_manager_proxy = NULL;
}

static void dbus_connected(GObject *source, GAsyncResult *res,
                           gpointer user_data)
{
	(void)source;
	(void)user_data;
	GDBusConnection *connection;
	GError *error = NULL;

	connection = g_bus_get_finish(res, &error);
	if(error) {
		g_error("failed to connect to system dbus: %s",
		        error->message);
		g_error_free(error);
		return;
	}

	g_bus_watch_name_on_connection(connection, "net.connman",
	                               G_BUS_NAME_WATCHER_FLAGS_NONE,
	                               connman_appeared, connman_disappeared,
	                               NULL, NULL);
	g_bus_watch_name_on_connection(connection, "net.connman.vpn",
	                               G_BUS_NAME_WATCHER_FLAGS_NONE,
	                               connman_vpn_appeared,
				       connman_vpn_disappeared,
	                               NULL, NULL);
}

static gboolean delete_event(GtkApplication *app, GdkEvent *event,
                             gpointer user_data)
{
	shutting_down = TRUE;
	agent_release();
	return FALSE;
}

static void activate(GtkApplication *app, gpointer user_data)
{
	g_bus_get(G_BUS_TYPE_SYSTEM, NULL, dbus_connected, NULL);

	main_window = gtk_application_window_new(app);
	g_signal_connect(main_window, "delete-event", G_CALLBACK(delete_event),
	                 NULL);
	gtk_window_set_title(GTK_WINDOW(main_window), _("Network Settings"));
	gtk_window_set_default_size(GTK_WINDOW(main_window), DEFAULT_WIDTH,
	                            DEFAULT_HEIGHT);

	create_content();

	gtk_widget_show_all(main_window);
}

static const GOptionEntry options[] = {
	{ "page", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &default_page,
		NULL, NULL },
	{ NULL }
};

int main(int argc, char *argv[])
{
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
	                                 g_free, remove_service_struct);

	app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
	g_application_add_main_option_entries(G_APPLICATION(app), options);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	g_object_unref(css_provider);

	return status;
}
