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

#include <string.h>

#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "dialog.h"
#include "style.h"
#include "service.h"
#include "settings.h"
#include "util.h"
#include "vpn.h"
#include "wireless.h"

static void update_name(struct service *serv)
{
	enum connection_type type = serv->type;
	gchar *name, *state, *state_r, *title, *error;
	name = service_get_property_string(serv, "Name", NULL);
	state = service_get_property_string(serv, "State", NULL);
	state_r = service_get_property_string_raw(serv, "State", NULL);
	if(!strcmp(state_r, "failure")) {
		const gchar *failure;
		error = service_get_property_string(serv, "Error", NULL);
		failure = failure_localized(error);
		if(strlen(failure))
			title = g_strdup_printf("%s - %s: %s", name,
						state, failure);
		else
			title = g_strdup_printf("%s - %s", name, state);
		g_free(error);
	}
	else if(type == CONNECTION_TYPE_WIRELESS && !strcmp(state_r, "idle"))
		title = g_strdup_printf("%s", name);
	else
		title = g_strdup_printf("%s - %s", name, state);
	gtk_label_set_text(GTK_LABEL(serv->title), title);
	g_free(name);
	g_free(state);
	g_free(state_r);
	g_free(title);
}

static void set_label(struct service *serv, GtkWidget *label,
		      const gchar *key, const gchar *subkey)
{
	gchar *value = service_get_property_string_raw(serv, key, subkey);
	gtk_label_set_text(GTK_LABEL(label), value);
	g_free(value);
}

void service_update_property_value(struct service *serv, const gchar *key,
				   const gchar *subkey, GVariant *value)
{
	hash_table_set_dual_key(serv->properties, key, subkey,
				g_variant_ref(value));
	if(serv->sett)
		settings_update(serv->sett, key, subkey, value);
}

void service_update_property(struct service *serv, const gchar *key,
                             GVariant *value)
{
	if(!strcmp(g_variant_get_type_string(value), "a{sv}")) {
		gchar *subkey;
		GVariantIter *iter = g_variant_iter_new(value);
		while(g_variant_iter_loop(iter, "{sv}", &subkey, &value))
			service_update_property_value(serv, key, subkey, value);
		g_variant_iter_free(iter);
	} else
		service_update_property_value(serv, key, NULL, value);

	update_name(serv);

	if(serv->type == CONNECTION_TYPE_WIRELESS)
		return;

	set_label(serv, serv->ipv4, "IPv4", "Address");
	set_label(serv, serv->ipv4gateway, "IPv4", "Gateway");
	set_label(serv, serv->ipv6, "IPv6", "Address");
	set_label(serv, serv->ipv6gateway, "IPv6", "Gateway");
	set_label(serv, serv->mac, "Ethernet", "Address");
}

static void show_field(GtkWidget *entry)
{
	GtkWidget *label = g_object_get_data(G_OBJECT(entry), "label");
	if(!label)
		return;

	gtk_widget_show(entry);
	gtk_widget_show(label);
}

static void hide_field(GtkWidget *entry)
{
	GtkWidget *label = g_object_get_data(G_OBJECT(entry), "label");
	if(!label)
		return;

	gtk_widget_hide(entry);
	gtk_widget_hide(label);
}

static void update_fields(struct service *serv)
{
	gchar *state = service_get_property_string_raw(serv, "State", NULL);
	if(!strcmp(state, "idle") || !strcmp(state, "failure") ||
	   !strcmp(state, "disconnect")) {
		hide_field(serv->ipv4);
		hide_field(serv->ipv4gateway);
		hide_field(serv->ipv6);
		hide_field(serv->ipv6gateway);
		hide_field(serv->mac);
	}
	else {
		show_field(serv->ipv4);
		show_field(serv->ipv4gateway);
		show_field(serv->ipv6);
		show_field(serv->ipv6gateway);
		show_field(serv->mac);
	}
	g_free(state);
}

void service_update(struct service *serv, GVariant *properties)
{
	GVariantIter *iter;
	gchar *key;
	GVariant *value;

	iter = g_variant_iter_new(properties);
	while(g_variant_iter_loop(iter, "{sv}", &key, &value))
		service_update_property(serv, key, value);
	g_variant_iter_free(iter);
	update_name(serv);

	technology_service_updated(serv->tech, serv);

	if(serv->type != CONNECTION_TYPE_WIRELESS) {
		update_fields(serv);
		return;
	}

	service_wireless_update(serv);
}

static void ensure_field(GVariantDict *dict, const gchar *field,
			 GVariant *value)
{
	if(g_variant_dict_contains(dict, field)) {
		g_variant_unref(value);
		return;
	}
	g_variant_dict_insert_value(dict, field, value);
}

static GVariant *add_missing_fields(const gchar *name, GVariant *value)
{
	GVariantDict *dict;

	if(strcmp(name, "Ethernet") && strncmp(name, "IPv", 3) &&
			strncmp(name, "Proxy", 5))
		return value;

	dict = g_variant_dict_new(value);
	if(!strcmp(name, "Ethernet")) {
		ensure_field(dict, "Method", g_variant_new("s", "auto"));
		ensure_field(dict, "Interface", g_variant_new("s", ""));
		ensure_field(dict, "Address", g_variant_new("s", ""));
		ensure_field(dict, "MTU", g_variant_new("q", 1500));
	} else if(!strcmp(name, "IPv4")) {
		ensure_field(dict, "Method", g_variant_new("s", "off"));
		ensure_field(dict, "Address", g_variant_new("s", ""));
		ensure_field(dict, "Netmask", g_variant_new("s", ""));
		ensure_field(dict, "Gateway", g_variant_new("s", ""));
	} else if(!strcmp(name, "IPv6")) {
		ensure_field(dict, "Method", g_variant_new("s", "off"));
		ensure_field(dict, "Address", g_variant_new("s", ""));
		ensure_field(dict, "PrefixLength", g_variant_new("s", ""));
		ensure_field(dict, "Gateway", g_variant_new("s", ""));
	}

	g_variant_unref(value);
	value = g_variant_dict_end(dict);
	g_variant_dict_unref(dict);
	return value;
}

static void service_proxy_signal(GDBusProxy *proxy, gchar *sender,
                                 gchar *signal, GVariant *parameters,
                                 gpointer user_data)
{
	struct service *serv = user_data;
	if(!strcmp(signal, "PropertyChanged")) {
		GVariant *name_v, *value_v, *value;
		const gchar *name;

		name_v = g_variant_get_child_value(parameters, 0);
		value_v = g_variant_get_child_value(parameters, 1);

		name = g_variant_get_string(name_v, NULL);
		value = g_variant_get_child_value(value_v, 0);
		value = add_missing_fields(name, value);

		service_update_property(serv, name, value);
		g_variant_unref(value);
		g_variant_unref(name_v);
		g_variant_unref(value_v);

		technology_service_updated(serv->tech, serv);
	}
}

static void settings_closed(struct service *serv)
{
	if(serv) {
		serv->sett = NULL;
		gtk_widget_set_sensitive(serv->settings_button, TRUE);
	}
}

static void settings_button_cb(GtkButton *button, gpointer user_data)
{
	struct service *serv = user_data;
	gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
	serv->sett = settings_create(serv, settings_closed);
}

static GtkWidget *add_label(GtkWidget *grid, gint y, const gchar *text)
{
	GtkWidget *label, *value;

	label = gtk_label_new(text);
	value = gtk_label_new(NULL);

	style_set_margin(label, MARGIN_SMALL);
	g_object_set_data(G_OBJECT(value), "label", label);
	style_set_margin_start(label, MARGIN_LARGE);
	gtk_style_context_add_class(gtk_widget_get_style_context(label),
	                            "dim-label");
	gtk_label_set_selectable(GTK_LABEL(value), TRUE);

	gtk_widget_set_hexpand(label, TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_widget_set_halign(value, GTK_ALIGN_START);

	gtk_grid_attach(GTK_GRID(grid), label, 0, y, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), value, 1, y, 1, 1);

	return value;
}

void service_init(struct service *serv, GDBusProxy *proxy, const gchar *path,
                  GVariant *properties)
{
	GtkGrid *item_grid;

	serv->proxy = proxy;
	serv->path = g_strdup(path);
	serv->properties = dual_hash_table_new((GDestroyNotify)g_variant_unref);
	serv->sett = NULL;

	serv->item = gtk_list_box_row_new();
	serv->header = gtk_grid_new();
	serv->title = gtk_label_new(NULL);
	serv->contents = gtk_grid_new();
	serv->settings_button = gtk_button_new_from_icon_name(
	                                "emblem-system-symbolic",
	                                GTK_ICON_SIZE_MENU);
	item_grid = GTK_GRID(gtk_grid_new());

	g_object_ref(serv->item);
	g_object_ref(serv->header);
	g_object_ref(serv->title);
	g_object_ref(serv->contents);
	g_object_set_data(G_OBJECT(serv->item), "service", serv);

	g_signal_connect(proxy, "g-signal", G_CALLBACK(service_proxy_signal),
	                 serv);
	g_signal_connect(serv->settings_button, "clicked",
	                 G_CALLBACK(settings_button_cb), serv);

	gtk_grid_set_column_homogeneous(GTK_GRID(serv->contents), TRUE);

	style_set_margin(serv->item, MARGIN_SMALL);
	style_set_margin(serv->title, MARGIN_SMALL);
	style_set_margin_start(serv->title, MARGIN_LARGE);
	style_set_margin_start(serv->contents, MARGIN_LARGE);
	gtk_widget_set_margin_top(serv->contents, 0);
	gtk_widget_set_margin_bottom(serv->contents, 0);

	gtk_widget_set_hexpand(serv->item, TRUE);
	gtk_widget_set_hexpand(serv->header, TRUE);
	gtk_widget_set_hexpand(serv->title, TRUE);
	gtk_widget_set_hexpand(serv->contents, TRUE);
	gtk_widget_set_halign(serv->title, GTK_ALIGN_START);

	gtk_grid_attach(GTK_GRID(serv->header), serv->title, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(serv->header), serv->settings_button,
	                1, 0, 1, 1);
	gtk_grid_attach(item_grid, serv->header, 0, 0, 1, 1);
	gtk_grid_attach(item_grid, serv->contents, 0, 1, 1, 1);
	gtk_container_add(GTK_CONTAINER(serv->item), GTK_WIDGET(item_grid));
	if(serv->type == CONNECTION_TYPE_WIRELESS) {
		gtk_widget_show_all(serv->item);
		return;
	}

	serv->ipv4 = add_label(serv->contents, 0, _("IPv4 address"));
	serv->ipv4gateway = add_label(serv->contents, 1, _("IPv4 gateway"));
	serv->ipv6 = add_label(serv->contents, 2, _("IPv6 address"));
	serv->ipv6gateway = add_label(serv->contents, 3, _("IPv6 gateway"));
	if(serv->type != CONNECTION_TYPE_VPN)
		serv->mac = add_label(serv->contents, 4, _("MAC address"));
	else
		serv->mac = gtk_label_new(NULL);

	gtk_widget_show_all(serv->item);
}

struct service *service_create(struct technology *tech, GDBusProxy *proxy,
                               const gchar *path, GVariant *properties)
{
	struct service *serv;
	enum connection_type type;
	GVariantDict *properties_d;
	GVariant *type_v;

	properties_d = g_variant_dict_new(properties);
	type_v = g_variant_dict_lookup_value(properties_d, "Type", NULL);
	type = connection_type_from_string(g_variant_get_string(type_v, NULL));
	g_variant_unref(type_v);
	g_variant_dict_unref(properties_d);

	serv = g_malloc(sizeof(*serv));
	serv->type = type;
	serv->tech = tech;
	serv->sett = NULL;

	service_init(serv, proxy, path, properties);
	if(serv->type == CONNECTION_TYPE_WIRELESS)
		service_wireless_init(serv, proxy, path, properties);

	service_update(serv, properties);

	return serv;
}

void service_free(struct service *serv)
{
	if(!serv)
		return;
	if(serv->sett) {
		serv->sett->serv = NULL;
		gtk_window_close(GTK_WINDOW(serv->sett->window));
	}
	g_object_unref(serv->proxy);
	g_free(serv->path);
	dual_hash_table_unref(serv->properties);
	gtk_widget_destroy(serv->item);
	g_object_unref(serv->item);
	g_object_unref(serv->contents);
	g_object_unref(serv->header);
	if(serv->type == CONNECTION_TYPE_WIRELESS)
		service_wireless_free(serv);
	g_free(serv);
}

static gboolean strv_contains(const gchar *const *strv, const gchar *str) {
	const gchar *const *iter = strv;
	while(*iter && strcmp(*iter, str))
		iter++;
	return *iter != NULL;
}

static void show_wireless_error(struct service *serv, const gchar *message)
{
	const gchar *e;
	gchar **security;

	e = _("IEEE8021x secured services have to be manually configured.");
	security = service_get_property_strv(serv, "Security", NULL);

	if(strv_contains((const gchar **)security, "ieee8021x"))
		show_error(_("Failed to toggle connection state."), e);

	g_strfreev(security);
}

static void service_toggle_connection_cb(GObject *source, GAsyncResult *res,
                gpointer user_data)
{
	struct service *serv;
	const gchar *ia = "GDBus.Error:net.connman.Error.InvalidArguments";
	const gchar *c = "GDBus.Error:net.connman.Error.Canceled";
	const gchar *ip = "GDBus.Error:net.connman.Error.InProgress";
	const gchar *oa = "GDBus.Error:net.connman.Error.OperationAborted";
	const gchar *f = "GDBus.Error:net.connman.Error.Failed";
	GError *error = NULL;
	GVariant *out;

	serv = user_data;
	out = g_dbus_proxy_call_finish(serv->proxy, res, &error);
	if(error) {
		/*
		 * InvalidArguments is thrown when user cancels the dialog,
		 * so ignore it, Failed is returned in 1.29 and older when
		 * cancelling connects to hidden wireless networks
		 */
		g_warning("failed to toggle connection state: %s",
			  error->message);
		if(strncmp(ia, error->message, strlen(ia)) &&
		   strncmp(c, error->message, strlen(c)) &&
		   strncmp(ip, error->message, strlen(ip)) &&
		   strncmp(oa, error->message, strlen(oa)) &&
		   strncmp(f, error->message, strlen(f)))
			show_error(_("Failed to toggle connection state."),
				   error->message);
		else if(serv->type == CONNECTION_TYPE_WIRELESS &&
				!strncmp(ia, error->message, strlen(ia)))
			show_wireless_error(serv, error->message);

		g_error_free(error);
		return;
	}
	g_variant_unref(out);
}

void service_toggle_connection(struct service *serv)
{
	const gchar *function;
	gchar *state;

	state = service_get_property_string_raw(serv, "State", NULL);

	if(!strcmp(state, "idle") || !strcmp(state, "failure"))
		function = "Connect";
	else
		function = "Disconnect";

	g_free(state);

	g_dbus_proxy_call(serv->proxy, function, NULL,
	                  G_DBUS_CALL_FLAGS_NONE, CONNECTION_TIMEOUT, NULL,
	                  service_toggle_connection_cb, serv);
}

GVariant *service_get_property(struct service *serv, const char *key,
                               const char *subkey)
{
	GVariant *variant;

	variant = hash_table_get_dual_key(serv->properties, key, subkey);
	if(!variant)
		return NULL;
	g_variant_ref(variant);
	return variant;
}

gchar *service_get_property_string_raw(struct service *serv, const char *key,
                                       const char *subkey)
{
	if(!serv || !key)
		return g_strdup("");
	GVariant *prop = service_get_property(serv, key, subkey);
	gchar *str = variant_to_str(prop);
	if(prop)
		g_variant_unref(prop);
	return str;
}

static gchar *wireless_name(struct service *serv)
{
	gchar *name, **security, **iter;
	const gchar *out;
	int security_level = 0;

	name = service_get_property_string_raw(serv, "Name", NULL);
	if(strlen(name))
		return name;
	g_free(name);

	security = service_get_property_strv(serv, "Security", NULL);
	for(iter = security; *iter; iter++) {
		if(!strcmp("ieee8021x", *iter))
			security_level = 3;
		else if(security_level < 2 && !strcmp("psk", *iter))
			security_level = 2;
		else if(security_level < 1 && !strcmp("wps", *iter))
			security_level = 1;
	}
	out = (security_level == 3 ? _("Hidden ieee8021x secured network") :
	       security_level == 2 ? _("Hidden WPA secured network") :
	       security_level == 1 ? _("Hidden WPS secured network") :
	       _("Hidden unsecured network"));
	g_strfreev(security);
	return g_strdup(out);
}

gchar *service_get_property_string(struct service *serv, const char *key,
                                   const char *subkey)
{
	if(!serv)
		return g_strdup("");

	if(key && subkey) {
		if(!strcmp(subkey, "PrefixLength") &&
		   (!strcmp(key, "IPv6") ||
		    !strcmp(key, "IPv6.Configuration"))) {
			int len = service_get_property_int(serv, key, subkey);
			if(!len)
				return g_strdup("");
			return g_strdup_printf("%d", len);
		} else if(!strcmp(key, "Proxy") && !strcmp(subkey, "Method")) {
			gchar *str = service_get_property_string_raw(serv, key,
								     subkey);
			const gchar *out;
			if(!strcmp(str, "direct"))
				out = _("Direct");
			else if(!strcmp(str, "auto"))
				out = _("Automatic");
			else
				out = _("None");
			g_free(str);
			return g_strdup(out);
		}
	}

	if(key) {
		if(!strcmp(key, "AutoConnect")) {
			gboolean ac;

			ac = service_get_property_boolean(serv, key, subkey);
			if(ac)
				return g_strdup(_("On"));
			return g_strdup(_("Off"));
		} else if(!strcmp(key, "State")) {
			gchar *str = service_get_property_string_raw(serv, key,
								     subkey);
			gchar *out = g_strdup(status_localized(str));
			g_free(str);
			return out;
		} else if(serv->type == CONNECTION_TYPE_ETHERNET &&
			  !strcmp(key, "Name")) {
			return service_get_property_string_raw(serv, "Ethernet",
							       "Interface");
		} else if(serv->type == CONNECTION_TYPE_WIRELESS &&
			  !strcmp(key, "Name")) {
			return wireless_name(serv);
		}

	}

	return service_get_property_string_raw(serv, key, subkey);
}

gchar **service_get_property_strv(struct service *serv, const char *key,
                                  const char *subkey)
{
	if(!serv || !key)
		return g_malloc0(sizeof(gchar *));
	GVariant *prop = service_get_property(serv, key, subkey);
	if(!prop)
		return g_malloc0(sizeof(gchar *));
	gchar **out = variant_to_strv(prop);
	if(prop)
		g_variant_unref(prop);
	return out;
}

gboolean service_get_property_boolean(struct service *serv, const char *key,
                                      const char *subkey)
{
	if(!serv || !key)
		return FALSE;
	GVariant *prop = service_get_property(serv, key, subkey);
	gboolean out = variant_to_bool(prop);
	if(prop)
		g_variant_unref(prop);
	return out;
}

int service_get_property_int(struct service *serv, const char *key,
                             const char *subkey)
{
	if(!serv || !key)
		return 0;
	GVariant *prop = service_get_property(serv, key, subkey);
	int out = variant_to_int(prop);
	if(prop)
		g_variant_unref(prop);
	return out;
}

void service_set_property(struct service *serv, const char *key,
                          GVariant *value)
{
	GVariant *ret;
	GVariant *parameters;
	GError *error = NULL;

	GVariant *old;
	gboolean equal = TRUE;
	if(strcmp(g_variant_get_type_string(value), "a{sv}")) {
		old = service_get_property(serv, key, NULL);
		equal = old && g_variant_equal(old, value);
		if(old)
			g_variant_unref(old);
	} else {
		gchar *subkey;
		GVariant *svalue;
		GVariantIter *iter = g_variant_iter_new(value);
		while(g_variant_iter_loop(iter, "{sv}", &subkey, &svalue)) {
			old = service_get_property(serv, key, subkey);
			equal = equal && old && g_variant_equal(old, svalue);
			if(old)
				g_variant_unref(old);
		}
		g_variant_iter_free(iter);
	}
	if(equal)
		return;

	parameters = g_variant_new("(sv)", key, value);
	ret = g_dbus_proxy_call_sync(serv->proxy, "SetProperty", parameters,
	                             G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if(error) {
		g_warning("failed to set property %s: %s", key, error->message);
		g_error_free(error);
		return;
	}
	g_variant_unref(ret);
}

void service_clear_properties(struct service *serv)
{
	GVariantBuilder *b;

	if(serv->type == CONNECTION_TYPE_VPN)
		return;

	b = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(b, "{sv}", "Method",
			      g_variant_new_string("dhcp"));
	service_set_property(serv, "IPv4.Configuration",
			     g_variant_builder_end(b));
	g_variant_builder_unref(b);

	b = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(b, "{sv}", "Method",
			      g_variant_new_string("auto"));
	g_variant_builder_add(b, "{sv}", "Privacy",
			      g_variant_new_string("disabled"));
	service_set_property(serv, "IPv6.Configuration",
			     g_variant_builder_end(b));
	g_variant_builder_unref(b);

	b = g_variant_builder_new(G_VARIANT_TYPE("as"));
	service_set_property(serv, "Nameservers.Configuration",
			     g_variant_builder_end(b));
	g_variant_builder_unref(b);

	b = g_variant_builder_new(G_VARIANT_TYPE("as"));
	service_set_property(serv, "Timeservers.Configuration",
			     g_variant_builder_end(b));
	g_variant_builder_unref(b);

	b = g_variant_builder_new(G_VARIANT_TYPE("as"));
	service_set_property(serv, "Domains.Configuration",
			     g_variant_builder_end(b));
	g_variant_builder_unref(b);

	b = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(b, "{sv}", "Method",
			      g_variant_new_string("direct"));
	service_set_property(serv, "Proxy.Configuration",
			     g_variant_builder_end(b));
	g_variant_builder_unref(b);
}

void service_remove(struct service *serv)
{
	GVariant *ret;
	GError *error = NULL;

	ret = g_dbus_proxy_call_sync(serv->proxy, "Remove", NULL,
	                             G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if(error) {
		g_warning("failed to remove service: %s", error->message);
		g_error_free(error);
		return;
	}
	g_variant_unref(ret);
}

void service_set_properties(struct service *serv, GVariant *properties)
{
	GVariantIter *iter;
	gchar *key;
	GVariant *value;

	iter = g_variant_iter_new(properties);
	while(g_variant_iter_loop(iter, "{sv}", &key, &value))
		service_set_property(serv, key, value);
	g_variant_iter_free(iter);
}
