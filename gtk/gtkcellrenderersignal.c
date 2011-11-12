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

#include "gtkcellrenderersignal.h"

enum
{
	PROP_0,
	PROP_DATA,
	PROP_PROBE,
	PROP_FOREGROUND,
	PROP_SCALE,
	PROP_OFFSET,
};

struct _GtkCellRendererSignalPrivate
{
	GArray *data;
	guint32 probe;
	GdkColor foreground;
	gdouble scale;
	gint offset;
};

static void gtk_cell_renderer_signal_finalize(GObject *object);
static void gtk_cell_renderer_signal_get_property(GObject *object,
				guint param_id, GValue *value,
				GParamSpec *pspec);
static void gtk_cell_renderer_signal_set_property(GObject *object,
				guint param_id, const GValue *value,
				GParamSpec *pspec);
static void gtk_cell_renderer_signal_get_size(GtkCellRenderer *cell,
				GtkWidget *widget,
				const GdkRectangle *cell_area,
				gint *x_offset,
				gint *y_offset,
				gint *width,
				gint *height);
static void gtk_cell_renderer_signal_render(GtkCellRenderer *cell,
				GdkWindow *window,
				GtkWidget *widget,
				const GdkRectangle *background_area,
				const GdkRectangle *cell_area,
				GtkCellRendererState flags);


G_DEFINE_TYPE(GtkCellRendererSignal, gtk_cell_renderer_signal, GTK_TYPE_CELL_RENDERER);

static void
gtk_cell_renderer_signal_class_init (GtkCellRendererSignalClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);

	object_class->finalize = gtk_cell_renderer_signal_finalize;
	object_class->get_property = gtk_cell_renderer_signal_get_property;
	object_class->set_property = gtk_cell_renderer_signal_set_property;

	cell_class->get_size = gtk_cell_renderer_signal_get_size;
	cell_class->render = gtk_cell_renderer_signal_render;

	g_object_class_install_property(object_class,
				PROP_DATA,
				g_param_spec_pointer("data",
						"Data",
						"Binary samples data",
						G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
				PROP_PROBE,
				g_param_spec_int("probe",
						"Probe",
						"Bit in Data to display",
						-1, G_MAXINT, -1,
						G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
				PROP_FOREGROUND,
				g_param_spec_string("foreground",
						"Foreground",
						"Foreground Colour",
						NULL,
						G_PARAM_WRITABLE));

	g_object_class_install_property (object_class,
				PROP_SCALE,
				g_param_spec_double("scale",
						"Scale",
						"Pixels per sample",
						0, 100, 1,
						G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
				PROP_OFFSET,
				g_param_spec_int("offset",
						"Offset",
						"Offset...",
						0, G_MAXINT, 0,
						G_PARAM_READWRITE));

	g_type_class_add_private (object_class,
			sizeof (GtkCellRendererSignalPrivate));
}

static void gtk_cell_renderer_signal_init(GtkCellRendererSignal *cel)
{
	GtkCellRendererSignalPrivate *priv;

	cel->priv = G_TYPE_INSTANCE_GET_PRIVATE(cel,
				GTK_TYPE_CELL_RENDERER_SIGNAL,
				GtkCellRendererSignalPrivate);
	priv = cel->priv;

	priv->data = NULL;
	priv->probe = -1;
	priv->scale = 1;
	priv->offset = 0;
}

GtkCellRenderer *gtk_cell_renderer_signal_new(void)
{
	return g_object_new(GTK_TYPE_CELL_RENDERER_SIGNAL, NULL);
}

static void gtk_cell_renderer_signal_finalize(GObject *object)
{
	GtkCellRendererSignal *cel = GTK_CELL_RENDERER_SIGNAL(object);
	GtkCellRendererSignalPrivate *priv = cel->priv;

	G_OBJECT_CLASS(gtk_cell_renderer_signal_parent_class)->finalize(object);
}

static void
gtk_cell_renderer_signal_get_property(GObject *object,
				guint param_id,
				GValue *value,
				GParamSpec *pspec)
{
	GtkCellRendererSignal *cel = GTK_CELL_RENDERER_SIGNAL(object);
	GtkCellRendererSignalPrivate *priv = cel->priv;

	switch (param_id) {
	case PROP_DATA:
		g_value_set_pointer(value, priv->data);
		break;
	case PROP_PROBE:
		g_value_set_int(value, priv->probe);
		break;
	case PROP_SCALE:
		g_value_set_double(value, priv->scale);
		break;
	case PROP_OFFSET:
		g_value_set_int(value, priv->offset);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	}
}

static void
gtk_cell_renderer_signal_set_property(GObject *object,
				guint param_id,
				const GValue *value,
				GParamSpec *pspec)
{
	GtkCellRendererSignal *cel = GTK_CELL_RENDERER_SIGNAL(object);
	GtkCellRendererSignalPrivate *priv = cel->priv;

	switch (param_id) {
	case PROP_DATA:
		priv->data = g_value_get_pointer(value);
		break;
	case PROP_PROBE:
		priv->probe = g_value_get_int(value);
		break;
	case PROP_FOREGROUND:
		gdk_color_parse(g_value_get_string(value), &priv->foreground);
		break;
	case PROP_SCALE:
		priv->scale = g_value_get_double(value);
		break;
	case PROP_OFFSET:
		priv->offset = g_value_get_int(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	}
}

static void
gtk_cell_renderer_signal_get_size(GtkCellRenderer *cell,
				GtkWidget *widget,
				const GdkRectangle *cell_area,
				gint *x_offset,
				gint *y_offset,
				gint *width,
				gint *height)
{
	/* FIXME: What about cell_area? */
	if (width) *width = 0;
	if (height) *height = 30;

	if (x_offset) *x_offset = 0;
	if (y_offset) *y_offset = 0;
}


static gboolean sample(GArray *data, gint probe, guint i)
{
	guint16 *tmp16;
	guint32 *tmp32;

	g_return_val_if_fail(i < (data->len / g_array_get_element_size(data)),
				FALSE);

	switch (g_array_get_element_size(data)) {
	case 1:
		return data->data[i] & (1 << probe);
	case 2:
		tmp16 = (guint16*)data->data;
		return tmp16[i] & (1 << probe);
	case 4:
		tmp32 = (guint32*)data->data;
		return tmp32[i] & (1 << probe);
	}
	return FALSE;
}

static void
gtk_cell_renderer_signal_render(GtkCellRenderer *cell,
				GdkWindow *window,
				GtkWidget *widget,
				const GdkRectangle *background_area,
				const GdkRectangle *cell_area,
				GtkCellRendererState flags)
{
	GtkCellRendererSignal *cel = GTK_CELL_RENDERER_SIGNAL(cell);
	GtkCellRendererSignalPrivate *priv= cel->priv;
	guint nsamples = priv->data->len / g_array_get_element_size(priv->data);
	gint xpad, ypad;
	int x, y, w, h;
	gint i, si;
	gdouble o;

	gtk_cell_renderer_get_padding (cell, &xpad, &ypad);
	x = cell_area->x + xpad;
	y = cell_area->y + ypad;
	w = cell_area->width - xpad * 2;
	h = cell_area->height - ypad * 2;

	cairo_t *cr = gdk_cairo_create(GDK_DRAWABLE(window));

	/* Set clipping region to background rectangle.
	 * This prevents us drawing over other cells.
	 */
	cairo_new_path(cr);
	gdk_cairo_rectangle(cr, background_area);
	cairo_clip(cr);

	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
	gdk_cairo_set_source_color(cr, &priv->foreground);
	/*cairo_set_line_width(cr, 1);*/
	cairo_new_path(cr);

	si = priv->offset / priv->scale;
	if (si >= nsamples)
		return;
	o = x - (priv->offset - si * priv->scale);

	cairo_move_to(cr, o, y +
		(sample(priv->data, priv->probe, si++) ? 0 : h));
	o += priv->scale;
	while((si < nsamples) && (o - priv->scale < x+w)) {
		cairo_line_to(cr, o, y +
			(sample(priv->data, priv->probe, si-1) ? 0 : h));
		cairo_line_to(cr, o, y +
			(sample(priv->data, priv->probe, si) ? 0 : h));
		o += priv->scale;
		si++;
	}

	cairo_stroke(cr);
}
