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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "connection.h"
#include "service.h"
#include "settings.h"
#include "style.h"

static struct {
	struct settings *(*create)(void);
	void (*free)(struct settings *sett);
	void (*init)(struct settings *sett);
	void (*property_changed)(struct settings *tech, const gchar *name);
} functions[CONNECTION_TYPE_COUNT] = {
	{}
};

static void page_selected(GtkListBox *box, GtkListBoxRow *row, gpointer data)
{
	if(!G_IS_OBJECT(row))
		return;
	struct settings *sett = data;
	GtkWidget *content = g_object_get_data(G_OBJECT(row), "content");
	gint num = gtk_notebook_page_num(GTK_NOTEBOOK(sett->notebook),
	                                 content);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(sett->notebook), num);
}

void settings_add_content(struct settings_page *page,
                          struct settings_content *content)
{
	gtk_grid_attach(GTK_GRID(page->grid), content->content,
	                page->index, 0, 1, 1);
	page->index++;
}

gboolean always_valid(gchar *value)
{
	return TRUE;
}

void free_content(GtkWidget *widget, gpointer user_data)
{
	struct settings_content *content = user_data;
	content->free(content);
}

GtkWidget *settings_add_text(struct settings_page *page, const gchar *label,
                             const gchar *value)
{
	GtkWidget *label_w, *value_w;
	struct settings_content *content = g_malloc(sizeof(*content));
	content->content = gtk_grid_new();
	content->valid = always_valid;
	content->free = g_free;

	label_w = gtk_label_new(label);
	value_w = gtk_label_new(value);

	g_object_set_data(G_OBJECT(content->content), "content", content);

	g_signal_connect(content->content, "destroy",
	                 G_CALLBACK(free_content), content);

	STYLE_ADD_MARGIN(content->content, MARGIN_LARGE);
	STYLE_ADD_CONTEXT(label_w);
	gtk_style_context_add_class(gtk_widget_get_style_context(label_w),
	                            "dim-label");
	gtk_widget_set_margin_end(label_w, MARGIN_SMALL);
	gtk_widget_set_margin_start(value_w, MARGIN_SMALL);

	gtk_widget_set_halign(label_w, GTK_ALIGN_END);
	gtk_widget_set_halign(value_w, GTK_ALIGN_START);
	gtk_widget_set_hexpand(content->content, TRUE);
	gtk_widget_set_hexpand(label_w, TRUE);
	gtk_widget_set_hexpand(value_w, TRUE);

	gtk_grid_attach(GTK_GRID(content->content), label_w, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(content->content), value_w, 1, 0, 1, 1);

	gtk_widget_show_all(content->content);

	settings_add_content(page, content);

	return value_w;
}

void free_page(GtkWidget *widget, gpointer user_data)
{
	struct settings_page *page = user_data;
	g_free(page);
}

struct settings_page *settings_add_page(struct settings *sett,
                                        const gchar *name)
{
	struct settings_page *page = g_malloc(sizeof(*page));

	page->index = 0;

	page->grid = gtk_grid_new();
	GtkWidget *item = gtk_list_box_row_new();
	GtkWidget *label = gtk_label_new(name);
	GtkWidget *frame = gtk_frame_new(NULL);

	g_object_set_data(G_OBJECT(item), "content", page->grid);

	g_signal_connect(page->grid, "destroy",
	                 G_CALLBACK(free_page), page);

	STYLE_ADD_MARGIN(label, MARGIN_SMALL);
	gtk_widget_set_margin_start(page->grid, MARGIN_LARGE);
	gtk_widget_set_margin_start(label, MARGIN_LARGE);

	gtk_widget_set_halign(label, GTK_ALIGN_START);

	gtk_widget_set_hexpand(frame, TRUE);
	gtk_widget_set_vexpand(frame, TRUE);

	gtk_container_add(GTK_CONTAINER(item), label);
	gtk_container_add(GTK_CONTAINER(page->grid), frame);
	gtk_container_add(GTK_CONTAINER(sett->list), item);

	gtk_widget_show_all(page->grid);

	gtk_notebook_append_page(GTK_NOTEBOOK(sett->notebook), page->grid,
	                         NULL);

	return page;
}

void settings_free(struct settings *sett)
{
	if(functions[sett->serv->type].free)
		functions[sett->serv->type].free(sett);
	else
		g_free(sett);
}

static gboolean delete_event(GtkWidget *window, GdkEvent *event,
                             gpointer user_data)
{
	settings_free(user_data);
	return FALSE;
}

void settings_init(struct settings *sett)
{
	GVariant *name_v;
	gchar *title;
	GtkWidget *frame;
	GtkGrid *grid = GTK_GRID(gtk_grid_new());

	sett->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	name_v = service_get_property(sett->serv, "Name", NULL);
	title = g_strdup_printf("%s - %s", _("Network Settings"),
	                        g_variant_get_string(name_v, NULL));
	gtk_window_set_title(GTK_WINDOW(sett->window), title);
	g_free(title);
	g_variant_unref(name_v);
	gtk_window_set_default_size(GTK_WINDOW(sett->window), SETTINGS_WIDTH,
	                            SETTINGS_HEIGHT);


	sett->list = gtk_list_box_new();
	sett->notebook = gtk_notebook_new();
	frame = gtk_frame_new(NULL);

	g_object_ref(sett->window);
	g_object_ref(sett->list);
	g_object_ref(sett->notebook);

	g_signal_connect(sett->window, "delete-event", G_CALLBACK(delete_event),
	                 sett);
	g_signal_connect(sett->list, "row-selected",
	                 G_CALLBACK(page_selected), sett);

	STYLE_ADD_MARGIN(GTK_WIDGET(grid), MARGIN_LARGE);

	gtk_widget_set_size_request(sett->list, SETTINGS_LIST_WIDTH, -1);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(sett->notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(sett->notebook), FALSE);
	gtk_widget_set_vexpand(sett->list, TRUE);
	gtk_widget_set_hexpand(sett->notebook, TRUE);
	gtk_widget_set_vexpand(sett->notebook, TRUE);

	gtk_container_add(GTK_CONTAINER(frame), sett->list);
	gtk_grid_attach(grid, frame, 0, 0, 1, 1);
	gtk_grid_attach(grid, sett->notebook, 1, 0, 1, 1);
	gtk_container_add(GTK_CONTAINER(sett->window), GTK_WIDGET(grid));

	settings_add_page(sett, "Info");
	settings_add_text(settings_add_page(sett, "IPv4"), "IP", "123.123.2.1");

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

	sett->serv = serv;

	settings_init(sett);
}
