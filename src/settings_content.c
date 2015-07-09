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
#include "settings_content_callback.h"
#include "style.h"
#include "util.h"

void settings_add_content(struct settings_page *page,
                          struct settings_content *content)
{
	gtk_grid_attach(GTK_GRID(page->grid), content->content,
	                0, page->index, 1, 1);
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

GVariant *settings_content_value_switch(struct settings_content *content)
{
	GtkWidget *toggle = g_object_get_data(G_OBJECT(content->content),
	                                      "toggle");
	return g_variant_new("b", gtk_switch_get_active(GTK_SWITCH(toggle)));
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

	gtk_widget_set_margin_bottom(content->content, MARGIN_SMALL);

	gtk_grid_set_column_homogeneous(GTK_GRID(content->content), TRUE);

	return content;
}

static void add_left_aligned(GtkGrid *grid, GtkWidget *a, GtkWidget *b)
{
	gtk_widget_set_margin_start(a, MARGIN_LARGE);
	gtk_widget_set_margin_end(a, MARGIN_SMALL);
	gtk_widget_set_margin_start(b, MARGIN_SMALL);

	gtk_widget_set_halign(a, GTK_ALIGN_START);
	gtk_widget_set_halign(b, GTK_ALIGN_START);
	gtk_widget_set_hexpand(a, TRUE);
	gtk_widget_set_hexpand(b, TRUE);

	gtk_grid_attach(grid, a, 0, 0, 1, 1);
	gtk_grid_attach(grid, b, 1, 0, 1, 1);
}

GtkWidget *settings_add_text(struct settings_page *page, const gchar *label,
                             const gchar *key, const gchar *subkey)
{
	GtkWidget *label_w, *value_w;
	gchar *value;
	struct settings_content *content = create_base_content(NULL, NULL);

	value = service_get_property_string(page->sett->serv, key, subkey);

	label_w = gtk_label_new(label);
	value_w = gtk_label_new(value);

	g_free(value);

	g_signal_connect(content->content, "destroy",
	                 G_CALLBACK(free_content), content);

	g_object_set_data(G_OBJECT(content->content), "value", value_w);


	STYLE_ADD_CONTEXT(label_w);
	gtk_style_context_add_class(gtk_widget_get_style_context(label_w),
	                            "dim-label");

	gtk_widget_set_hexpand(content->content, TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(value_w), TRUE);
	gtk_label_set_justify(GTK_LABEL(value_w), GTK_JUSTIFY_LEFT);
#if (GTK_MAJOR_VERSION > 3) || (GTK_MINOR_VERSION >= 16)
	if(gtk_get_major_version() > 3 || gtk_get_minor_version() >= 16)
		gtk_label_set_xalign(GTK_LABEL(value_w), 0);
	else
		gtk_misc_set_alignment(GTK_MISC(value_w), 0, 0.5);
#else
	/* deprecated at 3.14, but above only implemented at 3.16 */
	gtk_misc_set_alignment(GTK_MISC(value_w), 0, 0.5);
#endif

	add_left_aligned(GTK_GRID(content->content), label_w, value_w);

	settings_add_content(page, content);

	if(key) {
		struct content_callback *cb;
		cb = create_text_callback(value_w);
		settings_set_callback(page->sett, key, subkey, cb);
	}

	return value_w;
}

GtkWidget *settings_add_entry(struct settings_page *page, const gchar *label,
			      const gchar *key, const gchar *subkey,
			      const gchar *ekey, const gchar *esubkey,
			      settings_field_validator valid)
{
	GtkWidget *label_w, *entry;
	gchar *value;
	struct settings_content *content = create_base_content(ekey, esubkey);
	if(valid)
		content->valid = valid;
	content->free = g_free;
	content->value = settings_content_value_entry;

	label_w = gtk_label_new(label);
	entry = gtk_entry_new();

	value = service_get_property_string(page->sett->serv, key, subkey);
	gtk_entry_set_text(GTK_ENTRY(entry), value);
	g_free(value);

	g_signal_connect(content->content, "destroy",
	                 G_CALLBACK(free_content), content);
	g_object_set_data(G_OBJECT(content->content), "entry", entry);

	STYLE_ADD_CONTEXT(label_w);
	gtk_style_context_add_class(gtk_widget_get_style_context(label_w),
	                            "dim-label");

	add_left_aligned(GTK_GRID(content->content), label_w, entry);

	gtk_widget_show_all(content->content);

	settings_add_content(page, content);

	return entry;
}

GtkWidget *settings_add_switch(struct settings_page *page, const gchar *label,
                               const gchar *key, const gchar *subkey)
{
	GtkWidget *label_w, *toggle;
	struct settings_content *content = create_base_content(key, subkey);
	gboolean value;

	content->free = g_free;
	content->value = settings_content_value_switch;
	value = service_get_property_boolean(page->sett->serv, key, subkey);

	label_w = gtk_label_new(label);
	toggle = gtk_switch_new();

	gtk_switch_set_active(GTK_SWITCH(toggle), value);

	g_signal_connect(content->content, "destroy",
	                 G_CALLBACK(free_content), content);
	g_object_set_data(G_OBJECT(content->content), "toggle", toggle);

	STYLE_ADD_CONTEXT(label_w);
	gtk_style_context_add_class(gtk_widget_get_style_context(label_w),
	                            "dim-label");

	add_left_aligned(GTK_GRID(content->content), label_w, toggle);

	gtk_widget_show_all(content->content);

	settings_add_content(page, content);

	return toggle;
}
