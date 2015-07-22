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

gboolean never_write(struct settings_content *content)
{
	return FALSE;
}

gboolean always_write(struct settings_content *content)
{
	return TRUE;
}

gboolean write_if_selected(struct settings_content *content)
{
	GtkWidget *parent = gtk_widget_get_parent(content->data);
	return  !!g_object_get_data(G_OBJECT(parent), "selected");
}

gboolean always_valid(GtkWidget *entry)
{
	return TRUE;
}

static GVariant *content_value_null(struct settings_content *content)
{
	return NULL;
}

static GVariant *content_value_entry(struct settings_content *content)
{
	GtkWidget *entry = content->data;
	const gchar *str = gtk_entry_get_text(GTK_ENTRY(entry));
	if(!strcmp(content->key, "IPv6.Configuration") &&
	    !strcmp(content->subkey, "Gateway") && !strlen(str))
		return NULL;
	GVariant *var = g_variant_new("s", str);
	return var;
}

static GVariant *content_value_switch(struct settings_content *content)
{
	GtkWidget *toggle = content->data;
	gboolean active = gtk_switch_get_active(GTK_SWITCH(toggle));
	GVariant *var = g_variant_new("b", active);
	return var;
}

static GVariant *content_value_list(struct settings_content *content)
{
	GtkWidget *list = content->data;
	GtkComboBox *box = GTK_COMBO_BOX(list);
	GVariant *var = g_variant_new("s", gtk_combo_box_get_active_id(box));
	return var;
}

static GVariant *content_value_entry_list(struct settings_content *content)
{
	GtkWidget *list = content->data;
	GList *children = gtk_container_get_children(GTK_CONTAINER(list));
	GPtrArray *array = g_ptr_array_new();
	GList *l;
	for(l = children; l != NULL; l = l->next) {
		GtkWidget *child = l->data;
		GtkWidget *entry = g_object_get_data(G_OBJECT(child), "entry");
		const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
		g_ptr_array_insert(array, -1, (gchar *)text);
	}
	g_list_free(children);
	GVariant *var = g_variant_new_strv((const gchar * const *)array->pdata,
	                                   array->len);
	g_ptr_array_free(array, TRUE);
	return var;
}

void free_content(GtkWidget *widget, gpointer user_data)
{
	struct settings_content *content = user_data;
	content->free(content);
}

static GtkWidget *create_label(const gchar *text)
{
	GtkWidget *label;
	label = gtk_label_new(text);

	STYLE_ADD_CONTEXT(label);
	gtk_style_context_add_class(gtk_widget_get_style_context(label),
	                            "dim-label");
	gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
	return label;
}

static struct settings_content *create_base_content(struct settings *sett,
                settings_writable writable,
                const gchar *key,
                const gchar *subkey,
                const gchar *secondary_key)
{
	struct settings_content *content = g_malloc(sizeof(*content));
	content->value = content_value_null;
	content->free = g_free;
	content->sett = sett;
	content->writable = writable;
	content->key = key;
	content->subkey = subkey;
	content->secondary_key = secondary_key;

	return content;
}

static void add_left_aligned(GtkGrid *grid, GtkWidget *a, GtkWidget *b, int y)
{
	gtk_widget_set_margin_start(b, MARGIN_SMALL);
	gtk_widget_set_margin_bottom(b, MARGIN_SMALL);
	gtk_widget_set_halign(b, GTK_ALIGN_START);
	gtk_widget_set_hexpand(b, TRUE);

	if(a) {
		gtk_widget_set_margin_start(a, MARGIN_LARGE);
		gtk_widget_set_margin_end(a, MARGIN_SMALL);
		gtk_widget_set_margin_bottom(a, MARGIN_SMALL);
		gtk_widget_set_halign(a, GTK_ALIGN_START);
		gtk_widget_set_hexpand(a, TRUE);
		gtk_grid_attach(grid, a, 0, y, 1, 1);
		gtk_grid_attach(grid, b, 1, y, 1, 1);
	} else {
		gtk_widget_set_margin_start(b, MARGIN_LARGE);
		gtk_grid_attach(grid, b, 0, y, 2, 1);
	}
}

GtkWidget *settings_add_text(struct settings_page *page, const gchar *label,
                             const gchar *key, const gchar *subkey)
{
	GtkWidget *label_w, *value_w;
	gchar *value;
	struct settings_content *content;

	content = create_base_content(NULL, never_write, NULL, NULL, NULL);

	value = service_get_property_string(page->sett->serv, key, subkey);

	label_w = create_label(label);
	value_w = gtk_label_new(value);

	g_free(value);

	label_align_text_left(GTK_LABEL(value_w));

	add_left_aligned(GTK_GRID(page->grid), label_w, value_w, page->index++);
	gtk_widget_show_all(page->grid);

	if(key) {
		struct content_callback *cb;
		cb = create_callback(value_w, CONTENT_CALLBACK_TYPE_TEXT);
		settings_set_callback(page->sett, key, subkey, cb);
	}

	content->data = value_w;
	g_signal_connect(content->data, "destroy",
	                 G_CALLBACK(free_content), content);
	return value_w;
}

static void entry_changed(GtkEditable *editable, gpointer user_data)
{
	GtkWidget *entry = GTK_WIDGET(editable);
	GtkStyleContext *context = gtk_widget_get_style_context(entry);
	settings_entry_validator validator = g_object_get_data(G_OBJECT(entry),
							       "validator");
	struct settings_content *content = user_data;

	gboolean valid = validator(entry);

	if(!valid && !g_object_get_data(G_OBJECT(entry), "invalid")) {
		if(!content->sett->invalid_count)
			gtk_widget_set_sensitive(content->sett->apply, FALSE);
		g_object_set_data(G_OBJECT(entry), "invalid",
				  GINT_TO_POINTER(TRUE));
		content->sett->invalid_count++;
		gtk_style_context_add_class(context, "error");
	}
	else if (valid && g_object_get_data(G_OBJECT(entry), "invalid")) {
		g_object_set_data(G_OBJECT(entry), "invalid",
				  GINT_TO_POINTER(FALSE));
		content->sett->invalid_count--;
		gtk_style_context_remove_class(context, "error");
		if(!content->sett->invalid_count)
			gtk_widget_set_sensitive(content->sett->apply,
						 TRUE);
	}
}

GtkWidget *settings_add_entry(struct settings *sett, struct settings_page *page,
                              settings_writable writable, const gchar *label,
                              const gchar *key, const gchar *subkey,
                              const gchar *secondary_key,
                              settings_entry_validator valid)
{
	GtkWidget *label_w, *entry;
	gchar *value;
	struct settings_content *content;

	content = create_base_content(sett, writable, key, subkey,
	                              secondary_key);
	content->free = g_free;
	content->value = content_value_entry;

	if(label)
		label_w = create_label(label);
	else
		label_w = NULL;
	entry = gtk_entry_new();
	g_object_set_data(G_OBJECT(entry), "validator", valid);

	value = service_get_property_string(page->sett->serv, key, subkey);
	if(!strlen(value)) {
		g_free(value);
		value = service_get_property_string(page->sett->serv,
		                                    secondary_key, subkey);
	}
	gtk_entry_set_text(GTK_ENTRY(entry), value);
	g_free(value);

	if(!strncmp(key, "IPv4", 4))
		gtk_entry_set_width_chars(GTK_ENTRY(entry), 4*3 + 3);
	else if(!strncmp(key, "IPv6", 4))
		gtk_entry_set_width_chars(GTK_ENTRY(entry), 8*4 + 7);

	add_left_aligned(GTK_GRID(page->grid), label_w, entry, page->index++);
	gtk_widget_show_all(page->grid);

	hash_table_set_dual_key(sett->contents, key, subkey, content);

	content->data = entry;
	g_signal_connect(content->data, "destroy",
	                 G_CALLBACK(free_content), content);
	g_signal_connect(entry, "changed",
	                 G_CALLBACK(entry_changed), content);
	return entry;
}

GtkWidget *settings_add_switch(struct settings *sett,
                               struct settings_page *page,
                               settings_writable writable, const gchar *label,
                               const gchar *key, const gchar *subkey)
{
	GtkWidget *label_w, *toggle;
	struct settings_content *content;
	gboolean value;

	content = create_base_content(sett, writable, key, subkey, NULL);
	content->value = content_value_switch;
	value = service_get_property_boolean(page->sett->serv, key, subkey);

	label_w = create_label(label);
	toggle = gtk_switch_new();

	gtk_switch_set_active(GTK_SWITCH(toggle), value);

	add_left_aligned(GTK_GRID(page->grid), label_w, toggle, page->index++);
	gtk_widget_show_all(page->grid);

	hash_table_set_dual_key(sett->contents, key, subkey, content);

	content->data = toggle;
	g_signal_connect(content->data, "destroy",
	                 G_CALLBACK(free_content), content);
	return toggle;
}

static void free_combo_box(void *data)
{
	struct settings_content *content = data;
	g_hash_table_unref(g_object_get_data(G_OBJECT(content->data), "items"));
	g_free(content);
}

GtkWidget *settings_add_combo_box(struct settings *sett,
                                  struct settings_page *page,
                                  settings_writable writable,
                                  const gchar *label,
                                  const gchar *key, const gchar *subkey,
                                  const gchar *secondary_key)
{
	struct settings_content *content;
	GtkWidget *label_w, *notebook, *box;
	GHashTable *items;

	content = create_base_content(sett, writable, key, subkey,
	                              secondary_key);
	items = g_hash_table_new_full(g_str_hash, g_str_equal,
	                              g_free, NULL);

	content->free = free_combo_box;
	content->value = content_value_list;

	notebook = gtk_notebook_new();
	box = gtk_combo_box_text_new();
	label_w = create_label(label);

	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
	gtk_widget_set_hexpand(notebook, TRUE);
	gtk_widget_set_vexpand(notebook, FALSE);

	g_object_set_data(G_OBJECT(box), "notebook", notebook);
	g_object_set_data(G_OBJECT(box), "items", items);

	g_signal_connect(box, "changed", G_CALLBACK(combo_box_changed),
	                 notebook);

	add_left_aligned(GTK_GRID(page->grid), label_w, box, page->index++);
	gtk_widget_set_margin_bottom(label_w, MARGIN_LARGE);
	gtk_widget_set_margin_bottom(box, MARGIN_LARGE);
	gtk_grid_attach(GTK_GRID(page->grid), notebook, 0, page->index++, 2, 1);
	gtk_widget_show_all(page->grid);

	hash_table_set_dual_key(sett->contents, key, subkey, content);

	if(key) {
		struct content_callback *cb;
		cb = create_callback(box, CONTENT_CALLBACK_TYPE_LIST);
		settings_set_callback(page->sett, key, subkey, cb);
	}

	content->data = box;
	g_signal_connect(content->data, "destroy",
	                 G_CALLBACK(free_content), content);
	return box;
}

static void update_button_visibility(GtkWidget *list)
{
	int count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(list), "count"));
	GList *children = gtk_container_get_children(GTK_CONTAINER(list));
	GList *l;
	for(l = children; l != NULL; l = l->next) {
		GtkWidget *child = l->data;
		GtkWidget *rem = g_object_get_data(G_OBJECT(child), "button");
		gtk_widget_set_sensitive(rem, count != 1);
	}
	g_list_free(children);
}

static void destroy_entry(GtkButton *button, gpointer user_data)
{
	GtkWidget *box = gtk_widget_get_parent(GTK_WIDGET(user_data));
	int count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(box), "count"));
	count--;
	g_object_set_data(G_OBJECT(box), "count", GINT_TO_POINTER(count));
	gtk_widget_destroy(GTK_WIDGET(user_data));
	update_button_visibility(box);
}

void content_add_entry_to_list(GtkWidget *list, const gchar *value)
{
	struct settings_content *content = g_object_get_data(G_OBJECT(list),
							     "content");
	settings_entry_validator validator  = g_object_get_data(G_OBJECT(list),
								"validator");
	GtkWidget *entry = gtk_entry_new();
	GtkWidget *rem = gtk_button_new_from_icon_name("list-remove-symbolic",
	                 GTK_ICON_SIZE_MENU);
	GtkWidget *remgrid = gtk_grid_new();
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *row = gtk_list_box_row_new();

	if(value)
		gtk_entry_set_text(GTK_ENTRY(entry), value);

	g_signal_connect(entry, "changed",
	                 G_CALLBACK(entry_changed), content);
	g_object_set_data(G_OBJECT(entry), "validator", validator);
	g_object_set_data(G_OBJECT(row), "entry", entry);
	g_object_set_data(G_OBJECT(row), "button", rem);
	g_object_set_data(G_OBJECT(row), "destroy", destroy_entry);
	g_signal_connect(rem, "clicked", G_CALLBACK(destroy_entry), row);

	STYLE_ADD_MARGIN(entry, MARGIN_SMALL);
	gtk_widget_set_hexpand(entry, TRUE);
	gtk_widget_set_hexpand(row, TRUE);
	gtk_widget_set_vexpand(rem, FALSE);
	gtk_widget_set_halign(rem, GTK_ALIGN_END);
	gtk_widget_set_valign(rem, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(remgrid, GTK_ALIGN_CENTER);

	gtk_grid_attach(GTK_GRID(grid), entry, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(remgrid), rem, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), remgrid, 1, 0, 1, 1);
	gtk_container_add(GTK_CONTAINER(row), grid);
	gtk_widget_show_all(row);
	gtk_container_add(GTK_CONTAINER(list), row);

	int count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(list), "count"));
	count++;
	g_object_set_data(G_OBJECT(list), "count", GINT_TO_POINTER(count));
	update_button_visibility(list);
}

static void add_entry_to_list(GtkButton *button, gpointer user_data)
{
	GtkWidget *list = user_data;
	content_add_entry_to_list(list, NULL);
}

GtkWidget *settings_add_entry_list(struct settings *sett,
                                   struct settings_page *page,
                                   settings_writable writable,
                                   const gchar *label,
                                   const gchar *key, const gchar *subkey,
                                   const gchar *secondary_key,
				   settings_entry_validator valid)
{
	struct settings_content *content;
	gchar **values;
	gchar **iter;
	GtkWidget *box, *label_w, *toolbar, *frame, *button, *buttonbox, *grid;
	GtkToolItem *item, *buttonitem;

	content = create_base_content(sett, writable, key, subkey,
	                              secondary_key);
	box = gtk_list_box_new();
	label_w = create_label(label);
	toolbar = gtk_toolbar_new();
	frame = gtk_frame_new(NULL);
	button = gtk_button_new_from_icon_name("list-add-symbolic",
	                                       GTK_ICON_SIZE_MENU);
	/* needs to be a box and not a grid for inline-toolbar css */
	buttonbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	grid = gtk_grid_new();
	item = gtk_separator_tool_item_new();
	buttonitem = gtk_tool_item_new();

	content->value = content_value_entry_list;

	g_object_set_data(G_OBJECT(box), "count", GINT_TO_POINTER(0));
	g_object_set_data(G_OBJECT(box), "content", content);
	g_object_set_data(G_OBJECT(box), "validator", valid);
	g_signal_connect(button, "clicked", G_CALLBACK(add_entry_to_list), box);

	values = service_get_property_strv(sett->serv, key, subkey);
	if(!*values) {
		g_strfreev(values);
		values = service_get_property_strv(sett->serv, secondary_key,
		                                   subkey);
		if(!*values)
			content_add_entry_to_list(box, NULL);
	}
	for(iter = values; *iter; iter++)
		content_add_entry_to_list(box, *iter);
	g_strfreev(values);

	gtk_style_context_add_class(gtk_widget_get_style_context(toolbar),
	                            GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_tool_item_set_expand(item, TRUE);
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);

	gtk_widget_set_margin_start(grid, MARGIN_LARGE);
	gtk_widget_set_margin_bottom(label_w, MARGIN_MEDIUM);
	gtk_widget_set_hexpand(toolbar, TRUE);
	gtk_widget_set_halign(label_w, GTK_ALIGN_START);
	gtk_widget_set_halign(button, GTK_ALIGN_END);
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(box), GTK_SELECTION_NONE);

	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	gtk_container_add(GTK_CONTAINER(buttonitem), buttonbox);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item), 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), buttonitem, 1);
	gtk_container_add(GTK_CONTAINER(frame), box);
	gtk_grid_attach(GTK_GRID(grid), label_w, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), frame, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), toolbar, 0, 2, 1, 1);

	gtk_grid_attach(GTK_GRID(page->grid), grid, 0,
	                page->index++, 2, 1);
	gtk_widget_show_all(page->grid);

	hash_table_set_dual_key(sett->contents, key, subkey, content);

	if(key) {
		struct content_callback *cb;
		cb = create_callback(box, CONTENT_CALLBACK_TYPE_ENTRY_LIST);
		settings_set_callback(page->sett, key, subkey, cb);
	}

	content->data = box;
	g_signal_connect(content->data, "destroy",
	                 G_CALLBACK(free_content), content);
	return box;
}

static void set_entry_value(GtkWidget *entry, GVariantDict *dict,
			    const gchar *key, struct settings_content *content)
{
	GVariant *value_v;
	const gchar *value;

	if(dict) {
		value_v = g_variant_dict_lookup_value(dict, key, NULL);
		value = g_variant_get_string(value_v, NULL);
		gtk_entry_set_text(GTK_ENTRY(entry), value);
		g_variant_unref(value_v);
	}

	g_signal_connect(entry, "changed",
	                 G_CALLBACK(entry_changed), content);
	g_object_set_data(G_OBJECT(entry), "validator", always_valid);
	gtk_widget_set_hexpand(entry, TRUE);
	STYLE_ADD_MARGIN(entry, MARGIN_SMALL);
}

static void set_label_value(GtkWidget *label, GVariantDict *dict,
			    const gchar *key, struct settings_content *content)
{
	GVariant *value_v;
	const gchar *value;

	if(dict) {
		value_v = g_variant_dict_lookup_value(dict, key, NULL);
		value = g_variant_get_string(value_v, NULL);
		gtk_label_set_text(GTK_LABEL(label), value);
		g_variant_unref(value_v);
	}

	gtk_widget_set_hexpand(label, TRUE);
	STYLE_ADD_MARGIN(label, MARGIN_SMALL);
}

void route_ipv_changed(GtkComboBox *box, gpointer user_data)
{
	GtkWidget *netmask_l, *netmask, *prefix_l, *prefix;
	gchar *active;

	netmask_l = g_object_get_data(G_OBJECT(box), "netmask_l");
	netmask = g_object_get_data(G_OBJECT(box), "netmask");
	prefix_l = g_object_get_data(G_OBJECT(box), "prefix_l");
	prefix = g_object_get_data(G_OBJECT(box), "prefix");

	active = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(box));

	if(!strcmp(active, "IPv4")) {
		gtk_widget_show(netmask_l);
		gtk_widget_show(netmask);
		gtk_widget_hide(prefix_l);
		gtk_widget_hide(prefix);
	} else {
		gtk_widget_hide(netmask_l);
		gtk_widget_hide(netmask);
		gtk_widget_show(prefix_l);
		gtk_widget_show(prefix);
	}

	g_free(active);
}

void content_add_route_to_list(GtkWidget *list, GVariant *properties)
{
	struct settings_content *content = g_object_get_data(G_OBJECT(list),
							     "content");
	gboolean labels, ipv4 = TRUE;
	GtkWidget *ipv_l, *network_l, *netmask_l, *prefix_l, *gateway_l,
		  *ipv, *network, *netmask, *prefix, *gateway,
		  *rem, *remgrid, *grid, *row;
	GVariantDict *dict = NULL;

	ipv_l = ipv = rem = remgrid = NULL;
	labels = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(list), "labels"));
	if(!labels) {
		ipv_l = gtk_label_new(_("IP version"));
		ipv = gtk_combo_box_text_new();
	}
	network_l = gtk_label_new(_("Network"));
	netmask_l = gtk_label_new(_("Netmask"));
	prefix_l = gtk_label_new(_("Prefix length"));
	gateway_l = gtk_label_new(_("Gateway"));
	if(!labels) {
		network = gtk_entry_new();
		netmask = gtk_entry_new();
		prefix = gtk_entry_new();
		gateway = gtk_entry_new();
		rem = gtk_button_new_from_icon_name("list-remove-symbolic",
						    GTK_ICON_SIZE_MENU);
		remgrid = gtk_grid_new();
	} else {
		network = gtk_label_new(NULL);
		netmask = gtk_label_new(NULL);
		prefix = gtk_label_new(NULL);
		gateway = gtk_label_new(NULL);
	}
	grid = gtk_grid_new();
	row = gtk_list_box_row_new();

	if(properties)
		dict = g_variant_dict_new(properties);
	if(dict) {
		GVariant *version;

		version = g_variant_dict_lookup_value(dict, "ProtocolFamily",
						      NULL);
		ipv4 = variant_to_bool(version);
		g_variant_unref(version);
	}
	if(!labels) {
		set_entry_value(network, dict, "Network", content);
		set_entry_value(netmask, dict, "Netmask", content);
		set_entry_value(prefix, dict, "Netmask", content);
		set_entry_value(gateway, dict, "Gateway", content);
	} else {
		set_label_value(network, dict, "Network", content);
		set_label_value(netmask, dict, "Netmask", content);
		set_label_value(prefix, dict, "Netmask", content);
		set_label_value(gateway, dict, "Gateway", content);
	}
	if(dict)
		g_variant_dict_unref(dict);

	if(!labels) {
		g_object_set_data(G_OBJECT(ipv), "netmask_l", netmask_l);
		g_object_set_data(G_OBJECT(ipv), "netmask", netmask);
		g_object_set_data(G_OBJECT(ipv), "prefix_l", prefix_l);
		g_object_set_data(G_OBJECT(ipv), "prefix", prefix);
		g_object_set_data(G_OBJECT(row), "destroy", destroy_entry);
		g_object_set_data(G_OBJECT(row), "button", rem);
		g_signal_connect(rem, "clicked", G_CALLBACK(destroy_entry),
				 row);
		g_signal_connect(ipv, "changed", G_CALLBACK(route_ipv_changed),
				 NULL);

		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ipv), "IPv4");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ipv), "IPv6");

		STYLE_ADD_MARGIN(ipv_l, MARGIN_SMALL);
		STYLE_ADD_MARGIN(ipv, MARGIN_SMALL);
	}

	STYLE_ADD_MARGIN(network_l, MARGIN_SMALL);
	STYLE_ADD_MARGIN(netmask_l, MARGIN_SMALL);
	STYLE_ADD_MARGIN(prefix_l, MARGIN_SMALL);
	STYLE_ADD_MARGIN(gateway_l, MARGIN_SMALL);
	gtk_widget_set_hexpand(row, TRUE);

	if(!labels) {
		gtk_widget_set_vexpand(rem, FALSE);
		gtk_widget_set_halign(rem, GTK_ALIGN_END);
		gtk_widget_set_valign(rem, GTK_ALIGN_CENTER);
		gtk_widget_set_valign(remgrid, GTK_ALIGN_CENTER);

		gtk_grid_attach(GTK_GRID(grid), ipv_l, 0, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), ipv, 1, 0, 1, 1);
	}

	gtk_grid_attach(GTK_GRID(grid), network_l, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), network, 1, 1, 1, 1);
	if(!labels || ipv4) {
		gtk_grid_attach(GTK_GRID(grid), netmask_l, 0, 2, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), netmask, 1, 2, 1, 1);
	}
	if(!labels || !ipv4) {
		gtk_grid_attach(GTK_GRID(grid), prefix_l, 0, 3, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), prefix, 1, 3, 1, 1);
	}
	gtk_grid_attach(GTK_GRID(grid), gateway_l, 0, 4, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), gateway, 1, 4, 1, 1);
	if(!labels) {
		gtk_grid_attach(GTK_GRID(remgrid), rem, 0, 0, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), remgrid, 2, 2, 1, 2);
	}
	gtk_container_add(GTK_CONTAINER(row), grid);
	gtk_widget_show_all(row);
	gtk_container_add(GTK_CONTAINER(list), row);

	if(!labels && ipv4)
		gtk_combo_box_set_active(GTK_COMBO_BOX(ipv), 0);
	else if(!labels)
		gtk_combo_box_set_active(GTK_COMBO_BOX(ipv), 1);

	int count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(list), "count"));
	count++;
	g_object_set_data(G_OBJECT(list), "count", GINT_TO_POINTER(count));
	if(!labels)
		update_button_visibility(list);
}

static void add_route_to_list(GtkButton *button, gpointer user_data)
{
	GtkWidget *list = user_data;
	content_add_route_to_list(list, NULL);
}

GtkWidget *settings_add_route_list(struct settings *sett,
				   struct settings_page *page,
				   const gchar *key, gboolean labels,
				   settings_writable writable)
{
	struct settings_content *content;
	GtkWidget *grid, *box, *toolbar, *frame, *buttonbox, *button;
	GtkToolItem *item, *buttonitem;
	GVariantIter *iter;
	GVariant *values, *value;
	gboolean empty = TRUE;

	content = create_base_content(sett, writable, NULL, NULL, NULL);
	box = gtk_list_box_new();
	toolbar = gtk_toolbar_new();
	frame = gtk_frame_new(NULL);
	button = gtk_button_new_from_icon_name("list-add-symbolic",
	                                       GTK_ICON_SIZE_MENU);
	/* needs to be a box and not a grid for inline-toolbar css */
	buttonbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	grid = gtk_grid_new();
	item = gtk_separator_tool_item_new();
	buttonitem = gtk_tool_item_new();

	content->value = content_value_entry_list;

	g_object_set_data(G_OBJECT(box), "labels", GINT_TO_POINTER(labels));
	g_object_set_data(G_OBJECT(box), "count", GINT_TO_POINTER(0));
	g_object_set_data(G_OBJECT(box), "content", content);
	g_signal_connect(button, "clicked", G_CALLBACK(add_route_to_list), box);

	gtk_style_context_add_class(gtk_widget_get_style_context(toolbar),
	                            GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_tool_item_set_expand(item, TRUE);
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), FALSE);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);

	gtk_widget_set_margin_start(grid, MARGIN_LARGE);
	gtk_widget_set_hexpand(toolbar, TRUE);
	gtk_widget_set_halign(button, GTK_ALIGN_END);
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(box), GTK_SELECTION_NONE);

	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	gtk_container_add(GTK_CONTAINER(buttonitem), buttonbox);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(item), 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), buttonitem, 1);
	gtk_container_add(GTK_CONTAINER(frame), box);
	gtk_grid_attach(GTK_GRID(grid), frame, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), toolbar, 0, 2, 1, 1);

	gtk_grid_attach(GTK_GRID(page->grid), grid, 0,
	                page->index++, 2, 1);
	gtk_widget_show_all(page->grid);

	if(labels)
		gtk_widget_hide(toolbar);

	values = service_get_property(sett->serv, key, NULL);
	iter = g_variant_iter_new(values);

	while(g_variant_iter_loop(iter, "v", &value)) {
		empty = FALSE;
		content_add_route_to_list(box, value);
	}
	if(empty)
		content_add_route_to_list(box, NULL);

	g_variant_iter_free(iter);
	g_variant_unref(values);

	hash_table_set_dual_key(sett->contents, key, NULL, content);

	if(key) {
		struct content_callback *cb;
		cb = create_callback(box, CONTENT_CALLBACK_TYPE_ROUTE_LIST);
		settings_set_callback(page->sett, key, NULL, cb);
	}

	content->data = box;
	g_signal_connect(content->data, "destroy",
	                 G_CALLBACK(free_content), content);
	return box;
}
