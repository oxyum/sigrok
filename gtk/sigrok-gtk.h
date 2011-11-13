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

#ifndef __SIGROK_GTK_H
#define __SIGROK_GTK_H

#include <gtk/gtk.h>

/* sigview.c */
extern GtkListStore *siglist;

GtkWidget *sigview_init(void);
void sigview_zoom(GtkWidget *sigview, gdouble zoom, gint offset);

/* help.c */
void help_wiki(void);
void help_about(GtkAction *action, GtkWindow *parent);

/* devselect.c */
GtkWidget *dev_select_combo_box_new(GtkWindow *parent);
void dev_select_rescan(GtkAction *action, GtkWindow *parent);

/* log.c */
GtkWidget *log_init(void);
GtkWidget *toolbar_init(GtkWindow *parent);

#endif

