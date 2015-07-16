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

#include <glib.h>

#include "settings.h"

struct settings_content;

typedef gboolean (*settings_field_validator)(struct settings_content *content);
typedef gboolean (*settings_writable)(struct settings_content *content);
typedef GVariant *(*settings_field_value)(struct settings_content *content);

struct settings_content {
	GtkWidget *data;
	struct settings *sett;

	settings_field_validator valid;
	settings_field_value value;
	settings_writable writable;
	void (*free)(void *ptr);
	const gchar *key;
	const gchar *subkey;
};

gboolean never_write(struct settings_content *content);
gboolean always_write(struct settings_content *content);
gboolean write_if_selected(struct settings_content *content);
gboolean always_valid(struct settings_content *content);

void settings_add_content(struct settings_page *page,
                          struct settings_content *content);

GtkWidget *settings_add_text(struct settings_page *page, const gchar *label,
                             const gchar *key, const gchar *subkey);
GtkWidget *settings_add_entry(struct settings *sett, struct settings_page *page,
                              settings_writable writable, const gchar *label,
                              const gchar *key, const gchar *subkey,
                              const gchar *ekey, const gchar *esubkey,
                              settings_field_validator valid);
GtkWidget *settings_add_switch(struct settings *sett,
                               struct settings_page *page,
                               settings_writable writable, const gchar *label,
                               const gchar *key, const gchar *subkey);
GtkWidget *settings_add_combo_box(struct settings *sett,
                                  struct settings_page *page,
                                  settings_writable writable,
                                  const gchar *label,
                                  const gchar *key, const gchar *subkey,
                                  const gchar *ekey, const gchar *esubkey);
GtkWidget *settings_add_entry_list(struct settings *sett,
                                   struct settings_page *page,
                                   settings_writable writable,
                                   const gchar *label,
                                   const gchar *key, const gchar *subkey,
                                   const gchar *ekey, const gchar *esubkey);

void free_content(GtkWidget *widget, gpointer user_data);

#endif /* _CONNMAN_GTK_SETTINGS_CONTENT_H */
