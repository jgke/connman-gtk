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

#include <gtk/gtk.h>

#include "technology.h"
#include "ethernet.h"
#include "style.h"

struct technology_list_item *create_base_technology_list_item(const gchar *name) {
	GtkWidget *box;
	struct technology_list_item *item;

	item = g_malloc(sizeof(*item));

	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	STYLE_ADD_MARGIN(box, 5);

	item->item = gtk_list_box_row_new();
	g_object_ref(item->item);

	item->icon = gtk_image_new_from_icon_name("network-transmit-symbolic",
			GTK_ICON_SIZE_LARGE_TOOLBAR);
	g_object_ref(item->icon);

	item->label = gtk_label_new(name);
	g_object_ref(item->label);

	gtk_container_add(GTK_CONTAINER(box), item->icon);
	gtk_container_add(GTK_CONTAINER(box), item->label);
	gtk_container_add(GTK_CONTAINER(item->item), box);

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

struct technology_settings *create_base_technology_settings(const gchar *name) {
	struct technology_settings *item = g_malloc(sizeof(*item));

	item->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	g_object_ref(item->box);
	STYLE_ADD_MARGIN(item->box, 5);

	item->header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	g_object_ref(item->header);

	item->icon = gtk_image_new_from_icon_name("preferences-system-network",
			GTK_ICON_SIZE_DIALOG);
	g_object_ref(item->icon);

	item->label = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	g_object_ref(item->label);

	item->title = create_technology_settings_title(name);
	g_object_ref(item->title);
	gtk_widget_set_halign(item->title, GTK_ALIGN_START);

	item->status = gtk_label_new("Status");
	g_object_ref(item->status);
	gtk_widget_set_halign(item->status, GTK_ALIGN_START);

	item->contents = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	g_object_ref(item->contents);

	gtk_container_add(GTK_CONTAINER(item->label), item->title);
	gtk_container_add(GTK_CONTAINER(item->label), item->status);
	gtk_container_add(GTK_CONTAINER(item->header), item->icon);
	gtk_container_add(GTK_CONTAINER(item->header), item->label);
	gtk_container_add(GTK_CONTAINER(item->box), item->header);
	gtk_container_add(GTK_CONTAINER(item->box), item->contents);

	gtk_widget_show_all(item->box);
	return item;
}

void free_base_technology_settings(struct technology_settings *item) {
	if(!item)
		return;
	g_object_unref(item->box);
	g_object_unref(item->header);
	g_object_unref(item->icon);
	g_object_unref(item->label);
	g_object_unref(item->title);
	g_object_unref(item->status);
	g_object_unref(item->contents);
	g_free(item);
}

void init_technology(struct technology *tech, GVariantDict *properties) {
	GVariant *name_v;
	const gchar *name;
	GVariant *type_v;
	const gchar *type;

	name_v = g_variant_dict_lookup_value(properties, "Name", NULL);
	name = g_variant_get_string(name_v, NULL);
	type_v = g_variant_dict_lookup_value(properties, "Type", NULL);
	type = g_variant_get_string(type_v, NULL);

	tech->list_item = create_base_technology_list_item(name);
	tech->settings = create_base_technology_settings(name);
	tech->type = technology_type_from_string(type);

	g_variant_unref(name_v);
	g_variant_unref(type_v);

	switch(tech->type) {
	case TECHNOLOGY_TYPE_ETHERNET:
		technology_ethernet_init(tech, properties);
		break;
	case TECHNOLOGY_TYPE_WIRELESS:
	case TECHNOLOGY_TYPE_BLUETOOTH:
	case TECHNOLOGY_TYPE_CELLULAR:
	case TECHNOLOGY_TYPE_P2P:
	case TECHNOLOGY_TYPE_VPN:
	default:
		break;
	}
}

struct technology *create_technology(GVariant *path, GVariant *properties_v) {
	struct technology *item;
	GVariantDict *properties;

	properties = g_variant_dict_new(properties_v);

	item = g_malloc(sizeof(*item));
	init_technology(item, properties);

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
	if(!strcmp(str, "p2p"))
		return TECHNOLOGY_TYPE_CELLULAR;
	if(!strcmp(str, "cellular"))
		return TECHNOLOGY_TYPE_P2P;
	return TECHNOLOGY_TYPE_UNKNOWN;
}
