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
#include <glib/gi18n.h>

#include "config.h"
#include "connection.h"
#include "main.h"
#include "status.h"
#include "service.h"
#include "technology.h"

#ifdef USE_STATUS_ICON
GtkStatusIcon *icon;

static void status_activate(gpointer *ignored, gpointer user_data)
{
	g_signal_emit_by_name(user_data, "activate");
}

static void status_exit(gpointer *ignored, gpointer user_data)
{
	g_signal_emit_by_name(main_window, "destroy");
}

static void status_toggle_connection(gpointer *ignored, gpointer user_data)
{
	service_toggle_connection(user_data);
}

static GtkWidget *create_service_item(struct service *serv)
{
	gchar *name, *state, *state_r, *label;
	GtkWidget *item, *child;

	name = service_get_property_string(serv, "Name", NULL);
	state = service_get_property_string(serv, "State", NULL);
	state_r = service_get_property_string_raw(serv, "State", NULL);

	/* Todo: is autoupdate needed here? */
	if(strcmp(state_r, "idle"))
		label = g_markup_printf_escaped("<b>%s - %s</b>", name, state);
	else
		label = g_markup_printf_escaped("%s - %s", name, state);

	item = gtk_menu_item_new_with_label(NULL);
	child = gtk_bin_get_child(GTK_BIN(item));
	gtk_label_set_markup(GTK_LABEL(child), label);
	g_signal_connect(item, "activate", G_CALLBACK(status_toggle_connection),
			 serv);

	g_free(name);
	g_free(state);
	g_free(state_r);
	g_free(label);

	return item;
}

static void status_menu(GtkStatusIcon *icon, guint button, guint activate_time,
                        gpointer user_data)
{
	GtkMenu *menu;
	GtkWidget *open, *exit;
	int index;

	menu = GTK_MENU(gtk_menu_new());

	open = gtk_menu_item_new_with_label(_("Open app"));
	exit = gtk_menu_item_new_with_label(_("Exit"));
	g_signal_connect(open, "activate", G_CALLBACK(status_activate),
			 user_data);
	g_signal_connect(exit, "activate", G_CALLBACK(status_exit), user_data);
	gtk_container_add(GTK_CONTAINER(menu), open);
	gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());

	for(index = CONNECTION_TYPE_ETHERNET; index < CONNECTION_TYPE_COUNT; index++) {
		const gchar *label;
		struct technology *tech;
		GHashTableIter iter;
		gpointer key, service;
		GtkMenuItem *item;
		GtkMenu *submenu;

		tech = technologies[index];
		if(!tech)
			continue;
		if(!technology_get_property_bool(tech, "Powered"))
			continue;
		submenu = GTK_MENU(gtk_menu_new());
		label = translated_tech_name(tech->type);
		item = GTK_MENU_ITEM(gtk_menu_item_new_with_label(label));

		g_hash_table_iter_init(&iter, tech->services);
		while(g_hash_table_iter_next(&iter, &key, &service)) {
			GtkWidget *item = create_service_item(service);
			gtk_container_add(GTK_CONTAINER(submenu), item);
		}

		gtk_menu_item_set_submenu(item, GTK_WIDGET(submenu));
		gtk_container_add(GTK_CONTAINER(menu), GTK_WIDGET(item));
	}

	gtk_container_add(GTK_CONTAINER(menu), gtk_separator_menu_item_new());
	gtk_container_add(GTK_CONTAINER(menu), exit);
	gtk_widget_show_all(GTK_WIDGET(menu));
	gtk_menu_popup(menu, NULL, NULL, NULL, NULL, button, activate_time);
}

void status_update(void) {
	int best_status = 0;
	int index;

	if(!status_icon_enabled)
		return;

	for(index = CONNECTION_TYPE_ETHERNET; index < CONNECTION_TYPE_COUNT; index++) {
		struct technology *tech;
		GHashTableIter iter;
		gpointer key, service;

		tech = technologies[index];
		if(!tech)
			continue;

		g_hash_table_iter_init(&iter, tech->services);
		while(g_hash_table_iter_next(&iter, &key, &service)) {
			gchar *state;
			state = service_get_property_string_raw(service, "State",
			                                        NULL);
			switch(best_status) {
				case 0:
					if(!strcmp(state, "association"))
						best_status = 1;
				case 1:
					if(!strcmp(state, "configuration"))
						best_status = 2;
				case 2:
					if(!strcmp(state, "ready"))
						best_status = 3;
				case 3:
					if(!strcmp(state, "online"))
						best_status = 4;
			}

			g_free(state);

			if(best_status == 4)
				break;
		}
	}

	switch(best_status) {
		case 0:
			gtk_status_icon_set_from_icon_name(icon, "network-offline");
			break;
		case 1:
		case 2:
			gtk_status_icon_set_from_icon_name(icon, "preferences-system-network");
			break;
		case 3:
		case 4:
			gtk_status_icon_set_from_icon_name(icon, "network-transmit-receive");
			break;
	}
}

void status_init(GtkApplication *app)
{
	if(!status_icon_enabled)
		return;

	icon = gtk_status_icon_new_from_icon_name("preferences-system-network");
	g_signal_connect(icon, "activate", G_CALLBACK(status_activate), app);
	g_signal_connect(icon, "popup-menu", G_CALLBACK(status_menu), app);

	status_update();
}
#else
void status_init(GtkApplication *app)
{
}
void status_update(void)
{
}
#endif /* USE_STATUS_ICON */
