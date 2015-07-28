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

#ifndef _CONNMAN_GTK_STYLE_H
#define _CONNMAN_GTK_STYLE_H

#include <gtk/gtk.h>

#define DEFAULT_WIDTH 650
#define DEFAULT_HEIGHT 401
#define LIST_WIDTH 200

#define SETTINGS_WIDTH 650
#define SETTINGS_HEIGHT 401
#define SETTINGS_LIST_WIDTH 150

#define MARGIN_SMALL 5
#define MARGIN_MEDIUM 10
#define MARGIN_LARGE 15

extern GtkCssProvider *css_provider;

void style_init();
void label_align_text_left(GtkLabel *label);
void style_add_context(GtkWidget *widget);
void style_add_margin(GtkWidget *widget, gint margin);

#endif /* _CONNMAN_GTK_STYLE_H */
