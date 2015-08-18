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
#include "style.h"
#include "util.h"

static void free_page(GtkWidget *widget, gpointer user_data)
{
	struct settings_page *page = user_data;
	g_free(page);
}

static struct settings_page *create_page(GtkWidget *notebook, gboolean scrolled)
{
	struct settings_page *page = g_malloc(sizeof(*page));

	page->index = 0;
	page->grid = gtk_grid_new();
	page->item = page->grid;

	if(scrolled) {
		GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
					       GTK_POLICY_NEVER,
					       GTK_POLICY_AUTOMATIC);
		gtk_container_add(GTK_CONTAINER(scroll), page->item);
		page->item = scroll;
	}

	g_signal_connect(page->item, "destroy",
	                 G_CALLBACK(free_page), page);
	gtk_widget_set_margin_start(page->item, MARGIN_LARGE);
	gtk_widget_show_all(page->item);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page->item, NULL);
	return page;
}

void page_mnemonic(GtkWidget *widget, gboolean arg1, gpointer user_data)
{
	list_item_selected(NULL, GTK_LIST_BOX_ROW(widget), user_data);
}

static struct settings_page *add_page_to_settings(struct settings *sett,
                const gchar *name, gboolean scrolled)
{
	struct settings_page *page;
	GtkWidget *item, *label;

	page = create_page(sett->notebook, scrolled);
	page->sett = sett;

	item = gtk_list_box_row_new();
	label = gtk_label_new_with_mnemonic(name);

	g_signal_connect(item, "mnemonic-activate", G_CALLBACK(page_mnemonic),
			 sett->notebook);
	g_object_set_data(G_OBJECT(item), "content", page->item);

	style_set_margin(label, MARGIN_SMALL);
	gtk_widget_set_margin_start(label, MARGIN_LARGE);
	gtk_widget_set_margin_end(label, MARGIN_LARGE);

	gtk_widget_set_halign(label, GTK_ALIGN_START);

	gtk_container_add(GTK_CONTAINER(item), label);
	gtk_container_add(GTK_CONTAINER(sett->list), item);

	gtk_widget_show_all(item);

	return page;
}

static struct settings_page *add_page_to_combo_box(struct settings *sett,
                GtkWidget *box,
                const gchar *id,
                const gchar *name,
                gboolean active)
{
	GtkWidget *notebook = g_object_get_data(G_OBJECT(box), "notebook");
	GHashTable *items = g_object_get_data(G_OBJECT(box), "items");
	struct settings_page *page = create_page(notebook, FALSE);
	page->sett = sett;
	g_hash_table_insert(items, g_strdup(name), page->item);
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(box), id, name);
	if(active)
		g_object_set_data(G_OBJECT(page->item), "selected", page->item);
	else
		g_object_set_data(G_OBJECT(page->item), "selected", NULL);
	if(active)
		gtk_combo_box_set_active_id(GTK_COMBO_BOX(box), id);
	return page;
}

static void free_settings(struct settings *sett)
{
	sett->closed(sett->serv);
	dual_hash_table_unref(sett->callbacks);
	dual_hash_table_unref(sett->contents);
	g_free(sett);
}

static void add_security(struct settings *sett, struct settings_page *page)
{
	GVariant *security;
	GVariant *child;
	if(sett->serv->type != CONNECTION_TYPE_WIRELESS)
		return;
	security = service_get_property(sett->serv, "Security", NULL);
	child = g_variant_get_child_value(security, 0);
	if(!strcmp(g_variant_get_string(child, NULL), "none")) {
		settings_add_static_text(page, _("Security"), _("None"));
		goto out;
	}
	else
		settings_add_text(page, _("Security"), "Security", NULL);

out:
	g_variant_unref(child);
	g_variant_unref(security);
}

static void add_info_page(struct settings *sett)
 {
	struct settings_page *page = add_page_to_settings(sett, _("_Info"),
							  FALSE);

	settings_add_switch(sett, page, always_write, _("Autoconnect"),
			    "AutoConnect", NULL);

	settings_add_text(page, _("Name"), "Name", NULL);
	settings_add_text(page, _("State"), "State", NULL);
	add_security(sett, page);
	settings_add_text(page, _("MAC address"), "Ethernet", "Address");
	settings_add_text(page, _("Interface"), "Ethernet", "Interface");
	settings_add_text(page, _("IPv4 address"), "IPv4", "Address");
	settings_add_text(page, _("IPv6 address"), "IPv6", "Address");
	settings_add_text(page, _("Nameservers"), "Nameservers", NULL);
}

static gboolean valid_ipv4_entry(GtkWidget *entry)
{
	const gchar *value = gtk_entry_get_text(GTK_ENTRY(entry));
	return valid_ipv4(value);
}

static gboolean valid_ipv6_entry(GtkWidget *entry)
{
	const gchar *value = gtk_entry_get_text(GTK_ENTRY(entry));
	return valid_ipv6(value);
}

static gboolean valid_address_entry(GtkWidget *entry)
{
	const gchar *value = gtk_entry_get_text(GTK_ENTRY(entry));
	return valid_ipv4(value) || valid_ipv6(value);
}

static void add_immutable_ipv_page(struct settings *sett)
{
	struct settings_page *page;

	page = add_page_to_settings(sett, _("IP _details"), FALSE);
	settings_add_text(page, _("IPv4 method"), "IPv4", "Method");
	settings_add_text(page, _("Address"), "IPv4", "Address");
	settings_add_text(page, _("Gateway"), "IPv4", "Gateway");

	settings_add_text(page, "", NULL, NULL);

	settings_add_text(page, _("IPv6 method"), "IPv6", "Method");
	settings_add_text(page, _("Address"), "IPv6", "Address");
	settings_add_text(page, _("Gateway"), "IPv6", "Gateway");

	return;
}

static void add_ipv_page(struct settings *sett, int ipv)
{
	struct settings_page *page;
	settings_entry_validator validator;
	const char *local;
	const char *conf;
	const char *ipvs;
	GtkWidget *box;
	struct settings_page *none;
	struct settings_page *dhcp;
	struct settings_page *manual;
	GtkWidget *ipv6_privacy;

	if(ipv == 4) {
		local = _("IPv_4");
		validator = valid_ipv4_entry;
		conf = "IPv4.Configuration";
		ipvs = "IPv4";
	} else {
		local = _("IPv_6");
		validator = valid_ipv6_entry;
		conf = "IPv6.Configuration";
		ipvs = "IPv6";
	}
	char *cur = service_get_property_string_raw(sett->serv, ipvs, "Method");
	if(!strlen(cur)) {
		g_free(cur);
		cur = service_get_property_string_raw(sett->serv, conf, "Method");
		if(!strlen(cur)) {
			g_free(cur);
			return;
		}
	}

	page = add_page_to_settings(sett, local, FALSE);
	box = settings_add_combo_box(sett, page, always_write, _("Method"),
	                             conf, "Method", ipvs);

	none = add_page_to_combo_box(sett, box, "off", _("None"), TRUE);

	if(ipv == 4)
		dhcp = add_page_to_combo_box(sett, box, "dhcp", _("Automatic"),
		                             !strcmp("dhcp", cur) ||
					     !strcmp("fixed", cur));
	else
		dhcp = add_page_to_combo_box(sett, box, "auto", _("Automatic"),
		                             !strcmp("auto", cur) ||
		                             !strcmp("6to4", cur) ||
					     !strcmp("fixed", cur));

	if(!strcmp(cur, "fixed"))
		return;

	manual = add_page_to_combo_box(sett, box, "manual", _("Manual"),
	                               !strcmp("manual", cur));
	g_free(cur);

	settings_add_text(none, NULL, NULL, NULL);

	settings_add_text(dhcp, _("Address"), ipvs, "Address");
	settings_add_text(dhcp, _("Gateway"), ipvs, "Gateway");
	if(ipv == 4)
		settings_add_text(dhcp, _("Netmask"), ipvs, "Netmask");
	else
		settings_add_text(dhcp, _("Prefix length"), ipvs,
		                  "PrefixLength");

	settings_add_text(manual, _("Address"), NULL, NULL);
	settings_add_entry(sett, manual, write_if_selected, NULL, conf,
	                   "Address", ipvs, validator);
	settings_add_text(manual, _("Gateway"), NULL, NULL);
	settings_add_entry(sett, manual, write_if_selected, NULL, conf,
	                   "Gateway", ipvs, validator);
	if(ipv == 4) {
		settings_add_text(manual, _("Netmask"), NULL, NULL);
		settings_add_entry(sett, manual, write_if_selected, NULL, conf,
		                   "Netmask", ipvs, validator);
	} else {
		settings_add_prefix_entry(sett, manual, write_if_selected);
		gtk_widget_set_halign(manual->item, GTK_ALIGN_START);

		ipv6_privacy = settings_add_combo_box(sett, dhcp,
		                                      always_write,
		                                      _("Privacy"), conf,
		                                      "Privacy", ipvs);

		cur = service_get_property_string_raw(sett->serv, ipvs, "Privacy");
		add_page_to_combo_box(sett, ipv6_privacy, "disabled",
		                      _("Disabled"), TRUE);
		add_page_to_combo_box(sett, ipv6_privacy, "enabled",
		                      _("Enabled"), !strcmp("enabled", cur));
		add_page_to_combo_box(sett, ipv6_privacy, "prefered",
		                      _("Prefered"), !strcmp("prefered", cur));
		g_free(cur);
	}
}

static void add_immutable_server_page(struct settings *sett)
{
	struct settings_page *page;
	gchar *cur = service_get_property_string_raw(sett->serv,
						     "Proxy", "Method");
	page = add_page_to_settings(sett, _("_Servers"), TRUE);

	settings_add_text(page, _("Nameservers"), "Nameservers", NULL);
	settings_add_text(page, _("Timeservers"), "Timeservers", NULL);
	settings_add_text(page, _("Domains"), "Domains", NULL);
	settings_add_text(page, _("Proxy method"), "Proxy", "Method");

	if(!strcmp(cur, "direct"));
	else if(!strcmp(cur, "auto"))
		settings_add_text(page, _("URL"), "Proxy", "URL");
	else {
		settings_add_text(page, _("Servers"), "Proxy",
				  "Servers");
		settings_add_text(page, _("Excludes"), "Proxy",
				  "Excludes");
	}

	g_free(cur);
	return;
}

static void add_server_page(struct settings *sett)
{
	struct settings_page *page;

	page = add_page_to_settings(sett, _("_Nameservers"), TRUE);
	settings_add_entry_list(sett, page, always_write, _("Nameservers"),
				"Nameservers.Configuration", NULL,
				"Nameservers", valid_address_entry);

	page = add_page_to_settings(sett, _("_Timeservers"), TRUE);
	settings_add_text(page, _("Timeservers"), "Timeservers", NULL);
	settings_add_static_text(page, "", "");;
	settings_add_entry_list(sett, page, always_write,
				_("Custom Timeservers"),
				"Timeservers.Configuration", NULL,
				NULL, always_valid);

	page = add_page_to_settings(sett, _("_Domains"), TRUE);
	settings_add_entry_list(sett, page, always_write, _("Domains"),
				"Domains.Configuration", NULL,
				"Domains", always_valid);
}

static void add_proxy_page(struct settings *sett)
{
	struct settings_page *page, *direct, *automatic, *manual;
	GtkWidget *box;
	const gchar *conf = "Proxy.Configuration";
	gchar *cur;

	page = add_page_to_settings(sett, _("_Proxy"), TRUE);
	cur = service_get_property_string_raw(sett->serv, "Proxy", "Method");
	box = settings_add_combo_box(sett, page, always_write, _("Method"),
	                             conf, "Method", "Proxy");

	direct = add_page_to_combo_box(sett, box, "direct", _("Direct"),
	                               !strlen(cur) || !strcmp("direct", cur));
	automatic = add_page_to_combo_box(sett, box, "auto", _("Automatic"),
	                                  !strcmp("auto", cur));
	manual = add_page_to_combo_box(sett, box, "manual", _("Manual"),
	                               !strcmp("manual", cur));
	g_free(cur);

	settings_add_text(direct, NULL, NULL, NULL);

	settings_add_entry(sett, automatic, write_if_selected, _("URL"),
	                   conf, "URL", "Proxy", always_valid);

	settings_add_entry_list(sett, manual, write_if_selected, _("Servers"),
	                        conf, "Servers", "Proxy", always_valid);
	settings_add_entry_list(sett, manual, write_if_selected, _("Excludes"),
	                        conf, "Excludes", "Proxy", always_valid);
}

static void add_vpn_info_page(struct settings *sett)
{
	struct settings_page *page = add_page_to_settings(sett, _("_Info"),
							  FALSE);

	settings_add_text(page, _("Name"), "Name", NULL);
	settings_add_text(page, _("Type"), "Type", NULL);
	settings_add_text(page, _("State"), "State", NULL);
	settings_add_text(page, _("Host"), "Host", NULL);
	settings_add_text(page, _("IPv4 address"), "IPv4", "Address");
	settings_add_text(page, _("IPv6 address"), "IPv6", "Address");
	settings_add_text(page, _("Nameservers"), "Nameservers", NULL);
}

/*
 * TODO: Reorder text fields
 * TODO: Show empty fields properly
 */
static void add_immutable_openconnect_page(struct settings *sett)
{
	struct settings_page *page;

	page = add_page_to_settings(sett, "Openconnect", TRUE);
	settings_add_text(page, _("Server fingerprint"),
			  "OpenConnect.ServerCert", NULL);
	settings_add_text(page, _("Certificate Authority file"),
			  "OpenConnect.CACert", NULL);
	settings_add_text(page, _("Client certificate file"),
			  "OpenConnect.ClientCert", NULL);
	settings_add_text(page, _("VPN host"),
			  "OpenConnect.VPNHost", NULL);
}

static void add_immutable_openvpn_page(struct settings *sett)
{
	struct settings_page *page;

	page = add_page_to_settings(sett, "OpenVPN", TRUE);
	settings_add_text(page, _("Certificate Authority file"),
			  "OpenVPN.CACert", NULL);
	settings_add_text(page, _("Client certificate file"),
			  "OpenVPN.Cert", NULL);
	settings_add_text(page, _("Client private key file"),
			  "OpenVPN.Key", NULL);
	settings_add_text(page, _("Maximum transmission unit (MTU)"),
			  "OpenVPN.MTU", NULL);
	settings_add_text(page, _("Peer certificate type"),
			  "OpenVPN.NSCertType", NULL);
	settings_add_text(page, _("Protocol"), "OpenVPN.Proto", NULL);
	settings_add_text(page, _("Port"), "OpenVPN.Port", NULL);
	settings_add_text(page, _("Authenticate using username/password"),
			  "OpenVPN.AuthUserPass", NULL);
	settings_add_text(page, _("Private key password file"),
			  "OpenVPN.AskPass", NULL);
	settings_add_text(page, _("Do not cache password"),
			  "OpenVPN.AuthNoCache", NULL);
	settings_add_text(page, _("Cipher"), "OpenVPN.Cipher", NULL);
	settings_add_text(page,
			  _("Message digest algorithm for packet authentication"),
			  "OpenVPN.Auth", NULL);
	settings_add_text(page, _("Use fast LZO compression"),
			  "OpenVPN.CompLZO", NULL);
	settings_add_text(page, _("External configuration file"),
			  "OpenVPN.ConfigFile", NULL);
}

static void add_immutable_vpnc_page(struct settings *sett)
{
	struct settings_page *page;

	page = add_page_to_settings(sett, "VPNC", TRUE);
	settings_add_text(page, _("Group username"), "VPNC.IPSec.ID", NULL);
	settings_add_text(page, _("Username"), "VPNC.XAuth.ID", NULL);
	settings_add_text(page, _("IKE Authentication mode"),
			  "VPNC.IKE.Authmode", NULL);
	settings_add_text(page, _("Diffie-Hellman group"), "VPNC.PFS", NULL);
	settings_add_text(page, _("Domain"), "VPNC.Domain", NULL);
	settings_add_text(page, _("Vendor"), "VPNC.Vendor", NULL);
	settings_add_text(page, _("Local port"), "VPNC.LocalPort", NULL);
	settings_add_text(page, _("Cisco port"), "VPNC.CiscoPort", NULL);
	settings_add_text(page, _("Application version"), "VPNC.AppVersion",
			  NULL);
	settings_add_text(page, _("NAT traversal method"), "VPNC.NATTMode",
			  NULL);
	settings_add_text(page, _("DPD idle timeout"), "VPNC.DPDTimeout", NULL);
	settings_add_text(page, _("Single DES encryption"), "VPNC.SingleDES",
			  NULL);
	settings_add_text(page, _("No encryption"), "VPNC.NoEncryption", NULL);
}

static void add_immutable_l2tp_page(struct settings *sett)
{
	struct settings_page *page;

	page = add_page_to_settings(sett, "L2TP", TRUE);
	settings_add_text(page, _("Username"), "L2TP.User", NULL);
	settings_add_text(page, _("Maximum bandwidth (bps)"), "L2TP.BPS", NULL);
	settings_add_text(page, _("Maximum transmit bandwidth (tx)"),
			  "L2TP.TXBPS", NULL);
	settings_add_text(page, _("Maximum receive bandwidth (rx)"),
			  "L2TP.RXBPS", NULL);
	settings_add_text(page, _("Use length bit"), "L2TP.LengthBit", NULL);
	settings_add_text(page, _("Use challenge authentication"),
			  "L2TP.Challenge", NULL);
	settings_add_text(page, _("Default route"), "L2TP.DefaultRoute", NULL);
	settings_add_text(page, _("Use seq numbers"), "L2TP.FlowBit", NULL);
	settings_add_text(page, _("Window size"), "L2TP.TunnelRWS", NULL);
	settings_add_text(page, _("Use only one control channel"),
			  "L2TP.Exclusive", NULL);
	settings_add_text(page, _("Redial if disconnected"),
			  "L2TP.Redial", NULL);
	settings_add_text(page, _("Redial timeout"),
			  "L2TP.RedialTimeout", NULL);
	settings_add_text(page, _("Maximum redial tries"),
			  "L2TP.MaxRedials", NULL);
	settings_add_text(page, _("Require PAP"), "L2TP.RequirePAP", NULL);
	settings_add_text(page, _("Require CHAP"), "L2TP.RequireCHAP", NULL);
	settings_add_text(page, _("Require authentication"),
			  "L2TP.ReqAuth", NULL);
	settings_add_text(page, _("Accept only these peers"),
			  "L2TP.AccessControl", NULL);
	settings_add_text(page, _("Authentication file location"),
			  "L2TP.AuthFile", NULL);
	settings_add_text(page, _("Listen address"), "L2TP.ListenAddr", NULL);
	settings_add_text(page, _("Use IPSec SA"), "L2TP.IPsecSaref", NULL);
	settings_add_text(page, _("UDP Port"), "L2TP.Port", NULL);
}

static void add_immutable_pptp_page(struct settings *sett)
{
	struct settings_page *page;

	page = add_page_to_settings(sett, "PPTP", TRUE);
	settings_add_text(page, _("Username"), "PPTP.User", NULL);
}

static void add_immutable_pppd_page(struct settings *sett)
{
	struct settings_page *page;

	page = add_page_to_settings(sett, "PPPD", TRUE);
	settings_add_text(page, _("Dead peer check count"),
			  "PPPD.EchoFailure", NULL);
	settings_add_text(page, _("Dead peer check interval"),
			  "PPPD.EchoInterval", NULL);
	settings_add_text(page, _("Debug level"), "PPPD.Debug", NULL);
	settings_add_text(page, _("Refuse EAP authentication"),
			  "PPPD.RefuseEAP", NULL);
	settings_add_text(page, _("Refuse PAP authentication"),
			  "PPPD.RefusePAP", NULL);
	settings_add_text(page, _("Refuse CHAP authentication"),
			  "PPPD.RefusePAP", NULL);
	settings_add_text(page, _("Refuse MSCHAP authentication"),
			  "PPPD.RefuseMSCHAP", NULL);
	settings_add_text(page, _("Refuse MSCHAPv2 authentication"),
			  "PPPD.RefuseMSCHAP2", NULL);
	settings_add_text(page, _("Disable BSD compression"),
			  "PPPD.NoBSDComp", NULL);
	settings_add_text(page, _("Disable DEFLATE compression"),
			  "PPPD.NoDeflate", NULL);
	settings_add_text(page, _("Require the use of MPPE"),
			  "PPPD.RequirMPPE", NULL);
	settings_add_text(page, _("Require the use of MPPE 40 bit"),
			  "PPPD.RequirMPPE40", NULL);
	settings_add_text(page, _("Require the use of MPPE 128 bit"),
			  "PPPD.RequirMPPE128", NULL);
	settings_add_text(page, _("Allow MPPE to use stateful mode"),
			  "PPPD.RequirMPPEStateful", NULL);
	settings_add_text(page, _("No Van Jacobson compression"),
			  "PPPD.NoVJ", NULL);
}

static void clear_cb(GtkWidget *button, gpointer user_data)
{
	service_clear_properties(user_data);
}

static void remove_cb(GtkWidget *button, gpointer user_data)
{
	service_remove(user_data);
}

static void add_clear_page(struct settings *sett)
{
	struct settings_page *page;
	GtkWidget *clear, *clear_l, *remove, *remove_l;

	page = add_page_to_settings(sett, _("_Clear settings"), TRUE);

	clear = gtk_button_new_with_mnemonic(_("_Reset"));
	remove = gtk_button_new_with_mnemonic(_("_Forget"));
	clear_l = gtk_label_new(_("Set all properties to defaults."));
	remove_l = gtk_label_new(_("Remove this service."));

	g_signal_connect(clear, "clicked", G_CALLBACK(clear_cb), sett->serv);
	g_signal_connect(remove, "clicked", G_CALLBACK(remove_cb), sett->serv);

	gtk_widget_set_halign(clear_l, GTK_ALIGN_START);
	gtk_widget_set_halign(remove_l, GTK_ALIGN_START);
	style_set_margin(clear, MARGIN_LARGE);
	style_set_margin(remove, MARGIN_LARGE);
	gtk_widget_set_margin_top(clear, 0);
	gtk_widget_set_margin_bottom(clear, 0);

	gtk_grid_attach(GTK_GRID(page->grid), clear, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(page->grid), clear_l, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(page->grid), remove, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(page->grid), remove_l, 1, 1, 1, 1);

	gtk_widget_show_all(page->grid);

	if(sett->serv->type == CONNECTION_TYPE_ETHERNET) {
		gtk_widget_hide(remove);
		gtk_widget_hide(remove_l);
		gtk_widget_set_margin_top(remove, 0);
		gtk_widget_set_margin_bottom(remove, 0);
	}
}

static void add_pages(struct settings *sett)
{
	gboolean immutable = service_get_property_boolean(sett->serv,
							  "Immutable", NULL);
	if(immutable || sett->serv->type == CONNECTION_TYPE_VPN) {
		gchar *type;

		if(sett->serv->type != CONNECTION_TYPE_VPN)
			add_info_page(sett);
		else
			add_vpn_info_page(sett);

		add_immutable_ipv_page(sett);
		add_immutable_server_page(sett);
		if(sett->serv->type != CONNECTION_TYPE_VPN)
			return;

		type = service_get_property_string_raw(sett->serv,
						       "Type", NULL);
		if(!strcmp(type, "openconnect"))
			add_immutable_openconnect_page(sett);
		else if(!strcmp(type, "openvpn"))
			add_immutable_openvpn_page(sett);
		else if(!strcmp(type, "vpnc"))
			add_immutable_vpnc_page(sett);
		else if(!strcmp(type, "l2tp") || !strcmp(type, "pptp")) {
			if(!strcmp(type, "l2tp"))
				add_immutable_l2tp_page(sett);
			else
				add_immutable_pptp_page(sett);
			add_immutable_pppd_page(sett);
		}

		g_free(type);

		return;
	}

	add_info_page(sett);
	add_ipv_page(sett, 4);
	add_ipv_page(sett, 6);
	add_server_page(sett);
	add_proxy_page(sett);
	add_clear_page(sett);
}

static void append_dict_inner(const gchar *key, const gchar *subkey,
                              gpointer value, gpointer user_data)
{
	DualHashTable *dict = user_data;
	struct settings_content *content = value;
	if(!content->writable(content))
		return;
	GVariant *variant = content->value(content);
	if(!variant)
		return;
	hash_table_set_dual_key(dict, content->key, content->subkey,
	                        g_variant_ref(variant));

}

static void apply_cb(GtkWidget *window, gpointer user_data)
{
	struct settings *sett = user_data;
	DualHashTable *table =
	        dual_hash_table_new((GDestroyNotify)g_variant_unref);
	GVariant *out;

	dual_hash_table_foreach(sett->contents, append_dict_inner, table);
	out = dual_hash_table_to_variant(table);
	service_set_properties(sett->serv, out);
	g_variant_unref(out);
	dual_hash_table_unref(table);
}

static gboolean delete_event(GtkWidget *window, GdkEvent *event,
                             gpointer user_data)
{
	free_settings(user_data);
	return FALSE;
}

static void init_settings(struct settings *sett)
{
	gchar *name;
	gchar *title;
	GtkWidget *frame;
	GtkGrid *grid = GTK_GRID(gtk_grid_new());

	sett->invalid_count = 0;
	sett->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	name = service_get_property_string(sett->serv, "Name", NULL);
	title = g_strdup_printf("%s - %s", _("Network Settings"), name);
	gtk_window_set_title(GTK_WINDOW(sett->window), title);
	gtk_window_set_default_size(GTK_WINDOW(sett->window), SETTINGS_WIDTH,
	                            SETTINGS_HEIGHT);

	g_free(title);
	g_free(name);

	sett->list = gtk_list_box_new();
	sett->notebook = gtk_notebook_new();
	sett->apply = gtk_button_new_with_mnemonic(_("_Apply"));
	frame = gtk_frame_new(NULL);

	g_object_ref(sett->window);
	g_object_ref(sett->list);
	g_object_ref(sett->notebook);

	g_signal_connect(sett->window, "delete-event", G_CALLBACK(delete_event),
	                 sett);
	g_signal_connect(sett->list, "row-selected",
	                 G_CALLBACK(list_item_selected), sett->notebook);
	g_signal_connect(sett->apply, "clicked",
	                 G_CALLBACK(apply_cb), sett);

	style_set_margin(GTK_WIDGET(grid), MARGIN_LARGE);
	gtk_widget_set_margin_top(sett->apply, MARGIN_LARGE);

	gtk_widget_set_size_request(sett->list, SETTINGS_LIST_WIDTH, -1);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(sett->notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(sett->notebook), FALSE);
	gtk_widget_set_vexpand(sett->list, TRUE);
	gtk_widget_set_hexpand(sett->notebook, TRUE);
	gtk_widget_set_vexpand(sett->notebook, TRUE);
	gtk_widget_set_valign(sett->apply, GTK_ALIGN_END);
	gtk_widget_set_halign(sett->apply, GTK_ALIGN_END);

	gtk_container_add(GTK_CONTAINER(frame), sett->list);
	gtk_grid_attach(grid, frame, 0, 0, 1, 2);
	gtk_grid_attach(grid, sett->notebook, 1, 0, 1, 1);
	gtk_grid_attach(grid, sett->apply, 1, 1, 1, 1);
	gtk_container_add(GTK_CONTAINER(sett->window), GTK_WIDGET(grid));

	gtk_widget_show_all(sett->window);

	if(sett->serv->type == CONNECTION_TYPE_VPN)
		gtk_widget_hide(sett->apply);

	add_pages(sett);
}

struct settings *settings_create(struct service *serv,
                                 void (*closed)(struct service *serv))
{
	struct settings *sett;

	sett = g_malloc(sizeof(*sett));

	sett->serv = serv;
	sett->closed = closed;
	sett->callbacks = dual_hash_table_new(content_callback_free);
	sett->contents = dual_hash_table_new(NULL);

	init_settings(sett);

	return sett;
}

void settings_update(struct settings *sett, const gchar *key,
                     const gchar *subkey, GVariant *value)
{
	struct content_callback *cb;

	cb = hash_table_get_dual_key(sett->callbacks, key, subkey);
	if(cb)
		handle_content_callback(value, key, subkey, cb);
}

void settings_set_callback(struct settings *sett, const gchar *key,
                           const gchar *subkey, struct content_callback *cb)
{
	hash_table_set_dual_key(sett->callbacks, key, subkey, cb);
}

