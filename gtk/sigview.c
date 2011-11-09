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

#include "gtkcellrenderersignal.h"
#include "sigview.h"

/* FIXME: No globals */
GtkListStore *siglist;

static void format_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
		GtkTreeModel *siglist, GtkTreeIter *iter, gpointer h)
{
	GArray *data = g_object_get_data(G_OBJECT(siglist), "sampledata");
	int probe;
	char *colour;

	gtk_tree_model_get(siglist, iter, 1, &colour, 2, &probe, -1);

	g_object_set(G_OBJECT(cell), "data", data, "probe", probe,
				"foreground", colour, NULL);
}

static gboolean do_scroll_event(GtkTreeView *tv, GdkEventScroll *e,
				GObject *cel)
{
	gdouble scale;
	gint offset;
	gint x, y, cx;
	GtkTreeViewColumn *col;

	gtk_tree_view_widget_to_tree_coords(tv, e->x, e->y, &x, &y);
	if(!gtk_tree_view_get_path_at_pos(tv, x, y, NULL, &col, &cx, NULL))
		return FALSE;
	if(col != g_object_get_data(G_OBJECT(tv), "signalcol"))
		return FALSE;

	switch(e->direction) {
	case GDK_SCROLL_UP:
		sigview_zoom(GTK_WIDGET(tv), 1.2, cx);
		break;
	case GDK_SCROLL_DOWN:
		sigview_zoom(GTK_WIDGET(tv), 1/1.2, cx);
		break;
	}
	
	return TRUE;
}

static gboolean do_motion_event(GtkWidget *tv, GdkEventMotion *e,
				GObject *cel)
{
	GObject *siglist;
	gint x, dx;
	gint offset;
	GArray *data;
	guint nsamples;
	GtkTreeViewColumn *col;
	gint width;
	gdouble scale;

	x = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tv), "motion-x"));
	g_object_set_data(G_OBJECT(tv), "motion-x", GINT_TO_POINTER((gint)e->x));

	siglist = G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
	data = g_object_get_data(siglist, "sampledata");
	nsamples = (data->len / g_array_get_element_size(data)) - 1;
	col = g_object_get_data(G_OBJECT(tv), "signalcol");
	width = gtk_tree_view_column_get_width(col);

	g_object_get(cel, "offset", &offset, "scale", &scale, NULL);
	offset += x - e->x;
	if(offset < 0)
		offset = 0;
	if(offset > nsamples * scale - width)
		offset = nsamples * scale - width;
	g_object_set(cel, "offset", offset, NULL);
	gtk_widget_queue_draw(tv);
}

static gboolean do_button_event(GtkTreeView *tv, GdkEventButton *e,
				GObject *cel)
{
	int h;
	gint x, y;
	GtkTreeViewColumn *col;

	if(e->button != 3)
		return FALSE;

	gtk_tree_view_widget_to_tree_coords(tv, e->x, e->y, &x, &y);

	switch(e->type) {
	case GDK_BUTTON_PRESS:
		if(!gtk_tree_view_get_path_at_pos(tv, x, y, NULL, &col, NULL, NULL))
			return FALSE;
		if(col != g_object_get_data(G_OBJECT(tv), "signalcol"))
			return FALSE;
		h = g_signal_connect(tv, "motion-notify-event", G_CALLBACK(do_motion_event), cel);
		g_object_set_data(G_OBJECT(tv), "motion-handler", GINT_TO_POINTER(h));
		g_object_set_data(G_OBJECT(tv), "motion-x", GINT_TO_POINTER((gint)e->x));
		break;
	case GDK_BUTTON_RELEASE:
		h = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tv), "motion-handler"));
		if(!h)
			return;
		g_signal_handler_disconnect(GTK_WIDGET(tv), h);
		g_object_set_data(G_OBJECT(tv), "motion-handler", NULL);
		break;
	}
	return TRUE;
}

GtkWidget *sigview_init(void)
{
	GtkWidget *sw, *tv;
	GtkTreeViewColumn *col;
	GtkCellRenderer *cel;

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	tv = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(sw), tv);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), FALSE);
	g_object_set(G_OBJECT(tv), "reorderable", TRUE, NULL);

	col = gtk_tree_view_column_new_with_attributes(NULL,
					gtk_cell_renderer_text_new(),
					"text", 0, "foreground", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

	cel = gtk_cell_renderer_signal_new();
	g_object_set(G_OBJECT(cel), "ypad", 1, NULL);
	g_signal_connect(tv, "scroll-event", G_CALLBACK(do_scroll_event), cel);
	g_signal_connect(tv, "button-press-event", G_CALLBACK(do_button_event), cel);
	g_signal_connect(tv, "button-release-event", G_CALLBACK(do_button_event), cel);
	col = gtk_tree_view_column_new();
	g_object_set_data(G_OBJECT(tv), "signalcol", col);
	g_object_set_data(G_OBJECT(tv), "signalcel", cel);
	gtk_tree_view_column_pack_start(col, cel, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, cel, format_func,
					NULL, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

	siglist = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING,
					G_TYPE_INT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(siglist));

	return sw;
}

void sigview_zoom(GtkWidget *sigview, gdouble zoom, gint offset)
{
	GtkTreeViewColumn *col;
	GtkCellRendererSignal *cel;
	GArray *data;
	gdouble scale;
	gint ofs;
	gint width;
	guint nsamples;

	/* This is so that sigview_zoom() may be called with pointer
	 * to the GtkTreeView or containing GtkScrolledWindow, as is the
	 * case when called from outside this module.
	 */
	if (GTK_IS_SCROLLED_WINDOW(sigview))
		sigview = gtk_bin_get_child(GTK_BIN(sigview));

	g_return_if_fail(GTK_IS_TREE_VIEW(sigview));

	col = g_object_get_data(G_OBJECT(sigview), "signalcol");
	width = gtk_tree_view_column_get_width(col);

	data = g_object_get_data(
		G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(sigview))),
		"sampledata");
	if(!data)
		return;
	nsamples = (data->len / g_array_get_element_size(data)) - 1;

	cel = g_object_get_data(G_OBJECT(sigview), "signalcel");
	g_object_get(cel, "scale", &scale, "offset", &ofs, NULL);

	ofs += offset;
		
	scale *= zoom;
	ofs *= zoom;

	ofs -= offset;

	if(ofs < 0)
		ofs = 0;

	if(scale < (double)width/nsamples)
		scale = (double)width/nsamples;

	if(ofs > nsamples * scale - width)
		ofs = nsamples * scale - width;

	g_object_set(cel, "scale", scale, "offset", ofs, NULL);
	gtk_widget_queue_draw(GTK_WIDGET(sigview));
}


