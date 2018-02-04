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

#include "config.h"
#include "dialog.h"
#include "interfaces.h"
#include "main.h"
#include "technology.h"
#include "service.h"
#include "vpn.h"

static struct technology *tech;
static GDBusConnection *connection;
static GDBusProxy *proxy;
int connection_count;

static void add_connection(GDBusConnection *connection, GVariant *parameters)
{
	GVariant *path_v, *properties;
	const gchar *path;

	path_v = g_variant_get_child_value(parameters, 0);
	properties = g_variant_get_child_value(parameters, 1);
	path = g_variant_get_string(path_v, NULL);

	gtk_widget_show(tech->list_item->item);
	gtk_widget_show(tech->settings->grid);
	connection_count++;
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
	connection_count--;

	if(!connection_count) {
		gtk_widget_hide(tech->list_item->item);
		gtk_widget_hide(tech->settings->grid);
	}

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

static struct technology *create_vpn_technology(void)
{
	GVariant *properties;
	GVariantBuilder *b;

	b = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(b, "{sv}", "Name",
	                      g_variant_new_string(_("VPN")));
	g_variant_builder_add(b, "{sv}", "Type",
	                      g_variant_new_string("vpn"));
	g_variant_builder_add(b, "{sv}", "Connected",
	                      g_variant_new_boolean(FALSE));
	g_variant_builder_add(b, "{sv}", "Powered",
	                      g_variant_new_boolean(TRUE));
	properties = g_variant_builder_end(b);
	tech = technology_create(proxy, "/net/connman/technologies/vpn",
				 properties);
	gtk_widget_hide(tech->settings->power_switch);
	gtk_widget_hide(tech->settings->tethering);
	gtk_widget_hide(tech->list_item->item);
	gtk_widget_hide(tech->settings->grid);
	g_variant_unref(properties);
	g_variant_builder_unref(b);

	return tech;
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
		g_warning("Failed to connect to ConnMan: %s", error->message);
		show_error(_("Failed to connect to ConnMan."), error->message);
		g_error_free(error);
		return NULL;
	}

	g_signal_connect(proxy, "g-signal", G_CALLBACK(vpn_signal), NULL);
	tech = create_vpn_technology();
	return tech;
}

void vpn_update_status(struct technology *tech)
{
	GHashTableIter iter;
	gpointer key, value;
	struct technology_settings *item;
	int status = 0;

	item = tech->settings;
	g_hash_table_iter_init(&iter, tech->services);
	while(g_hash_table_iter_next(&iter, &key, &value)) {
		gchar *state;

		state = service_get_property_string_raw(value, "State", NULL);
		if(status < 2 && !strcmp(state, "ready"))
			status = 2;
		else if(status < 1 && !strcmp(state, "configuration"))
			status = 1;

		g_free(state);
	}

	gtk_label_set_text(GTK_LABEL(item->title), _("VPN"));
	if(status == 2)
		gtk_label_set_text(GTK_LABEL(item->status), _("Connected"));
	else if(status == 1)
		gtk_label_set_text(GTK_LABEL(item->status), _("Connecting"));
	else
		gtk_label_set_text(GTK_LABEL(item->status), _("Not connected"));
}

void vpn_get_connections(GDBusProxy *manager_proxy)
{
	GVariant *data, *child;
	GError *error = NULL;
	proxy = manager_proxy;
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
	connection_count = 0;
	proxy = NULL;
	tech = NULL;
}
