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

#define DEFAULT_WIDTH 600
#define DEFAULT_HEIGHT 371
#define LIST_WIDTH 200

#define SETTINGS_WIDTH DEFAULT_WIDTH
#define SETTINGS_HEIGHT DEFAULT_HEIGHT
#define SETTINGS_LIST_WIDTH 150

#define MARGIN_SMALL 5
#define MARGIN_MEDIUM 10
#define MARGIN_LARGE 15

#define STYLE_ADD_CONTEXT(widget) \
        do { \
                gtk_style_context_add_provider( \
                                gtk_widget_get_style_context(widget), \
                                GTK_STYLE_PROVIDER(css_provider), \
                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION); \
        } while(0)

#define STYLE_ADD_MARGIN(widget, margin) \
        do { \
                gtk_widget_set_margin_start(widget, margin); \
                gtk_widget_set_margin_end(widget, margin); \
                gtk_widget_set_margin_top(widget, margin); \
                gtk_widget_set_margin_bottom(widget, margin); \
        } while(0)


extern GtkCssProvider *css_provider;

void style_init();

#endif /* _CONNMAN_GTK_STYLE_H */
