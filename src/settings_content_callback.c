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
#include <gtk/gtk.h>
#include <string.h>

#include "settings_content.h"
#include "settings_content_callback.h"
#include "util.h"

struct content_callback *create_text_callback(GtkWidget *label)
{
	struct content_callback *cb = g_malloc(sizeof(*cb));
	cb->type = CONTENT_CALLBACK_TYPE_TEXT;
	cb->data = label;
	return cb;
}

void handle_content_callback(GVariant *value, const gchar *key,
			     const gchar *subkey, struct content_callback *cb)
{
	switch(cb->type) {
	case CONTENT_CALLBACK_TYPE_TEXT: {
		GtkWidget *label = cb->data;
		gchar *str = variant_to_str(value);
		if(!strcmp(key, "State"))
			gtk_label_set_text(GTK_LABEL(label),
					   status_localized(str));
		else
			gtk_label_set_text(GTK_LABEL(label), str);
		g_free(str);
		return;
	}
	default:
		g_warning("Unknown callback type");
	}
}

void content_callback_free(void *cb_v)
{
	struct content_callback *cb = cb_v;
	switch(cb->type) {
	case CONTENT_CALLBACK_TYPE_TEXT:
		break;
	default:
		g_warning("Unknown callback type");
	}
	g_free(cb);
}
