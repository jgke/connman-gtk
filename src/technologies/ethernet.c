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

#include <gio/gio.h>
#include <gtk/gtk.h>

#include "ethernet.h"
#include "technology.h"

struct ethernet_technology {
	struct technology parent;
};

struct technology *technology_ethernet_create(GVariant *properties,
		GDBusProxy *proxy) {
	struct ethernet_technology *item = g_malloc(sizeof(*item));
	technology_init((struct technology *)item, properties, proxy);
	gtk_image_set_from_icon_name(GTK_IMAGE(item->parent.list_item->icon),
			"network-wired-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_image_set_from_icon_name(GTK_IMAGE(item->parent.settings->icon),
			"network-wired", GTK_ICON_SIZE_DIALOG);
	return (struct technology *)item;
}

void technology_ethernet_free(struct technology *tech) {
	technology_free(tech);
}

void technology_ethernet_service_add(struct technology *item,
		struct service *service) {
	technology_add_service(item, service);
}

void technology_ethernet_service_remove(struct technology *item,
		const gchar *path) {
	technology_remove_service(item, path);
}
