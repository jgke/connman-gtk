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

#include "config.h"
#include "style.h"

GtkCssProvider *css_provider;

void style_init()
{
	static const char *stylesheet;
	GError *error = NULL;

	stylesheet = "" \
		      ".cm-header-title {" \
		      "  font-weight: bold;" \
		      "}" \
		      ".cm-wireless-hidden {" \
		      "  font-style: italic;" \
		      "}" \
		      ".cm-log {" \
		      "  background-color: white;" \
		      "  padding: 5px;" \
		      "}" \
		      "";

	css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(css_provider, stylesheet, -1, &error);
	if(error != NULL) {
		g_warning("couldn't load stylesheet: %s", error->message);
		g_error_free(error);
	}
}

void label_align_text(GtkLabel *label, gfloat xalign, gfloat yalign)
{
	gtk_label_set_line_wrap(label, TRUE);
	gtk_label_set_justify(label, GTK_JUSTIFY_LEFT);
#if (GTK_MAJOR_VERSION > 3) || (GTK_MINOR_VERSION >= 16)
	if(xalign >= 0)
		gtk_label_set_xalign(label, xalign);
	if(yalign >= 0)
		gtk_label_set_xalign(label, yalign);
#else
	/* deprecated at 3.14, but above only implemented at 3.16 */
	if(xalign < 0)
		gtk_misc_get_alignment(GTK_MISC(label), &xalign, NULL);
	if(yalign < 0)
		gtk_misc_get_alignment(GTK_MISC(label), NULL, &yalign);
	gtk_misc_set_alignment(GTK_MISC(label), xalign, yalign);
#endif
}

void style_add_context(GtkWidget *widget)
{
	gtk_style_context_add_provider(gtk_widget_get_style_context(widget),
				       GTK_STYLE_PROVIDER(css_provider),
				       GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

void style_set_margin(GtkWidget *widget, gint margin)
{
	style_set_margin_start(widget, margin);
	style_set_margin_end(widget, margin);
	gtk_widget_set_margin_top(widget, margin);
	gtk_widget_set_margin_bottom(widget, margin);
}

void style_set_margin_start(GtkWidget *widget, gint margin)
{
#if (GTK_MAJOR_VERSION > 3) || (GTK_MINOR_VERSION >= 12)
	gtk_widget_set_margin_start(widget, margin);
#else
	if(gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL)
		gtk_widget_set_margin_right(widget, margin);
	else
		gtk_widget_set_margin_left(widget, margin);
#endif
}

void style_set_margin_end(GtkWidget *widget, gint margin)
{
#if (GTK_MAJOR_VERSION > 3) || (GTK_MINOR_VERSION >= 12)
	gtk_widget_set_margin_end(widget, margin);
#else
	if(gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL)
		gtk_widget_set_margin_left(widget, margin);
	else
		gtk_widget_set_margin_right(widget, margin);
#endif
}
