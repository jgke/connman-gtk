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

typedef int (*process_form_fn) (void *privdata, struct oc_auth_form *form);
typedef void (*show_progress_fn) (void *privdata, int level,
                                  const char *fmt, ...);

typedef void (*vpninfo_free_fn)(struct openconnect_info *vpninfo);
typedef int (*obtain_cookie_fn)(struct openconnect_info *vpninfo);

#if OPENCONNECT_CHECK_VER(5, 0)
typedef int (*validate_cert_fn) (void *privdata, const char *reason);
typedef const char *(*get_peer_cert_hash_fn)(struct openconnect_info *vpninfo);
#else /* !OPENCONNECT_CHECK_VER(5, 0) */
typedef int (*validate_cert_fn) (void *privdata, OPENCONNECT_X509 *cert,
				 const char *reason);
typedef OPENCONNECT_X509 *(*get_peer_cert_fn)(struct openconnect_info *);
typedef int (*get_cert_sha1_fn)(struct openconnect_info *vpninfo,
				OPENCONNECT_X509 *cert, char *buf);
#endif /* !OPENCONNECT_CHECK_VER(5, 0) */

#if OPENCONNECT_CHECK_VER(4, 1)
typedef int (*init_ssl_fn)(void);
#else
typedef void (*init_ssl_fn)(void);
#endif

#if OPENCONNECT_CHECK_VER(4, 0)
typedef int (*write_config_fn) (void *privdata, const char *buf, int buflen);

typedef struct openconnect_info *(*vpninfo_new_fn)(const char *useragent,
                                                   validate_cert_fn,
                                                   write_config_fn,
                                                   process_form_fn,
                                                   show_progress_fn,
                                                   void *privdata);
typedef const char *(*get_cookie_fn)(struct openconnect_info *);
typedef const char *(*get_hostname_fn)(struct openconnect_info *);

typedef int (*set_client_cert_fn)(struct openconnect_info *, const char *cert,
                                  const char *sslkey);
typedef int (*passphrase_from_fsid_fn)(struct openconnect_info *vpninfo);
typedef int (*set_cafile_fn)(struct openconnect_info *, const char *);
typedef int (*parse_url_fn)(struct openconnect_info *vpninfo, const char *url);
typedef int (*set_option_value_fn)(struct oc_form_opt *opt, const char *value);
#else /* !OPENCONNECT_CHECK_VERSION(4, 0) */
typedef int (*write_config_fn) (void *privdata, char *buf, int buflen);

typedef struct openconnect_info *(*vpninfo_new_fn)(char *useragent,
                                                   validate_cert_fn,
                                                   write_config_fn,
                                                   process_form_fn,
                                                   show_progress_fn,
                                                   void *privdata);
typedef char *(*get_cookie_fn)(struct openconnect_info *);
typedef char *(*get_hostname_fn)(struct openconnect_info *);

typedef void (*set_client_cert_fn)(struct openconnect_info *, char *cert,
                                  char *sslkey);
typedef int (*passphrase_from_fsid_fn)(struct openconnect_info *vpninfo);
typedef void (*set_cafile_fn)(struct openconnect_info *, char *);
typedef int (*parse_url_fn)(struct openconnect_info *vpninfo, char *url);
#endif /* !OPENCONNECT_CHECK_VERSION(4, 0) */

static init_ssl_fn init_ssl;
static vpninfo_free_fn vpninfo_free;
static obtain_cookie_fn obtain_cookie;

static vpninfo_new_fn _vpninfo_new;
static get_cookie_fn _get_cookie;
static get_hostname_fn _get_hostname;
static set_client_cert_fn _set_client_cert;
static passphrase_from_fsid_fn _passphrase_from_fsid;
static set_cafile_fn _set_cafile;
static parse_url_fn _parse_url;

#if OPENCONNECT_CHECK_VER(5, 0)
static get_peer_cert_hash_fn _get_peer_cert_hash;
#else
static get_peer_cert_fn _get_peer_cert;
static get_cert_sha1_fn _get_cert_sha1;
#endif

#if OPENCONNECT_CHECK_VER(4, 0)
static set_option_value_fn _set_option_value;
#endif

static gboolean init_lib(void) {
	static gboolean initialized = FALSE;
	static gboolean loaded = FALSE;
	if(initialized)
		return loaded;
	initialized = TRUE;
#ifndef USE_OPENCONNECT_DYNAMIC
	init_ssl = openconnect_init_ssl;
	vpninfo_free = openconnect_vpninfo_free;
	obtain_cookie = openconnect_obtain_cookie;

	_vpninfo_new = openconnect_vpninfo_new;
	_get_cookie = openconnect_get_cookie;
	_get_hostname = openconnect_get_hostname;
	_set_client_cert = openconnect_set_client_cert;
	_passphrase_from_fsid = openconnect_passphrase_from_fsid;
	_set_cafile = openconnect_set_cafile;
	_parse_url = openconnect_parse_url;
#if OPENCONNECT_CHECK_VER(5, 0)
	_get_peer_cert_hash = openconnect_get_peer_cert_hash;
#else
	_get_peer_cert = openconnect_get_peer_cert;
	_get_cert_sha1 = openconnect_get_cert_sha1;
#endif

#if OPENCONNECT_CHECK_VER(4, 0)
	_set_option_value = openconnect_set_option_value;
#endif

#else
	void *lib = dlopen("libopenconnect.so", RTLD_NOW);
	/* technically this shouldn't fail since getting here depends
	 * on ConnMan having openconnect support */
	if(!lib)
		return FALSE;
	init_ssl = dlsym(lib, "openconnect_init_ssl");
	vpninfo_free = dlsym(lib, "openconnect_vpninfo_free");
	obtain_cookie = dlsym(lib, "openconnect_obtain_cookie");

	_vpninfo_new = dlsym(lib, "openconnect_vpninfo_new");
	_get_cookie = dlsym(lib, "openconnect_get_cookie");
	_get_hostname = dlsym(lib, "openconnect_get_hostname");
	_set_client_cert = dlsym(lib, "openconnect_set_client_cert");
	_passphrase_from_fsid = dlsym(lib, "openconnect_passphrase_from_fsid");
	_set_cafile = dlsym(lib, "openconnect_set_cafile");
	_parse_url = dlsym(lib, "openconnect_parse_url");
#if OPENCONNECT_CHECK_VER(5, 0)
	_get_peer_cert_hash = dlsym(lib, "openconnect_get_peer_cert_hash");
#else
	_get_peer_cert = dlsym(lib, "openconnect_get_peer_cert");
	_get_cert_sha1 = dlsym(lib, "openconnect_get_cert_sha1");
#endif

#if OPENCONNECT_CHECK_VER(4, 0)
	_set_option_value = dlsym(lib, "openconnect_set_option_value");
#endif

#endif /* USE_OPENCONNECT_DYNAMIC */
	loaded = TRUE;
	return TRUE;
};

#if OPENCONNECT_CHECK_VER(4, 0)
struct openconnect_info *vpninfo_new(const char *useragent,
				     validate_cert_fn validate,
				     write_config_fn new_config,
				     process_form_fn process_form,
				     show_progress_fn show_progress,
				     void *privdata)
{
	return _vpninfo_new(useragent, validate, new_config,
			    process_form, show_progress, privdata);
}

void set_client_cert(struct openconnect_info *info, const char *key,
		     const char *sslkey)
{
	_set_client_cert(info, key, sslkey);
}

void set_cafile(struct openconnect_info *info, const char *file)
{
	_set_cafile(info, file);
}

void parse_url(struct openconnect_info *info, const char *url)
{
	_parse_url(info, url);
}

static char *get_cookie(struct openconnect_info *info)
{
	return g_strdup(_get_cookie(info));
}

static char *get_hostname(struct openconnect_info *info)
{
	return g_strdup(_get_hostname(info));
}

static char *get_peer_cert_hash(struct openconnect_info *info)
{
	return g_strdup(_get_peer_cert_hash(info));
}

static void set_option_value(struct oc_form_opt *opt, char *value)
{
	_set_option_value(opt, value);
	g_free(value);
}

#else

struct openconnect_info *vpninfo_new(const char *useragent,
				     validate_cert_fn validate,
				     write_config_fn new_config,
				     process_form_fn process_form,
				     show_progress_fn show_progress,
				     void *privdata)
{
	return _vpninfo_new(g_strdup(useragent), validate, new_config,
			    process_form, show_progress, privdata);
}

void set_client_cert(struct openconnect_info *info, const char *key,
		     const char *sslkey)
{
	_set_client_cert(info, g_strdup(key), g_strdup(sslkey));
}

void set_cafile(struct openconnect_info *info, const char *file)
{
	_set_cafile(info, g_strdup(file));
}

void parse_url(struct openconnect_info *info, const char *url)
{
	_parse_url(info, g_strdup(url));
}

static char *get_cookie(struct openconnect_info *info)
{
	return _get_cookie(info);
}

static char *get_hostname(struct openconnect_info *info)
{
	return _get_hostname(info);
}

static char *get_peer_cert_hash(struct openconnect_info *info)
{
	char buf[41] = {0, };
	char *out;
	OPENCONNECT_X509 *cert = _get_peer_cert(info);

	if(!cert)
		return g_strdup("");

	_get_cert_sha1(info, cert, buf);
	out = g_strdup(buf);
	/* openconnect's documentation states that cert should be now freed
	 * with X509_free... */
	return out;
}

static void set_option_value(struct oc_form_opt *opt, char *value)
{
	opt->value = value;
}

#endif

GString *progress;

#if OPENCONNECT_CHECK_VER(4, 0)
static int invalid_cert(void *data, const char *reason)
{
	printf("%s\n", reason);
	return 0;
}

static int new_config(void *data, const char *buf, int buflen)
{
	return 0;
}
#else /* !OPENCONNECT_CHECK_VER(4, 0) */
static int new_config(void *data, char *buf, int buflen)
{
	return 0;
}

static int invalid_cert(void *data, OPENCONNECT_X509 *cert, const char *reason)
{
	printf("%s\n", reason);
	return 0;
}
#endif /* !OPENCONNECT_CHECK_VER(4, 0) */

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
			elem = token_new_entry(opt->label, FALSE);
			break;
		case OC_FORM_OPT_PASSWORD:
			elem = token_new_entry(opt->label, TRUE);
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
			set_option_value(opt, elem->value);
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
	gchar *hash, *cookie, *hostname;
	struct openconnect_info *vpninfo;
	int status;

	init_ssl();
	progress = g_string_new(NULL);
	vpninfo = vpninfo_new("linux-64", invalid_cert, new_config,
			      ask_pass, show_progress, NULL);

	host = g_hash_table_lookup(info, "Host");

	cert = g_hash_table_lookup(info, "OpenConnect.ClientCert");
	if(cert)
		set_client_cert(vpninfo, cert, NULL);

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

	hash = get_peer_cert_hash(vpninfo);
	cookie = get_cookie(vpninfo);
	hostname = get_hostname(vpninfo);
	g_variant_dict_insert(tokens, "OpenConnect.ServerCert", "s", hash);
	g_variant_dict_insert(tokens, "OpenConnect.Cookie", "s", cookie);
	g_variant_dict_insert(tokens, "OpenConnect.VPNHost", "s", hostname);
	g_free(hash);
	g_free(cookie);
	g_free(hostname);
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
