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

#include "settings.h"
#include "settings_content.h"
#include "style.h"

void settings_add_content(struct settings_page *page,
                          struct settings_content *content)
{
	gtk_grid_attach(GTK_GRID(page->grid), content->content,
	                page->index, 0, 1, 1);
	page->index++;
}

gboolean settings_content_valid_always(struct settings_content *content)
{
	return TRUE;
}

GVariant *settings_content_value_null(struct settings_content *content)
{
	return NULL;
}

GVariant *settings_content_value_entry(struct settings_content *content)
{
	GtkWidget *entry = g_object_get_data(G_OBJECT(content->content),
	                                     "entry");
	const gchar *str = gtk_entry_get_text(GTK_ENTRY(entry));
	return g_variant_new("s", str);
}

void free_content(GtkWidget *widget, gpointer user_data)
{
	struct settings_content *content = user_data;
	content->free(content);
}

static struct settings_content *create_base_content(const gchar *key,
                const gchar *subkey)
{
	struct settings_content *content = g_malloc(sizeof(*content));
	content->content = gtk_grid_new();
	content->valid = settings_content_valid_always;
	content->value = settings_content_value_null;
	content->free = g_free;
	content->key = key;
	content->subkey = subkey;
	g_object_set_data(G_OBJECT(content->content), "content", content);
	return content;
}

static void add_centered(GtkGrid *grid, GtkWidget *a, GtkWidget *b)
{
	gtk_widget_set_margin_end(a, MARGIN_SMALL);
	gtk_widget_set_margin_start(b, MARGIN_SMALL);

	gtk_widget_set_halign(a, GTK_ALIGN_END);
	gtk_widget_set_halign(b, GTK_ALIGN_START);
	gtk_widget_set_hexpand(a, TRUE);
	gtk_widget_set_hexpand(b, TRUE);

	gtk_grid_attach(grid, a, 0, 0, 1, 1);
	gtk_grid_attach(grid, b, 1, 0, 1, 1);
}

GtkWidget *settings_add_text(struct settings_page *page, const gchar *key,
                             const gchar *subkey, const gchar *label,
                             const gchar *value)
{
	GtkWidget *label_w, *value_w;
	struct settings_content *content = create_base_content(key, subkey);

	label_w = gtk_label_new(label);
	value_w = gtk_label_new(value);

	g_signal_connect(content->content, "destroy",
	                 G_CALLBACK(free_content), content);

	STYLE_ADD_MARGIN(content->content, MARGIN_LARGE);
	STYLE_ADD_CONTEXT(label_w);
	gtk_style_context_add_class(gtk_widget_get_style_context(label_w),
	                            "dim-label");

	gtk_widget_set_hexpand(content->content, TRUE);

	add_centered(GTK_GRID(content->content), label_w, value_w);

	settings_add_content(page, content);

	return value_w;
}

GtkWidget *settings_add_entry(struct settings_page *page, const gchar *key,
                              const gchar *subkey, const gchar *label,
                              const gchar *value, settings_field_validator valid)
{
	GtkWidget *label_w, *entry;
	struct settings_content *content = create_base_content(key, subkey);
	if(valid)
		content->valid = valid;
	content->free = g_free;
	content->value = settings_content_value_entry;

	label_w = gtk_label_new(label);
	entry = gtk_entry_new();
	if(value)
		gtk_entry_set_text(GTK_ENTRY(entry), value);

	g_signal_connect(content->content, "destroy",
	                 G_CALLBACK(free_content), content);
	g_object_set_data(G_OBJECT(content->content), "entry", entry);

	STYLE_ADD_MARGIN(content->content, MARGIN_LARGE);
	STYLE_ADD_CONTEXT(label_w);
	gtk_style_context_add_class(gtk_widget_get_style_context(label_w),
	                            "dim-label");

	add_centered(GTK_GRID(content->content), label_w, entry);

	gtk_widget_show_all(content->content);

	settings_add_content(page, content);

	return entry;
}
