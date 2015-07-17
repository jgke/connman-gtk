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
#include <glib.h>
#include <gtk/gtk.h>

#include "connection.h"
#include "service.h"

struct technology;

struct technology_list_item {
	struct technology *technology;

	GtkWidget *item;

	GtkWidget *icon;
	GtkWidget *label;
};

struct technology_settings {
	struct technology *technology;
	struct service *selected;
	GHashTable *properties;

	GDBusProxy *proxy;

	GtkWidget *grid;

	GtkWidget *title;
	GtkWidget *status;
	GtkWidget *icon;
	GtkWidget *power_switch;
	gint powersig;

	GtkWidget *contents;
	GtkWidget *services;

	GtkWidget *buttons;
	GtkWidget *tethering;
	GtkWidget *filler;
	GtkWidget *connect_button;
};

struct technology {
	struct technology_list_item *list_item;
	struct technology_settings *settings;
	GHashTable *services;
	gchar *path;
	enum connection_type type;
};

struct technology_list_item *technology_create_item(struct technology *tech,
                const gchar *name);
struct technology_settings *technology_create_settings(struct technology *tech,
                GVariant *properties,
                GDBusProxy *proxy);
struct technology *technology_create(GDBusProxy *proxy, const gchar *path,
                                     GVariant *properties);
void technology_init(struct technology *tech, GVariant *properties_v,
                     GDBusProxy *proxy);
void technology_property_changed(struct technology *item, const gchar *key);
void technology_services_updated(struct technology *item);
void technology_add_service(struct technology *item, struct service *serv);
void technology_update_service(struct technology *item, struct service *serv,
                               GVariant *properties);
void technology_remove_service(struct technology *item, const gchar *path);
void technology_set_property(struct technology *item, const gchar *key,
                             GVariant *value);
void technology_free(struct technology *item);

#endif /* _CONNMAN_GTK_TECHNOLOGY_H */
