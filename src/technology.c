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

#include <locale.h>
#include <string.h>

#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "main.h"
#include "connection.h"
#include "style.h"
#include "technology.h"
#include "connections/bluetooth.h"
#include "connections/cellular.h"
#include "connections/ethernet.h"
#include "connections/p2p.h"
#include "connections/vpn.h"
#include "connections/wireless.h"

static struct {
	void (*init)(struct technology *tech, GVariant *properties,
	             GDBusProxy *proxy);
	struct technology *(*create)(void);
	void (*free)(struct technology *tech);
	void (*services_updated)(struct technology *tech);
	void (*property_changed)(struct technology *tech, const gchar *name);
} functions[CONNECTION_TYPE_COUNT] = {
	{},
	{technology_ethernet_init},
	{
		technology_wireless_init, technology_wireless_create,
		technology_wireless_free
	},
	{technology_bluetooth_init},
	{technology_cellular_init},
	{},
	{technology_vpn_init}
};

static struct technology_list_item *create_list_item(struct technology *tech,
                const gchar *name)
{
	GtkWidget *grid;
	struct technology_list_item *item;

	item = g_malloc(sizeof(*item));
	item->technology = tech;

	grid = gtk_grid_new();
	item->item = gtk_list_box_row_new();
	item->icon = gtk_image_new_from_icon_name("network-transmit-symbolic",
	                GTK_ICON_SIZE_LARGE_TOOLBAR);
	item->label = gtk_label_new(name);

	g_object_ref(item->item);
	g_object_ref(item->icon);
	g_object_ref(item->label);

	g_object_set_data(G_OBJECT(item->item), "content",
	                  tech->settings->grid);

	STYLE_ADD_MARGIN(grid, MARGIN_SMALL);
	gtk_widget_set_margin_start(item->label, MARGIN_SMALL);

	gtk_container_add(GTK_CONTAINER(grid), item->icon);
	gtk_container_add(GTK_CONTAINER(grid), item->label);
	gtk_container_add(GTK_CONTAINER(item->item), grid);

	gtk_widget_show_all(item->item);
	return item;
}

static void free_list_item(struct technology_list_item *item)
{
	if(!item)
		return;
	g_object_unref(item->icon);
	g_object_unref(item->label);
	g_object_unref(item->item);
	gtk_widget_destroy(item->item);

	g_free(item);
}

static gboolean toggle_power(GtkSwitch *widget, GParamSpec *pspec,
                             gpointer user_data)
{
	struct technology_settings *item = user_data;
	gboolean state = gtk_switch_get_active(widget);

	technology_set_property(item->technology, "Powered",
				g_variant_new("b", state));
	return TRUE;
}

static void update_status(struct technology_settings *item)
{
	gboolean connected = g_variant_get_boolean(g_hash_table_lookup(
	                             item->properties, "Connected"));
	gboolean powered = g_variant_get_boolean(g_hash_table_lookup(
	                           item->properties, "Powered"));
	if(connected)
		gtk_label_set_text(GTK_LABEL(item->status),
		                   _("Connected"));
	else {
		if(powered) {
			gtk_label_set_text(GTK_LABEL(item->status),
			                   _("Not connected"));
			gtk_widget_show(item->buttons);
		} else {
			gtk_label_set_text(GTK_LABEL(item->status),
			                   _("Disabled"));
			gtk_widget_hide(item->buttons);
		}
	}
}

static void update_power(struct technology_settings *item)
{
	gboolean powered = g_variant_get_boolean(g_hash_table_lookup(
	                           item->properties, "Powered"));

	g_signal_handler_block(G_OBJECT(item->power_switch),
	                       item->powersig);
	gtk_switch_set_active(GTK_SWITCH(item->power_switch),
	                      powered);
	g_signal_handler_unblock(G_OBJECT(item->power_switch),
	                         item->powersig);
	update_status(item);
}

static void handle_proxy_signal(GDBusProxy *proxy, gchar *sender,
                                gchar *signal, GVariant *parameters,
                                gpointer user_data)
{
	struct technology_settings *item = user_data;
	if(!strcmp(signal, "PropertyChanged")) {
		GVariant *name_v, *value_v, *value;
		gchar *name;
		name_v = g_variant_get_child_value(parameters, 0);
		value_v = g_variant_get_child_value(parameters, 1);
		value = g_variant_get_child_value(value_v, 0);
		name = g_variant_dup_string(name_v, NULL);
		g_hash_table_replace(item->properties, name, value);
		technology_property_changed(item->technology, name);

		g_variant_unref(name_v);
		g_variant_unref(value_v);
	}
}

static void update_service_separator(GtkListBoxRow *row, GtkListBoxRow *before,
                                     gpointer user_data)
{
	GtkWidget *cur;
	if(!before) {
		gtk_list_box_row_set_header(row, NULL);
		return;
	}
	cur = gtk_list_box_row_get_header(row);
	if(!cur) {
		cur = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
		gtk_widget_show(cur);
		gtk_list_box_row_set_header(row, cur);
	}
}

static void connect_button_cb(GtkButton *widget, gpointer user_data)
{
	struct technology_settings *tech = user_data;
	if(tech->selected)
		service_toggle_connection(tech->selected);
}

static void tether_button_cb(GtkButton *widget, gpointer user_data)
{
	struct technology_settings *tech = user_data;
	GVariant *state_v = g_hash_table_lookup(tech->properties, "Tethering");
	gboolean state = variant_to_bool(state_v);
	if(tech->technology->type != CONNECTION_TYPE_WIRELESS || state) {
		technology_set_property(tech->technology, "Tethering",
					g_variant_new("b", !state));
	}
	else
		technology_wireless_tether(tech->technology);
}

static void update_tethering(struct technology_settings *tech)
{

	GVariant *state_v = g_hash_table_lookup(tech->properties, "Tethering");
	gboolean state = variant_to_bool(state_v);
	GtkButton *button = GTK_BUTTON(tech->tethering);
	if(state)
		gtk_button_set_label(button, _("Disable _tethering"));
	else
		gtk_button_set_label(button, _("Enable _tethering"));
}

static void update_connect_button(struct technology_settings *tech)
{
	const gchar *button_state;
	gchar *state;
	if(!tech->selected) {
		if(!shutting_down)
			gtk_widget_set_sensitive(tech->connect_button, FALSE);
		gtk_widget_set_can_focus(tech->connect_button, FALSE);
		return;
	}
	state = service_get_property_string_raw(tech->selected, "State", NULL);

	gtk_widget_set_sensitive(tech->connect_button, TRUE);
	gtk_widget_set_can_focus(tech->connect_button, TRUE);
	if(!strcmp(state, "idle") || !strcmp(state, "disconnect"))
		button_state = _("_Connect");
	else if(!strcmp(state, "failure"))
		button_state = _("Re_connect");
	else
		button_state = _("Dis_connect");
	gtk_button_set_label(GTK_BUTTON(tech->connect_button), button_state);

	g_free(state);
}

static void service_selected(GtkListBox *box, GtkListBoxRow *row,
                             gpointer user_data)
{
	struct technology_settings *tech = user_data;
	struct service *serv = NULL;
	if(row)
		serv = g_object_get_data(G_OBJECT(row), "service");
	tech->selected = serv;
	update_connect_button(tech);
}

struct technology_settings *create_technology_settings(struct technology *tech,
                GVariant *properties, GDBusProxy *proxy)
{
	struct technology_settings *item = g_malloc(sizeof(*item));

	GVariantIter *iter;
	gchar *key;
	GVariant *value;
	const gchar *name;
	GtkWidget *powerbox, *frame, *scrolled_window;

	item->technology = tech;
	item->properties = g_hash_table_new_full(g_str_hash, g_str_equal,
	                   g_free, (GDestroyNotify)g_variant_unref);
	item->selected = NULL;

	iter = g_variant_iter_new(properties);
	while(g_variant_iter_loop(iter, "{sv}", &key, &value)) {
		gchar *hkey = g_strdup(key);
		g_variant_ref(value);
		g_hash_table_insert(item->properties, hkey, value);
	}
	g_variant_iter_free(iter);

	name = g_variant_get_string(g_hash_table_lookup(item->properties,
	                            "Name"), NULL);

	item->proxy = proxy;
	g_signal_connect(proxy, "g-signal", G_CALLBACK(handle_proxy_signal),
	                 item);

	item->grid = gtk_grid_new();
	item->icon = gtk_image_new_from_icon_name("preferences-system-network",
	                GTK_ICON_SIZE_DIALOG);
	item->title = gtk_label_new(name);
	item->status = gtk_label_new(NULL);
	item->power_switch = gtk_switch_new();
	item->contents = gtk_grid_new();
	item->services = gtk_list_box_new();
	item->buttons = gtk_grid_new();
	item->connect_button = gtk_button_new_with_mnemonic(_("_Connect"));
	item->filler = gtk_label_new(NULL);
	powerbox = gtk_grid_new();
	frame = gtk_frame_new(NULL);
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	item->tethering = gtk_button_new_with_mnemonic(_("Enable _tethering"));

	g_object_ref(item->grid);
	g_object_ref(item->icon);
	g_object_ref(item->title);
	g_object_ref(item->status);
	g_object_ref(item->power_switch);
	g_object_ref(item->contents);
	g_object_ref(item->services);
	g_object_ref(item->buttons);
	g_object_ref(item->connect_button);
	g_object_ref(item->filler);
	g_object_ref(item->tethering);

	item->powersig = g_signal_connect(item->power_switch, "notify::active",
	                                  G_CALLBACK(toggle_power), item);
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(item->services),
	                                GTK_SELECTION_SINGLE);
	gtk_list_box_set_header_func(GTK_LIST_BOX(item->services),
	                             update_service_separator, NULL, NULL);
	g_signal_connect(item->services, "row-selected",
	                 G_CALLBACK(service_selected), item);
	g_signal_connect(item->connect_button, "clicked",
	                 G_CALLBACK(connect_button_cb), item);
	g_signal_connect(item->tethering, "clicked",
			 G_CALLBACK(tether_button_cb), item);

	gtk_widget_set_margin_start(item->grid, MARGIN_LARGE);
	gtk_widget_set_margin_end(item->grid, MARGIN_LARGE);
	gtk_widget_set_margin_start(item->title, MARGIN_MEDIUM);
	gtk_widget_set_margin_end(item->title, MARGIN_MEDIUM);
	STYLE_ADD_CONTEXT(item->title);
	gtk_style_context_add_class(gtk_widget_get_style_context(item->title),
	                            "cm-header-title");
	gtk_widget_set_margin_start(item->status, MARGIN_MEDIUM);
	gtk_widget_set_margin_end(item->status, MARGIN_MEDIUM);
	gtk_widget_set_margin_top(item->contents, MARGIN_LARGE);
	gtk_widget_set_margin_top(item->buttons, MARGIN_SMALL);

	gtk_widget_set_hexpand(item->title, TRUE);
	gtk_widget_set_hexpand(item->status, TRUE);
	gtk_widget_set_vexpand(powerbox, FALSE);
	gtk_widget_set_hexpand(item->contents, TRUE);
	gtk_widget_set_vexpand(item->contents, TRUE);
	gtk_widget_set_vexpand(scrolled_window, TRUE);
	gtk_widget_set_hexpand(item->filler, TRUE);
	gtk_widget_set_hexpand(item->buttons, TRUE);

	gtk_widget_set_halign(item->icon, GTK_ALIGN_START);
	gtk_widget_set_halign(item->title, GTK_ALIGN_START);
	gtk_widget_set_halign(item->status, GTK_ALIGN_START);
	gtk_widget_set_halign(item->power_switch, GTK_ALIGN_END);
	gtk_widget_set_valign(item->power_switch, GTK_ALIGN_START);
	gtk_widget_set_valign(powerbox, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(powerbox, GTK_ALIGN_END);
	gtk_widget_set_valign(item->buttons, GTK_ALIGN_END);
	gtk_widget_set_halign(item->tethering, GTK_ALIGN_START);
	gtk_widget_set_halign(item->connect_button, GTK_ALIGN_END);

	gtk_grid_attach(GTK_GRID(powerbox), item->power_switch, 0, 0, 1, 1);
	gtk_container_add(GTK_CONTAINER(scrolled_window), item->services);
	gtk_container_add(GTK_CONTAINER(frame), scrolled_window);
	gtk_grid_attach(GTK_GRID(item->contents), frame, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(item->buttons), item->tethering, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(item->buttons), item->filler, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(item->buttons), item->connect_button,
	                2, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(item->grid), item->icon, 0, 0, 1, 2);
	gtk_grid_attach(GTK_GRID(item->grid), item->title,1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(item->grid), item->status, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(item->grid), powerbox, 2, 0, 1, 2);
	gtk_grid_attach(GTK_GRID(item->grid), item->contents,0, 2, 3, 1);
	gtk_grid_attach(GTK_GRID(item->grid), item->buttons, 0, 3, 3, 1);

	gtk_widget_show_all(item->grid);

	update_connect_button(item);
	update_status(item);
	update_power(item);
	update_tethering(item);
	return item;
}

void free_technology_settings(struct technology_settings *item)
{
	if(!item)
		return;

	g_object_unref(item->icon);
	g_object_unref(item->title);
	g_object_unref(item->status);
	g_object_unref(item->power_switch);
	g_object_unref(item->contents);
	g_object_unref(item->services);
	g_object_unref(item->buttons);
	g_object_unref(item->connect_button);
	g_object_unref(item->grid);
	gtk_widget_destroy(item->grid);

	g_object_unref(item->proxy);
	g_hash_table_unref(item->properties);

	g_free(item);
}


void technology_property_changed(struct technology *item, const gchar *key)
{
	if(!strcmp(key, "Powered"))
		update_power(item->settings);
	else if(!strcmp(key, "Connected"))
		update_status(item->settings);
	else if(!strcmp(key, "Tethering"))
		update_tethering(item->settings);

	if(functions[item->type].property_changed)
		functions[item->type].property_changed(item, key);
}

void technology_services_updated(struct technology *item)
{
	int count = g_hash_table_size(item->services);
	if(count) {
		gtk_widget_show(item->settings->contents);
		gtk_widget_show(item->settings->connect_button);
	} else {
		gtk_widget_hide(item->settings->contents);
		gtk_widget_hide(item->settings->connect_button);
	}
	if(functions[item->type].services_updated)
		functions[item->type].services_updated(item);
}

void technology_add_service(struct technology *item, struct service *serv)
{
	gtk_container_add(GTK_CONTAINER(item->settings->services), serv->item);
	g_hash_table_insert(item->services, g_strdup(serv->path), serv);
	technology_services_updated(item);
}

void technology_update_service(struct technology *item, struct service *serv,
                               GVariant *properties)
{
	service_update(serv, properties);
	if(item->settings->selected == serv);
	update_connect_button(item->settings);
	technology_services_updated(item);
}

void technology_remove_service(struct technology *item, const gchar *path)
{
	if(item->settings->selected == g_hash_table_lookup(item->services,
	                path)) {
		item->settings->selected = NULL;
		update_connect_button(item->settings);
	}
	g_hash_table_remove(item->services, path);
	technology_services_updated(item);
}

void technology_free(struct technology *item)
{
	if(!item)
		return;
	free_list_item(item->list_item);
	free_technology_settings(item->settings);
	g_hash_table_unref(item->services);
	g_free(item->path);
	if(functions[item->type].free)
		functions[item->type].free(item);
	else
		g_free(item);
}

void technology_init(struct technology *tech, GVariant *properties_v,
                     GDBusProxy *proxy)
{
	GVariant *name_v;
	const gchar *name;
	GVariant *type_v;
	const gchar *type;
	GVariantDict *properties;

	properties = g_variant_dict_new(properties_v);
	name_v = g_variant_dict_lookup_value(properties, "Name", NULL);
	name = g_variant_get_string(name_v, NULL);
	type_v = g_variant_dict_lookup_value(properties, "Type", NULL);
	type = g_variant_get_string(type_v, NULL);
	g_variant_dict_unref(properties);

	tech->services = g_hash_table_new_full(g_str_hash, g_str_equal,
	                                       g_free, NULL);
	tech->settings = create_technology_settings(tech, properties_v,
	                 proxy);
	tech->list_item = create_list_item(tech, name);
	technology_services_updated(tech);
	tech->type = connection_type_from_string(type);
	g_object_set_data(G_OBJECT(tech->list_item->item), "technology-type",
	                  &tech->type);

	g_variant_unref(name_v);
	g_variant_unref(type_v);
}

struct technology *technology_create(GDBusProxy *proxy, const gchar *path,
                                     GVariant *properties)
{
	struct technology *item;
	GVariantDict *properties_d;
	GVariant *type_v;
	enum connection_type type;

	properties_d = g_variant_dict_new(properties);
	type_v = g_variant_dict_lookup_value(properties_d, "Type", NULL);
	type = connection_type_from_string(g_variant_get_string(type_v, NULL));
	g_variant_unref(type_v);
	g_variant_dict_unref(properties_d);

	if(functions[type].create)
		item = functions[type].create();
	else
		item = g_malloc(sizeof(*item));
	item->type = type;
	item->path = g_strdup(path);

	technology_init(item, properties, proxy);
	if(functions[type].init)
		functions[type].init(item, properties, proxy);

	return item;
}

void technology_set_property(struct technology *tech, const gchar *key,
			     GVariant *value)
{
	GVariant *ret;
	GError *error = NULL;

	ret = g_dbus_proxy_call_sync(tech->settings->proxy, "SetProperty",
	                             g_variant_new("(sv)", key, value),
	                             G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if(error) {
		g_warning("failed to set technology property %s: %s",
		          key, error->message);
		g_error_free(error);
		return;
	}
	g_variant_unref(ret);
}
