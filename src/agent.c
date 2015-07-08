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

#include <gio/gio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>

#include "agent.h"
#include "interfaces.h"
#include "main.h"
#include "style.h"

GDBusConnection *conn = NULL;
gint agent_id;

void release(GDBusMethodInvocation *invocation)
{
	g_dbus_method_invocation_return_value(invocation, NULL);
}

void report_error(GDBusMethodInvocation *invocation, GVariant *parameters)
{
	g_dbus_method_invocation_return_dbus_error(invocation,
	                "net.connman.Agent.Error.Retry", "");
}

void report_peer_error(GDBusMethodInvocation *invocation, GVariant *parameters)
{
	g_dbus_method_invocation_return_value(invocation, NULL);
}

void request_browser(GDBusMethodInvocation *invocation, GVariant *parameters)
{
	g_dbus_method_invocation_return_dbus_error(invocation,
	                "net.connman.Agent.Error.Canceled",
	                "User canceled browser");
}

struct token_entry {
	GtkWidget *label;
	GtkWidget *entry;
};

static void add_field(GPtrArray *array, const char *label, gboolean secret)
{
	struct token_entry *entry = g_malloc(sizeof(*entry));
	entry->label = gtk_label_new(label);
	entry->entry = gtk_entry_new();
	STYLE_ADD_MARGIN(entry->label, MARGIN_LARGE);
	STYLE_ADD_MARGIN(entry->entry, MARGIN_LARGE);
	gtk_entry_set_visibility(GTK_ENTRY(entry->entry), FALSE);
	g_ptr_array_add(array, entry);
}

static GPtrArray *generate_entries(GVariant *args)
{
	gchar *key;
	GVariant *value;
	GVariant *service, *parameters;
	GVariantIter *iter;
	GPtrArray *array = g_ptr_array_new_full(1, g_free);

	service = g_variant_get_child_value(args, 0);
	parameters = g_variant_get_child_value(args, 1);
	iter = g_variant_iter_new(parameters);
	while(g_variant_iter_loop(iter, "{sv}", &key, &value)) {
		GVariantIter *propertyiter = g_variant_iter_new(value);
		gchar *prop;
		GVariant *val;
		while(g_variant_iter_loop(propertyiter, "{sv}", &prop, &val)) {
			if(!strcmp(prop, "Requirement")) {
				const gchar *req = g_variant_get_string(val, NULL);
				if(!strcmp(req, "mandatory")) {
					add_field(array, key, TRUE);
				}

			}
		}
		g_variant_iter_free(propertyiter);
	}
	g_variant_iter_free(iter);
	g_variant_unref(parameters);
	g_variant_unref(service);

	return array;
}

void request_input(GDBusMethodInvocation *invocation, GVariant *parameters)
{
	GtkWidget *grid = gtk_grid_new();
	GPtrArray *entries;
	int i;
	GtkWidget *window = gtk_dialog_new_with_buttons(
	                            _("Authentication required"),
	                            GTK_WINDOW(main_window),
	                            GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
	                            _("_OK"), GTK_RESPONSE_ACCEPT,
	                            _("_Cancel"), GTK_RESPONSE_CANCEL, NULL);
	entries = generate_entries(parameters);
	for(i = 0; i < entries->len; i++) {
		struct token_entry *entry = g_ptr_array_index(entries, i);
		gtk_grid_attach(GTK_GRID(grid), entry->label, 0, i, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), entry->entry, 1, i, 1, 1);
	}
	gtk_widget_show_all(grid);
	gtk_container_add(GTK_CONTAINER(
	                          gtk_dialog_get_content_area(GTK_DIALOG(window))),
	                  grid);
	i = gtk_dialog_run(GTK_DIALOG(window));
	if(i == GTK_RESPONSE_ACCEPT) {
		GVariantDict *dict = g_variant_dict_new(NULL);
		GVariant *out_v, *out;
		for(i = 0; i < entries->len; i++) {
			struct token_entry *entry = g_ptr_array_index(entries, i);
			GtkEntryBuffer *buf = gtk_entry_get_buffer(GTK_ENTRY(entry->entry));
			const gchar *key = gtk_label_get_text(GTK_LABEL(entry->label));
			const gchar *value = gtk_entry_buffer_get_text(buf);
			g_variant_dict_insert(dict, key, "s", value);
		}
		g_variant_dict_insert(dict, "foo", "s", "bar");
		out = g_variant_dict_end(dict);
		out_v = g_variant_new("(@a{sv})", out);
		g_variant_ref_sink(out_v);
		g_dbus_method_invocation_return_value(invocation, out_v);
		g_variant_unref(out_v);
		g_variant_dict_unref(dict);
	} else
		g_dbus_method_invocation_return_dbus_error(invocation,
		                "net.connman.Agent.Error.Canceled",
		                "User canceled password dialog");

	g_ptr_array_free(entries, TRUE);
	gtk_widget_destroy(window);
}

void request_peer_authorization(GDBusMethodInvocation *invocation,
                                GVariant *parameters)
{
	g_dbus_method_invocation_return_dbus_error(invocation,
	                "net.connman.Agent.Error.Canceled",
	                "User canceled password dialog");
}

void cancel(GDBusMethodInvocation *invocation)
{
	g_dbus_method_invocation_return_value(invocation, NULL);
}

void method_call(GDBusConnection *connection, const gchar *sender,
                 const gchar *object_path, const gchar *interface_name,
                 const gchar *method_name, GVariant *parameters,
                 GDBusMethodInvocation *invocation, gpointer user_data)
{
	if(!strcmp(method_name, "Release"))
		release(invocation);
	else if(!strcmp(method_name, "ReportError"))
		report_error(invocation, parameters);
	else if(!strcmp(method_name, "ReportPeerError"))
		report_peer_error(invocation, parameters);
	else if(!strcmp(method_name, "RequestBrowser"))
		request_browser(invocation, parameters);
	else if(!strcmp(method_name, "RequestInput"))
		request_input(invocation, parameters);
	else if(!strcmp(method_name, "RequestPeerAuthorization"))
		request_peer_authorization(invocation, parameters);
	else
		cancel(invocation);
}

static char *agent_path(void)
{
	static char *path = NULL;
	if(!path)
		path = g_strdup_printf("/net/connmangtk/agent%d", getpid());
	return path;
}

static const GDBusInterfaceVTable vtable = {
	method_call,
	NULL,
	NULL
};

void register_agent(GDBusConnection *connection, GDBusProxy *manager)
{
	GError *error = NULL;
	GDBusNodeInfo *info = g_dbus_node_info_new_for_xml(AGENT_INTERFACE,
	                      &error);
	GVariant *ret;
	if(error) {
		g_critical("Failed to load agent interface: %s",
		           error->message);
		g_error_free(error);
		return;
	}
	agent_id = g_dbus_connection_register_object(connection, agent_path(),
	                g_dbus_node_info_lookup_interface(info,
	                                "net.connman.Agent"),
	                &vtable, NULL, NULL, &error);
	conn = connection;
	g_dbus_node_info_unref(info);
	if(error) {
		g_critical("Failed to register agent object: %s",
		           error->message);
		g_error_free(error);
		return;
	}
	ret = g_dbus_proxy_call_sync(manager, "RegisterAgent",
	                             g_variant_new("(o)", agent_path()),
	                             G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if(error) {
		g_critical("Failed to register agent: %s",
		           error->message);
		g_error_free(error);
		return;
	}
	g_variant_unref(ret);
}

void agent_release(void)
{
	if(conn && agent_id)
		g_dbus_connection_unregister_object(conn, agent_id);
	conn = NULL;
	agent_id = 0;
}
