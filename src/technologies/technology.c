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
#include <gtk/gtk.h>

#include "style.h"
#include "technologies/bluetooth.h"
#include "technologies/cellular.h"
#include "technologies/ethernet.h"
#include "technologies/p2p.h"
#include "technologies/technology.h"
#include "technologies/vpn.h"
#include "technologies/wireless.h"

struct technology_list_item *create_base_technology_list_item(const gchar *name) {
	GtkWidget *grid;
	struct technology_list_item *item;

	item = g_malloc(sizeof(*item));

	grid = gtk_grid_new();
	STYLE_ADD_MARGIN(grid, MARGIN_SMALL);

	item->item = gtk_list_box_row_new();
	g_object_ref(item->item);

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
	g_object_unref(item->item);
	g_object_unref(item->icon);
	g_object_unref(item->label);
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

void technology_proxy_signal(GDBusProxy *proxy, gchar *sender, gchar *signal,
		GVariant *parameters, gpointer user_data) {
	struct technology_settings *item = user_data;
	if(!strcmp(signal, "PropertyChanged")) {
		GVariant *name_v, *value_v;
		const gchar *name;
		name_v = g_variant_get_child_value(parameters, 0);
		value_v = g_variant_get_child_value(parameters, 1);
		name = g_variant_get_string(name_v, NULL);
		if(!strcmp(name, "Powered")) {
			gboolean state;
			GVariant *state_v = g_variant_get_child_value(value_v, 0);
			state = g_variant_get_boolean(state_v);
			g_variant_unref(state_v);

			gtk_switch_set_state(GTK_SWITCH(item->power_switch),
					state);
		}

		g_variant_unref(name_v);
		g_variant_unref(value_v);
	}
}

struct technology_settings *create_base_technology_settings(GVariantDict *properties,
		GDBusProxy *proxy) {
	struct technology_settings *item = g_malloc(sizeof(*item));

	GVariant *variant;
	const gchar *name;
	gboolean powered;

	variant = g_variant_dict_lookup_value(properties, "Powered", NULL);
	powered = g_variant_get_boolean(variant);
	g_variant_unref(variant);
	variant = g_variant_dict_lookup_value(properties, "Name", NULL);
	name = g_variant_get_string(variant, NULL);

	item->proxy = proxy;
	g_signal_connect(proxy, "g-signal", G_CALLBACK(technology_proxy_signal),
			item);

	item->grid = gtk_grid_new();
	g_object_ref(item->grid);
	STYLE_ADD_MARGIN(item->grid, MARGIN_LARGE);
	gtk_widget_set_margin_top(item->grid, 0);

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

	item->status = gtk_label_new("Status");
	g_object_ref(item->status);
	gtk_widget_set_margin_start(item->status, MARGIN_MEDIUM);
	gtk_widget_set_margin_end(item->status, MARGIN_MEDIUM);
	gtk_widget_set_halign(item->status, GTK_ALIGN_START);
	gtk_widget_set_hexpand(item->status, TRUE);

	item->power_switch = gtk_switch_new();
	g_object_ref(item->power_switch);
	gtk_switch_set_active(GTK_SWITCH(item->power_switch), powered);
	gtk_widget_set_valign(item->power_switch, GTK_ALIGN_START);
	gtk_widget_set_halign(item->power_switch, GTK_ALIGN_END);
	g_signal_connect(item->power_switch, "notify::active",
			G_CALLBACK(technology_toggle_power), item);

	item->contents = gtk_grid_new();
	g_object_ref(item->contents);
	STYLE_ADD_MARGIN(item->contents, MARGIN_LARGE);
	gtk_widget_set_hexpand(item->contents, TRUE);
	gtk_widget_set_vexpand(item->contents, TRUE);

	gtk_grid_attach(GTK_GRID(item->grid), item->icon,	  0, 0, 1, 2);
	gtk_grid_attach(GTK_GRID(item->grid), item->title,	  1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(item->grid), item->status,	  1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(item->grid), item->power_switch, 2, 0, 1, 2);
	gtk_grid_attach(GTK_GRID(item->grid), item->contents,	  0, 2, 3, 1);

	gtk_widget_show_all(item->grid);
	g_variant_unref(variant);
	return item;
}

void free_base_technology_settings(struct technology_settings *item) {
	if(!item)
		return;
	g_object_unref(item->grid);

	g_object_unref(item->header);
	g_object_unref(item->icon);
	g_object_unref(item->power_switch);

	g_object_unref(item->label);
	g_object_unref(item->title);
	g_object_unref(item->status);

	g_object_unref(item->contents);

	g_free(item);
}

void init_technology(struct technology *tech, GVariantDict *properties,
		GDBusProxy *proxy) {
	GVariant *name_v;
	const gchar *name;
	GVariant *type_v;
	const gchar *type;

	name_v = g_variant_dict_lookup_value(properties, "Name", NULL);
	name = g_variant_get_string(name_v, NULL);
	type_v = g_variant_dict_lookup_value(properties, "Type", NULL);
	type = g_variant_get_string(type_v, NULL);

	tech->list_item = create_base_technology_list_item(name);
	tech->settings = create_base_technology_settings(properties, proxy);
	tech->type = technology_type_from_string(type);
	g_object_set_data(G_OBJECT(tech->list_item->item), "technology-type",
			&tech->type);

	g_variant_unref(name_v);
	g_variant_unref(type_v);

	switch(tech->type) {
	case TECHNOLOGY_TYPE_ETHERNET:
		technology_ethernet_init(tech, properties);
		break;
	case TECHNOLOGY_TYPE_WIRELESS:
		technology_wireless_init(tech, properties);
		break;
	case TECHNOLOGY_TYPE_BLUETOOTH:
		technology_bluetooth_init(tech, properties);
		break;
	case TECHNOLOGY_TYPE_CELLULAR:
		technology_cellular_init(tech, properties);
		break;
	case TECHNOLOGY_TYPE_P2P:
		technology_p2p_init(tech, properties);
		break;
	case TECHNOLOGY_TYPE_VPN:
		technology_vpn_init(tech, properties);
		break;
	default:
		break;
	}
}

struct technology *create_technology(GDBusProxy *proxy, GVariant *path,
		GVariant *properties_v) {
	struct technology *item;
	GVariantDict *properties;

	properties = g_variant_dict_new(properties_v);

	item = g_malloc(sizeof(*item));
	init_technology(item, properties, proxy);

	g_variant_dict_unref(properties);
	return item;
}

void technology_set_id(struct technology *item, gint id) {
	item->id = id;
	g_object_set_data(G_OBJECT(item->list_item->item), "technology-id",
			&item->id);
}

void free_technology(struct technology *item) {
	if(!item)
		return;
	free_base_technology_list_item(item->list_item);
	free_base_technology_settings(item->settings);
}

enum technology_type technology_type_from_string(const gchar *str) {
	if(!strcmp(str, "ethernet"))
		return TECHNOLOGY_TYPE_ETHERNET;
	if(!strcmp(str, "wifi"))
		return TECHNOLOGY_TYPE_WIRELESS;
	if(!strcmp(str, "bluetooth"))
		return TECHNOLOGY_TYPE_BLUETOOTH;
	if(!strcmp(str, "cellular"))
		return TECHNOLOGY_TYPE_CELLULAR;
	if(!strcmp(str, "p2p"))
		return TECHNOLOGY_TYPE_P2P;
	return TECHNOLOGY_TYPE_UNKNOWN;
}
