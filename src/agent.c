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

#ifdef USE_OPENCONNECT
#include "openconnect_helper.h"
#endif

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

static void add_token_entry(GtkGrid *grid, struct token_entry *entry, int y)
{
	if(GTK_IS_ENTRY(entry->entry))
		gtk_entry_set_activates_default(GTK_ENTRY(entry->entry), TRUE);
	gtk_grid_attach(GTK_GRID(grid), entry->label, 0, y, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), entry->entry, 1, y, 1, 1);
}

static gchar *get_token_entry_value(struct token_entry *entry)
{
	gchar *value;
	GtkWidget *e = entry->entry;

	if(GTK_IS_ENTRY(e)) {
		GtkEntryBuffer *buf;

		buf = gtk_entry_get_buffer(GTK_ENTRY(e));
		value = g_strdup(gtk_entry_buffer_get_text(buf));
	} else {
		GtkComboBoxText *box = GTK_COMBO_BOX_TEXT(e);
		value = gtk_combo_box_text_get_active_text(box);
	}

	return value;
}

struct token_window_params {
	GPtrArray *entries;
	GCond *cond;
	GMutex *mutex;
	GVariantDict *tokens;
	gboolean returned;
};

static gboolean ask_tokens_window_sync(void *data)
{
	struct token_window_params *params = data;
	GPtrArray *entries = params->entries;

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
		add_token_entry(GTK_GRID(grid), entry, i);
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
		const gchar *key;
		gchar *value;

		entry = g_ptr_array_index(entries, i);
		key = gtk_label_get_text(GTK_LABEL(entry->label));
		value = get_token_entry_value(entry);
		g_variant_dict_insert(dict, key, "s", value);
		g_free(value);
	}

out:
	gtk_widget_destroy(window);
	g_mutex_lock(params->mutex);
	params->tokens = dict;
	params->returned = TRUE;
	g_cond_signal(params->cond);
	g_mutex_unlock(params->mutex);
	return FALSE;
}

static GVariantDict *ask_tokens_window_lock(GPtrArray *entries)
{
	struct token_window_params params = {};
	GMutex mutex;
	GCond cond;

	params.entries = entries;
	params.mutex = &mutex;
	params.cond = &cond;
	g_mutex_init(&mutex);
	g_cond_init(&cond);
	g_main_context_invoke(NULL, (GSourceFunc)ask_tokens_window_sync,
			      &params);
	g_mutex_lock(&mutex);
	while(!params.returned)
		g_cond_wait(&cond, &mutex);
	g_mutex_unlock(&mutex);
	g_mutex_clear(&mutex);
	g_cond_clear(&cond);
	return params.tokens;
}

static void add_field(GPtrArray *array, const char *label, gboolean secret)
{
	struct token_entry *entry = g_malloc(sizeof(*entry));
	entry->label = gtk_label_new(label);
	entry->entry = gtk_entry_new();
	style_add_margin(entry->label, MARGIN_LARGE);
	style_add_margin(entry->entry, MARGIN_LARGE);
	if(secret && strcmp(label, "Name") && strcmp(label, "Identity") &&
	   strcmp(label, "WPS") && strcmp(label, "Username") &&
	   strcmp(label, "Host") && strcmp(label, "OpenConnect.CACert") &&
	   strcmp(label, "OpenConnect.ServerCert") &&
	   strcmp(label, "OpenConnect.VPNHost"))
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

static GPtrArray *get_tokens(GPtrArray *tokens)
{
	GPtrArray *array;
	GVariantDict *dict;
	int i;

	array = g_ptr_array_new_full(1, g_free);
	for(i = 0; i < tokens->len; i++) {
		struct auth_token *token = tokens->pdata[i];
		if(!token->list)
			add_field(array, token->label, token->hidden);
		else {
			struct token_entry *entry = g_malloc(sizeof(*entry));
			GtkComboBoxText *box;
			int i;

			entry->label = gtk_label_new(token->label);
			entry->entry = gtk_combo_box_text_new();
			box = GTK_COMBO_BOX_TEXT(entry->entry);
			style_add_margin(entry->label, MARGIN_LARGE);
			style_add_margin(entry->entry, MARGIN_LARGE);

			for(i = 0; i < token->options->len; i++) {
				const gchar *opt = token->options->pdata[i];
				gtk_combo_box_text_append_text(box, opt);
			}

			g_ptr_array_add(array, entry);
		}
	}
	dict = ask_tokens_window_lock(array);
	g_ptr_array_free(array, TRUE);
	if(!dict)
		return NULL;
	for(i = 0; i < tokens->len; i++) {
		GVariant *value_v;
		struct auth_token *token = tokens->pdata[i];
		value_v = g_variant_dict_lookup_value(dict, token->label, NULL);
		if(value_v) {
			token->value = g_variant_dup_string(value_v, NULL);
			g_variant_unref(value_v);
		}
		else
			token->value = g_strdup("");
	}
	g_variant_dict_unref(dict);
	return tokens;
}

#ifdef USE_OPENCONNECT
static gboolean is_openconnect(GVariant *args)
{
	GVariantIter *iter;
	gchar *path;
	GVariant *value, *service, *parameters;
	gboolean openconnect = FALSE;

	service = g_variant_get_child_value(args, 0);
	parameters = g_variant_get_child_value(args, 1);

	iter = g_variant_iter_new(parameters);

	while(g_variant_iter_loop(iter, "{sv}", &path, &value))
		openconnect = openconnect || strstr(path, "OpenConnect");
	g_variant_iter_free(iter);

	g_variant_unref(service);
	g_variant_unref(parameters);

	return openconnect;
}
#endif

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
	GVariantDict *dict;
	GVariant *ret, *ret_v;
	GPtrArray *entries = NULL;

#ifdef USE_OPENCONNECT
	if(!is_openconnect(parameters)) {
		entries = generate_entries(parameters);
		dict = ask_tokens_window_lock(entries);
	}
	else
		dict = openconnect_handle(invocation, parameters, get_tokens);
#else
	entries = generate_entries(parameters);
	dict = ask_tokens_window_lock(entries);
#endif

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
