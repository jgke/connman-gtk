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

#include "dialog.h"
#include "main.h"
#include "style.h"

static struct token_element *create_new(enum token_element_type type,
					const gchar *name, GtkWidget *content)
{
	struct token_element *elem = g_malloc(sizeof(*elem));

	elem->type = type;
	elem->value = NULL;
	elem->name = g_strdup(name);

	if(strchr(name, '_')) {
		elem->label = gtk_label_new_with_mnemonic(name);
		gtk_label_set_mnemonic_widget(GTK_LABEL(elem->label), content);
	}
	else
		elem->label = gtk_label_new(name);
	elem->content = content;

	style_set_margin(elem->label, MARGIN_LARGE);
	style_set_margin(elem->content, MARGIN_LARGE);
	gtk_widget_set_margin_top(elem->label, MARGIN_MEDIUM);
	gtk_widget_set_margin_bottom(elem->label, MARGIN_MEDIUM);
	gtk_widget_set_margin_top(elem->content, MARGIN_MEDIUM);
	gtk_widget_set_margin_bottom(elem->content, MARGIN_MEDIUM);

	return elem;
}

struct token_element *token_new_text(const gchar *name, const gchar *value)
{
	GtkWidget *content;
	struct token_element *elem;

	content = gtk_label_new(value);
	elem = create_new(TOKEN_ELEMENT_TEXT, name, content);

	return elem;
}

static void check_entry(GtkWidget *entry, gpointer user_data)
{
	gboolean valid = TRUE;
	gboolean (*check)(GtkWidget *entry);
	GtkDialog *window;
	GtkStyleContext *context;

	check = g_object_get_data(G_OBJECT(entry), "check");
	window = g_object_get_data(G_OBJECT(entry), "window");
	context = gtk_widget_get_style_context(entry);
	if(check)
		valid = check(entry);

	if(!valid) {
		gtk_style_context_add_class(context, "error");
		gtk_dialog_set_response_sensitive(window,
						  GTK_RESPONSE_ACCEPT, FALSE);
	} else {
		gtk_style_context_remove_class(context, "error");
		gtk_dialog_set_response_sensitive(window,
						  GTK_RESPONSE_ACCEPT, TRUE);
	}
}
struct token_element *token_new_entry_full(const gchar *name, gboolean secret,
					   const gchar *value,
					   gboolean (*check)(GtkWidget *entry))
{
	GtkWidget *content;
	struct token_element *elem;

	content = gtk_entry_new();
	elem = create_new(TOKEN_ELEMENT_ENTRY, name, content);
	gtk_entry_set_activates_default(GTK_ENTRY(content), TRUE);

	if(secret && strcmp(name, "Name") && strcmp(name, "Identity") &&
	   strcmp(name, "WPS") && strcmp(name, "Username") &&
	   strcmp(name, "Host") && strcmp(name, "OpenConnect.CACert") &&
	   strcmp(name, "OpenConnect.ServerCert") &&
	   strcmp(name, "OpenConnect.VPNHost"))
		gtk_entry_set_visibility(GTK_ENTRY(content), FALSE);

	if(value)
		gtk_entry_set_text(GTK_ENTRY(content), value);

	g_object_set_data(G_OBJECT(content), "check", check);
	g_signal_connect(content, "changed", G_CALLBACK(check_entry), NULL);

	return elem;
}

struct token_element *token_new_entry(const gchar *name, gboolean secret)
{
	return token_new_entry_full(name, secret, NULL, NULL);
}

struct token_element *token_new_list(const gchar *name, GPtrArray *options)
{
	GtkComboBoxText *box;
	GtkWidget *content;
	struct token_element *elem;
	int i;

	content = gtk_combo_box_text_new();
	elem = create_new(TOKEN_ELEMENT_LIST, name, content);
	box = GTK_COMBO_BOX_TEXT(content);

	for(i = 0; i < options->len; i++)
		gtk_combo_box_text_append_text(box, options->pdata[i]);

	return elem;
}

struct token_element *token_new_checkbox(const gchar *name)
{
	GtkWidget *content;
	struct token_element *elem;

	content = gtk_check_button_new();
	elem = create_new(TOKEN_ELEMENT_CHECKBOX, name, content);

	return elem;
}

gchar *get_element_value(struct token_element *elem)
{
	gchar *value = NULL;
	if(elem->type == TOKEN_ELEMENT_ENTRY) {
		GtkEntryBuffer *buf;

		buf = gtk_entry_get_buffer(GTK_ENTRY(elem->content));
		value = g_strdup(gtk_entry_buffer_get_text(buf));
	} else if(elem->type == TOKEN_ELEMENT_LIST) {
		GtkComboBoxText *box = GTK_COMBO_BOX_TEXT(elem->content);
		value = gtk_combo_box_text_get_active_text(box);
	} else if(elem->type == TOKEN_ELEMENT_CHECKBOX) {
		GtkToggleButton *button = GTK_TOGGLE_BUTTON(elem->content);
		value = GINT_TO_POINTER(gtk_toggle_button_get_active(button));
	}

	return value;
}

struct token_window_params {
	GPtrArray *elements;
	const gchar *title;
	GCond *cond;
	GMutex *mutex;
	gboolean returned;
	gint value;
};

static gboolean ask_tokens_sync(gpointer data)
{
	struct token_window_params *params = data;
	GPtrArray *elements = params->elements;

	GtkDialog *dialog;
	GtkWidget *grid, *window;
	int i;
	int flags = GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL;

	grid = gtk_grid_new();
	window = gtk_dialog_new_with_buttons(params->title,
					     GTK_WINDOW(main_window), flags,
					     _("_OK"), GTK_RESPONSE_ACCEPT,
					     _("_Cancel"), GTK_RESPONSE_CANCEL,
					     NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(window),
					GTK_RESPONSE_ACCEPT);
	for(i = 0; i < elements->len; i++) {
		struct token_element *elem = g_ptr_array_index(elements, i);
		gtk_grid_attach(GTK_GRID(grid), elem->label, 0, i, 1, 1);
		gtk_grid_attach(GTK_GRID(grid), elem->content, 1, i, 1, 1);
		g_object_set_data(G_OBJECT(elem->content), "window", window);
		if(elem->type == TOKEN_ELEMENT_ENTRY)
			check_entry(elem->content, NULL);
	}

	gtk_widget_show_all(grid);
	dialog = GTK_DIALOG(window);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(dialog)),
	                  grid);

	i = gtk_dialog_run(GTK_DIALOG(window));

	params->value = i == GTK_RESPONSE_ACCEPT;
	if(i != GTK_RESPONSE_ACCEPT)
		goto out;

	for(i = 0; i < elements->len; i++) {
		struct token_element *elem = elements->pdata[i];

		elem->value = get_element_value(elem);
	}

out:
	gtk_widget_destroy(window);
	g_mutex_lock(params->mutex);
	params->returned = TRUE;
	g_cond_signal(params->cond);
	g_mutex_unlock(params->mutex);
	return FALSE;
}

gboolean dialog_ask_tokens(const gchar *title, GPtrArray *elements)
{
	struct token_window_params params = {};
	GMutex mutex;
	GCond cond;

	params.elements = elements;
	params.title = title;
	params.mutex = &mutex;
	params.cond = &cond;
	g_mutex_init(&mutex);
	g_cond_init(&cond);
	g_main_context_invoke(NULL, (GSourceFunc)ask_tokens_sync, &params);
	g_mutex_lock(&mutex);
	while(!params.returned)
		g_cond_wait(&cond, &mutex);
	g_mutex_unlock(&mutex);
	g_mutex_clear(&mutex);
	g_cond_clear(&cond);
	return !!params.value;
}

void free_token_element(struct token_element *elem)
{
	g_free(elem->name);
	if(elem->type != TOKEN_ELEMENT_CHECKBOX)
		g_free(elem->value);
	g_free(elem);
}

struct error_params {
	gchar *text;
	gchar *log;
};

static gboolean show_error_sync(gpointer data)
{
	struct error_params *params = data;

	static const int max_log_length = 100;

	const gchar *title;
	int flags;
	GtkWidget *window, *area, *grid, *text, *log, *box = NULL;

	grid = gtk_grid_new();

	if(params->log)
		g_strstrip(params->log);

	text = gtk_label_new(params->text);
	title = _("Operation failed");
	flags = GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL;
	window = gtk_dialog_new_with_buttons(title, GTK_WINDOW(main_window),
	                                     flags,
	                                     _("_OK"), GTK_RESPONSE_NONE, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(window), GTK_RESPONSE_NONE);

	gtk_widget_set_halign(text, GTK_ALIGN_START);
	style_set_margin(text, MARGIN_LARGE);
	gtk_grid_attach(GTK_GRID(grid), text, 0, 0, 1, 1);

	if(params->log) {
		gtk_widget_set_margin_bottom(text, 0);

		log = gtk_label_new(params->log);
		style_set_margin(log, MARGIN_LARGE);
		gtk_widget_set_hexpand(log, TRUE);
		gtk_widget_set_vexpand(log, TRUE);

		if(strlen(params->log) >= max_log_length) {
			label_align_text(GTK_LABEL(log), 0, 0.5);
			GtkStyleContext *context;
			GtkScrolledWindow *scroll;

			box = gtk_scrolled_window_new(NULL, NULL);
			scroll = GTK_SCROLLED_WINDOW(box);
			gtk_scrolled_window_set_policy(scroll,
						       GTK_POLICY_NEVER,
						       GTK_POLICY_AUTOMATIC);
			style_set_margin(log, 0);
			style_set_margin(box, MARGIN_LARGE);
			gtk_widget_set_margin_top(box, 0);
			style_add_context(log);
			context = gtk_widget_get_style_context(log);
			gtk_style_context_add_class(context, "cm-log");
			gtk_scrolled_window_set_shadow_type(scroll,
							    GTK_SHADOW_IN);

			gtk_container_add(GTK_CONTAINER(box), log);
			gtk_grid_attach(GTK_GRID(grid), box, 0, 1, 1, 1);
		} else
			gtk_grid_attach(GTK_GRID(grid), log, 0, 1, 1, 1);
	}
	else
		log = NULL;

	gtk_widget_show_all(grid);

	area = gtk_dialog_get_content_area(GTK_DIALOG(window));
	gtk_container_add(GTK_CONTAINER(area), grid);

	if(box) {
		int mwidth, width, mheight, height;
		const int max_height = 300;
		GtkScrolledWindow *scroll;

		scroll = GTK_SCROLLED_WINDOW(box);
		gtk_widget_get_preferred_width(log, &mwidth, &width);
		if(mwidth > width)
			width = mwidth;
		gtk_widget_get_preferred_height(log, &mheight, &height);
		if(mheight > height)
			height = mheight;
		if(height > max_height)
			height = max_height;
		gtk_scrolled_window_set_min_content_width(scroll, width);
		gtk_scrolled_window_set_min_content_height(scroll, height);
	} else
		gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

	gtk_dialog_run(GTK_DIALOG(window));

	gtk_widget_destroy(window);

	g_free(params->text);
	g_free(params->log);
	g_free(params);

	return FALSE;
}

void show_error(const gchar *text, const gchar *log)
{
	struct error_params *params = g_malloc(sizeof(*params));
	params->text = g_strdup(text);
	if(log)
		params->log = g_strdup(log);
	else
		params->log = NULL;
	g_main_context_invoke(NULL, show_error_sync, params);
}
