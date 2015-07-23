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

#include <glib.h>
#include <glib/gi18n.h>

#include "interfaces.h"
#include "main.h"
#include "technology.h"
#include "vpn.h"

static struct technology tech;
static GDBusProxy *proxy;
static GDBusConnection *connection;

static void add_connection(GDBusConnection *connection, GVariant *parameters)
{
	GVariant *path_v, *properties;
	const gchar *path;

	path_v = g_variant_get_child_value(parameters, 0);
	properties = g_variant_get_child_value(parameters, 1);
	path = g_variant_get_string(path_v, NULL);

	modify_service(g_dbus_proxy_get_connection(proxy), path,
		       properties);

	g_variant_unref(path_v);
	g_variant_unref(properties);
}

static void remove_connection(GVariant *parameters)
{
	GVariant *path_v;
	const gchar *path;

	path_v = g_variant_get_child_value(parameters, 0);
	path = g_variant_get_string(path_v, NULL);

	remove_service(path);

	g_variant_unref(path_v);
}

static void vpn_signal(GDBusProxy *proxy, gchar *sender, gchar *signal,
                       GVariant *parameters, gpointer user_data)
{
	if(!strcmp(signal, "ConnectionAdded"))
		add_connection(g_dbus_proxy_get_connection(proxy), parameters);
	else if(!strcmp(signal, "ConnectionRemoved"))
		remove_connection(parameters);
}

static void add_all_connections(GDBusConnection *Connection,
				GVariant *connections_v)
{
	int i;
	int size = g_variant_n_children(connections_v);
	for(i = 0; i < size; i++) {
		GVariant *child = g_variant_get_child_value(connections_v, i);
		add_connection(connection, child);
		g_variant_unref(child);
	}
}

static void init_vpn_technology(void)
{
	GVariant *properties;
	GVariantBuilder *b;
	tech.services = g_hash_table_new_full(g_str_hash, g_str_equal,
	                                      g_free, NULL);
	b = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(b, "{sv}", "Name",
	                      g_variant_new_string(_("VPN")));
	g_variant_builder_add(b, "{sv}", "Connected",
	                      g_variant_new_boolean(FALSE));
	g_variant_builder_add(b, "{sv}", "Powered",
	                      g_variant_new_boolean(TRUE));
	properties = g_variant_builder_end(b);
	tech.type = CONNECTION_TYPE_VPN;
	tech.settings = technology_create_settings(&tech, properties, proxy);
	tech.list_item = technology_create_item(&tech, _("VPN"));
	gtk_image_set_from_icon_name(GTK_IMAGE(tech.list_item->icon),
	                             "network-vpn-symbolic",
	                             GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_image_set_from_icon_name(GTK_IMAGE(tech.settings->icon),
	                             "network-vpn", GTK_ICON_SIZE_DIALOG);
	gtk_widget_hide(tech.settings->power_switch);
	gtk_widget_hide(tech.settings->tethering);
	g_variant_unref(properties);
	g_variant_builder_unref(b);
}

void vpn_clear_properties(struct service *serv)
{

}

void vpn_free(struct technology *tech)
{
	g_hash_table_unref(tech->services);
}

struct technology *vpn_register(GDBusConnection *conn, GtkWidget *list,
                                GtkWidget *notebook)
{
	GDBusNodeInfo *info;
	GDBusInterfaceInfo *interface;
	GError *error = NULL;

	connection = conn;
	info = g_dbus_node_info_new_for_xml(VPN_MANAGER_INTERFACE, &error);
	if(error) {
		/* TODO: show user error message */
		g_critical("Failed to load VPN manager interface: %s",
		           error->message);
		g_error_free(error);
		return NULL;
	}

	interface = g_dbus_node_info_lookup_interface(info, VPN_MANAGER_NAME);
	proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE,
	                              interface, CONNMAN_PATH ".vpn", "/",
	                              VPN_MANAGER_NAME, NULL, &error);
	g_dbus_node_info_unref(info);
	if(error) {
		g_warning("failed to connect to ConnMan: %s", error->message);
		g_error_free(error);
		return NULL;
	}

	g_signal_connect(proxy, "g-signal", G_CALLBACK(vpn_signal), NULL);
	init_vpn_technology();
	return &tech;
}

void vpn_get_connections(void)
{
	GVariant *data, *child;
	GError *error = NULL;
	data = g_dbus_proxy_call_sync(proxy, "GetConnections", NULL,
	                              G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if(error) {
		g_warning("failed to get vpn connections: %s", error->message);
		g_error_free(error);
		g_object_unref(proxy);
		return;
	}

	child = g_variant_get_child_value(data, 0);
	add_all_connections(connection, child);
	g_variant_unref(data);
	g_variant_unref(child);
}

void vpn_release(void)
{
	g_object_unref(proxy);
}
