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

#ifndef _CONNMAN_GTK_CONNECTION_H
#define _CONNMAN_GTK_CONNECTION_H

enum connection_type {
	CONNECTION_TYPE_UNKNOWN,
	CONNECTION_TYPE_ETHERNET,
	CONNECTION_TYPE_WIRELESS,
	CONNECTION_TYPE_BLUETOOTH,
	CONNECTION_TYPE_CELLULAR,
	CONNECTION_TYPE_P2P,
	CONNECTION_TYPE_VPN,
	CONNECTION_TYPE_COUNT
};

enum connection_type connection_type_from_string(const gchar *str);
enum connection_type connection_type_from_path(const gchar *str);

#endif /* _CONNMAN_GTK_CONNECTION_H */
