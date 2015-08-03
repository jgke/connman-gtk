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

#include <arpa/inet.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include "main.h"
#include "style.h"
#include "util.h"

gchar *variant_str_array_to_str(GVariant *variant)
{
	GString *str;
	const gchar **arr, **iter;
	gchar *out;
	arr = g_variant_get_strv(variant, NULL);
	str = g_string_new(arr[0]);
	iter = arr + 1;
	while(*arr && *iter)
		g_string_append_printf(str, ", %s", *iter++);
	g_free(arr);
	out = str->str;
	g_string_free(str, FALSE);
	return out;
}

gchar **variant_to_strv(GVariant *variant)
{
	if(!variant)
		return g_malloc(sizeof(gchar *));
	return g_variant_dup_strv(variant, NULL);
}

gchar *variant_to_str(GVariant *variant)
{
	if(!variant)
		return g_strdup("");
	if(!strcmp(g_variant_get_type_string(variant), "as"))
		return variant_str_array_to_str(variant);
	else
		return g_variant_dup_string(variant, NULL);
}

gboolean variant_to_bool(GVariant *variant)
{
	if(!variant)
		return FALSE;
	return g_variant_get_boolean(variant);
}

guint64 variant_to_uint(GVariant *variant)
{
	if(!variant)
		return 0;
	if(g_variant_is_of_type(variant, G_VARIANT_TYPE_BYTE))
		return g_variant_get_byte(variant);
	if(g_variant_is_of_type(variant, G_VARIANT_TYPE_UINT16))
		return g_variant_get_uint16(variant);
	if(g_variant_is_of_type(variant, G_VARIANT_TYPE_UINT32))
		return g_variant_get_uint32(variant);
	if(g_variant_is_of_type(variant, G_VARIANT_TYPE_UINT64))
		return g_variant_get_uint64(variant);
	g_warning("Converting signed type to unsigned");
	if(g_variant_is_of_type(variant, G_VARIANT_TYPE_INT16))
		return g_variant_get_int16(variant);
	if(g_variant_is_of_type(variant, G_VARIANT_TYPE_INT32))
		return g_variant_get_int32(variant);
	if(g_variant_is_of_type(variant, G_VARIANT_TYPE_INT64))
		return g_variant_get_int64(variant);
	return 0;
}

gint64 variant_to_int(GVariant *variant)
{
	if(!variant)
		return 0;
	if(g_variant_is_of_type(variant, G_VARIANT_TYPE_BYTE))
		return g_variant_get_byte(variant);
	if(g_variant_is_of_type(variant, G_VARIANT_TYPE_INT16))
		return g_variant_get_int16(variant);
	if(g_variant_is_of_type(variant, G_VARIANT_TYPE_INT32))
		return g_variant_get_int32(variant);
	if(g_variant_is_of_type(variant, G_VARIANT_TYPE_INT64))
		return g_variant_get_int64(variant);
	if(g_variant_is_of_type(variant, G_VARIANT_TYPE_UINT16))
		return g_variant_get_uint16(variant);
	if(g_variant_is_of_type(variant, G_VARIANT_TYPE_UINT32))
		return g_variant_get_uint32(variant);
	if(g_variant_is_of_type(variant, G_VARIANT_TYPE_UINT64)) {
		g_warning("Converting uint64 to int64");
		return g_variant_get_uint64(variant);
	}
	return 0;
}

const gchar *status_localized(const gchar *state)
{
	if(!strcmp(state, "idle"))
		return _("Idle");
	else if(!strcmp(state, "failure"))
		return _("Failure");
	else if(!strcmp(state, "association"))
		return _("Association");
	else if(!strcmp(state, "configuration"))
		return _("Configuration");
	else if(!strcmp(state, "ready"))
		return _("Ready");
	else if(!strcmp(state, "disconnect"))
		return _("Disconnected");
	else if(!strcmp(state, "online"))
		return _("Online");
	else
		return _("Error");
}

const gchar *failure_localized(const gchar *state)
{
	if(!strcmp(state, "out-of-range"))
		return _("Out of range");
	else if(!strcmp(state, "pin-missing"))
		return _("PIN missing");
	else if(!strcmp(state, "dhcp-failed"))
		return _("DHCP failed");
	else if(!strcmp(state, "connect-failed"))
		return _("Connecting failed");
	else if(!strcmp(state, "login-failed"))
		return _("Login failed");
	else if(!strcmp(state, "auth-failed"))
		return _("Authentication failed");
	else if(!strcmp(state, "invalid-key"))
		return _("Invalid key");
	else
		return "";
}

gboolean valid_ipv4(const gchar *address)
{
	char str[INET_ADDRSTRLEN];
	return inet_pton(AF_INET, address, str) == 1;
}

gboolean valid_ipv6(const gchar *address)
{
	char str[INET6_ADDRSTRLEN];
	return inet_pton(AF_INET6, address, str) == 1;
}

static void item_selected(GtkWidget *notebook, GtkWidget *content)
{
	gint num = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), content);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), num);
	g_object_set_data(G_OBJECT(notebook), "selected", content);
	g_object_set_data(G_OBJECT(content), "selected", content);
}

void list_item_selected(GtkListBox *box, GtkListBoxRow *row,
                        gpointer data)
{
	if(!G_IS_OBJECT(row))
		return;
	GtkWidget *notebook = data;
	GtkWidget *content = g_object_get_data(G_OBJECT(row), "content");
	item_selected(notebook, content);
}

void combo_box_changed(GtkComboBox *box, gpointer data)
{
	GtkWidget *notebook = data;
	GtkWidget *content;
	content = g_object_get_data(G_OBJECT(notebook), "selected");
	if(content)
		g_object_set_data(G_OBJECT(content), "selected", NULL);
	GHashTable *table = g_object_get_data(G_OBJECT(box), "items");
	GtkComboBoxText *b = GTK_COMBO_BOX_TEXT(box);
	gchar *str = gtk_combo_box_text_get_active_text(b);
	content = g_hash_table_lookup(table, str);
	item_selected(notebook, content);
	g_free(str);
}

struct DualHashTable_t {
	GHashTable *table;
	GDestroyNotify free_value;
	gint refcount;
};

DualHashTable *dual_hash_table_new(GDestroyNotify free_value)
{
	DualHashTable *dt = g_malloc(sizeof(*dt));
	GDestroyNotify free_func = (GDestroyNotify)g_hash_table_unref;
	GHashTable *table = g_hash_table_new_full(g_str_hash, g_str_equal,
	                    g_free, free_func);
	dt->table = table;
	dt->free_value = free_value;
	dt->refcount = 1;
	return dt;
}

DualHashTable *dual_hash_table_ref(DualHashTable *table)
{
	g_atomic_int_inc(&table->refcount);
	g_hash_table_ref(table->table);
	return table;
}

void dual_hash_table_unref(DualHashTable *table)
{
	g_hash_table_unref(table->table);
	if(g_atomic_int_dec_and_test(&table->refcount))
		g_free(table);
}

void *hash_table_get_dual_key(DualHashTable *dtable, const gchar *key,
                              const gchar *subkey)
{
	GHashTable *table = dtable->table;
	GHashTable *t = g_hash_table_lookup(table, key);
	if(!t)
		return NULL;
	if(!subkey)
		subkey = "";
	return g_hash_table_lookup(t, subkey);
}

void hash_table_set_dual_key(DualHashTable *dtable, const gchar *key,
                             const gchar *subkey, void *value)
{
	GHashTable *table = dtable->table;
	GHashTable *t;
	t = g_hash_table_lookup(table, key);
	if(!t) {
		t = g_hash_table_new_full(g_str_hash, g_str_equal,
		                          g_free, dtable->free_value);
		g_hash_table_insert(table, g_strdup(key), t);
	}
	if(!subkey)
		subkey = "";
	g_hash_table_insert(t, g_strdup(subkey), value);
}

struct table_cb_data {
	DualHashTableIter cb;
	gpointer user_data;
	const gchar *key;
};

static void iter_cb_cb(gpointer subkey, gpointer value, gpointer user_data)
{
	struct table_cb_data *data = user_data;
	if(!strcmp(subkey, ""))
		subkey = NULL;
	data->cb(data->key, subkey, value, data->user_data);
}

static void iter_cb(gpointer key, gpointer value, gpointer user_data)
{
	struct table_cb_data *data = user_data;
	data->key = key;
	g_hash_table_foreach(value, iter_cb_cb, data);
}

void dual_hash_table_foreach(DualHashTable *table, DualHashTableIter cb,
                             gpointer user_data)
{
	struct table_cb_data data = {cb, user_data};
	g_hash_table_foreach(table->table, iter_cb, &data);
}

static void append_to_variant(const gchar *key, const gchar *subkey,
                              gpointer value, gpointer user_data)
{
	GVariant *inner_v;
	GVariantDict *inner;
	if(!subkey) {
		g_variant_dict_insert_value(user_data, key, value);
		return;
	}
	inner_v = g_variant_dict_lookup_value(user_data, key, NULL);
	if(!inner_v)
		inner = g_variant_dict_new(NULL);
	else
		inner = g_variant_dict_new(inner_v);
	g_variant_dict_insert_value(inner, subkey, value);
	if(inner_v)
		g_variant_unref(inner_v);
	inner_v = g_variant_dict_end(inner);
	g_variant_dict_insert_value(user_data, key, inner_v);
	g_variant_dict_unref(inner);
}

GVariant *dual_hash_table_to_variant(DualHashTable *dtable)
{
	GVariantDict *dict = g_variant_dict_new(NULL);
	dual_hash_table_foreach(dtable, append_to_variant, dict);
	GVariant *out = g_variant_dict_end(dict);
	g_variant_dict_unref(dict);
	return out;
}

struct error_params {
	gchar *text;
	gchar *log;
};

static gboolean show_error_sync(gpointer data)
{
	struct error_params *params = data;

	const gchar *title;
	int flags, height;
	GtkWidget *window, *area, *grid, *text, *logl, *log, *box;

	if(params->log)
		g_strstrip(params->log);

	grid = gtk_grid_new();
	text = gtk_label_new(params->text);
	logl = gtk_label_new(_("Details:"));
	log = gtk_label_new(params->log);
	box = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(box),
				       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	title = _("Operation failed");
	flags = GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL;
	window = gtk_dialog_new_with_buttons(title, GTK_WINDOW(main_window),
	                                     flags,
	                                     _("_OK"), GTK_RESPONSE_NONE, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(window), GTK_RESPONSE_NONE);

	style_add_context(log);
	gtk_style_context_add_class(gtk_widget_get_style_context(log),
				    "cm-log");
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(box),
					    GTK_SHADOW_IN);
	gtk_widget_set_halign(text, GTK_ALIGN_START);
	label_align_text(GTK_LABEL(logl), 0, 0);
	label_align_text(GTK_LABEL(log), 0, 0);
	style_add_margin(text, MARGIN_LARGE);
	style_add_margin(logl, MARGIN_LARGE);
	style_add_margin(box, MARGIN_LARGE);
	gtk_widget_set_hexpand(log, TRUE);
	gtk_widget_set_vexpand(log, TRUE);
	gtk_widget_set_margin_bottom(text, 0);
	gtk_widget_set_margin_bottom(logl, 0);
	gtk_widget_set_margin_top(box, 0);

	gtk_container_add(GTK_CONTAINER(box), log);
	gtk_grid_attach(GTK_GRID(grid), text, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), logl, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), box, 0, 2, 1, 1);
	gtk_widget_show_all(grid);

	gtk_widget_get_preferred_height(log, NULL, &height);
	if(height > 150)
		height = 150;
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(box),
						   height);

	if(!params->log) {
		gtk_widget_hide(box);
		gtk_widget_hide(logl);
	}

	area = gtk_dialog_get_content_area(GTK_DIALOG(window));
	gtk_container_add(GTK_CONTAINER(area), grid);
	gtk_dialog_run(GTK_DIALOG(window));

	gtk_widget_destroy(window);

	g_free(params->text);
	g_free(params->log);
	g_free(params);

	return FALSE;
}

void show_error(const gchar *text, const gchar *log)
{
	struct error_params *params = g_malloc(sizeof(*params));
	params->text = g_strdup(text);
	if(log)
		params->log = g_strdup(log);
	else
		params->log = NULL;
	g_main_context_invoke(NULL, show_error_sync, params);
}
