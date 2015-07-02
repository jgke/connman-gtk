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
	return CONNECTION_TYPE_UNKNOWN;
}

enum connection_type connection_type_from_path(const gchar *str)
{
	gchar *path = g_strdup(str);
	/* find and replace first _ with 0 */
	*strchr(path, '_') = '\0';
	str = strrchr(path, '/') + 1;
	enum connection_type type = connection_type_from_string(str);
	g_free(path);
	return type;
}
