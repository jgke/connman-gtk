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

#ifndef _CONNMAN_GTK_DIALOG_H
#define _CONNMAN_GTK_DIALOG_H

#include <glib.h>
#include <gtk/gtk.h>

#include "config.h"

enum token_element_type {
	TOKEN_ELEMENT_INVALID,
	TOKEN_ELEMENT_TEXT,
	TOKEN_ELEMENT_ENTRY,
	TOKEN_ELEMENT_LIST,
	TOKEN_ELEMENT_CHECKBOX
};

struct token_element {
	enum token_element_type type;
	GtkWidget *label, *content;
	gchar *name, *value;
};

struct token_element *token_new_text(const gchar *name, const gchar *content);
struct token_element *token_new_entry_full(const gchar *name, gboolean secret,
					   const gchar *value,
					   gboolean (*check)(GtkWidget *entry));
struct token_element *token_new_entry(const gchar *name, gboolean secret);
struct token_element *token_new_list(const gchar *name, GPtrArray *options);
struct token_element *token_new_checkbox(const gchar *name, gboolean state);
void free_token_element(struct token_element *elem);

gboolean dialog_ask_tokens(const gchar *title, GPtrArray *elements);

void show_error(const gchar *text, const gchar *message);

#endif /* _CONNMAN_GTK_DIALOG_H */
