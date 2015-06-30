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

#include <locale.h>
#include <string.h>

#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "connection.h"
#include "style.h"
#include "technology.h"
#include "connections/bluetooth.h"
#include "connections/cellular.h"
#include "connections/ethernet.h"
#include "connections/p2p.h"
#include "connections/vpn.h"
#include "connections/wireless.h"

static struct {
	void (*init)(struct technology *tech, GVariant *properties, GDBusProxy *proxy);
	struct technology *(*create)(void);
	void (*free)(struct technology *tech);
	void (*property_changed)(struct technology *tech, const gchar *name);
	void (*add_service)(struct technology *tech, struct service *serv);
	void (*update_service)(struct technology *tech, struct service *serv,
			GVariant *properties);
	void (*remove_service)(struct technology *tech, const gchar *path);
} functions[CONNECTION_TYPE_COUNT] = {
	{},
	{technology_ethernet_init, technology_ethernet_create, technology_ethernet_free},
	{technology_wireless_init},
	{technology_bluetooth_init},
	{technology_cellular_init},
	{},
	{technology_vpn_init}
};

struct technology_list_item *create_base_technology_list_item(struct technology *tech,
		const gchar *name) {
	GtkWidget *grid;
	struct technology_list_item *item;

	item = g_malloc(sizeof(*item));

	item->technology = tech;

	grid = gtk_grid_new();
	STYLE_ADD_MARGIN(grid, MARGIN_SMALL);

	item->item = gtk_list_box_row_new();
	g_object_ref(item->item);
	g_object_set_data(G_OBJECT(item->item), "technology", tech);

	item->icon = gtk_image_new_from_icon_name("network-transmit-symbolic",
			GTK_ICON_SIZE_LARGE_TOOLBAR);
	g_object_ref(item->icon);

	item->label = gtk_label_new(name);
	g_object_ref(item->label);

	gtk_container_add(GTK_CONTAINER(grid), item->icon);
	gtk_container_add(GTK_CONTAINER(grid), item->label);
	gtk_container_add(GTK_CONTAINER(item->item), grid);

	gtk_widget_show_all(item->item);
	return item;
}

void free_base_technology_list_item(struct technology_list_item *item) {
	if(!item)
		return;
	g_object_unref(item->icon);
	g_object_unref(item->label);

	g_object_unref(item->item);

	gtk_widget_destroy(item->item);

	g_free(item);
}

GtkWidget *create_technology_settings_title(const char *title) {
	GtkWidget *label = gtk_label_new(title);
	STYLE_ADD_CONTEXT(label);
	gtk_style_context_add_class(gtk_widget_get_style_context(label),
			"cm-header-title");
	return label;
}

gboolean technology_toggle_power(GtkSwitch *widget, GParamSpec *pspec,
		gpointer user_data) {
	struct technology_settings *item = user_data;
	GVariant *ret;
	GError *error = NULL;
	gboolean state = gtk_switch_get_active(widget);

	ret = g_dbus_proxy_call_sync(item->proxy, "SetProperty",
			g_variant_new("(sv)", "Powered", g_variant_new("b", state)),
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if(error) {
		g_warning("failed to toggle technology state: %s", error->message);
		g_error_free(error);
		return TRUE;
	}
	g_variant_unref(ret);
	return FALSE;
}

void technology_update_status(struct technology_settings *item) {
	gboolean connected = g_variant_get_boolean(g_hash_table_lookup(
				item->properties, "Connected"));
	gboolean powered = g_variant_get_boolean(g_hash_table_lookup(
				item->properties, "Powered"));
	if(connected)
		gtk_label_set_text(GTK_LABEL(item->status),
				_("Connected"));
	else {
		if(powered)
			gtk_label_set_text(GTK_LABEL(item->status),
					_("Not connected"));
		else
			gtk_label_set_text(GTK_LABEL(item->status),
					_("Disabled"));
	}
}

void technology_update_power(struct technology_settings *item) {
	gboolean powered = g_variant_get_boolean(g_hash_table_lookup(
				item->properties, "Powered"));

	g_signal_handler_block(G_OBJECT(item->power_switch),
			item->powersig);
	gtk_switch_set_active(GTK_SWITCH(item->power_switch),
			powered);
	g_signal_handler_unblock(G_OBJECT(item->power_switch),
			item->powersig);
	technology_update_status(item);
}

void technology_proxy_signal(GDBusProxy *proxy, gchar *sender, gchar *signal,
		GVariant *parameters, gpointer user_data) {
	struct technology_settings *item = user_data;
	if(!strcmp(signal, "PropertyChanged")) {
		GVariant *name_v, *value_v, *value;
		gchar *name;
		name_v = g_variant_get_child_value(parameters, 0);
		value_v = g_variant_get_child_value(parameters, 1);
		value = g_variant_get_child_value(value_v, 0);
		name = g_variant_dup_string(name_v, NULL);
		g_hash_table_replace(item->properties, name, value);
		if(!strcmp(name, "Powered")) {
			technology_update_power(item);
		}
		else if(!strcmp(name, "Connected")) {
			technology_update_status(item);
		}
		technology_property_changed(item->technology, name);

		g_variant_unref(name_v);
		g_variant_unref(value_v);
	}
}

struct technology_settings *create_base_technology_settings(struct technology *tech,
		GVariant *properties, GDBusProxy *proxy) {
	struct technology_settings *item = g_malloc(sizeof(*item));

	GVariantIter *iter;
	gchar *key;
	GVariant *value;
	const gchar *name;
	gboolean powered;
	gboolean connected;
	GtkWidget *powerbox, *frame, *scrolled_window;

	item->technology = tech;
	item->properties = g_hash_table_new_full(g_str_hash, g_str_equal,
			g_free, (GDestroyNotify)g_variant_unref);

	iter = g_variant_iter_new(properties);
	while(g_variant_iter_loop(iter, "{sv}", &key, &value)) {
		gchar *hkey = g_strdup(key);
		g_variant_ref(value);
		g_hash_table_insert(item->properties, hkey, value);
	}
	g_variant_iter_free(iter);

	powered = g_variant_get_boolean(g_hash_table_lookup(item->properties,
				"Powered"));
	connected = g_variant_get_boolean(g_hash_table_lookup(item->properties,
				"Connected"));
	name = g_variant_get_string(g_hash_table_lookup(item->properties,
				"Name"), NULL);

	item->proxy = proxy;
	g_signal_connect(proxy, "g-signal", G_CALLBACK(technology_proxy_signal),
			item);

	item->grid = gtk_grid_new();
	g_object_ref(item->grid);
	gtk_widget_set_margin_start(item->grid, MARGIN_LARGE);
	gtk_widget_set_margin_end(item->grid, MARGIN_LARGE);
	g_object_set_data(G_OBJECT(item->grid), "technology", tech);

	item->icon = gtk_image_new_from_icon_name("preferences-system-network",
			GTK_ICON_SIZE_DIALOG);
	g_object_ref(item->icon);
	gtk_widget_set_halign(item->icon, GTK_ALIGN_START);

	item->title = create_technology_settings_title(name);
	g_object_ref(item->title);
	gtk_widget_set_margin_start(item->title, MARGIN_MEDIUM);
	gtk_widget_set_margin_end(item->title, MARGIN_MEDIUM);
	gtk_widget_set_halign(item->title, GTK_ALIGN_START);
	gtk_widget_set_hexpand(item->title, TRUE);

	if(connected)
		item->status = gtk_label_new(_("Connected"));
	else
		item->status = gtk_label_new(_("Not connected"));
	g_object_ref(item->status);
	gtk_widget_set_margin_start(item->status, MARGIN_MEDIUM);
	gtk_widget_set_margin_end(item->status, MARGIN_MEDIUM);
	gtk_widget_set_halign(item->status, GTK_ALIGN_START);
	gtk_widget_set_hexpand(item->status, TRUE);

	item->power_switch = gtk_switch_new();
	powerbox = gtk_grid_new();
	g_object_ref(item->power_switch);
	gtk_switch_set_active(GTK_SWITCH(item->power_switch), powered);
	gtk_widget_set_halign(item->power_switch, GTK_ALIGN_END);
	gtk_widget_set_valign(item->power_switch, GTK_ALIGN_START);
	gtk_widget_set_valign(powerbox, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(powerbox, GTK_ALIGN_END);
	gtk_widget_set_vexpand(powerbox, FALSE);
	item->powersig = g_signal_connect(item->power_switch, "notify::active",
			G_CALLBACK(technology_toggle_power), item);
	gtk_container_add(GTK_CONTAINER(powerbox), item->power_switch);

	item->contents = gtk_grid_new();
	g_object_ref(item->contents);
	gtk_widget_set_margin_top(item->contents, MARGIN_LARGE);
	gtk_widget_set_hexpand(item->contents, TRUE);
	gtk_widget_set_vexpand(item->contents, TRUE);

	frame = gtk_frame_new(NULL);
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_vexpand(scrolled_window, TRUE);
	item->services = gtk_list_box_new();
	g_object_ref(item->services);
	gtk_container_add(GTK_CONTAINER(scrolled_window), item->services);
	gtk_container_add(GTK_CONTAINER(frame), scrolled_window);
	gtk_grid_attach(GTK_GRID(item->contents), frame, 0, 0, 1, 1);

	gtk_grid_attach(GTK_GRID(item->grid), item->icon,	0, 0, 1, 2);
	gtk_grid_attach(GTK_GRID(item->grid), item->title,	1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(item->grid), item->status,	1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(item->grid), powerbox,		2, 0, 1, 2);
	gtk_grid_attach(GTK_GRID(item->grid), item->contents,	0, 2, 3, 1);

	gtk_widget_show_all(item->grid);
	return item;
}

void free_base_technology_settings(struct technology_settings *item) {
	if(!item)
		return;

	g_object_unref(item->icon);
	g_object_unref(item->title);
	g_object_unref(item->status);
	g_object_unref(item->power_switch);

	g_object_unref(item->contents);
	g_object_unref(item->services);

	g_object_unref(item->grid);
	gtk_widget_destroy(item->grid);

	g_object_unref(item->proxy);
	g_hash_table_unref(item->properties);

	g_free(item);
}

void technology_property_changed(struct technology *item, const gchar *key) {
	if(functions[item->type].property_changed)
		functions[item->type].property_changed(item, key);
}

void technology_add_service(struct technology *item, struct service *serv) {
	if(functions[item->type].add_service)
		functions[item->type].add_service(item, serv);
	gtk_container_add(GTK_CONTAINER(item->settings->services), serv->item);
	g_hash_table_insert(item->services, g_strdup(serv->path), serv);
}

void technology_update_service(struct technology *item, struct service *serv,
		GVariant *properties) {
	if(functions[item->type].update_service)
		functions[item->type].update_service(item, serv, properties);
	service_update(serv, properties);
}

void technology_remove_service(struct technology *item, const gchar *path) {
	if(functions[item->type].remove_service)
		functions[item->type].remove_service(item, path);
	g_hash_table_remove(item->services, path);
}

void technology_free(struct technology *item) {
	if(!item)
		return;
	if(functions[item->type].free)
		functions[item->type].free(item);
	free_base_technology_list_item(item->list_item);
	free_base_technology_settings(item->settings);
	g_hash_table_unref(item->services);
	g_free(item);
}

void technology_init(struct technology *tech, GVariant *properties_v,
		GDBusProxy *proxy) {
	GVariant *name_v;
	const gchar *name;
	GVariant *type_v;
	const gchar *type;
	GVariantDict *properties;

	properties = g_variant_dict_new(properties_v);
	name_v = g_variant_dict_lookup_value(properties, "Name", NULL);
	name = g_variant_get_string(name_v, NULL);
	type_v = g_variant_dict_lookup_value(properties, "Type", NULL);
	type = g_variant_get_string(type_v, NULL);
	g_variant_dict_unref(properties);

	tech->list_item = create_base_technology_list_item(tech, name);
	tech->settings = create_base_technology_settings(tech, properties_v,
			proxy);
	tech->type = connection_type_from_string(type);
	g_object_set_data(G_OBJECT(tech->list_item->item), "technology-type",
			&tech->type);

	g_variant_unref(name_v);
	g_variant_unref(type_v);

	tech->services = g_hash_table_new_full(g_str_hash, g_str_equal,
			g_free, NULL);
}

struct technology *technology_create(GDBusProxy *proxy, GVariant *path,
		GVariant *properties) {
	struct technology *item;
	GVariantDict *properties_d;
	GVariant *type_v;
	enum connection_type type;

	properties_d = g_variant_dict_new(properties);
	type_v = g_variant_dict_lookup_value(properties_d, "Type", NULL);
	type = connection_type_from_string(g_variant_get_string(type_v, NULL));
	g_variant_unref(type_v);
	g_variant_dict_unref(properties_d);

	if(functions[type].create)
		item = functions[type].create();
	else
		item = g_malloc(sizeof(*item));
	item->type = type;

	technology_init(item, properties, proxy);
	if(functions[type].init)
		functions[type].init(item, properties, proxy);

	return item;
}
