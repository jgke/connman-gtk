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

#ifndef _CONNMAN_GTK_SETTINGS_CONTENT_H
#define _CONNMAN_GTK_SETTINGS_CONTENT_H

#include "settings.h"

typedef gboolean (*settings_field_validator)(gchar *value);

struct settings_content {
	GtkWidget *content;

	settings_field_validator valid;
	void (*free)(void *ptr);
	gchar *original;
};

void settings_add_content(struct settings_page *page,
		struct settings_content *content);

gboolean settings_content_always_valid(gchar *value);

GtkWidget *settings_add_text(struct settings_page *page, const gchar *label,
                             const gchar *value);

GtkWidget *settings_add_entry(struct settings_page *page, const gchar *label,
                             const gchar *value, settings_field_validator valid);

void free_content(GtkWidget *widget, gpointer user_data);

#endif /* _CONNMAN_GTK_SETTINGS_CONTENT_H */
