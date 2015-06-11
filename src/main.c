/*
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

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

static void activate(GtkApplication *app, gpointer user_data) {
	GtkWidget *window;
	GtkWidget *label;

	window = gtk_application_window_new(app);
	label = gtk_label_new(_("Settings"));
	gtk_container_add(GTK_CONTAINER(window), label);
	gtk_window_set_title(GTK_WINDOW(window), _("Network Settings"));
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);
	gtk_widget_show_all(window);
}

int main(int argc, char *argv[])
{
	GtkApplication *app;
	int status;

	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, CONNMAN_GTK_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
