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
#include <gio/gio.h>
#include <openconnect.h>
#include <string.h>

GVariantDict *openconnect_handle(GDBusMethodInvocation *invocation,
				 GVariant *args,
				 gchar *(*ask_token)(const gchar *label,
						     gboolean hidden))
{
	GHashTable *info, *required;
	GVariantDict *out;
	GVariantIter *iter;
	gchar *key;
	GVariant *value;
	const gchar *user, *pass, *CACert, *ClientCert;
	gchar *Cookie, *ServerCert, *Host;
	GVariant *parameters;

	parameters = g_variant_get_child_value(args, 1);

	user = pass = CACert = ClientCert = Cookie = ServerCert = Host = NULL;

	info = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	required = g_hash_table_new_full(g_str_hash, g_str_equal,
					 g_free, g_free);

	iter = g_variant_iter_new(parameters);
	while(g_variant_iter_loop(iter, "{sv}", &key, &value)) {
		GVariantDict *dict = g_variant_dict_new(value);
		GVariant *req_v;
		GVariant *val_v;
		gchar *req, *val;

		req_v = g_variant_dict_lookup_value(dict, "Requirement", NULL);
		val_v = g_variant_dict_lookup_value(dict, "Value", NULL);
		req = g_variant_dup_string(req_v, NULL);
		if(val_v) {
			val = g_variant_dup_string(val_v, NULL);
			if(!strcmp(req, "informational"))
				g_hash_table_insert(info, g_strdup(key), val);
			g_variant_unref(val_v);
		} else if(!strcmp(req, "mandatory"))
			g_hash_table_insert(required, g_strdup(key), NULL);
		g_variant_unref(req_v);
		g_variant_dict_unref(dict);
	}
	g_variant_iter_free(iter);

	user = g_hash_table_lookup(info, "Username");
	pass = g_hash_table_lookup(info, "Password");
	CACert = g_hash_table_lookup(info, "CACert");
	ClientCert = g_hash_table_lookup(info, "ClientCert");
	Cookie = g_hash_table_lookup(info, "Cookie");
	ServerCert = g_hash_table_lookup(info, "ServerCert");
	Host = g_hash_table_lookup(info, "Host");

	out = g_variant_dict_new(NULL);
	g_variant_dict_insert(out, "OpenConnect.Cookie", "s",
			      ask_token("Cookie", TRUE));
	g_variant_dict_insert(out, "OpenConnect.ServerCert", "s",
			      ask_token("Server cert hash", TRUE));
	g_variant_dict_insert(out, "OpenConnect.VPNHost", "s",
			      ask_token("VPN host", TRUE));

	g_hash_table_unref(required);
	g_hash_table_unref(info);

	return out;
}
