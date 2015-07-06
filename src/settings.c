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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "connection.h"
#include "service.h"
#include "settings.h"
#include "style.h"

struct settings {
	GtkWidget *window;

	struct service *serv;
};

static struct {
	struct settings *(*create)(void);
	void (*free)(struct settings *sett);
	void (*init)(struct settings *sett);
	void (*property_changed)(struct settings *tech, const gchar *name);
} functions[CONNECTION_TYPE_COUNT] = {
	{}
};

void settings_init(struct settings *sett)
{
	GVariant *name_v;
	gchar *title;

	name_v = service_get_property(sett->serv, "Name", NULL);
	title = g_strdup_printf("%s - %s", _("Network Settings"),
	                        g_variant_get_string(name_v, NULL));
	gtk_window_set_title(GTK_WINDOW(sett->window), title);
	g_free(title);
	gtk_window_set_default_size(GTK_WINDOW(sett->window), DEFAULT_WIDTH,
	                            DEFAULT_HEIGHT);

	if(functions[sett->serv->type].init)
		functions[sett->serv->type].init(sett);

	gtk_widget_show_all(sett->window);
}

void settings_create(struct service *serv)
{
	struct settings *sett;

	if(functions[serv->type].create)
		sett = functions[serv->type].create();
	else
		sett = g_malloc(sizeof(*sett));

	sett->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	sett->serv = serv;

	settings_init(sett);
}
