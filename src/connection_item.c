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

#include "connection_item.h"

struct connection_list_item *create_base_connection_list_item() {
	struct connection_list_item *item = g_malloc(sizeof(*item));
	item->item = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	item->icon = gtk_image_new_from_icon_name("network-wired-symbolic",
			GTK_ICON_SIZE_LARGE_TOOLBAR);
	item->label = gtk_label_new("Connection item");
	gtk_container_add(GTK_CONTAINER(item->item), item->icon);
	gtk_container_add(GTK_CONTAINER(item->item), item->label);
	return item;
}

GtkWidget *create_connection_item_title(const char *title) {
	GtkWidget *label;
	const char *format = "<b>%s</b>";
	char *markup;

	label = gtk_label_new(NULL);
	markup = g_markup_printf_escaped(format, title);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);
	return label;
}

struct connection_settings_item *create_base_connection_settings_item() {
	struct connection_settings_item *item = g_malloc(sizeof(*item));
	item->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	item->header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	item->icon = gtk_image_new_from_icon_name("network-wired",
			GTK_ICON_SIZE_DIALOG);
	item->label = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	item->title = create_connection_item_title("Connection item");
	item->status = gtk_label_new("Status");
	item->contents = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

	gtk_container_add(GTK_CONTAINER(item->label), item->title);
	gtk_container_add(GTK_CONTAINER(item->label), item->status);
	gtk_container_add(GTK_CONTAINER(item->header), item->icon);
	gtk_container_add(GTK_CONTAINER(item->header), item->label);
	gtk_container_add(GTK_CONTAINER(item->box), item->header);
	gtk_container_add(GTK_CONTAINER(item->box), item->contents);
	return item;
}

struct connection_item create_connection_item() {
	struct connection_item item;
	item.list_item = create_base_connection_list_item();
	item.settings = create_base_connection_settings_item();
	return item;
}
