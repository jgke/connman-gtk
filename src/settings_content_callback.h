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

#ifndef _CONNMAN_GTK_SETTINGS_CONTENT_CALLBACK_H
#define _CONNMAN_GTK_SETTINGS_CONTENT_CALLBACK_H

#include <glib.h>

enum content_callback_type {
	CONTENT_CALLBACK_TYPE_UNKNOWN,
	CONTENT_CALLBACK_TYPE_TEXT,
	CONTENT_CALLBACK_TYPE_LIST,
};

struct content_callback {
	enum content_callback_type type;
	void *data;
};

struct content_callback *create_text_callback(GtkWidget *label);
struct content_callback *create_list_callback(GtkWidget *list);
void handle_content_callback(GVariant *value, const gchar *key,
                             const gchar *subkey, struct content_callback *cb);
void content_callback_free(void *cb);

#endif /* _CONNMAN_GTK_SETTINGS_CONTENT_CALLBACK_H */
