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
#include <string.h>

#include "main.h"
#include "style.h"
#include "technology.h"
#include "wireless.h"

struct wireless_technology {
	struct technology parent;

	GtkWidget *hidden_button;

	guint scan_id;
};

struct wireless_service {
	struct service parent;

	GtkWidget *security;
	GtkWidget *signal;
};

static void scan_cb_cb(GObject *source, GAsyncResult *res, gpointer user_data)
{
	GVariant *ret;
	GError *error = NULL;
	ret = g_dbus_proxy_call_finish((GDBusProxy *)user_data, res, &error);
	if(error) {
		g_warning("failed to scan wifi: %s", error->message);
		g_error_free(error);
		return;
	}
	g_variant_unref(ret);
}

static gboolean scan_cb(gpointer user_data)
{
	struct wireless_technology *tech = user_data;
	GHashTable *properties = tech->parent.settings->properties;
	if(!variant_to_bool(g_hash_table_lookup(properties, "Powered")))
		return TRUE;
	if(!variant_to_bool(g_hash_table_lookup(properties, "Tethering")))
		return TRUE;

	g_dbus_proxy_call(tech->parent.settings->proxy, "Scan", NULL,
	                  G_DBUS_CALL_FLAGS_NONE, -1, NULL, scan_cb_cb,
	                  tech->parent.settings->proxy);
	return TRUE;
}

static void hidden_cb_iter(gpointer key, gpointer value, gpointer user_data)
{
	struct service *serv = value;
	gchar *name;

	name = service_get_property_string(serv, "Name", NULL);
	if(!*name)
		g_ptr_array_add(user_data, serv);
}

static void hidden_cb(GtkWidget *button, gpointer user_data)
{
	struct wireless_technology *item = user_data;
	GPtrArray *arr = g_ptr_array_new();
	g_hash_table_foreach(item->parent.services, hidden_cb_iter, arr);
	//if(arr->len == 1)
		service_toggle_connection(g_ptr_array_index(arr, 0));
	printf("%d\n", arr->len);
	g_ptr_array_free(arr, TRUE);
}

static void hidden_service_iter(gpointer key, gpointer value, gpointer
				user_data)
{
	struct wireless_technology *item = user_data;
	gchar *name;

	name = service_get_property_string(value, "Name", NULL);
	if(!*name)
		gtk_widget_set_sensitive(item->hidden_button, TRUE);
	g_free(name);
}

void technology_wireless_hidden_update(struct technology *tech)
{
	struct wireless_technology *item = (struct wireless_technology *)tech;
	gtk_widget_set_sensitive(item->hidden_button, FALSE);
	g_hash_table_foreach(tech->services, hidden_service_iter, tech);
}

struct technology *technology_wireless_create(void)
{
	struct wireless_technology *tech = g_malloc(sizeof(*tech));
	const gchar *hidden_text = _("Connect to Hidden Network...");
	tech->hidden_button = gtk_button_new_with_mnemonic(hidden_text);
	return (struct technology *)tech;
}

void technology_wireless_free(struct technology *tech)
{
	struct wireless_technology *item = (struct wireless_technology *)tech;

	g_source_remove(item->scan_id);
	g_free(item);
}

void technology_wireless_init(struct technology *tech, GVariant *properties,
                              GDBusProxy *proxy)
{
	struct wireless_technology *item = (struct wireless_technology *)tech;
	gtk_image_set_from_icon_name(GTK_IMAGE(tech->list_item->icon),
	                             "network-wireless-symbolic",
	                             GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_image_set_from_icon_name(GTK_IMAGE(tech->settings->icon),
	                             "network-wireless", GTK_ICON_SIZE_DIALOG);
	g_signal_connect(item->hidden_button, "clicked",
			 G_CALLBACK(hidden_cb), item);
	gtk_widget_set_margin_start(item->hidden_button, MARGIN_SMALL);
	gtk_widget_set_margin_end(item->hidden_button, MARGIN_SMALL);
	gtk_grid_attach_next_to(GTK_GRID(tech->settings->buttons),
				item->hidden_button, tech->settings->tethering,
				GTK_POS_RIGHT, 1, 1);
	gtk_widget_show(item->hidden_button);

	scan_cb(item);
	item->scan_id = g_timeout_add_seconds(WIRELESS_SCAN_INTERVAL,
	                                      scan_cb, item);

}

void technology_wireless_tether(struct technology *tech)
{
	const gchar *title;
	const gchar *ssid, *pass;
	int flags, status;
	GtkWidget *window, *area;
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *ssid_l = gtk_label_new(_("SSID"));
	GtkWidget *ssid_e = gtk_entry_new();
	GtkWidget *passphrase_l = gtk_label_new(_("Passphrase"));
	GtkWidget *passphrase_e = gtk_entry_new();

	STYLE_ADD_MARGIN(ssid_l, MARGIN_LARGE);
	STYLE_ADD_MARGIN(ssid_e, MARGIN_LARGE);
	gtk_widget_set_margin_bottom(ssid_l, 0);
	gtk_widget_set_margin_bottom(ssid_e, 0);
	STYLE_ADD_MARGIN(passphrase_l, MARGIN_LARGE);
	STYLE_ADD_MARGIN(passphrase_e, MARGIN_LARGE);

	gtk_grid_attach(GTK_GRID(grid), ssid_l, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), ssid_e, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), passphrase_l, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), passphrase_e, 1, 1, 1, 1);
	gtk_widget_show_all(grid);

	title = _("Set Access Point SSID and passphrase");
	flags = GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL;
	window = gtk_dialog_new_with_buttons(title, GTK_WINDOW(main_window),
	                                     flags,
	                                     _("_OK"), GTK_RESPONSE_ACCEPT,
	                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
	                                     NULL);

	area = gtk_dialog_get_content_area(GTK_DIALOG(window));
	gtk_container_add(GTK_CONTAINER(area), grid);
	status = gtk_dialog_run(GTK_DIALOG(window));

	if(status != GTK_RESPONSE_ACCEPT)
		goto out;
	ssid = gtk_entry_get_text(GTK_ENTRY(ssid_e));
	pass = gtk_entry_get_text(GTK_ENTRY(passphrase_e));
	printf("%s %s\n", ssid, pass);
	technology_set_property(tech, "TetheringIdentifier",
	                        g_variant_new("s", ssid));
	technology_set_property(tech, "TetheringPassphrase",
	                        g_variant_new("s", pass));
	technology_set_property(tech, "Tethering",
	                        g_variant_new("b", TRUE));
out:
	gtk_widget_destroy(window);
}

struct service *service_wireless_create(void)
{
	struct wireless_service *serv = g_malloc(sizeof(*serv));
	return (struct service *)serv;
}

void service_wireless_free(struct service *serv)
{
	struct wireless_service *item = (struct wireless_service *)serv;

	g_object_unref(item->security);
	g_object_unref(item->signal);
	g_free(item);
}

void service_wireless_init(struct service *serv, GDBusProxy *proxy,
                           const gchar *path, GVariant *properties)
{
	struct wireless_service *item = (struct wireless_service *)serv;

	item->security = gtk_image_new_from_icon_name("", GTK_ICON_SIZE_MENU);
	item->signal = gtk_image_new_from_icon_name("", GTK_ICON_SIZE_MENU);

	g_object_ref(item->security);
	g_object_ref(item->signal);

	STYLE_ADD_MARGIN(item->security, MARGIN_SMALL);
	STYLE_ADD_MARGIN(item->signal, MARGIN_SMALL);

	gtk_widget_set_halign(item->signal, GTK_ALIGN_END);
	gtk_widget_set_halign(item->security, GTK_ALIGN_END);
	gtk_widget_set_valign(item->signal, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(item->security, GTK_ALIGN_CENTER);

	gtk_grid_insert_column(GTK_GRID(serv->header), 1);
	gtk_grid_insert_column(GTK_GRID(serv->header), 1);
	gtk_grid_attach(GTK_GRID(serv->header), item->security, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(serv->header), item->signal, 2, 0, 1, 1);

	gtk_widget_show_all(serv->header);
	gtk_widget_hide(serv->contents);

	service_wireless_update(serv);
}

void service_wireless_update(struct service *serv)
{
	struct wireless_service *item = (struct wireless_service *)serv;

	GVariant *variant;
	gchar *name, *state;
	int strength;

	name = service_get_property_string(serv, "Name", NULL);
	state = service_get_property_string_raw(serv, "State", NULL);
	if(!strcmp(state, "idle"))
		gtk_label_set_text(GTK_LABEL(serv->title), name);
	g_free(state);

	if(strlen(name))
		gtk_widget_show(serv->item);
	else
		gtk_widget_hide(serv->item);
	g_free(name);

	variant = service_get_property(serv, "Security", NULL);
	if(variant) {
		const gchar **value;
		const gchar **cur;
		int security = 0;
		const gchar *icon_name;
		value = g_variant_get_strv(variant, NULL);
		for(cur = value; *cur; cur++) {
			if(!strcmp("ieee8021x", *cur)) {
				security = 3;
				break;
			}
			if(!strcmp("psk", *cur))
				security = 2;
			else if(security < 2 && !strcmp("wps", *cur))
				security = 1;
		}
		g_free(value);
		icon_name = (security == 3 ? "security-high-symbolic" :
		             security == 2 ? "security-medium-symbolic" :
		             security == 1 ? "security-low-symbolic" :
		             NULL);
		gtk_image_set_from_icon_name(GTK_IMAGE(item->security),
		                             icon_name, GTK_ICON_SIZE_MENU);
		if(!security)
			gtk_widget_hide(item->security);
		else
			gtk_widget_show(item->security);
		g_variant_unref(variant);
	} else
		gtk_widget_hide(item->security);

	strength = service_get_property_int(serv, "Strength", NULL);
	gtk_image_set_from_icon_name(GTK_IMAGE(item->signal),
	                             SIGNAL_TO_ICON("wireless", strength),
	                             GTK_ICON_SIZE_MENU);
}
