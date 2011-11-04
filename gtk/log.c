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

#include <gtk/gtk.h>

static void logger(const gchar *log_domain, GLogLevelFlags log_level,
		   const gchar *message, gpointer user_data)
{
	GtkTextView *tv = user_data;
	g_return_if_fail(GTK_IS_TEXT_VIEW(tv));
	GtkTextBuffer *tb = gtk_text_view_get_buffer(tv);
	GtkTextIter iter;

	/* Avoid compiler warnings. */
	(void)log_domain;
	(void)log_level;

	gtk_text_buffer_get_end_iter(tb, &iter);
	gtk_text_buffer_insert(tb, &iter, message, -1);
	gtk_text_buffer_insert(tb, &iter, "\n", -1);
	gtk_text_view_scroll_mark_onscreen(tv, gtk_text_buffer_get_insert(tb));
}

GtkWidget *log_init(void)
{
	GtkWidget *sw, *tv;

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	tv = gtk_text_view_new();
	gtk_widget_modify_font(tv,
                        pango_font_description_from_string("Monospace"));
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv), GTK_WRAP_CHAR);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);

	gtk_container_add(GTK_CONTAINER(sw), tv);

	g_log_set_default_handler(logger, tv);

	return sw;
}

