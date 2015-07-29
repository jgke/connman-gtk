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
#include <string.h>

#include "connection.h"

enum connection_type connection_type_from_string(const gchar *str)
{
	if(!strcmp(str, "ethernet"))
		return CONNECTION_TYPE_ETHERNET;
	if(!strcmp(str, "wifi"))
		return CONNECTION_TYPE_WIRELESS;
	if(!strcmp(str, "bluetooth"))
		return CONNECTION_TYPE_BLUETOOTH;
	if(!strcmp(str, "cellular"))
		return CONNECTION_TYPE_CELLULAR;
	if(!strcmp(str, "p2p"))
		return CONNECTION_TYPE_P2P;
	if(!strcmp(str, "vpn"))
		return CONNECTION_TYPE_VPN;
	if(!strcmp(str, "openconnect") || !strcmp(str, "openvpn") ||
	   !strcmp(str, "vpnc") || !strcmp(str, "l2tp") || !strcmp(str, "pptp"))
		return CONNECTION_TYPE_VPN;
	return CONNECTION_TYPE_UNKNOWN;
}

enum connection_type connection_type_from_properties(GVariant *properties)
{
	enum connection_type type = CONNECTION_TYPE_UNKNOWN;
	GVariantDict *dict = g_variant_dict_new(properties);
	GVariant *type_v = g_variant_dict_lookup_value(dict, "Type", NULL);
	if(type_v) {
		const gchar *type_s;

		type_s = g_variant_get_string(type_v, NULL);
		type = connection_type_from_string(type_s);
		g_variant_unref(type_v);
	}
	g_variant_dict_unref(dict);

	return type;
}
