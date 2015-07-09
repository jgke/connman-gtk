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

#ifndef _CONNMAN_GTK_UTIL_H
#define _CONNMAN_GTK_UTIL_H

gchar *variant_str_array_to_str(GVariant *variant);
gchar *variant_to_str(GVariant *variant);
gboolean variant_to_bool(GVariant *variant);
const gchar *status_localized(const gchar *status);
gboolean valid_ipv4(const gchar *address);
gboolean valid_ipv6(const gchar *address);

#endif /* _CONNMAN_GTK_UTIL_H */
