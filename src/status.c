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

#include <gtk/gtk.h>

#include "status.h"

#ifdef USE_STATUS_ICON
static void status_activate(GtkStatusIcon *icon, gpointer user_data)
{
	g_signal_emit_by_name(user_data, "activate");
}

void status_init(GtkApplication *app)
{
	GtkStatusIcon *icon;

	icon = gtk_status_icon_new_from_icon_name("preferences-system-network");
	g_signal_connect(icon, "activate", G_CALLBACK(status_activate), app);
}
#else
void status_init(GtkApplication *app)
{
}
#endif /* USE_STATUS_ICON */
