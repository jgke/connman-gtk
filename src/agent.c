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
#include "dialog.h"
#include "interfaces.h"
#include "main.h"
#include "style.h"
#include "openconnect_helper.h"

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

static GPtrArray *generate_entries(GVariant *args)
{
	gchar *key;
	GVariant *value, *parameters;
	GVariantIter *iter;
	GVariantDict *argdict;
	GPtrArray *array;

	array = g_ptr_array_new_full(1, (GDestroyNotify)free_token_element);
	parameters = g_variant_get_child_value(args, 1);
	argdict = g_variant_dict_new(parameters);
	iter = g_variant_iter_new(parameters);
	while(g_variant_iter_loop(iter, "{sv}", &key, &value)) {
		struct token_element *elem;
		const gchar *req, *previous, *pp = "PreviousPassphrase";
		GVariantDict *prevdict, *dict;
		GVariant *prevv;

		dict = g_variant_dict_new(value);
		g_variant_dict_lookup(dict, "Requirement", "&s", &req);

		if(strcmp(req, "mandatory")) {
			g_variant_dict_unref(dict);
			continue;
		}

		if(strcmp(key, "Passphrase") ||
		   !g_variant_dict_contains(argdict, pp)) {
			elem = token_new_entry(key, TRUE);

			g_ptr_array_add(array, elem);
			g_variant_dict_unref(dict);
			continue;
		}

		prevv = g_variant_dict_lookup_value(argdict, pp, NULL);
		prevdict = g_variant_dict_new(prevv);
		g_variant_dict_lookup(prevdict, "Value", "&s", &previous);
		elem = token_new_entry_full(key, TRUE, previous, NULL);
		g_variant_dict_unref(prevdict);
		g_variant_unref(prevv);

		g_ptr_array_add(array, elem);
		g_variant_dict_unref(dict);
	}
	g_variant_iter_free(iter);
	g_variant_unref(parameters);
	g_variant_dict_unref(argdict);

	return array;
}

static GVariantDict *generate_dict(GPtrArray *elements)
{
	GVariantDict *dict;
	int i;

	dict = g_variant_dict_new(NULL);
	for(i = 0; i < elements->len; i++) {
		struct token_element *elem = elements->pdata[i];
		if(elem->type > TOKEN_ELEMENT_TEXT)
			g_variant_dict_insert(dict, elem->name, "s",
					      elem->value);
	}
	return dict;
}

struct request_input_params {
	struct agent *agent;
	GDBusMethodInvocation *invocation;
	GVariant *parameters;
};

gpointer request_input(gpointer data)
{
	struct request_input_params *params = data;
	struct agent *agent = params->agent;
	GDBusMethodInvocation *invocation = params->invocation;
	GVariant *parameters = params->parameters;;
	GVariantDict *dict = NULL;
	GVariant *ret, *ret_v;
	GPtrArray *entries = NULL;
	int success = 0;

	if(!is_openconnect(parameters)) {
		entries = generate_entries(parameters);
		success = dialog_ask_tokens(_("Authentication required"),
					   entries);
		if(success)
			dict = generate_dict(entries);
	}
	else
		dict = openconnect_handle(invocation, parameters);

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
	if(entries)
		g_ptr_array_free(entries, TRUE);
	g_free(data);
	return NULL;
}

void request_input_async(struct agent *agent, GDBusMethodInvocation *invocation,
			 GVariant *parameters)
{
	struct request_input_params *params = g_malloc(sizeof(*params));
	params->agent = agent;
	params->invocation = invocation;
	params->parameters = parameters;
	g_thread_new("request_input", (GThreadFunc)request_input, params);
}

void request_peer_authorization(struct agent *agent,
				GDBusMethodInvocation *invocation,
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
		request_input_async(user_data, invocation, parameters);
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
