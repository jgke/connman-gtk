
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

#include <gtk/gtk.h>

#if defined(USE_OPENCONNECT) || defined(USE_STATUS_ICON)
#define HAVE_CONFIG_SETTINGS 1
#endif

// Enable the status icon, or, minimize to tray on close
extern gboolean status_icon_enabled;

// Use fsid with openconnect by default
extern gboolean openconnect_use_fsid_by_default;

// Service name in this hashset -> enable fsid
extern GHashTable *openconnect_fsid_table;

void config_load(GtkApplication *app);
void config_window_open(GtkApplication *app);

#endif /* _CONNMAN_GTK_CONFIG_H */
