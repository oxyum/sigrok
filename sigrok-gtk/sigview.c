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

#include <libsigrok/libsigrok.h>
#include <gtk/gtk.h>

#include <string.h>
#include <math.h>

#include "gtkcellrenderersignal.h"
#include "sigrok-gtk.h"

/* FIXME: No globals */
GtkListStore *siglist;

static void format_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
		GtkTreeModel *siglist, GtkTreeIter *iter, gpointer cb_data)
{
	int probe;
	char *colour;
	GArray *data;

	(void)tree_column;
	(void)cb_data;

	/* TODO: In future we will get data here from tree model.
	 * PD data streams will be kept in the list.  The logic stream
	 * will be used if data is NULL.
	 */
	gtk_tree_model_get(siglist, iter, 1, &colour, 2, &probe, -1);

	/* Try get summary data from the list */
	data = g_object_get_data(G_OBJECT(siglist), "summarydata");
	if (!data)
		data = g_object_get_data(G_OBJECT(siglist), "sampledata");

	g_object_set(G_OBJECT(cell), "data", data, "probe", probe,
				"foreground", colour, NULL);
}

static gboolean do_scroll_event(GtkTreeView *tv, GdkEventScroll *e)
{
	gint x, y, cx;
	GtkTreeViewColumn *col;

	gtk_tree_view_widget_to_tree_coords(tv, e->x, e->y, &x, &y);
	if (!gtk_tree_view_get_path_at_pos(tv, x, y, NULL, &col, &cx, NULL))
		return FALSE;
	if (col != g_object_get_data(G_OBJECT(tv), "signalcol"))
		return FALSE;

	switch (e->direction) {
	case GDK_SCROLL_UP:
		sigview_zoom(GTK_WIDGET(tv), 1.2, cx);
		break;
	case GDK_SCROLL_DOWN:
		sigview_zoom(GTK_WIDGET(tv), 1/1.2, cx);
		break;
	default:
		/* Surpress warning about unswitch enum values */
		break;
	}
	
	return TRUE;
}

static gboolean do_motion_event(GtkWidget *tv, GdkEventMotion *e,
				GObject *cel)
{
	GObject *siglist;
	gint x;
	gint offset;
	GArray *data;
	guint nsamples;
	GtkTreeViewColumn *col;
	gint width;
	gdouble scale, *rscale;
	GtkAdjustment *adj;

	x = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tv), "motion-x"));
	g_object_set_data(G_OBJECT(tv), "motion-x", GINT_TO_POINTER((gint)e->x));
	adj = g_object_get_data(G_OBJECT(tv), "hadj");

	siglist = G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
	data = g_object_get_data(siglist, "sampledata");
	rscale = g_object_get_data(siglist, "rscale");
	nsamples = (data->len / g_array_get_element_size(data)) - 1;
	col = g_object_get_data(G_OBJECT(tv), "signalcol");
	width = gtk_tree_view_column_get_width(col);

	g_object_get(cel, "offset", &offset, "scale", &scale, NULL);
	offset += x - e->x;
	if (offset < 0)
		offset = 0;
	if (offset > nsamples * *rscale - width)
		offset = nsamples * *rscale - width;

	gtk_adjustment_set_value(adj, offset);

	return TRUE;
}

static gboolean do_button_event(GtkTreeView *tv, GdkEventButton *e,
				GObject *cel)
{
	int h;
	gint x, y;
	GtkTreeViewColumn *col;

	if (e->button != 3)
		return FALSE;

	gtk_tree_view_widget_to_tree_coords(tv, e->x, e->y, &x, &y);

	switch (e->type) {
	case GDK_BUTTON_PRESS:
		if (!gtk_tree_view_get_path_at_pos(tv, x, y, NULL, &col, NULL, NULL))
			return FALSE;
		if (col != g_object_get_data(G_OBJECT(tv), "signalcol"))
			return FALSE;
		h = g_signal_connect(tv, "motion-notify-event", G_CALLBACK(do_motion_event), cel);
		g_object_set_data(G_OBJECT(tv), "motion-handler", GINT_TO_POINTER(h));
		g_object_set_data(G_OBJECT(tv), "motion-x", GINT_TO_POINTER((gint)e->x));
		break;
	case GDK_BUTTON_RELEASE:
		h = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tv), "motion-handler"));
		if (!h)
			return TRUE;
		g_signal_handler_disconnect(GTK_WIDGET(tv), h);
		g_object_set_data(G_OBJECT(tv), "motion-handler", NULL);
		break;
	default:
		/* Surpress warning about unswitch enum values */
		break;
	}
	return TRUE;
}

static void col_resized(GtkWidget *col)
{
	sigview_zoom(col, 1, 0);
}

static void pan_changed(GtkAdjustment *adj, GtkWidget *sigview)
{
	GObject *cel = g_object_get_data(G_OBJECT(sigview), "signalcel");
	g_object_set(cel, "offset", (gint)gtk_adjustment_get_value(adj), NULL);
	gtk_widget_queue_draw(sigview);
}

GtkWidget *sigview_init(void)
{
	GtkWidget *sw, *tv;
	GtkTreeViewColumn *col;
	GtkCellRenderer *cel;

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				GTK_POLICY_ALWAYS, GTK_POLICY_AUTOMATIC);
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
	g_signal_connect_swapped(col, "notify::width",
					G_CALLBACK(col_resized), tv);

	siglist = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING,
					G_TYPE_INT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(siglist));

	GtkObject *pan = gtk_adjustment_new(0, 0, 0, 1, 1, 1);
	g_object_set_data(G_OBJECT(tv), "hadj", pan);
	gtk_range_set_adjustment(GTK_RANGE(GTK_SCROLLED_WINDOW(sw)->hscrollbar),
				GTK_ADJUSTMENT(pan));
	g_signal_connect(pan, "value-changed", G_CALLBACK(pan_changed), tv);
	
	return sw;
}

static GArray *summarize(GArray *in, gdouble *scale)
{
	GArray *ret;
	int skip = 1 / (*scale * 4);
	guint64 i, j, k, l;
	unsigned unitsize = g_array_get_element_size(in);
	uint8_t s[unitsize];
	uint8_t smask[unitsize];

	g_return_val_if_fail(skip > 1, NULL);

	ret = g_array_sized_new(FALSE, FALSE, unitsize, in->len / skip);
	ret->len = in->len / skip;
	*scale *= skip;

	memset(s, 0, unitsize);
	for (i = 0, k = 0; i < (in->len/unitsize); i += skip, k++) {
		memset(smask, 0xFF, unitsize);
		for (j = i; j < i+skip; j++) {
			for (l = 0; l < unitsize; l++) {
				uint8_t ns = (in->data[(j*unitsize)+l] ^ s[l]) & smask[l];
				/* ns is now bits we need to toggle */
				s[l] ^= ns;
				smask[l] &= ~ns;
			}
		}
		memcpy(&ret->data[k*unitsize], s, unitsize);
	}
	return ret;
}

void sigview_zoom(GtkWidget *sigview, gdouble zoom, gint offset)
{
	GObject *siglist;
	GtkTreeViewColumn *col;
	GtkCellRendererSignal *cel;
	GtkAdjustment *adj;
	/* data and scale refer to summary */
	GArray *data;
	gdouble scale;
	/* rdata and rscale refer to complete data */
	GArray *rdata;
	gdouble *rscale;
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
	adj = g_object_get_data(G_OBJECT(sigview), "hadj");
	width = gtk_tree_view_column_get_width(col);

	siglist = G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(sigview)));
	rdata = g_object_get_data(siglist, "sampledata");
	rscale = g_object_get_data(siglist, "rscale");
	if (!rscale) {
		rscale = g_malloc(sizeof(rscale));
		*rscale = 1;
		g_object_set_data(siglist, "rscale", rscale);
	}
	data = g_object_get_data(siglist, "summarydata");
	if (!rdata)
		return;
	if (!data)
		data = rdata;
	nsamples = (rdata->len / g_array_get_element_size(rdata)) - 1;
	if ((fabs(*rscale - (double)width/nsamples) < 1e-12) && (zoom < 1))
		return;

	cel = g_object_get_data(G_OBJECT(sigview), "signalcel");
	g_object_get(cel, "scale", &scale, "offset", &ofs, NULL);

	ofs += offset;
		
	scale *= zoom;
	*rscale *= zoom;
	ofs *= zoom;

	ofs -= offset;

	if (ofs < 0)
		ofs = 0;

	if (*rscale < (double)width/nsamples) {
		*rscale = (double)width/nsamples;
		scale = *rscale;
		if (data && (data != rdata))
			g_array_free(data, TRUE);
		data = rdata;
	}

	if (ofs > nsamples * *rscale - width)
		ofs = nsamples * *rscale - width;

	gtk_adjustment_configure(adj, ofs, 0, nsamples * *rscale, 
			width/16, width/2, width);

	if (scale < 0.125) {
		g_object_set_data(siglist,
			"summarydata", summarize(data, &scale));
		if (data && (data != rdata))
			g_array_free(data, TRUE);
	} else if ((scale > 1) && (*rscale < 1)) {
		scale = *rscale;
		g_object_set_data(siglist,
			"summarydata", summarize(rdata, &scale));
		if (data && (data != rdata))
			g_array_free(data, TRUE);
	}

	g_object_set(cel, "scale", scale, "offset", ofs, NULL);
	gtk_widget_queue_draw(GTK_WIDGET(sigview));
}

