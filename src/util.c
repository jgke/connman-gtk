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
#include <string.h>

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
	else if(!strcmp(state, "disconnected"))
		return _("Disconnected");
	else if(!strcmp(state, "online"))
		return _("Online");
	else
		return _("Error");
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

static void append_to_variant_inner(gpointer subkey, gpointer value,
				    gpointer user_data)
{
	g_variant_dict_insert_value(user_data, subkey, value);
}

static void append_to_variant(gpointer key, gpointer value, gpointer user_data)
{
	GVariantDict *out = user_data;
	GVariantDict *inner;
	GHashTable *table = value;
	GVariant *var = g_hash_table_lookup(table, "");
	if(var) {
		g_variant_dict_insert_value(out, key, var);
		return;
	}
	inner = g_variant_dict_new(NULL);
	g_hash_table_foreach(table, append_to_variant_inner, inner);
	var = g_variant_dict_end(inner);
	g_variant_dict_unref(inner);
	g_variant_dict_insert_value(out, key, var);
}

GVariant *dual_hash_table_to_variant(DualHashTable *dtable)
{
	GHashTable *table = dtable->table;
	GVariantDict *dict = g_variant_dict_new(NULL);
	g_hash_table_foreach(table, append_to_variant, dict);
	GVariant *out = g_variant_dict_end(dict);
	g_variant_dict_unref(dict);
	return out;
}
