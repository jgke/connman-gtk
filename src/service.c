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

#include "service.h"
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
	{service_ethernet_init, service_ethernet_create, service_ethernet_free,
		service_ethernet_update},
	{service_wireless_init}
};

void service_proxy_signal(GDBusProxy *proxy, gchar *sender, gchar *signal,
		GVariant *parameters, gpointer user_data) {
	struct service *serv = user_data;
	if(!strcmp(signal, "PropertyChanged")) {
		GVariant *name_v, *value_v, *value;
		gchar *name;

		name_v = g_variant_get_child_value(parameters, 0);
		value_v = g_variant_get_child_value(parameters, 1);

		name = g_variant_dup_string(name_v, NULL);
		value = g_variant_get_child_value(value_v, 0);

		g_hash_table_replace(serv->properties, name, value);

		g_variant_unref(name_v);
		g_variant_unref(value_v);
	}
}

void service_init(struct service *serv, GDBusProxy *proxy, const gchar *path,
		GVariant *properties) {
	GVariantIter *iter;
	gchar *key;
	GVariant *value;

	serv->proxy = proxy;
	serv->path = g_strdup(path);
	serv->properties = g_hash_table_new_full(g_str_hash, g_str_equal,
			g_free, (GDestroyNotify)g_variant_unref);

	iter = g_variant_iter_new(properties);
	while(g_variant_iter_loop(iter, "{sv}", &key, &value)) {
		gchar *hkey = g_strdup(key);
		g_variant_ref(value);
		g_hash_table_insert(serv->properties, hkey, value);
	}
	g_variant_iter_free(iter);

	serv->item = gtk_list_box_row_new();
	g_object_ref(serv->item);
	gtk_widget_set_hexpand(serv->item, TRUE);

	serv->contents = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(serv->item), serv->contents);
	gtk_widget_show_all(serv->item);

	g_signal_connect(proxy, "g-signal", G_CALLBACK(service_proxy_signal),
			serv);
}

struct service *service_create(GDBusProxy *proxy, const gchar *path,
		GVariant *properties) {
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
	service_init(serv, proxy, path, properties);
	if(functions[type].init)
		functions[type].init(serv, proxy, path, properties);

	return serv;
}

void service_update(struct service *serv, GVariant *properties) {
	GVariantIter *iter;
	gchar *key;
	GVariant *value;

	iter = g_variant_iter_new(properties);
	while(g_variant_iter_loop(iter, "{sv}", &key, &value)) {
		gchar *hkey = g_strdup(key);
		g_variant_ref(value);
		g_hash_table_replace(serv->properties, hkey, value);
	}
	g_variant_iter_free(iter);
	if(functions[serv->type].update)
		functions[serv->type].update(serv);
}

void service_free(struct service *serv) {
	if(!serv)
		return;
	g_object_unref(serv->proxy);
	g_object_unref(serv->item);
	g_free(serv->path);
	g_hash_table_unref(serv->properties);
	gtk_widget_destroy(serv->item);
	if(functions[serv->type].free)
		functions[serv->type].free(serv);
	else
		g_free(serv);
}
