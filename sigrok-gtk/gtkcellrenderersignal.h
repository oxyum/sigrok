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

#ifndef SIGROK_GTK_GTKCELLRENDERERSIGNAL_H
#define SIGROK_GTK_GTKCELLRENDERERSIGNAL_H

#include <gtk/gtk.h>
#include <glib-object.h>

#define GTK_TYPE_CELL_RENDERER_SIGNAL \
	(gtk_cell_renderer_signal_get_type ())
#define GTK_CELL_RENDERER_SIGNAL(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_CELL_RENDERER_SIGNAL, \
	GtkCellRendererSignal))
#define GTK_CELL_RENDERER_SIGNAL_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_CELL_RENDERER_SIGNAL, \
	GtkCellRendererSignalClass))
#define GTK_IS_CELL_RENDERER_SIGNAL(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_CELL_RENDERER_SIGNAL))
#define GTK_IS_CELL_RENDERER_SIGNAL_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_CELL_RENDERER_SIGNAL))
#define GTK_CELL_RENDERER_SIGNAL_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_CELL_RENDERER_SIGNAL, \
	GtkCellRendererSignalClass))

typedef struct _GtkCellRendererSignal GtkCellRendererSignal;
typedef struct _GtkCellRendererSignalClass GtkCellRendererSignalClass;
typedef struct _GtkCellRendererSignalPrivate GtkCellRendererSignalPrivate;

struct _GtkCellRendererSignal
{
	GtkCellRenderer parent_instance;

	/*< private >*/
	GtkCellRendererSignalPrivate *priv;
};

struct _GtkCellRendererSignalClass
{
	GtkCellRendererClass parent_class;
};

GType gtk_cell_renderer_signal_get_type(void);
GtkCellRenderer* gtk_cell_renderer_signal_new(void);

#endif
