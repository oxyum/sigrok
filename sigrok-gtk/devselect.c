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
#include "sigrok-gtk.h"

static void dev_selected(GtkComboBox *devbox, GObject *parent)
{
	GtkTreeModel *devlist = gtk_combo_box_get_model(devbox);
	GtkEntry *timesamples = g_object_get_data(parent, "timesamples");
	GtkComboBox *timeunit = g_object_get_data(parent, "timeunit");
	GtkTreeIter iter;
	const gchar *name;
	GtkCheckMenuItem *menuitem;
	struct sr_dev *dev;

	if (!gtk_combo_box_get_active_iter(devbox, &iter)) {
		g_object_set_data(parent, "dev", NULL);
		return;
	}
	gtk_tree_model_get(devlist, &iter, 0, &name, 1, &dev,
				2, &menuitem, -1);

	gtk_check_menu_item_set_active(menuitem, TRUE);

	sr_session_dev_remove_all();
	if (sr_session_dev_add(dev) != SR_OK) {
		g_critical("Failed to use device.");
		sr_session_destroy();
		dev = NULL;
	}
	g_object_set_data(parent, "dev", dev);

	/*
	 * Grey out the time unless the device is valid,
	 * and it supports sample limiting
	 */
	const gboolean limit_samples = dev &&
		sr_driver_hwcap_exists(dev->driver,
		SR_HWCAP_LIMIT_SAMPLES);
	gtk_widget_set_sensitive((GtkWidget*)timesamples, limit_samples);
	gtk_widget_set_sensitive((GtkWidget*)timeunit, limit_samples);
}

static void dev_menuitem_toggled(GtkMenuItem *item, GtkComboBox *combo)
{
	GtkTreeModel *model = gtk_combo_box_get_model(combo);
	GtkTreeIter iter;

	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item)))
		return;
	
	if (gtk_tree_model_get_iter_first(model, &iter)) do {
		gchar *name;
		gtk_tree_model_get(model, &iter, 0, &name, -1);
		if (g_str_equal(name, gtk_menu_item_get_label(item))) {
			gtk_combo_box_set_active_iter(combo, &iter);
			return;
		}
	} while (gtk_tree_model_iter_next(model, &iter));
}

#define GET_DEV_INST(dev) \
	(dev)->driver->dev_info_get((dev)->driver_index, SR_DI_INST);

void dev_select_rescan(GtkAction *action, GtkWindow *parent)
{
	GtkComboBox *devbox = g_object_get_data(G_OBJECT(parent), "devcombo");
	g_return_if_fail(devbox != NULL);
	GtkListStore *devlist = GTK_LIST_STORE(gtk_combo_box_get_model(devbox));
	GtkTreeIter iter;
	struct sr_dev *dev;
	const struct sr_dev_inst *sdi;
	gchar *sdevname = NULL;
	GSList *devs, *l;
	GtkUIManager *ui = g_object_get_data(G_OBJECT(parent), "ui_manager");
	GtkWidget *menuitem = gtk_ui_manager_get_widget(ui,
					"/menubar/DevMenu/DevSelectMenu");
	GtkMenuShell *devmenu = GTK_MENU_SHELL(gtk_menu_item_get_submenu(GTK_MENU_ITEM(menuitem)));
	GSList *radiolist = NULL;

	(void)action;

	/* Make a copy of the selected device's short name for comparison.
	 * We wish to select the same device after the refresh if possible.
	 */
	if (gtk_combo_box_get_active_iter(devbox, &iter)) {
		gtk_tree_model_get(GTK_TREE_MODEL(devlist), &iter, 1, &dev, -1);
		/* FIXME: Use something other than dev->driver->name */
		sdevname = g_strdup(dev->driver->name);
	}

	/* Destroy the old menu items */
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(devlist), &iter)) do {
		GtkMenuItem *item;
		gtk_tree_model_get(GTK_TREE_MODEL(devlist), &iter, 2, &item, -1);
		gtk_object_destroy(GTK_OBJECT(item));
	} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(devlist), &iter));

	gtk_list_store_clear(devlist);

	/* Scan for new devices and update our list */
	/* TODO: Fix this in libsigrok first. */
	/*sr_dev_scan();*/
	devs = sr_dev_list();
	for (l = devs; l; l = l->next) {
		dev = l->data;
		sdi = GET_DEV_INST(dev);
		gchar *name = sdi->model ? sdi->model : sdi->vendor;
		if (!name)
			name = "(unknown)";

		menuitem = gtk_radio_menu_item_new_with_label(radiolist, name);
		gtk_widget_show(GTK_WIDGET(menuitem));
		if (!radiolist)
			radiolist = gtk_radio_menu_item_get_group(
					GTK_RADIO_MENU_ITEM(menuitem));
		g_signal_connect(menuitem, "toggled",
				G_CALLBACK(dev_menuitem_toggled), devbox);
		gtk_menu_shell_prepend(devmenu, menuitem);

		gtk_list_store_append(devlist, &iter);
		gtk_list_store_set(devlist, &iter,
				0, name,
				1, dev,
				2, menuitem,
				-1);

		if (sdevname && g_str_equal(sdevname, dev->driver->name))
			gtk_combo_box_set_active_iter(devbox, &iter);
	}
	if (sdevname)
		g_free(sdevname);

	/* Select a default if nothing selected */
	if (!gtk_combo_box_get_active_iter(devbox, &iter)) {
		if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(devlist), &iter))
			return;
		/* Skip demo if there's another available */
		GtkTreeIter first = iter;
		if (gtk_tree_model_iter_next(GTK_TREE_MODEL(devlist), &iter))
			gtk_combo_box_set_active_iter(devbox, &iter);
		else
			gtk_combo_box_set_active_iter(devbox, &first);
	}
}

GtkWidget *dev_select_combo_box_new(GtkWindow *parent)
{
	GtkWidget *devbox = gtk_combo_box_new();

	/* Populate device list */
	GtkListStore *devlist = gtk_list_store_new(3,
			G_TYPE_STRING, G_TYPE_POINTER, GTK_TYPE_CHECK_MENU_ITEM);
	GtkCellRenderer *cel = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(devbox), cel, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(devbox), cel, "text", 0);
	gtk_combo_box_set_model(GTK_COMBO_BOX(devbox), GTK_TREE_MODEL(devlist));
	g_signal_connect(devbox, "changed", G_CALLBACK(dev_selected), parent);

	g_object_set_data(G_OBJECT(parent), "devcombo", devbox);
	dev_select_rescan(NULL, parent);

	return devbox;
}

