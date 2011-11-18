/*
 * This file is part of the sigrok project.
 *
 * Copyright (C) 2011 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include <sigrok.h>
#include <gtk/gtk.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

static struct {
	gchar *filename;
	gchar *stock_id;
} stock_icons[] = {
	{"sigrok_logo.png", "sigrok-logo"},
};

static gint n_stock_icons = G_N_ELEMENTS (stock_icons);

void icons_register(void)
{
	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set;
	GtkIconSource *icon_source;
	gint i;
	gchar *tmp;

#ifdef G_OS_WIN32
	gchar *sysdir;
	sysdir = g_win32_get_package_installation_directory_of_module(NULL);
	g_build_filename(sysdir, "share", PACKAGE_TARNAME, "icons", NULL);
	g_free(sysdir);
#else
        tmp = g_build_filename(PKG_DATA_DIR, "icons", NULL);
#endif
	icon_factory = gtk_icon_factory_new ();
	gtk_icon_factory_add_default (icon_factory);
	icon_source = gtk_icon_source_new ();

	for(i = 0; i < n_stock_icons; i++) {
		gchar *filename = 
			g_build_filename(tmp, stock_icons[i].filename, NULL);

		icon_set = gtk_icon_set_new ();
		gtk_icon_source_set_filename(icon_source, filename);
		gtk_icon_source_set_icon_name(icon_source, stock_icons[i].stock_id);
		gtk_icon_set_add_source (icon_set, icon_source);
		gtk_icon_factory_add (icon_factory, stock_icons[i].stock_id, icon_set);

		g_free(filename);
	}

	gtk_icon_source_free (icon_source);
	g_object_unref (icon_factory);
        g_free(tmp);
}

