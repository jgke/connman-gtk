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

struct agent {
	gint id;
	const gchar *cancel;
} *normal_agent, *vpn_agent;

void release(struct agent *agent, GDBusMethodInvocation *invocation)
{
	g_dbus_method_invocation_return_value(invocation, NULL);
}

void report_error(struct agent *agent, GDBusMethodInvocation *invocation,
		  GVariant *parameters)
{
	g_dbus_method_invocation_return_dbus_error(invocation,
	                "net.connman.Agent.Error.Retry", "");
}

void report_peer_error(struct agent *agent, GDBusMethodInvocation *invocation,
		       GVariant *parameters)
{
	g_dbus_method_invocation_return_value(invocation, NULL);
}

void request_browser(struct agent *agent, GDBusMethodInvocation *invocation,
		     GVariant *parameters)
{
	g_dbus_method_invocation_return_dbus_error(invocation,
	                "net.connman.Agent.Error.Canceled",
	                "User canceled browser");
}

struct token_entry {
	GtkWidget *label;
	GtkWidget *entry;
};

static GVariantDict *get_tokens(GPtrArray *entries)
{
	GtkDialog *dialog;
	GtkWidget *grid, *window;
	GVariantDict *dict = NULL;
	int i;
	int flags = GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL;

	grid = gtk_grid_new();
	window = gtk_dialog_new_with_buttons(_("Authentication required"),
					     GTK_WINDOW(main_window), flags,
					     _("_OK"), GTK_RESPONSE_ACCEPT,
					     _("_Cancel"), GTK_RESPONSE_CANCEL,
					     NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(window),
					GTK_RESPONSE_ACCEPT);
	for(i = 0; i < entries->len; i++) {
		struct token_entry *entry = g_ptr_array_index(entries, i);
		gtk_grid_attach(GTK_GRID(grid), entry->label, 0, i, 1, 1);
		gtk_entry_set_activates_default(GTK_ENTRY(entry->entry), TRUE);
		gtk_grid_attach(GTK_GRID(grid), entry->entry, 1, i, 1, 1);
	}

	gtk_widget_show_all(grid);
	dialog = GTK_DIALOG(window);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(dialog)),
	                  grid);

	i = gtk_dialog_run(GTK_DIALOG(window));
	if(i != GTK_RESPONSE_ACCEPT)
		goto out;

	dict = g_variant_dict_new(NULL);
	for(i = 0; i < entries->len; i++) {
		struct token_entry *entry;
		GtkEntryBuffer *buf;
		const gchar *key;
		const gchar *value;

		entry = g_ptr_array_index(entries, i);
		buf = gtk_entry_get_buffer(GTK_ENTRY(entry->entry));
		key = gtk_label_get_text(GTK_LABEL(entry->label));
		value = gtk_entry_buffer_get_text(buf);
		g_variant_dict_insert(dict, key, "s", value);
	}

out:
	gtk_widget_destroy(window);
	return dict;
}

static void add_field(GPtrArray *array, const char *label, gboolean secret)
{
	struct token_entry *entry = g_malloc(sizeof(*entry));
	entry->label = gtk_label_new(label);
	entry->entry = gtk_entry_new();
	style_add_margin(entry->label, MARGIN_LARGE);
	style_add_margin(entry->entry, MARGIN_LARGE);
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

void request_input(struct agent *agent, GDBusMethodInvocation *invocation,
		   GVariant *parameters)
{
	GVariantDict *dict;
	GVariant *ret, *ret_v;
	GPtrArray *entries;

	entries = generate_entries(parameters);
	dict = get_tokens(entries);

	if(!dict) {
		g_dbus_method_invocation_return_dbus_error(invocation,
				agent->cancel, "User canceled password dialog");
		goto out;
	}

	ret = g_variant_dict_end(dict);
	ret_v = g_variant_new("(@a{sv})", ret);
	g_variant_ref_sink(ret_v);
	g_dbus_method_invocation_return_value(invocation, ret_v);
	g_variant_unref(ret_v);
	g_variant_dict_unref(dict);

out:
	g_ptr_array_free(entries, TRUE);
}

void request_peer_authorization(struct agent *agent, GDBusMethodInvocation *invocation,
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
		release(user_data, invocation);
	else if(!strcmp(method_name, "ReportError"))
		report_error(user_data, invocation, parameters);
	else if(!strcmp(method_name, "ReportPeerError"))
		report_peer_error(user_data, invocation, parameters);
	else if(!strcmp(method_name, "RequestBrowser"))
		request_browser(user_data, invocation, parameters);
	else if(!strcmp(method_name, "RequestInput"))
		request_input(user_data, invocation, parameters);
	else if(!strcmp(method_name, "RequestPeerAuthorization"))
		request_peer_authorization(user_data, invocation, parameters);
	else
		cancel(invocation);
}

static char *agent_path(void)
{
	static char *path = NULL;
	if(!path)
		path = g_strdup_printf("/net/connman/gtk/agent%d", getpid());
	return path;
}

static char *vpn_agent_path(void)
{
	static char *path = NULL;
	if(!path)
		path = g_strdup_printf("/net/connman/gtk/vpn/agent%d",
				       getpid());
	return path;
}

static const GDBusInterfaceVTable vtable = {
	method_call,
	NULL,
	NULL
};

static struct agent *agent_create(GDBusConnection *connection,
				  GDBusProxy *manager,
				  const gchar *interface_name,
				  const gchar *agent_path, const gchar *path,
				  const gchar *cancel)
{
	GError *error = NULL;
	struct agent *agent;
	GVariant *ret;
	GDBusNodeInfo *info;
	GDBusInterfaceInfo *interface;

	info = g_dbus_node_info_new_for_xml(interface_name, &error);
	if(error) {
		g_critical("Failed to load agent interface: %s",
		           error->message);
		g_error_free(error);
		return NULL;
	}

	agent = g_malloc(sizeof(*agent));
	interface = g_dbus_node_info_lookup_interface(info, agent_path),
	agent->id = g_dbus_connection_register_object(connection, path,
						      interface, &vtable, agent,
						      NULL, &error);
	conn = connection;
	g_dbus_node_info_unref(info);
	if(error) {
		g_critical("Failed to register agent object: %s",
		           error->message);
		g_error_free(error);
		g_free(agent);
		return NULL;
	}

	ret = g_dbus_proxy_call_sync(manager, "RegisterAgent",
	                             g_variant_new("(o)", path),
	                             G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if(error) {
		g_critical("Failed to register agent: %s",
		           error->message);
		g_error_free(error);
		g_free(agent);
		return NULL;
	}
	g_variant_unref(ret);

	agent->cancel = cancel;
	return agent;
}


void register_agent(GDBusConnection *connection, GDBusProxy *manager)
{
	normal_agent = agent_create(connection, manager, AGENT_INTERFACE,
				    AGENT_NAME, agent_path(),
				    "net.connman.Agent.Error.Canceled");
}

void register_vpn_agent(GDBusConnection *connection, GDBusProxy *vpn_manager)
{
	vpn_agent  = agent_create(connection, vpn_manager, VPN_AGENT_INTERFACE,
				  VPN_AGENT_NAME, vpn_agent_path(),
				  "net.connman.vpn.Agent.Error.Canceled");
}

void agent_release(void)
{
	if(conn && normal_agent) {
		g_dbus_connection_unregister_object(conn, normal_agent->id);
		g_free(normal_agent);
	}
	normal_agent = NULL;
}

void vpn_agent_release(void)
{
	if(conn && vpn_agent) {
		g_dbus_connection_unregister_object(conn, vpn_agent->id);
		g_free(vpn_agent);
	}
	vpn_agent = NULL;
}
