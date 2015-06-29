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

struct service *service_create(GDBusProxy *proxy, const gchar *path,
		GVariant *properties) {
	struct service *serv;
	GVariantIter *iter;
	gchar *key;
	GVariant *value;
	
	serv = g_malloc(sizeof(*serv));

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

	serv->contents = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(serv->item), serv->contents);
	gtk_container_add(GTK_CONTAINER(serv->contents), gtk_label_new("service"));
	gtk_widget_show_all(serv->item);

	g_signal_connect(proxy, "g-signal", G_CALLBACK(service_proxy_signal),
			serv);

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
}

void service_free(struct service *serv) {
	if(!serv)
		return;
	g_object_unref(serv->proxy);
	g_object_unref(serv->item);
	g_free(serv->path);
	g_hash_table_unref(serv->properties);
	gtk_widget_destroy(serv->item);
	g_free(serv);
}
