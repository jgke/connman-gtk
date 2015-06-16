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
#include <stdlib.h>

#include <gtk/gtk.h>

#include "technology.h"
#include "style.h"

struct technology_list_item *create_base_technology_list_item(gchar *name) {
	struct technology_list_item *item = g_malloc(sizeof(*item));
	item->item = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	g_object_ref(item->item);
	item->icon = gtk_image_new_from_icon_name("network-wired-symbolic",
			GTK_ICON_SIZE_LARGE_TOOLBAR);
	g_object_ref(item->icon);
	item->label = gtk_label_new(name);
	g_object_ref(item->label);
	gtk_container_add(GTK_CONTAINER(item->item), item->icon);
	gtk_container_add(GTK_CONTAINER(item->item), item->label);
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
	gtk_style_context_add_provider(gtk_widget_get_style_context(label),
			GTK_STYLE_PROVIDER(css_provider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	gtk_style_context_add_class(gtk_widget_get_style_context(label),
			"cm-header-title");
	return label;
}

struct technology_settings *create_base_technology_settings(gchar *name) {
	struct technology_settings *item = g_malloc(sizeof(*item));
	item->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	g_object_ref(item->box);
	item->header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	g_object_ref(item->header);
	item->icon = gtk_image_new_from_icon_name("network-wired",
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

struct technology *create_technology(gchar *name) {
	struct technology *item;
	item = g_malloc(sizeof(*item));
	item->list_item = create_base_technology_list_item(name);
	item->settings = create_base_technology_settings(name);
	return item;
}

void free_technology(struct technology *item) {
	if(!item)
		return;
	free_base_technology_list_item(item->list_item);
	free_base_technology_settings(item->settings);
}
