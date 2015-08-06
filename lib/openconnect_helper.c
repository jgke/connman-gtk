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
#include <gio/gio.h>

#ifdef USE_OPENCONNECT

#include <openconnect.h>
#include <glib/gi18n.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "openconnect_helper.h"
#include "../src/util.h"
#include "../src/dialog.h"

#ifdef USE_OPENCONNECT_DYNAMIC
#include <dlfcn.h>
#endif /* USE_OPENCONNECT_DYNAMIC */

typedef int (*init_ssl_fn)(void);
typedef int (*validate_cert_fn) (void *privdata, const char *reason);
typedef int (*write_config_fn) (void *privdata, const char *buf, int buflen);
typedef int (*process_form_fn) (void *privdata, struct oc_auth_form *form);
typedef void (*show_progress_fn) (void *privdata, int level,
                                  const char *fmt, ...);
typedef struct openconnect_info *(*vpninfo_new_fn)(const char *useragent,
                                                   validate_cert_fn,
                                                   write_config_fn,
                                                   process_form_fn,
                                                   show_progress_fn,
                                                   void *privdata);
typedef const char *(*get_cookie_fn)(struct openconnect_info *);
typedef const char *(*get_hostname_fn)(struct openconnect_info *);
typedef void (*vpninfo_free_fn)(struct openconnect_info *vpninfo);
typedef int (*set_client_cert_fn)(struct openconnect_info *, const char *cert,
                                  const char *sslkey);
typedef int (*passphrase_from_fsid_fn)(struct openconnect_info *vpninfo);
typedef int (*set_cafile_fn)(struct openconnect_info *, const char *);
typedef int (*parse_url_fn)(struct openconnect_info *vpninfo, const char *url);
typedef int (*obtain_cookie_fn)(struct openconnect_info *vpninfo);
typedef const char *(*get_peer_cert_hash_fn)(struct openconnect_info *vpninfo);

static init_ssl_fn init_ssl;
static vpninfo_new_fn vpninfo_new;
static get_cookie_fn get_cookie;
static get_hostname_fn get_hostname;
static vpninfo_free_fn vpninfo_free;
static set_client_cert_fn set_client_cert;
static passphrase_from_fsid_fn passphrase_from_fsid;
static set_cafile_fn set_cafile;
static parse_url_fn parse_url;
static obtain_cookie_fn obtain_cookie;
static get_peer_cert_hash_fn get_peer_cert_hash;

static gboolean init_lib(void) {
	static gboolean initialized = FALSE;
	static gboolean loaded = FALSE;
	if(initialized)
		return loaded;
	initialized = TRUE;
#ifndef USE_OPENCONNECT_DYNAMIC
	init_ssl = openconnect_init_ssl;
	vpninfo_new = openconnect_vpninfo_new;
	get_cookie = openconnect_get_cookie;
	get_hostname = openconnect_get_hostname;
	vpninfo_free = openconnect_vpninfo_free;
	set_client_cert = openconnect_set_client_cert;
	passphrase_from_fsid = openconnect_passphrase_from_fsid;
	set_cafile = openconnect_set_cafile;
	parse_url = openconnect_parse_url;
	obtain_cookie = openconnect_obtain_cookie;
	get_peer_cert_hash = openconnect_get_peer_cert_hash;
#else
	void *lib = dlopen("libopenconnect.so", RTLD_NOW);
	/* technically this shouldn't fail since getting here depends
	 * on ConnMan having openconnect support */
	if(!lib)
		return FALSE;
	init_ssl = dlsym(lib, "openconnect_init_ssl");
	vpninfo_new = dlsym(lib, "openconnect_vpninfo_new");
	get_cookie = dlsym(lib, "openconnect_get_cookie");
	get_hostname = dlsym(lib, "openconnect_get_hostname");
	vpninfo_free = dlsym(lib, "openconnect_vpninfo_free");
	set_client_cert = dlsym(lib, "openconnect_set_client_cert");
	passphrase_from_fsid = dlsym(lib, "openconnect_passphrase_from_fsid");
	set_cafile = dlsym(lib, "openconnect_set_cafile");
	parse_url = dlsym(lib, "openconnect_parse_url");
	obtain_cookie = dlsym(lib, "openconnect_obtain_cookie");
	get_peer_cert_hash = dlsym(lib, "openconnect_get_peer_cert_hash");
#endif /* USE_OPENCONNECT_DYNAMIC */
	loaded = TRUE;
	return TRUE;
};

GString *progress;

static int invalid_cert(void *data, const char *reason)
{
	printf("%s\n", reason);
	return 0;
}

static int new_config(void *data, const char *buf, int buflen)
{
	return 0;
}

static int ask_pass(void *data, struct oc_auth_form *form)
{
	struct oc_form_opt *opt;
	GPtrArray *tokens;
	int i;

	tokens = g_ptr_array_new_full(0, (GDestroyNotify)free_token_element);

	for(opt = form->opts; opt; opt = opt->next) {
		struct token_element *elem = NULL;

		if(opt->flags & OC_FORM_OPT_IGNORE)
			continue;

		switch(opt->type) {
		case OC_FORM_OPT_SELECT: {
			struct oc_form_opt_select *select;
			GPtrArray *options = g_ptr_array_new();
			int i;

			select = (struct oc_form_opt_select *)opt;
			for(i = 0; i < select->nr_choices; i++)
				g_ptr_array_add(options,
						select->choices[i]->label);
			elem = token_new_list(select->form.label, options);
			break;
		}
		case OC_FORM_OPT_TEXT:
			if(!strcmp(opt->name, "username"))
				elem = token_new_entry(_("Username"), FALSE);
			else
				elem = token_new_entry(opt->name, FALSE);
			break;
		case OC_FORM_OPT_PASSWORD:
			if(!strcmp(opt->name, "password"))
				elem = token_new_entry(_("Password"), TRUE);
			else
				elem = token_new_entry(opt->name, TRUE);
			break;
		case OC_FORM_OPT_TOKEN:
			break;
		}
		if(elem)
			g_ptr_array_add(tokens, elem);
	}

	if(!dialog_ask_tokens(_("Enter AnyConnect credintials"), tokens)) {
		g_ptr_array_free(tokens, TRUE);
		return OC_FORM_RESULT_CANCELLED;
	}

	i = 0;
	for(opt = form->opts; opt; opt = opt->next) {
		struct token_element  *elem = NULL;

		if(opt->flags & OC_FORM_OPT_IGNORE)
			continue;

		switch(opt->type) {
		case OC_FORM_OPT_SELECT:
		case OC_FORM_OPT_TEXT:
		case OC_FORM_OPT_PASSWORD:
			elem = tokens->pdata[i++];
			opt->_value = elem->value;
			elem->value = NULL;
			break;
		case OC_FORM_OPT_TOKEN:
			break;
		}
	}

	g_ptr_array_free(tokens, TRUE);

	return OC_FORM_RESULT_OK;
}

static void show_progress(void *data, int level, const char *fmt, ...)
{
	gchar *msg;
	va_list argp;

	va_start(argp, fmt);
	msg = g_strdup_vprintf(fmt, argp);
	va_end(argp);
	printf("%s", msg);
	g_string_append(progress, msg);
	g_free(msg);
}

static GVariantDict *get_tokens(GHashTable *info)
{
	GVariantDict *tokens = NULL;
	gchar *host, *cert;
	struct openconnect_info *vpninfo;
	int status;

	progress = g_string_new(NULL);
	init_ssl();
	vpninfo = vpninfo_new("linux-64", invalid_cert, new_config,
			      ask_pass, show_progress, NULL);

	host = g_hash_table_lookup(info, "Host");

	cert = g_hash_table_lookup(info, "OpenConnect.ClientCert");
	if(cert) {
		set_client_cert(vpninfo, cert, NULL);

		// XXX: connman doesn't support configuring this
		passphrase_from_fsid(vpninfo);
	}

	cert = g_hash_table_lookup(info, "OpenConnect.CACert");
	if(cert)
		set_cafile(vpninfo, cert);

	parse_url(vpninfo, host);

	status = obtain_cookie(vpninfo);
	if(status) {
		if(status != OC_FORM_RESULT_CANCELLED)
			show_error(_("Connecting to VPN failed."),
				   progress->str);
		goto out;
	}

	tokens = g_variant_dict_new(NULL);

	g_variant_dict_insert(tokens, "OpenConnect.ServerCert", "s",
			      get_peer_cert_hash(vpninfo));
	g_variant_dict_insert(tokens, "OpenConnect.Cookie", "s",
			      get_cookie(vpninfo));
	g_variant_dict_insert(tokens, "OpenConnect.VPNHost", "s",
			      get_hostname(vpninfo));

out:
	vpninfo_free(vpninfo);
	g_string_free(progress, TRUE);
	return tokens;
}

GVariantDict *openconnect_handle(GDBusMethodInvocation *invocation,
				 GVariant *args)
{
	GHashTable *info, *required;
	GVariantDict *out;
	GVariantIter *iter;
	gchar *key;
	GVariant *value;
	GVariant *parameters;

	if(!init_lib())
		return NULL;

	parameters = g_variant_get_child_value(args, 1);

	info = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	required = g_hash_table_new_full(g_str_hash, g_str_equal,
					 g_free, g_free);

	iter = g_variant_iter_new(parameters);
	while(g_variant_iter_loop(iter, "{sv}", &key, &value)) {
		GVariantDict *dict = g_variant_dict_new(value);
		GVariant *req_v, *val_v;
		const gchar *req;
		gchar *val;

		req_v = g_variant_dict_lookup_value(dict, "Requirement", NULL);
		val_v = g_variant_dict_lookup_value(dict, "Value", NULL);
		req = g_variant_get_string(req_v, NULL);
		if(val_v) {
			val = g_variant_dup_string(val_v, NULL);
			if(!strcmp(req, "informational"))
				g_hash_table_insert(info, g_strdup(key), val);
			g_variant_unref(val_v);
		} else if(!strcmp(req, "mandatory"))
			g_hash_table_insert(required, g_strdup(key), NULL);
		g_variant_unref(req_v);
		g_variant_dict_unref(dict);
	}
	g_variant_iter_free(iter);

	out = get_tokens(info);

	g_hash_table_unref(required);
	g_hash_table_unref(info);

	return out;
}

gboolean is_openconnect(GVariant *args)
{
	GVariantIter *iter;
	gchar *path;
	GVariant *value, *service, *parameters;
	gboolean openconnect = FALSE;

	service = g_variant_get_child_value(args, 0);
	parameters = g_variant_get_child_value(args, 1);

	iter = g_variant_iter_new(parameters);

	while(g_variant_iter_loop(iter, "{sv}", &path, &value))
		openconnect = openconnect || strstr(path, "OpenConnect");
	g_variant_iter_free(iter);

	g_variant_unref(service);
	g_variant_unref(parameters);

	return openconnect && init_lib();
}

#else

GVariantDict *openconnect_handle(GDBusMethodInvocation *invocation,
				 GVariant *args)
{
	return NULL;
}

gboolean is_openconnect(GVariant *args)
{
	return FALSE;
}

#endif /* USE_OPENCONNECT */
