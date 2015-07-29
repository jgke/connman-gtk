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

struct wireless_service {
	struct service *parent;

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
	struct technology *tech = user_data;
	GHashTable *properties = tech->settings->properties;
	if(!variant_to_bool(g_hash_table_lookup(properties, "Powered")))
		return TRUE;
	if(!variant_to_bool(g_hash_table_lookup(properties, "Tethering")))
		return TRUE;

	g_dbus_proxy_call(tech->settings->proxy, "Scan", NULL,
	                  G_DBUS_CALL_FLAGS_NONE, -1, NULL, scan_cb_cb,
	                  tech->settings->proxy);
	return TRUE;
}

void technology_wireless_free(struct technology *tech)
{
	g_source_remove(GPOINTER_TO_INT(tech->data));
}

void technology_wireless_init(struct technology *tech, GVariant *properties,
                              GDBusProxy *proxy)
{
	int id;

	scan_cb(tech);
	id = g_timeout_add_seconds(WIRELESS_SCAN_INTERVAL,
				   scan_cb, tech);
	tech->data = GINT_TO_POINTER(id);

}

void service_wireless_free(struct service *tech)
{
	g_free(tech->data);
}

static void check_passphrase(GtkWidget *entry, gpointer user_data)
{
	const gchar *str = gtk_entry_get_text(GTK_ENTRY(entry));
	GtkStyleContext *context = gtk_widget_get_style_context(entry);

	if(strlen(str) < 8 || strlen(str) > 63) {
		gtk_style_context_add_class(context, "error");
		gtk_dialog_set_response_sensitive(user_data,
						  GTK_RESPONSE_ACCEPT, FALSE);
	} else {
		gtk_style_context_remove_class(context, "error");
		gtk_dialog_set_response_sensitive(user_data,
						  GTK_RESPONSE_ACCEPT, TRUE);
	}
}

static void toggle_entry_mode(GtkToggleButton *button, gpointer user_data)
{
	GtkEntry *entry = GTK_ENTRY(user_data);
	gboolean mode = gtk_toggle_button_get_active(button);

	if(mode) {
		gtk_entry_set_visibility(entry, TRUE);
		gtk_entry_set_input_purpose(entry, GTK_INPUT_PURPOSE_FREE_FORM);
	} else {
		gtk_entry_set_visibility(entry, FALSE);
		gtk_entry_set_input_purpose(entry, GTK_INPUT_PURPOSE_PASSWORD);
	}
}

void technology_wireless_tether(struct technology *tech)
{
	const gchar *title;
	const gchar *ssid, *pass;
	GVariant *old_ssid, *old_pass;
	int flags, status;
	GtkWidget *window, *area, *grid, *ssid_l, *ssid_e, *passphrase_l,
		  *passphrase_e, *toggle;

	grid = gtk_grid_new();
	ssid_l = gtk_label_new(_("SSID"));
	ssid_e = gtk_entry_new();
	passphrase_l = gtk_label_new(_("Passphrase"));
	passphrase_e = gtk_entry_new();
	toggle = gtk_check_button_new_with_mnemonic(_("_Show password"));
	title = _("Set Access Point SSID and passphrase");
	flags = GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL;
	window = gtk_dialog_new_with_buttons(title, GTK_WINDOW(main_window),
	                                     flags,
	                                     _("_OK"), GTK_RESPONSE_ACCEPT,
	                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
	                                     NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(window),
					GTK_RESPONSE_ACCEPT);

	old_ssid = g_hash_table_lookup(tech->settings->properties,
				       "TetheringIdentifier");
	old_pass = g_hash_table_lookup(tech->settings->properties,
				       "TetheringPassphrase");

	if(old_ssid)
		gtk_entry_set_text(GTK_ENTRY(ssid_e),
				   g_variant_get_string(old_ssid, NULL));
	if(old_pass)
		gtk_entry_set_text(GTK_ENTRY(passphrase_e),
				   g_variant_get_string(old_pass, NULL));

	gtk_entry_set_activates_default(GTK_ENTRY(ssid_e), TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(passphrase_e), TRUE);
	gtk_entry_set_visibility(GTK_ENTRY(passphrase_e), FALSE);
	gtk_entry_set_input_purpose(GTK_ENTRY(passphrase_e),
				    GTK_INPUT_PURPOSE_PASSWORD);

	g_signal_connect(toggle, "toggled", G_CALLBACK(toggle_entry_mode),
			 passphrase_e);
	g_signal_connect(passphrase_e, "changed", G_CALLBACK(check_passphrase),
			 window);
	check_passphrase(passphrase_e, window);

	gtk_widget_set_halign(ssid_l, GTK_ALIGN_END);
	gtk_widget_set_halign(passphrase_l, GTK_ALIGN_END);
	style_add_margin(ssid_l, MARGIN_LARGE);
	style_add_margin(ssid_e, MARGIN_LARGE);
	gtk_widget_set_margin_bottom(ssid_l, 0);
	gtk_widget_set_margin_bottom(ssid_e, 0);
	style_add_margin(passphrase_l, MARGIN_LARGE);
	style_add_margin(passphrase_e, MARGIN_LARGE);
	gtk_widget_set_margin_bottom(passphrase_e, 0);
	style_add_margin(toggle, MARGIN_LARGE);
	gtk_widget_set_margin_top(toggle, MARGIN_SMALL);

	gtk_grid_attach(GTK_GRID(grid), ssid_l, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), ssid_e, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), passphrase_l, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), passphrase_e, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), toggle, 1, 2, 1, 1);
	gtk_widget_show_all(grid);

	area = gtk_dialog_get_content_area(GTK_DIALOG(window));
	gtk_container_add(GTK_CONTAINER(area), grid);
	status = gtk_dialog_run(GTK_DIALOG(window));

	if(status != GTK_RESPONSE_ACCEPT)
		goto out;
	ssid = gtk_entry_get_text(GTK_ENTRY(ssid_e));
	pass = gtk_entry_get_text(GTK_ENTRY(passphrase_e));
	technology_set_property(tech, "TetheringIdentifier",
	                        g_variant_new("s", ssid));
	technology_set_property(tech, "TetheringPassphrase",
	                        g_variant_new("s", pass));
	technology_set_property(tech, "Tethering",
	                        g_variant_new("b", TRUE));
out:
	gtk_widget_destroy(window);
}

void service_wireless_init(struct service *serv, GDBusProxy *proxy,
                           const gchar *path, GVariant *properties)
{
	struct wireless_service *item = g_malloc(sizeof(*serv));

	serv->data = item;
	item->parent = serv;
	item->security = gtk_image_new_from_icon_name("", GTK_ICON_SIZE_MENU);
	item->signal = gtk_image_new_from_icon_name("", GTK_ICON_SIZE_MENU);

	style_add_margin(item->security, MARGIN_SMALL);
	style_add_margin(item->signal, MARGIN_SMALL);

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
	struct wireless_service *item = serv->data;

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
