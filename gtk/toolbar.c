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
	struct sr_device *device;

	if(!gtk_combo_box_get_active_iter(dev, &iter)) {
		g_object_set_data(parent, "device", NULL);
		return;
	}
	gtk_tree_model_get(devlist, &iter, 0, &name, 1, &device, -1);

	g_object_set_data(parent, "device", device);
}

void prop_edited(GtkCellRendererText *renderer, gchar *path,
		gchar *new_text, GtkListStore *props)
{
	GtkTreeIter iter;
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(props), &iter, path);
	gtk_list_store_set(props, &iter, 2, new_text, -1);
}

void dev_set_options(GtkWindow *parent)
{
	struct sr_device *device = g_object_get_data(G_OBJECT(parent), "device");
	if(!device)
		return;

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Device Properties",
					parent, GTK_DIALOG_MODAL,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					NULL);
	GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(sw, 300, 200);
	GtkWidget *tv = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(sw), tv);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), sw,
				TRUE, TRUE, 0);

	/* Populate list store with config options */
	GtkListStore *props = gtk_list_store_new(3, G_TYPE_INT,
					G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(props));
	int *capabilities = device->plugin->get_capabilities();
	int cap;
	for (cap = 0; capabilities[cap]; cap++) {
		struct sr_hwcap_option *hwo;
		if (!(hwo = sr_find_hwcap_option(capabilities[cap])))
			continue;
		GtkTreeIter iter;
		gtk_list_store_append(props, &iter);
		gtk_list_store_set(props, &iter, 0, capabilities[cap],
					1, hwo->shortname, -1);
	}

	/* Add columns to the tree view */
	GtkTreeViewColumn *col;
	col = gtk_tree_view_column_new_with_attributes("Property",
				gtk_cell_renderer_text_new(),
				"text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);
	GtkCellRenderer *cel = gtk_cell_renderer_text_new();
	g_object_set(cel, "editable", TRUE, NULL);
	g_signal_connect(cel, "edited", G_CALLBACK(prop_edited), props);
	col = gtk_tree_view_column_new_with_attributes("Value",
				cel, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

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
		gtk_list_store_set(devlist, &iter,
				0, sdi->model ? sdi->model : sdi->vendor,
				1, device,
				-1);
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
	GtkListStore *devlist = gtk_list_store_new(2,
			G_TYPE_STRING, G_TYPE_POINTER);
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

