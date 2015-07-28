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

#include "style.h"

GtkCssProvider *css_provider;

void style_init()
{
	GError *error = NULL;
	css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_path(css_provider,
	                                CONNMAN_GTK_UIDIR "stylesheet.css",
	                                &error);
	if(error != NULL) {
		g_warning("couldn't load stylesheet %s: %s",
		          CONNMAN_GTK_UIDIR "stylesheet.css",
		          error->message);
		g_error_free(error);
	}
}

void label_align_text_left(GtkLabel *label)
{
	gtk_label_set_line_wrap(label, TRUE);
	gtk_label_set_justify(label, GTK_JUSTIFY_LEFT);
#if (GTK_MAJOR_VERSION > 3) || (GTK_MINOR_VERSION >= 16)
	if(gtk_get_major_version() > 3 || gtk_get_minor_version() >= 16)
		gtk_label_set_xalign(label, 0);
	else
#endif
		/* deprecated at 3.14, but above only implemented at 3.16 */
		gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
}

void style_add_context(GtkWidget *widget)
{
	gtk_style_context_add_provider(gtk_widget_get_style_context(widget),
				       GTK_STYLE_PROVIDER(css_provider),
				       GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

void style_add_margin(GtkWidget *widget, gint margin)
{
	gtk_widget_set_margin_start(widget, margin);
	gtk_widget_set_margin_end(widget, margin);
	gtk_widget_set_margin_top(widget, margin);
	gtk_widget_set_margin_bottom(widget, margin);
}
