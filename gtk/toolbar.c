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

#include "sigview.h"

static void dev_selected(GtkComboBox *dev, GObject *parent)
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

	sr_session_device_clear();
	if (sr_session_device_add(device) != SR_OK) {
		g_critical("Failed to use device.");
		sr_session_destroy();
		device = NULL;
	}
	g_object_set_data(parent, "device", device);
}

static void prop_edited(GtkCellRendererText *cel, gchar *path, gchar *text,
			GtkListStore *props)
{
	(void)cel;

	struct sr_device *device = g_object_get_data(G_OBJECT(props), "device");
	GtkTreeIter iter;
	int type, cap;
	guint64 tmp_u64;
	int ret;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(props), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(props), &iter,
				0, &cap, 1, &type, -1);

	switch (type) {
	case SR_T_UINT64:
		tmp_u64 = sr_parse_sizestring(text);
		ret = device->plugin->set_configuration(device->plugin_index,
				cap, &tmp_u64);
		break;
	case SR_T_CHAR:
		ret = device->plugin-> set_configuration(device->plugin_index,
				cap, text);
		break;
	case SR_T_NULL:
		ret = device->plugin->set_configuration(device->plugin_index,
				cap, NULL);
		break;
	}

	if(!ret)
		gtk_list_store_set(props, &iter, 3, text, -1);
}

static void dev_set_options(GtkWindow *parent)
{
	struct sr_device *device = g_object_get_data(G_OBJECT(parent), "device");
	if(!device)
		return;

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Device Properties",
					parent, GTK_DIALOG_MODAL,
					GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					NULL);
	GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(sw, 300, 200);
	GtkWidget *tv = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(sw), tv);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), sw,
				TRUE, TRUE, 0);

	/* Populate list store with config options */
	GtkListStore *props = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_INT,
					G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(props));
	int *capabilities = device->plugin->get_capabilities();
	int cap;
	GtkTreeIter iter;
	for (cap = 0; capabilities[cap]; cap++) {
		struct sr_hwcap_option *hwo;
		if (!(hwo = sr_find_hwcap_option(capabilities[cap])))
			continue;
		gtk_list_store_append(props, &iter);
		gtk_list_store_set(props, &iter, 0, capabilities[cap],
					1, hwo->type, 2, hwo->shortname, -1);
	}

	/* Save device with list so that property can be set by edited
	 * handler. */
	g_object_set_data(G_OBJECT(props), "device", device);

	/* Add columns to the tree view */
	GtkTreeViewColumn *col;
	col = gtk_tree_view_column_new_with_attributes("Property",
				gtk_cell_renderer_text_new(),
				"text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);
	GtkCellRenderer *cel = gtk_cell_renderer_text_new();
	g_object_set(cel, "editable", TRUE, NULL);
	g_signal_connect(cel, "edited", G_CALLBACK(prop_edited), props);
	col = gtk_tree_view_column_new_with_attributes("Value",
				cel, "text", 3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
}

static void probe_toggled(GtkCellRenderer *cel, gchar *path,
			GtkTreeModel *probes)
{
	struct sr_device *device = g_object_get_data(G_OBJECT(probes), "device");
	GtkTreeIter iter;
	struct sr_probe *probe;
	gint i;
	gboolean en;

	(void)cel;

	gtk_tree_model_get_iter_from_string(probes, &iter, path);
	gtk_tree_model_get(probes, &iter, 0, &i, 1, &en, -1);
	probe = sr_device_probe_find(device, i);
	probe->enabled = !en;
	gtk_list_store_set(GTK_LIST_STORE(probes), &iter, 1, probe->enabled, -1);
}

static void probe_named(GtkCellRendererText *cel, gchar *path, gchar *text,
			GtkTreeModel *probes)
{
	struct sr_device *device = g_object_get_data(G_OBJECT(probes), "device");
	GtkTreeIter iter;
	gint i;

	(void)cel;

	gtk_tree_model_get_iter_from_string(probes, &iter, path);
	gtk_tree_model_get(probes, &iter, 0, &i, -1);
	sr_device_probe_name(device, i, text);
	gtk_list_store_set(GTK_LIST_STORE(probes), &iter, 2, text, -1);
}

static void dev_set_probes(GtkWindow *parent)
{
	struct sr_device *device = g_object_get_data(G_OBJECT(parent), "device");
	if(!device)
		return;

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Configure Probes",
					parent, GTK_DIALOG_MODAL,
					GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					NULL);
	GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(sw, 300, 200);
	GtkWidget *tv = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(sw), tv);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), sw,
				TRUE, TRUE, 0);

	/* Populate list store with probe options */
	GtkListStore *probes = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_BOOLEAN,
					G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(probes));
	GtkTreeIter iter;
	GSList *p;
	int i;
	for (p = device->probes, i = 1; p; p = g_slist_next(p), i++) {
		struct sr_probe *probe = p->data;
		gtk_list_store_append(probes, &iter);
		gtk_list_store_set(probes, &iter, 0, i,
					1, probe->enabled, 2, probe->name, -1);
	}

	/* Save device with list so that property can be set by edited
	 * handler. */
	g_object_set_data(G_OBJECT(probes), "device", device);

	/* Add columns to the tree view */
	GtkTreeViewColumn *col;
	col = gtk_tree_view_column_new_with_attributes("Probe",
				gtk_cell_renderer_text_new(),
				"text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);
	GtkCellRenderer *cel = gtk_cell_renderer_toggle_new();
	g_object_set(cel, "activatable", TRUE, NULL);
	g_signal_connect(cel, "toggled", G_CALLBACK(probe_toggled), probes);
	col = gtk_tree_view_column_new_with_attributes("En",
				cel, "active", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);
	cel = gtk_cell_renderer_text_new();
	g_object_set(cel, "editable", TRUE, NULL);
	g_signal_connect(cel, "edited", G_CALLBACK(probe_named), probes);
	col = gtk_tree_view_column_new_with_attributes("Signal Name",
				cel, "text", 2, "sensitive", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

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

static void capture_run(GObject *toolitem, GObject *parent)
{
	struct sr_device *device = g_object_get_data(G_OBJECT(parent), "device");
	GtkEntry *timesamples = g_object_get_data(toolitem, "timesamples");
	GtkComboBox *timeunit = g_object_get_data(toolitem, "timeunit");
	gint i = gtk_combo_box_get_active(timeunit);
	guint64 time_msec = 0;
	guint64 limit_samples = 0;
	
	switch(i) {
	case 0: /* Samples */
		limit_samples = sr_parse_sizestring(
					gtk_entry_get_text(timesamples));
		break;
	case 1: /* Milliseconds */
		time_msec = strtoull(gtk_entry_get_text(timesamples), NULL, 10);
		break;
	case 2: /* Seconds */
		time_msec = strtoull(gtk_entry_get_text(timesamples), NULL, 10)
				* 1000;
		break;
	}

	if(time_msec) {
		int *capabilities = device->plugin->get_capabilities();
		if (sr_find_hwcap(capabilities, SR_HWCAP_LIMIT_MSEC)) {
			if (device->plugin->set_configuration(device->plugin_index,
							  SR_HWCAP_LIMIT_MSEC, &time_msec) != SR_OK) {
				g_critical("Failed to configure time limit.");
				sr_session_destroy();
				return;
			}
		}
		else {
			/* time limit set, but device doesn't support this...
			 * convert to samples based on the samplerate.
			 */
			limit_samples = 0;
			if (sr_device_has_hwcap(device, SR_HWCAP_SAMPLERATE)) {
				guint64 tmp_u64;
				tmp_u64 = *((uint64_t *) device->plugin->get_device_info(
						device->plugin_index, SR_DI_CUR_SAMPLERATE));
				limit_samples = tmp_u64 * time_msec / (uint64_t) 1000;
			}
			if (limit_samples == 0) {
				g_critical("Not enough time at this samplerate.");
				return;
			}

			if (device->plugin->set_configuration(device->plugin_index,
						  SR_HWCAP_LIMIT_SAMPLES, &limit_samples) != SR_OK) {
				g_critical("Failed to configure time-based sample limit.");
				return;
			}
		}
	}
	if(limit_samples) {
		if (device->plugin->set_configuration(device->plugin_index,
					SR_HWCAP_LIMIT_SAMPLES, &limit_samples) != SR_OK) {
			g_critical("Failed to configure sample limit.");
			return;
		}
	}

	if (sr_session_start() != SR_OK) {
		g_critical("Failed to start session.");
		return;
	}

	sr_session_run();
}

void toggle_log(GtkToggleToolButton *button, GObject *parent)
{
	GtkWidget *log = g_object_get_data(parent, "logview");
	gtk_widget_set_visible(log, gtk_toggle_tool_button_get_active(button));
}

void zoom_in(GtkToolButton *button, GObject *parent)
{
	GtkWidget *sigview = g_object_get_data(parent, "sigview");
	sigview_zoom(sigview, 1.5, 0);
}

void zoom_out(GtkToolButton *button, GObject *parent)
{
	GtkWidget *sigview = g_object_get_data(parent, "sigview");
	sigview_zoom(sigview, 1/1.5, 0);
}

GtkWidget *toolbar_init(GtkWindow *parent)
{
	GtkToolbar *toolbar = GTK_TOOLBAR(gtk_toolbar_new());

	/* Device selection GtkComboBox */
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

	/* Device Refresh button */
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH);
	gtk_toolbar_insert(toolbar, toolitem, -1);
	g_signal_connect_swapped(toolitem, "clicked",
				G_CALLBACK(dev_list_refresh), devlist);
	
	/* Device Properties button */
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_PROPERTIES);
	gtk_toolbar_insert(toolbar, toolitem, -1);
	g_signal_connect_swapped(toolitem, "clicked",
				G_CALLBACK(dev_set_options), parent);
	
	/* Probe Configuration button */
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_COLOR_PICKER);
	gtk_toolbar_insert(toolbar, toolitem, -1);
	g_signal_connect_swapped(toolitem, "clicked",
				G_CALLBACK(dev_set_probes), parent);

	gtk_toolbar_insert(toolbar, gtk_separator_tool_item_new(), -1);

	/* Time/Samples entry */
	toolitem = gtk_tool_item_new();
	GtkWidget *timesamples = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(timesamples), "100");
	gtk_entry_set_alignment(GTK_ENTRY(timesamples), 1.0);
	gtk_widget_set_size_request(timesamples, 100, -1);
	gtk_container_add(GTK_CONTAINER(toolitem), timesamples);
	gtk_toolbar_insert(toolbar, toolitem, -1);

	/* Time unit combo box */
	toolitem = gtk_tool_item_new();
	GtkWidget *timeunit = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(timeunit), "samples");
	gtk_combo_box_append_text(GTK_COMBO_BOX(timeunit), "ms");
	gtk_combo_box_append_text(GTK_COMBO_BOX(timeunit), "s");
	gtk_combo_box_set_active(GTK_COMBO_BOX(timeunit), 0);
	gtk_container_add(GTK_CONTAINER(toolitem), timeunit);
	gtk_toolbar_insert(toolbar, toolitem, -1);

	/* Run Capture button */
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_EXECUTE);
	g_object_set_data(G_OBJECT(toolitem), "timesamples", timesamples);
	g_object_set_data(G_OBJECT(toolitem), "timeunit", timeunit);
	gtk_toolbar_insert(toolbar, toolitem, -1);
	g_signal_connect(toolitem, "clicked",
				G_CALLBACK(capture_run), parent);

	gtk_toolbar_insert(toolbar, gtk_separator_tool_item_new(), -1);

	/* Zoom-in button */
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_IN);
	gtk_toolbar_insert(toolbar, toolitem, -1);
	g_signal_connect(toolitem, "clicked",
				G_CALLBACK(zoom_in), parent);

	/* Zoom-out button */
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_ZOOM_OUT);
	gtk_toolbar_insert(toolbar, toolitem, -1);
	g_signal_connect(toolitem, "clicked",
				G_CALLBACK(zoom_out), parent);

	gtk_toolbar_insert(toolbar, gtk_separator_tool_item_new(), -1);

	/* View Log toggle button */
	toolitem = gtk_toggle_tool_button_new_from_stock(GTK_STOCK_JUSTIFY_LEFT);
	gtk_toolbar_insert(toolbar, toolitem, -1);
	g_signal_connect(toolitem, "toggled",
				G_CALLBACK(toggle_log), parent);

	return GTK_WIDGET(toolbar);
}

