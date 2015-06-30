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

#include <gio/gio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "ethernet.h"
#include "technology.h"
#include "service.h"
#include "style.h"

struct ethernet_technology {
	struct technology parent;
};

struct ethernet_service {
	struct service parent;

	GtkGrid *header;
	GtkWidget *interface;

	GtkGrid *properties;
	GtkWidget *ipv4label;
	GtkWidget *ipv4;
	GtkWidget *ipv4gatewaylabel;
	GtkWidget *ipv4gateway;
	GtkWidget *ipv6label;
	GtkWidget *ipv6;
	GtkWidget *ipv6gatewaylabel;
	GtkWidget *ipv6gateway;
	GtkWidget *maclabel;
	GtkWidget *mac;
};

struct technology *technology_ethernet_create(void) {
	struct ethernet_technology *item = g_malloc(sizeof(*item));
	return (struct technology *)item;
}

void technology_ethernet_init(struct technology *item, GVariant *properties,
		GDBusProxy *proxy) {
	struct ethernet_technology *tech = (struct ethernet_technology *)item;
	gtk_image_set_from_icon_name(GTK_IMAGE(tech->parent.list_item->icon),
			"network-wired-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_image_set_from_icon_name(GTK_IMAGE(tech->parent.settings->icon),
			"network-wired", GTK_ICON_SIZE_DIALOG);
}

void technology_ethernet_free(struct technology *tech) {
	technology_free(tech);
}

void service_ethernet_init(struct service *serv, GDBusProxy *proxy,
		const gchar *path, GVariant *properties) {
	struct ethernet_service *item = (struct ethernet_service *)serv;
	item->header = GTK_GRID(gtk_grid_new());
	g_object_ref(item->header);
	item->interface = gtk_label_new("Interface");
	g_object_ref(item->interface);
	gtk_grid_attach(item->header, item->interface,  0, 0, 1, 1);

	item->properties = GTK_GRID(gtk_grid_new());
	g_object_ref(item->properties);
	STYLE_ADD_MARGIN(GTK_WIDGET(item->properties), MARGIN_LARGE);
	gtk_grid_set_column_homogeneous(item->properties, TRUE);

	item->ipv4label = gtk_label_new(_("IPv4 address"));
	item->ipv4gatewaylabel = gtk_label_new(_("IPv4 gateway"));
	item->ipv6label = gtk_label_new(_("IPv6 address"));
	item->ipv6gatewaylabel = gtk_label_new(_("IPv6 gateway"));
	item->maclabel = gtk_label_new(_("MAC address"));
	item->ipv4 = gtk_label_new(NULL);
	item->ipv4gateway = gtk_label_new(NULL);
	item->ipv6 = gtk_label_new(NULL);
	item->ipv6gateway = gtk_label_new(NULL);
	item->mac = gtk_label_new(NULL);

	g_object_ref(item->ipv4label);
	g_object_ref(item->ipv4gatewaylabel);
	g_object_ref(item->ipv6label);
	g_object_ref(item->ipv6gatewaylabel);
	g_object_ref(item->maclabel);
	g_object_ref(item->ipv4);
	g_object_ref(item->ipv4gateway);
	g_object_ref(item->ipv6);
	g_object_ref(item->ipv6gateway);
	g_object_ref(item->mac);

	gtk_widget_set_halign(item->ipv4label, GTK_ALIGN_START);
	gtk_widget_set_halign(item->ipv4gatewaylabel, GTK_ALIGN_START);
	gtk_widget_set_halign(item->ipv6label, GTK_ALIGN_START);
	gtk_widget_set_halign(item->ipv6gatewaylabel, GTK_ALIGN_START);
	gtk_widget_set_halign(item->maclabel, GTK_ALIGN_START);
	gtk_widget_set_halign(item->ipv4, GTK_ALIGN_START);
	gtk_widget_set_halign(item->ipv4gateway, GTK_ALIGN_START);
	gtk_widget_set_halign(item->ipv6, GTK_ALIGN_START);
	gtk_widget_set_halign(item->ipv6gateway, GTK_ALIGN_START);
	gtk_widget_set_halign(item->mac, GTK_ALIGN_START);

	gtk_grid_attach(item->properties, item->ipv4label,	  0, 0, 1, 1);
	gtk_grid_attach(item->properties, item->ipv4gatewaylabel, 0, 1, 1, 1);
	gtk_grid_attach(item->properties, item->ipv6label,	  0, 2, 1, 1);
	gtk_grid_attach(item->properties, item->ipv6gatewaylabel, 0, 3, 1, 1);
	gtk_grid_attach(item->properties, item->maclabel,	  0, 4, 1, 1);
	gtk_grid_attach(item->properties, item->ipv4,		  1, 0, 1, 1);
	gtk_grid_attach(item->properties, item->ipv4gateway,	  1, 1, 1, 1);
	gtk_grid_attach(item->properties, item->ipv6,		  1, 2, 1, 1);
	gtk_grid_attach(item->properties, item->ipv6gateway,	  1, 3, 1, 1);
	gtk_grid_attach(item->properties, item->mac,		  1, 4, 1, 1);

	gtk_grid_attach(GTK_GRID(serv->contents), GTK_WIDGET(item->header),
			0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(serv->contents), GTK_WIDGET(item->properties),
			0, 1, 1, 1);

	gtk_widget_show_all(serv->contents);

	service_ethernet_update(serv);
}

struct service *service_ethernet_create(void) {
	struct ethernet_service *serv = g_malloc(sizeof(*serv));
	return (struct service *)serv;
}

void service_ethernet_free(struct service *serv) {
	struct ethernet_service *item = (struct ethernet_service *)serv;
	g_object_unref(item->interface);
	g_object_unref(item->header);
	g_object_unref(item->properties);
	g_object_unref(item->ipv4label);
	g_object_unref(item->ipv4gatewaylabel);
	g_object_unref(item->ipv6label);
	g_object_unref(item->ipv6gatewaylabel);
	g_object_unref(item->maclabel);
	g_object_unref(item->ipv4);
	g_object_unref(item->ipv4gateway);
	g_object_unref(item->ipv6);
	g_object_unref(item->ipv6gateway);
	g_object_unref(item->mac);
	g_free(item);
}

void service_ethernet_update(struct service *serv) {
	struct ethernet_service *item = (struct ethernet_service *)serv;
	GVariant *variant, *value_v;
	GVariantDict *dict;
	const gchar *value;

	variant = g_hash_table_lookup(serv->properties, "IPv4");
	dict = g_variant_dict_new(variant);

	value_v = g_variant_dict_lookup_value(dict, "Address", NULL);
	if(value_v) {
		value = g_variant_get_string(value_v, NULL);
		gtk_label_set_text(GTK_LABEL(item->ipv4), value);
		g_variant_unref(value_v);
	}

	value_v = g_variant_dict_lookup_value(dict, "Gateway", NULL);
	if(value_v) {
		value = g_variant_get_string(value_v, NULL);
		gtk_label_set_text(GTK_LABEL(item->ipv4gateway), value);
		g_variant_unref(value_v);
	}

	g_variant_dict_unref(dict);

	variant = g_hash_table_lookup(serv->properties, "IPv6");
	dict = g_variant_dict_new(variant);

	value_v = g_variant_dict_lookup_value(dict, "Address", NULL);
	if(value_v) {
		value = g_variant_get_string(value_v, NULL);
		gtk_label_set_text(GTK_LABEL(item->ipv6), value);
		g_variant_unref(value_v);
	}

	value_v = g_variant_dict_lookup_value(dict, "Gateway", NULL);
	if(value_v) {
		value = g_variant_get_string(value_v, NULL);
		gtk_label_set_text(GTK_LABEL(item->ipv6gateway), value);
		g_variant_unref(value_v);
	}

	g_variant_dict_unref(dict);

	variant = g_hash_table_lookup(serv->properties, "Ethernet");
	dict = g_variant_dict_new(variant);

	value_v = g_variant_dict_lookup_value(dict, "Interface", NULL);
	if(value_v) {
		value = g_variant_get_string(value_v, NULL);
		gtk_label_set_text(GTK_LABEL(item->interface), value);
		g_variant_unref(value_v);
	}

	value_v = g_variant_dict_lookup_value(dict, "Address", NULL);
	if(value_v) {
		value = g_variant_get_string(value_v, NULL);
		gtk_label_set_text(GTK_LABEL(item->mac), value);
		g_variant_unref(value_v);
	}

	g_variant_dict_unref(dict);

}
