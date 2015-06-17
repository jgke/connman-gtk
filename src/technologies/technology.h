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

#ifndef _CONNMAN_GTK_TECHNOLOGY_H
#define _CONNMAN_GTK_TECHNOLOGY_H

#include <gio/gio.h>
#include <gtk/gtk.h>

enum technology_type {
	TECHNOLOGY_TYPE_UNKNOWN,
	TECHNOLOGY_TYPE_ETHERNET,
	TECHNOLOGY_TYPE_WIRELESS,
	TECHNOLOGY_TYPE_BLUETOOTH,
	TECHNOLOGY_TYPE_CELLULAR,
	TECHNOLOGY_TYPE_P2P,
	TECHNOLOGY_TYPE_VPN,
	TECHNOLOGY_TYPE_COUNT
};

struct technology_list_item {
	GtkWidget *item;

	GtkWidget *icon;
	GtkWidget *label;
};

struct technology_settings {
	GDBusProxy *proxy;

	GtkWidget *box;

	GtkWidget *header;
	GtkWidget *icon;

	GtkWidget *label;
	GtkWidget *title;
	GtkWidget *status;

	GtkWidget *contents;
};

struct technology {
	struct technology_list_item *list_item;
	struct technology_settings *settings;
	gint id;
	enum technology_type type;
};

struct technology *create_technology(GDBusProxy *proxy, GVariant *path,
		GVariant *properties);
void free_technology(struct technology *item);
void technology_set_id(struct technology *item, gint id);
enum technology_type technology_type_from_string(const gchar *str);


#endif /* _CONNMAN_GTK_TECHNOLOGY_H */
