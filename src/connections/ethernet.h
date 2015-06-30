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

#ifndef _CONNMAN_GTK_ETHERNET_H
#define _CONNMAN_GTK_ETHERNET_H

#include <gio/gio.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "technology.h"

void technology_ethernet_init(struct technology *tech, GVariant *properties,
		GDBusProxy *proxy);

void service_ethernet_init(struct service *serv, GDBusProxy *proxy, 
		const gchar *path, GVariant *properties);
struct service *service_ethernet_create(void);
void service_ethernet_free(struct service *serv);
void service_ethernet_update(struct service *serv);

#endif /* _CONNMAN_GTK_ETHERNET_H */

