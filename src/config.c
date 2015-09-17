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

#ifndef _CONNMAN_GTK_CONFIG_H
#define _CONNMAN_GTK_CONFIG_H

#include <glib.h>
#include <gtk/gtk.h>

#include "config.h"

gboolean status_icon_enabled;
GHashTable *openconnect_fsid_table;
GSettings *settings;

void config_load(GtkApplication *app)
{
	settings = g_settings_new("net.connman.gtk");
	GVariant *value;

	openconnect_fsid_table = g_hash_table_new_full(g_str_hash, g_str_equal,
						       g_free, NULL);

	value = g_settings_get_value(settings, "status-icon-enabled");
	status_icon_enabled = g_variant_get_boolean(value);
	g_variant_unref(value);
}

void config_window_open(GtkApplication *app)
{

}

#endif /* _CONNMAN_GTK_CONFIG_H */
