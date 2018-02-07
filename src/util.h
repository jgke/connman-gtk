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

#ifndef _CONNMAN_GTK_UTIL_H
#define _CONNMAN_GTK_UTIL_H

#include <glib.h>
#include <gtk/gtk.h>
#include "config.h"

gchar *variant_str_array_to_str(GVariant *variant);
gchar **variant_to_strv(GVariant *variant);
gchar *variant_to_str(GVariant *variant);
gboolean variant_to_bool(GVariant *variant);
guint64 variant_to_uint(GVariant *variant);
gint64 variant_to_int(GVariant *variant);
const gchar *status_localized(const gchar *status);
const gchar *failure_localized(const gchar *status);
gboolean valid_ipv4(const gchar *address);
gboolean valid_ipv6(const gchar *address);
void list_item_selected(GtkListBox *box, GtkListBoxRow *row, gpointer data);
void combo_box_changed(GtkComboBox *widget, gpointer data);

typedef struct DualHashTable_t DualHashTable;
typedef void(*DualHashTableIter)(const gchar *key, const gchar *subkey,
                                 gpointer value, gpointer user_data);
DualHashTable *dual_hash_table_new(GDestroyNotify free_value);
DualHashTable *dual_hash_table_ref(DualHashTable *table);
void dual_hash_table_unref(DualHashTable *table);
void *hash_table_get_dual_key(DualHashTable *table, const gchar *key,
                              const gchar *subkey);
void hash_table_set_dual_key(DualHashTable *table, const gchar *key,
                             const gchar *subkey, void *value);
void dual_hash_table_foreach(DualHashTable *table, DualHashTableIter cb,
                             gpointer user_data);
GVariant *dual_hash_table_to_variant(DualHashTable *table);

#endif /* _CONNMAN_GTK_UTIL_H */
