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

#include "connection.h"
#include "service.h"
#include "settings.h"
#include "settings_content.h"
#include "settings_content_callback.h"
#include "style.h"
#include "util.h"

static struct {
	struct settings *(*create)(void);
	void (*free)(struct settings *sett);
	void (*init)(struct settings *sett);
	void (*property_changed)(struct settings *tech, const gchar *name);
} functions[CONNECTION_TYPE_COUNT] = {
	{}
};

static void page_selected(GtkListBox *box, GtkListBoxRow *row, gpointer data)
{
	if(!G_IS_OBJECT(row))
		return;
	struct settings *sett = data;
	struct settings_page *page = g_object_get_data(G_OBJECT(row), "content");
	GtkWidget *content = page->grid;
	gint num = gtk_notebook_page_num(GTK_NOTEBOOK(sett->notebook),
	                                 content);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(sett->notebook), num);
}

void free_page(GtkWidget *widget, gpointer user_data)
{
	struct settings_page *page = user_data;
	g_free(page);
}

struct settings_page *settings_add_page(struct settings *sett,
                                        const gchar *name)
{
	struct settings_page *page = g_malloc(sizeof(*page));

	page->index = 0;
	page->sett = sett;

	page->grid = gtk_grid_new();
	GtkWidget *item = gtk_list_box_row_new();
	GtkWidget *label = gtk_label_new(name);

	g_object_set_data(G_OBJECT(item), "content", page);

	g_signal_connect(page->grid, "destroy",
	                 G_CALLBACK(free_page), page);

	STYLE_ADD_MARGIN(label, MARGIN_SMALL);
	gtk_widget_set_margin_start(page->grid, MARGIN_LARGE);
	gtk_widget_set_margin_start(label, MARGIN_LARGE);

	gtk_widget_set_halign(label, GTK_ALIGN_START);

	gtk_container_add(GTK_CONTAINER(item), label);
	gtk_container_add(GTK_CONTAINER(sett->list), item);

	gtk_widget_show_all(page->grid);

	gtk_notebook_append_page(GTK_NOTEBOOK(sett->notebook), page->grid,
	                         NULL);

	return page;
}

void settings_free(struct settings *sett)
{
	sett->closed(sett->serv);
	g_hash_table_unref(sett->callbacks);
	if(functions[sett->serv->type].free)
		functions[sett->serv->type].free(sett);
	else
		g_free(sett);
}

static void append_variant(GVariantDict *dict, struct settings_content *content)
{
	if(!content->key)
		return;
	GVariant *variant = content->value(content);
	if(!variant)
		return;
	if(!content->subkey)
		goto end;
	GVariant *subdict_v;
	subdict_v = g_variant_dict_lookup_value(dict, content->key, NULL);
	if(!subdict_v)
		subdict_v = g_variant_new("a{sv}", NULL);
	GVariantDict *subdict = g_variant_dict_new(subdict_v);
	g_variant_unref(subdict_v);

	g_variant_dict_insert_value(subdict, content->subkey, variant);
	variant = g_variant_dict_end(subdict);
	g_variant_dict_unref(subdict);
end:
	g_variant_dict_insert_value(dict, content->key, variant);
}

static void append_values(GVariantDict *dict, struct settings_page *page)
{
	GList *children = gtk_container_get_children(GTK_CONTAINER(page->grid));
	GList *l;
	for(l = children; l != NULL; l = l->next) {
		GtkWidget *child = l->data;
		if(!g_object_get_data(G_OBJECT(child), "content"))
			continue;
		struct settings_content *content =
		        g_object_get_data(G_OBJECT(child), "content");
		append_variant(dict, content);
	}
	g_list_free(children);
}

static void add_info_page(struct settings *sett)
{
	struct settings_page *page = settings_add_page(sett, _("Info"));

	settings_add_switch(page, _("Autoconnect"), "AutoConnect", NULL);

	settings_add_text(page, _("Name"), "Name", NULL);
	settings_add_text(page, _("State"), "State", NULL);
	settings_add_text(page, _("MAC address"), "Ethernet", "Address");
	settings_add_text(page, _("Interface"), "Ethernet", "Interface");
	settings_add_text(page, _("IPv4 address"), "IPv4", "Address");
	settings_add_text(page, _("IPv6 address"), "IPv6", "Address");
	settings_add_text(page, _("IPv6 gateway"), "IPv6", "Gateway");
	settings_add_text(page, _("IPv6 netmask"), "IPv6", "Netmask");
	settings_add_text(page, _("Nameservers"), "Nameservers", NULL);
}

static gboolean valid_ipv4_entry(struct settings_content *content)
{
	GVariant *variant = content->value(content);
	gboolean valid = valid_ipv4(g_variant_get_string(variant, NULL));
	g_variant_unref(variant);
	return valid;
}

static gboolean valid_ipv6_entry(struct settings_content *content)
{
	GVariant *variant = content->value(content);
	gboolean valid = valid_ipv6(g_variant_get_string(variant, NULL));
	g_variant_unref(variant);
	return valid;
}

static void add_ipv_page(struct settings *sett, int ipv)
{
	struct settings_page *page;
	settings_field_validator validator;
	const char *conf;
	const char *ipvs;

	if(ipv == 4) {
		page = settings_add_page(sett, _("IPv4"));
		validator = valid_ipv4_entry;
		conf = "IPv4.Configuration";
		ipvs = "IPv4";
	}
	else {
		page = settings_add_page(sett, _("IPv6"));
		validator = valid_ipv6_entry;
		conf = "IPv6.Configuration";
		ipvs = "IPv6";
	}

	settings_add_text(page, _("Address"), ipvs, "Address");
	settings_add_text(page, _("Gateway"), ipvs, "Gateway");
	settings_add_text(page, _("Netmask"), ipvs, "Netmask");
	settings_add_entry(page, _("Method"), ipvs, "Method",
			   conf, "Method", settings_content_valid_always);
	settings_add_entry(page, _("Address"), ipvs, "Address",
			   conf, "Address", validator);
	settings_add_entry(page, _("Netmask"), ipvs, "Netmask",
			   conf, "Netmask", validator);
	settings_add_entry(page, _("Gateway"), ipvs, "Gateway",
			   conf, "Gateway", validator);
}

static void apply_cb(GtkWidget *window, gpointer user_data)
{
	struct settings *sett = user_data;
	GtkWidget *list = sett->list;
	GVariantDict *dict = g_variant_dict_new(NULL);
	GVariant *out;
	GList *children = gtk_container_get_children(GTK_CONTAINER(list));
	GList *l;
	for(l = children; l != NULL; l = l->next) {
		GtkWidget *child = l->data;
		struct settings_page *page = g_object_get_data(G_OBJECT(child),
		                             "content");
		append_values(dict, page);
	}
	g_list_free(children);
	out = g_variant_dict_end(dict);
	service_set_properties(sett->serv, out);
	g_variant_unref(out);
	g_variant_dict_unref(dict);
}

static gboolean delete_event(GtkWidget *window, GdkEvent *event,
                             gpointer user_data)
{
	settings_free(user_data);
	return FALSE;
}

void settings_init(struct settings *sett)
{
	GVariant *name_v;
	gchar *title;
	GtkWidget *frame, *apply;
	GtkGrid *grid = GTK_GRID(gtk_grid_new());

	sett->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	name_v = service_get_property(sett->serv, "Name", NULL);
	title = g_strdup_printf("%s - %s", _("Network Settings"),
	                        g_variant_get_string(name_v, NULL));
	gtk_window_set_title(GTK_WINDOW(sett->window), title);
	gtk_window_set_default_size(GTK_WINDOW(sett->window), SETTINGS_WIDTH,
	                            SETTINGS_HEIGHT);

	sett->list = gtk_list_box_new();
	sett->notebook = gtk_notebook_new();
	frame = gtk_frame_new(NULL);
	apply = gtk_button_new_with_mnemonic(_("_Apply"));

	g_object_ref(sett->window);
	g_object_ref(sett->list);
	g_object_ref(sett->notebook);

	g_signal_connect(sett->window, "delete-event", G_CALLBACK(delete_event),
	                 sett);
	g_signal_connect(sett->list, "row-selected",
	                 G_CALLBACK(page_selected), sett);
	g_signal_connect(apply, "clicked",
	                 G_CALLBACK(apply_cb), sett);

	STYLE_ADD_MARGIN(GTK_WIDGET(grid), MARGIN_LARGE);
	gtk_widget_set_margin_top(apply, MARGIN_LARGE);

	gtk_widget_set_size_request(sett->list, SETTINGS_LIST_WIDTH, -1);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(sett->notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(sett->notebook), FALSE);
	gtk_widget_set_vexpand(sett->list, TRUE);
	gtk_widget_set_hexpand(sett->notebook, TRUE);
	gtk_widget_set_vexpand(sett->notebook, TRUE);
	gtk_widget_set_valign(apply, GTK_ALIGN_END);
	gtk_widget_set_halign(apply, GTK_ALIGN_END);

	gtk_container_add(GTK_CONTAINER(frame), sett->list);
	gtk_grid_attach(grid, frame, 0, 0, 1, 2);
	gtk_grid_attach(grid, sett->notebook, 1, 0, 1, 1);
	gtk_grid_attach(grid, apply, 1, 1, 1, 1);
	gtk_container_add(GTK_CONTAINER(sett->window), GTK_WIDGET(grid));

	add_info_page(sett);
	add_ipv_page(sett, 4);
	add_ipv_page(sett, 6);
	settings_add_page(sett, _("Security"));

	if(functions[sett->serv->type].init)
		functions[sett->serv->type].init(sett);

	gtk_widget_show_all(sett->window);

	g_free(title);
	g_variant_unref(name_v);
}

struct settings *settings_create(struct service *serv,
                                 void (*closed)(struct service *serv))
{
	struct settings *sett;
	GHashTable *t;

	if(functions[serv->type].create)
		sett = functions[serv->type].create();
	else
		sett = g_malloc(sizeof(*sett));

	sett->serv = serv;
	sett->closed = closed;
	t = g_hash_table_new_full(g_str_hash, g_str_equal,
	                          g_free, (GDestroyNotify)g_hash_table_unref);
	sett->callbacks = t;

	settings_init(sett);

	return sett;
}

void settings_update(struct settings *sett, const gchar *key,
                     const gchar *subkey, GVariant *value)
{
	GHashTable *t = g_hash_table_lookup(sett->callbacks, key);
	if(!subkey)
		subkey = "";
	if(t && g_hash_table_contains(t, subkey))
		handle_content_callback(value, key, subkey,
					g_hash_table_lookup(t, subkey));
}

void settings_set_callback(struct settings *sett, const gchar *key,
                           const gchar *subkey, struct content_callback *cb)
{
	GHashTable *t;
	if(!g_hash_table_contains(sett->callbacks, key)) {
		t = g_hash_table_new_full(g_str_hash, g_str_equal,
		                          g_free,
		                          content_callback_free);
		g_hash_table_insert(sett->callbacks, g_strdup(key), t);
	} else
		t = g_hash_table_lookup(sett->callbacks, key);
	g_hash_table_insert(t, g_strdup(subkey ? subkey : ""), cb);
}

