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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "config.h"
#include "dialog.h"

gboolean status_icon_enabled;
gboolean launch_to_tray_by_default;
gboolean openconnect_use_fsid_by_default;
GHashTable *openconnect_fsid_table;
GSettings *settings;

#ifdef HAVE_CONFIG_SETTINGS

void config_load(GtkApplication *app)
{
	settings = g_settings_new("net.connman.gtk");
	g_settings_delay(settings);
	openconnect_fsid_table = g_hash_table_new_full(g_str_hash, g_str_equal,
						       g_free, NULL);

	status_icon_enabled = g_settings_get_boolean(settings,
						     "status-icon-enabled");

	launch_to_tray_by_default = g_settings_get_boolean(settings,
							   "launch-to-tray");

	openconnect_use_fsid_by_default = g_settings_get_boolean(settings,
					     "openconnect-use-fsid-by-default");
}

void config_window_open(GtkApplication *ignored, gpointer user_data)
{
	GPtrArray *entries;
	struct token_element *element;
	int index = 0;

	entries = g_ptr_array_new_full(1, (GDestroyNotify)free_token_element);
#ifdef USE_OPENCONNECT
	g_ptr_array_add(entries,
			token_new_checkbox(_("Use fsid with openconnect"),
					   openconnect_use_fsid_by_default));
#endif
#ifdef USE_STATUS_ICON
	g_ptr_array_add(entries,
			token_new_checkbox(_("Use status icon"),
					   status_icon_enabled));
	g_ptr_array_add(entries,
			token_new_checkbox(_("Launch to tray by default"),
					   launch_to_tray_by_default));
#endif

	if(!dialog_ask_tokens(_("Settings"), entries)) {
		g_ptr_array_free(entries, TRUE);
		return;
	}

#ifdef USE_OPENCONNECT
	element = entries->pdata[index];
	openconnect_use_fsid_by_default = !!element->value;
	index++;
	g_settings_set_boolean(settings, "openconnect-use-fsid-by-default",
			       openconnect_use_fsid_by_default);
#endif
#ifdef USE_STATUS_ICON
	element = entries->pdata[index];
	status_icon_enabled = !!element->value;
	index++;
	g_settings_set_boolean(settings, "status-icon-enabled",
			       status_icon_enabled);

	element = entries->pdata[index];
	launch_to_tray_by_default = !!element->value;
	index++;
	g_settings_set_boolean(settings, "launch-to-tray",
			       launch_to_tray_by_default);
#endif

	g_ptr_array_free(entries, TRUE);

	g_settings_apply(settings);
}

#else

void config_load(GtkApplication *app) {}
void config_window_open(gpointer *ignored, gpointer user_data) {}

#endif /* HAVE_CONFIG_SETTINGS */
