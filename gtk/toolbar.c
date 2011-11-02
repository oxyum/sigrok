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

#include <sigrokdecode.h> /* First, so we avoid a _POSIX_C_SOURCE warning. */
#include <sigrok.h>

#include <gtk/gtk.h>

void dev_selected(GtkComboBox *dev, GObject *parent)
{
	GtkTreeModel *devlist = gtk_combo_box_get_model(dev);
	GtkTreeIter iter;
	const gchar *name;

	if(!gtk_combo_box_get_active_iter(dev, &iter)) {
		g_message("Selection cleared\n");
		return;
	}
	gtk_tree_model_get(devlist, &iter, 0, &name, -1);
	g_message("Selected %s\n", name);
}

void dev_set_options(GtkWindow *parent)
{
	GtkWidget *dialog = gtk_dialog_new_with_buttons("Device Properties",
					parent, GTK_DIALOG_MODAL,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					NULL);
	GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *tv = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(sw), tv);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), sw,
				TRUE, TRUE, 0);

	gtk_widget_show_all(dialog);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
		gtk_widget_destroy(dialog);
		return;
	}

	gtk_widget_destroy(dialog);
}

static void dev_list_refresh(GtkListStore *devlist)
{
	GSList *devices, *l;

	gtk_list_store_clear(devlist);

	devices = sr_device_list();
	for (l = devices; l; l = l->next) {
		struct sr_device *device = l->data;
		struct sr_device_instance *sdi =
			device->plugin->get_device_info(device->plugin_index,
							SR_DI_INSTANCE);
		GtkTreeIter iter;
		gtk_list_store_append(devlist, &iter);
		gtk_list_store_set(devlist, &iter, 0,
			sdi->model ? sdi->model : sdi->vendor, -1);
	}
}

GtkWidget *toolbar_init(GtkWindow *parent)
{
	GtkToolbar *toolbar = GTK_TOOLBAR(gtk_toolbar_new());
	GtkToolItem *toolitem = gtk_tool_item_new();
	GtkWidget *dev = gtk_combo_box_new();

	gtk_container_add(GTK_CONTAINER(toolitem), dev);
	gtk_toolbar_insert(toolbar, toolitem, -1);

	/* Populate device list */
	GtkListStore *devlist = gtk_list_store_new(1, G_TYPE_STRING);
	dev_list_refresh(devlist);
	GtkCellRenderer *cel = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(dev), cel, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(dev), cel, "text", 0);
	gtk_combo_box_set_model(GTK_COMBO_BOX(dev), GTK_TREE_MODEL(devlist));
	g_signal_connect(dev, "changed", G_CALLBACK(dev_selected), parent);

	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH);
	gtk_toolbar_insert(toolbar, toolitem, -1);
	g_signal_connect_swapped(toolitem, "clicked",
				G_CALLBACK(dev_list_refresh), devlist);
	
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_PROPERTIES);
	gtk_toolbar_insert(toolbar, toolitem, -1);
	g_signal_connect_swapped(toolitem, "clicked",
				G_CALLBACK(dev_set_options), parent);
	
	return GTK_WIDGET(toolbar);
}

