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

#ifndef _CONNMAN_GTK_SERVICE_H
#define _CONNMAN_GTK_SERVICE_H

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib.h>

#include "connection.h"
#include "technology.h"
#include "settings.h"
#include "util.h"

struct service {
	enum connection_type type;
	struct technology *tech;
	GDBusProxy *proxy;
	gchar *path;
	DualHashTable *properties;
	GtkWidget *item;
	GtkWidget *header;
	GtkWidget *title;
	GtkWidget *contents;
	GtkWidget *settings_button;
	struct settings *sett;
};

#define SIGNAL_TO_ICON(type, strength) \
        ((strength) > 80 ? ("network-" type "-signal-excellent-symbolic") : \
        (strength) > 55 ? ("network-" type "-signal-good-symbolic") : \
        (strength) > 30 ? ("network-" type "-signal-ok-symbolic") : \
        (strength) > 5 ? ("network-" type "-signal-weak-symbolic") : \
        ("network-" type "-signal-none-symbolic"))

struct service *service_create(struct technology *tech, GDBusProxy *proxy,
                               const gchar *path, GVariant *properties);
void service_init(struct service *serv, GDBusProxy *proxy, const gchar *path,
                  GVariant *properties);
void service_update(struct service *serv, GVariant *properties);
void service_free(struct service *serv);
void service_toggle_connection(struct service *serv);

GVariant *service_get_property(struct service *serv, const char *key,
                               const char *subkey);
gchar *service_get_property_string(struct service *serv, const char *key,
                                   const char *subkey);
gboolean service_get_property_boolean(struct service *serv, const char *key,
                                      const char *subkey);
int service_get_property_int(struct service *serv, const char *key,
                             const char *subkey);
void service_set_property(struct service *serv, const char *key,
                          GVariant *value);
void service_set_properties(struct service *serv, GVariant *properties);

#endif /* _CONNMAN_GTK_SERVICE_H */
