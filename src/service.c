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

#include <string.h>

#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "style.h"
#include "service.h"
#include "settings.h"
#include "util.h"
#include "connections/ethernet.h"
#include "connections/wireless.h"

static struct {
	void (*init)(struct service *serv, GDBusProxy *proxy, const gchar *path,
	             GVariant *properties);
	struct service *(*create)(void);
	void (*free)(struct service *serv);
	void (*update)(struct service *serv);
} functions[CONNECTION_TYPE_COUNT] = {
	{},
	{
		service_ethernet_init, service_ethernet_create,
		service_ethernet_free, service_ethernet_update
	},
	{
		service_wireless_init, service_wireless_create,
		service_wireless_free, service_wireless_update
	}
};

void service_update_property(struct service *serv, const gchar *key,
                             GVariant *value)
{
	if(strcmp(g_variant_get_type_string(value), "a{sv}")) {
		hash_table_set_dual_key(serv->properties, key, NULL,
		                        g_variant_ref(value));
		if(serv->sett)
			settings_update(serv->sett, key, NULL, value);
	} else {
		gchar *subkey;
		GVariantIter *iter = g_variant_iter_new(value);
		while(g_variant_iter_loop(iter, "{sv}", &subkey, &value)) {
			hash_table_set_dual_key(serv->properties, key, subkey,
			                        g_variant_ref(value));
			if(serv->sett)
				settings_update(serv->sett, key, subkey, value);
		}
		g_variant_iter_free(iter);
	}
}

void service_update(struct service *serv, GVariant *properties)
{
	GVariantIter *iter;
	gchar *key;
	GVariant *value;

	iter = g_variant_iter_new(properties);
	while(g_variant_iter_loop(iter, "{sv}", &key, &value))
		service_update_property(serv, key, value);
	g_variant_iter_free(iter);
	if(functions[serv->type].update)
		functions[serv->type].update(serv);
}

static void service_proxy_signal(GDBusProxy *proxy, gchar *sender,
                                 gchar *signal, GVariant *parameters,
                                 gpointer user_data)
{
	struct service *serv = user_data;
	if(!strcmp(signal, "PropertyChanged")) {
		GVariant *name_v, *value_v, *value;
		const gchar *name;

		name_v = g_variant_get_child_value(parameters, 0);
		value_v = g_variant_get_child_value(parameters, 1);

		name = g_variant_get_string(name_v, NULL);
		value = g_variant_get_child_value(value_v, 0);

		service_update_property(serv, name, value);
		g_variant_unref(value);
		g_variant_unref(name_v);
		g_variant_unref(value_v);
	}
}

static void settings_closed(struct service *serv)
{
	serv->sett = NULL;
}

static void settings_button_cb(GtkButton *button, gpointer user_data)
{
	struct service *serv = user_data;
	serv->sett = settings_create(serv, settings_closed);
}

void service_init(struct service *serv, GDBusProxy *proxy, const gchar *path,
                  GVariant *properties)
{
	GtkGrid *item_grid;

	serv->proxy = proxy;
	serv->path = g_strdup(path);
	serv->properties = dual_hash_table_new((GDestroyNotify)g_variant_unref);
	serv->sett = NULL;

	serv->item = gtk_list_box_row_new();
	serv->header = gtk_grid_new();
	serv->title = gtk_label_new(NULL);
	serv->contents = gtk_grid_new();
	serv->settings_button = gtk_button_new_from_icon_name(
	                                "emblem-system-symbolic",
	                                GTK_ICON_SIZE_MENU);
	item_grid = GTK_GRID(gtk_grid_new());

	g_object_ref(serv->item);
	g_object_ref(serv->header);
	g_object_ref(serv->title);
	g_object_ref(serv->contents);
	g_object_set_data(G_OBJECT(serv->item), "service", serv);

	g_signal_connect(proxy, "g-signal", G_CALLBACK(service_proxy_signal),
	                 serv);
	g_signal_connect(serv->settings_button, "clicked",
	                 G_CALLBACK(settings_button_cb), serv);

	gtk_grid_set_column_homogeneous(GTK_GRID(serv->contents), TRUE);

	STYLE_ADD_MARGIN(serv->item, MARGIN_SMALL);
	STYLE_ADD_MARGIN(serv->title, MARGIN_SMALL);
	gtk_widget_set_margin_start(serv->title, MARGIN_LARGE);
	gtk_widget_set_margin_start(serv->contents, MARGIN_LARGE);
	gtk_widget_set_margin_top(serv->contents, 0);
	gtk_widget_set_margin_bottom(serv->contents, 0);

	gtk_widget_set_hexpand(serv->item, TRUE);
	gtk_widget_set_hexpand(serv->header, TRUE);
	gtk_widget_set_hexpand(serv->title, TRUE);
	gtk_widget_set_hexpand(serv->contents, TRUE);
	gtk_widget_set_halign(serv->title, GTK_ALIGN_START);

	gtk_grid_attach(GTK_GRID(serv->header), serv->title, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(serv->header), serv->settings_button,
	                1, 0, 1, 1);
	gtk_grid_attach(item_grid, serv->header, 0, 0, 1, 1);
	gtk_grid_attach(item_grid, serv->contents, 0, 1, 1, 1);
	gtk_container_add(GTK_CONTAINER(serv->item), GTK_WIDGET(item_grid));

	gtk_widget_show_all(serv->item);
}

struct service *service_create(struct technology *tech, GDBusProxy *proxy,
                               const gchar *path, GVariant *properties)
{
	struct service *serv;
	enum connection_type type;
	GVariantDict *properties_d;
	GVariant *type_v;

	properties_d = g_variant_dict_new(properties);
	type_v = g_variant_dict_lookup_value(properties_d, "Type", NULL);
	type = connection_type_from_string(g_variant_get_string(type_v, NULL));
	if(type == CONNECTION_TYPE_UNKNOWN)
		type = connection_type_from_path(path);
	g_variant_unref(type_v);
	g_variant_dict_unref(properties_d);

	if(functions[type].create)
		serv = functions[type].create();
	else
		serv = g_malloc(sizeof(*serv));

	serv->type = type;
	serv->tech = tech;
	serv->sett = NULL;

	service_init(serv, proxy, path, properties);
	if(functions[type].init)
		functions[type].init(serv, proxy, path, properties);

	service_update(serv, properties);

	return serv;
}

void service_free(struct service *serv)
{
	if(!serv)
		return;
	g_object_unref(serv->proxy);
	g_object_unref(serv->item);
	g_free(serv->path);
	dual_hash_table_unref(serv->properties);
	gtk_widget_destroy(serv->item);
	if(functions[serv->type].free)
		functions[serv->type].free(serv);
	else
		g_free(serv);
}

static void service_toggle_connection_cb(GObject *source, GAsyncResult *res,
                gpointer user_data)
{
	const gchar *ia = "GDBus.Error:net.connman.Error.InvalidArguments";
	GError *error = NULL;
	GVariant *out;
	out = g_dbus_proxy_call_finish((GDBusProxy *)user_data, res, &error);
	if(error) {
		/*
		 * InvalidArguments is thrown when user cancels the dialog,
		 * so ignore it
		 */
		if(strncmp(ia, error->message, strlen(ia)))
			g_warning("failed to toggle connection state: %s",
			          error->message);
		g_error_free(error);
		return;
	}
	g_variant_unref(out);
}

void service_toggle_connection(struct service *serv)
{
	const gchar *function;
	gchar *state;

	state = service_get_property_string(serv, "State", NULL);

	if(!strcmp(state, "idle") || !strcmp(state, "failure"))
		function = "Connect";
	else
		function = "Disconnect";

	g_free(state);

	g_dbus_proxy_call(serv->proxy, function, NULL,
	                  G_DBUS_CALL_FLAGS_NONE, -1, NULL,
	                  service_toggle_connection_cb, serv->proxy);
}

GVariant *service_get_property(struct service *serv, const char *key,
                               const char *subkey)
{
	GVariant *variant;

	variant = hash_table_get_dual_key(serv->properties, key, subkey);
	if(!variant)
		return NULL;
	g_variant_ref(variant);
	return variant;
}

gchar *service_get_property_string(struct service *serv, const char *key,
                                   const char *subkey)
{
	if(!serv || !key)
		return g_strdup("");
	GVariant *prop = service_get_property(serv, key, subkey);
	gchar *str = variant_to_str(prop);
	gchar *out;
	if(!strcmp(key, "State"))
		out = g_strdup(status_localized(str));
	else
		out = g_strdup(str);
	if(prop)
		g_variant_unref(prop);
	g_free(str);
	return out;
}

gboolean service_get_property_boolean(struct service *serv, const char *key,
                                      const char *subkey)
{
	if(!serv || !key)
		return FALSE;
	GVariant *prop = service_get_property(serv, key, subkey);
	gboolean out = variant_to_bool(prop);
	if(prop)
		g_variant_unref(prop);
	return out;
}

int service_get_property_int(struct service *serv, const char *key,
                             const char *subkey)
{
	if(!serv || !key)
		return 0;
	GVariant *prop = service_get_property(serv, key, subkey);
	int out = variant_to_int(prop);
	if(prop)
		g_variant_unref(prop);
	return out;
}

void service_set_property(struct service *serv, const char *key,
                          GVariant *value)
{
	GVariant *ret;
	GVariant *parameters;
	GError *error = NULL;

	GVariant *old;
	gboolean equal = FALSE;
	if(strcmp(g_variant_get_type_string(value), "a{sv}")) {
		old = service_get_property(serv, key, NULL);
		equal = !old || g_variant_equal(old, value);
		if(old)
			g_variant_unref(old);
	} else {
		gchar *subkey;
		GVariant *svalue;
		GVariantIter *iter = g_variant_iter_new(value);
		while(g_variant_iter_loop(iter, "{sv}", &subkey, &svalue)) {
			old = service_get_property(serv, key, subkey);
			equal = equal || !old || g_variant_equal(old, svalue);
			if(old)
				g_variant_unref(old);
		}
		g_variant_iter_free(iter);
	}
	if(equal)
		return;

	parameters = g_variant_new("(sv)", key, value);
	ret = g_dbus_proxy_call_sync(serv->proxy, "SetProperty", parameters,
	                             G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if(error) {
		g_warning("failed to set property %s: %s", key, error->message);
		g_error_free(error);
		return;
	}
	g_variant_unref(ret);
}

void service_set_properties(struct service *serv, GVariant *properties)
{
	GVariantIter *iter;
	gchar *key;
	GVariant *value;

	iter = g_variant_iter_new(properties);
	while(g_variant_iter_loop(iter, "{sv}", &key, &value))
		service_set_property(serv, key, value);
	g_variant_iter_free(iter);
}

const gchar *service_status_localized(struct service *serv)
{
	const gchar *out;
	gchar *state;

	state = service_get_property_string(serv, "State", NULL);
	if(!strcmp(state, "idle"))
		out = _("Idle");
	else if(!strcmp(state, "failure"))
		out = _("Failure");
	else if(!strcmp(state, "association"))
		out = _("Association");
	else if(!strcmp(state, "configuration"))
		out = _("Configuration");
	else if(!strcmp(state, "ready"))
		out = _("Ready");
	else if(!strcmp(state, "disconnected"))
		out = _("Disconnected");
	else if(!strcmp(state, "online"))
		out = _("Online");
	else
		out = _("Error");
	g_free(state);
	return out;
}
