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
#include <string.h>

#include "util.h"

gchar *variant_str_array_to_str(GVariant *variant)
{
	GString *str;
	const gchar **arr, **iter;
	gchar *out;
	arr = g_variant_get_strv(variant, NULL);
	str = g_string_new(arr[0]);
	iter = arr + 1;
	while(*arr && *iter)
		g_string_append_printf(str, ", %s", *iter++);
	g_free(arr);
	out = str->str;
	g_string_free(str, FALSE);
	return out;
}

gchar *variant_to_str(GVariant *variant)
{
	if(!strcmp(g_variant_get_type_string(variant), "as"))
		return variant_str_array_to_str(variant);
	else
		return g_variant_dup_string(variant, NULL);
}
