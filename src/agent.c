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
#include <string.h>
#include <unistd.h>

#include "agent.h"
#include "interfaces.h"

void release(GDBusMethodInvocation *invocation)
{
	g_dbus_method_invocation_return_value(invocation, NULL);
}

void report_error(GDBusMethodInvocation *invocation, GVariant *parameters)
{
	g_dbus_method_invocation_return_value(invocation, NULL);
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

void request_input(GDBusMethodInvocation *invocation, GVariant *parameters)
{
	g_dbus_method_invocation_return_dbus_error(invocation,
	                "net.connman.Agent.Error.Canceled",
	                "User canceled password dialog");
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
	g_dbus_connection_register_object(connection, agent_path(),
	                                  g_dbus_node_info_lookup_interface(info,
	                                                  "net.connman.Agent"),
	                                  &vtable, NULL, NULL, &error);
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
