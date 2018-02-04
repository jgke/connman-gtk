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

#ifndef _CONNMAN_GTK_OPENCONNECT_H
#define _CONNMAN_GTK_OPENCONNECT_H

#include <glib.h>
#include <gio/gio.h>

GVariantDict *openconnect_handle(GDBusMethodInvocation *invocation,
				 GVariant *args);
gboolean is_openconnect(GVariant *args);

#endif /* _CONNMAN_GTK_OPENCONNECT_H */
