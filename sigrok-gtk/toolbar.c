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

#include <stdlib.h>

#include <sigrok.h>

#include <gtk/gtk.h>

#include <stdlib.h>

#include "sigrok-gtk.h"

enum {
	DEV_PROP_HWCAP,
	DEV_PROP_TYPE,
	DEV_PROP_SHORTNAME,
	DEV_PROP_DESCRIPTION,
	DEV_PROP_IS_TEXT,
	DEV_PROP_TEXTVALUE,
	DEV_PROP_BOOLVALUE,
	MAX_DEV_PROP
};

static void prop_edited(GtkCellRendererText *cel, gchar *path, gchar *text,
			GtkListStore *props)
{
	(void)cel;

	struct sr_dev *dev = g_object_get_data(G_OBJECT(props), "dev");
	GtkTreeIter iter;
	int type, cap;
	guint64 tmp_u64;
	int ret = SR_ERR;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(props), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(props), &iter,
					DEV_PROP_HWCAP, &cap, 
					DEV_PROP_TYPE, &type, -1);

	switch (type) {
	case SR_T_UINT64:
		if (sr_parse_sizestring(text, &tmp_u64) != SR_OK)
			return;

		ret = dev->driver->dev_config_set(dev->driver_index,
						  cap, &tmp_u64);
		break;
	case SR_T_CHAR:
		ret = dev->driver->dev_config_set(dev->driver_index, cap, text);
		break;
	/* SR_T_BOOL will be handled by prop_toggled */
	}

	if (!ret)
		gtk_list_store_set(props, &iter, DEV_PROP_TEXTVALUE, text, -1);
}

static void prop_toggled(GtkCellRendererToggle *cel, gchar *path,
			GtkListStore *props)
{
	struct sr_dev *dev = g_object_get_data(G_OBJECT(props), "dev");
	GtkTreeIter iter;
	int type, cap;
	int ret;
	gboolean val;
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(props), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(props), &iter,
					DEV_PROP_HWCAP, &cap, 
					DEV_PROP_TYPE, &type, -1);

	val = !gtk_cell_renderer_toggle_get_active(cel);
	ret = dev->driver->dev_config_set(dev->driver_index, cap, 
					  GINT_TO_POINTER(val));

	if (!ret)
		gtk_list_store_set(props, &iter, DEV_PROP_BOOLVALUE, val, -1);
}

void dev_prop_bool_data_func(GtkCellLayout *cell_layout,
				GtkCellRenderer *cell,
				GtkTreeModel *tree_model,
				GtkTreeIter *iter,
				gpointer data)
{
	(void)cell_layout;
	(void)data;

	gboolean istext, val;
	gtk_tree_model_get(tree_model, iter, 
				DEV_PROP_IS_TEXT, &istext, 
				DEV_PROP_BOOLVALUE, &val, -1);
	g_object_set(G_OBJECT(cell), "visible", !istext, "active", val, NULL);
}

static void dev_set_options(GtkAction *action, GtkWindow *parent)
{
	(void)action;

	struct sr_dev *dev = g_object_get_data(G_OBJECT(parent), "dev");
	if (!dev)
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
	GtkListStore *props = gtk_list_store_new(MAX_DEV_PROP, 
					G_TYPE_INT, G_TYPE_INT,
					G_TYPE_STRING, G_TYPE_STRING,
					G_TYPE_BOOLEAN, G_TYPE_STRING,
					G_TYPE_BOOLEAN);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(props));
	const int *hwcaps = dev->driver->hwcap_get_all();
	int cap;
	GtkTreeIter iter;
	for (cap = 0; hwcaps[cap]; cap++) {
		const struct sr_hwcap_option *hwo;
		if (!(hwo = sr_hw_hwcap_get(hwcaps[cap])))
			continue;
		gtk_list_store_append(props, &iter);
		gtk_list_store_set(props, &iter, 
					DEV_PROP_HWCAP, hwcaps[cap],
					DEV_PROP_TYPE, hwo->type,
					DEV_PROP_SHORTNAME, hwo->shortname,
					DEV_PROP_DESCRIPTION, hwo->description,
					DEV_PROP_IS_TEXT, hwo->type != SR_T_BOOL,
					-1);
	}

	/* Popup tooltop containing description if mouse hovers */
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(tv), 
					DEV_PROP_DESCRIPTION);

	/* Save device with list so that property can be set by edited
	 * handler. */
	g_object_set_data(G_OBJECT(props), "dev", dev);

	/* Add columns to the tree view */
	GtkTreeViewColumn *col;
	col = gtk_tree_view_column_new_with_attributes("Property",
				gtk_cell_renderer_text_new(),
				"text", DEV_PROP_SHORTNAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);
	/* We pack both a text and toggle renderer.  Only one will be visible.
	 * depending on type.
	 */
	GtkCellRenderer *cel = gtk_cell_renderer_text_new();
	g_object_set(cel, "editable", TRUE, NULL);
	g_signal_connect(cel, "edited", G_CALLBACK(prop_edited), props);
	col = gtk_tree_view_column_new_with_attributes("Value",
				cel, "text", DEV_PROP_TEXTVALUE, 
				"visible", DEV_PROP_IS_TEXT, NULL);
	cel = gtk_cell_renderer_toggle_new();
	g_signal_connect(cel, "toggled", G_CALLBACK(prop_toggled), props);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(col), cel, TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(col), cel, 
				dev_prop_bool_data_func, NULL, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);


	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
}

enum {
	PROBE_NUMBER,
	PROBE_ENABLED,
	PROBE_NAME,
	PROBE_TRIGGER,
	MAX_PROBE
};

static void probe_toggled(GtkCellRenderer *cel, gchar *path,
			GtkTreeModel *probes)
{
	struct sr_dev *dev = g_object_get_data(G_OBJECT(probes), "dev");
	GtkTreeIter iter;
	struct sr_probe *probe;
	gint i;
	gboolean en;

	(void)cel;

	gtk_tree_model_get_iter_from_string(probes, &iter, path);
	gtk_tree_model_get(probes, &iter, PROBE_NUMBER, &i, 
					PROBE_ENABLED, &en, -1);
	probe = sr_dev_probe_find(dev, i);
	probe->enabled = !en;
	gtk_list_store_set(GTK_LIST_STORE(probes), &iter, 
					PROBE_ENABLED, probe->enabled, -1);
}

static void probe_named(GtkCellRendererText *cel, gchar *path, gchar *text,
			GtkTreeModel *probes)
{
	struct sr_dev *dev = g_object_get_data(G_OBJECT(probes), "dev");
	GtkTreeIter iter;
	gint i;

	(void)cel;

	gtk_tree_model_get_iter_from_string(probes, &iter, path);
	gtk_tree_model_get(probes, &iter, PROBE_NUMBER, &i, -1);
	sr_dev_probe_name_set(dev, i, text);
	gtk_list_store_set(GTK_LIST_STORE(probes), &iter, PROBE_NAME, text, -1);
}

static void probe_trigger_set(GtkCellRendererText *cel, gchar *path, 
			gchar *text, GtkTreeModel *probes)
{
	struct sr_dev *dev = g_object_get_data(G_OBJECT(probes), "dev");
	GtkTreeIter iter;
	gint i;

	(void)cel;

	gtk_tree_model_get_iter_from_string(probes, &iter, path);
	gtk_tree_model_get(probes, &iter, PROBE_NUMBER, &i, -1);
	sr_dev_trigger_set(dev, i, text);
	gtk_list_store_set(GTK_LIST_STORE(probes), &iter, 
					PROBE_TRIGGER, text, -1);
}

static void dev_set_probes(GtkAction *action, GtkWindow *parent)
{
	(void)action;

	struct sr_dev *dev = g_object_get_data(G_OBJECT(parent), "dev");
	if (!dev)
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
	GtkListStore *probes = gtk_list_store_new(MAX_PROBE, 
					G_TYPE_INT, G_TYPE_BOOLEAN,
					G_TYPE_STRING, GTK_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(probes));
	GtkTreeIter iter;
	GSList *p;
	int i;
	for (p = dev->probes, i = 1; p; p = g_slist_next(p), i++) {
		struct sr_probe *probe = p->data;
		gtk_list_store_append(probes, &iter);
		gtk_list_store_set(probes, &iter, PROBE_NUMBER, i,
					PROBE_ENABLED, probe->enabled, 
					PROBE_NAME, probe->name, 
					PROBE_TRIGGER, probe->trigger, 
					-1);
	}

	/* Save device with list so that property can be set by edited
	 * handler. */
	g_object_set_data(G_OBJECT(probes), "dev", dev);

	/* Add columns to the tree view */
	GtkTreeViewColumn *col;
	col = gtk_tree_view_column_new_with_attributes("Probe",
				gtk_cell_renderer_text_new(),
				"text", PROBE_NUMBER, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);
	GtkCellRenderer *cel = gtk_cell_renderer_toggle_new();
	g_object_set(cel, "activatable", TRUE, NULL);
	g_signal_connect(cel, "toggled", G_CALLBACK(probe_toggled), probes);
	col = gtk_tree_view_column_new_with_attributes("En",
				cel, "active", PROBE_ENABLED, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);
	cel = gtk_cell_renderer_text_new();
	g_object_set(cel, "editable", TRUE, NULL);
	g_signal_connect(cel, "edited", G_CALLBACK(probe_named), probes);
	col = gtk_tree_view_column_new_with_attributes("Signal Name", cel, 
					"text", PROBE_NAME, 
					"sensitive", PROBE_ENABLED, NULL);
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);
	cel = gtk_cell_renderer_text_new();
	g_object_set(cel, "editable", TRUE, NULL);
	g_signal_connect(cel, "edited", G_CALLBACK(probe_trigger_set), probes);
	col = gtk_tree_view_column_new_with_attributes("Trigger", cel, 
					"text", PROBE_TRIGGER, 
					"sensitive", PROBE_ENABLED, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
}

static void capture_run(GtkAction *action, GObject *parent)
{
	(void)action;

	struct sr_dev *dev = g_object_get_data(G_OBJECT(parent), "dev");
	GtkEntry *timesamples = g_object_get_data(parent, "timesamples");
	GtkComboBox *timeunit = g_object_get_data(parent, "timeunit");
	gint i = gtk_combo_box_get_active(timeunit);
	guint64 time_msec = 0;
	guint64 limit_samples = 0;
	
	switch (i) {
	case 0: /* Samples */
		sr_parse_sizestring(gtk_entry_get_text(timesamples), 
				&limit_samples);
		break;
	case 1: /* Milliseconds */
		time_msec = strtoull(gtk_entry_get_text(timesamples), NULL, 10);
		break;
	case 2: /* Seconds */
		time_msec = strtoull(gtk_entry_get_text(timesamples), NULL, 10)
				* 1000;
		break;
	}

	if (time_msec) {
		if (sr_driver_hwcap_exists(dev->driver, SR_HWCAP_LIMIT_MSEC)) {
			if (dev->driver->dev_config_set(dev->driver_index,
							SR_HWCAP_LIMIT_MSEC,
							&time_msec) != SR_OK) {
				g_critical("Failed to configure time limit.");
				sr_session_destroy();
				return;
			}
		} else {
			/* time limit set, but device doesn't support this...
			 * convert to samples based on the samplerate.
			 */
			limit_samples = 0;
			if (sr_dev_has_hwcap(dev, SR_HWCAP_SAMPLERATE)) {
				guint64 tmp_u64;
				tmp_u64 = *((uint64_t *)dev->driver->dev_info_get(
							dev->driver_index,
							SR_DI_CUR_SAMPLERATE));
				limit_samples = tmp_u64 * time_msec / (uint64_t) 1000;
			}
			if (limit_samples == 0) {
				g_critical("Not enough time at this samplerate.");
				return;
			}

			if (dev->driver->dev_config_set(dev->driver_index,
						SR_HWCAP_LIMIT_SAMPLES,
						&limit_samples) != SR_OK) {
				g_critical("Failed to configure time-based sample limit.");
				return;
			}
		}
	}
	if (limit_samples) {
		if (dev->driver->dev_config_set(dev->driver_index,
						SR_HWCAP_LIMIT_SAMPLES,
						&limit_samples) != SR_OK) {
			g_critical("Failed to configure sample limit.");
			return;
		}
	}

	if (dev->driver->dev_config_set(dev->driver_index,
	    SR_HWCAP_PROBECONFIG, (char *)dev->probes) != SR_OK) {
		printf("Failed to configure probes.\n");
		sr_session_destroy();
		return;
	}

	if (sr_session_start() != SR_OK) {
		g_critical("Failed to start session.");
		return;
	}

	sr_session_run();
}

static void dev_file_open(GtkAction *action, GtkWindow *parent)
{
	(void)action;
	static GtkWidget *dialog;
	const gchar *filename;

	if(!dialog)
		dialog = gtk_file_chooser_dialog_new("Open", parent,
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	g_signal_connect(dialog, "delete-event",
					G_CALLBACK(gtk_widget_hide_on_delete),
					NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
		/* Dialog was cancelled or closed */
		gtk_widget_hide(dialog);
		return;
	}

	gtk_widget_hide(dialog);

	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	load_input_file(parent, filename);
}

void toggle_log(GtkToggleAction *action, GObject *parent)
{
	GtkWidget *log = g_object_get_data(parent, "logview");
	gtk_widget_set_visible(log, gtk_toggle_action_get_active(action));
}

void zoom_in(GtkAction *action, GObject *parent)
{
	(void)action;

	GtkWidget *sigview = g_object_get_data(parent, "sigview");
	sigview_zoom(sigview, 1.5, 0);
}

void zoom_out(GtkAction *action, GObject *parent)
{
	(void)action;

	GtkWidget *sigview = g_object_get_data(parent, "sigview");
	sigview_zoom(sigview, 1/1.5, 0);
}

void zoom_fit(GtkAction *action, GObject *parent)
{
	(void)action;

	GtkWidget *sigview = g_object_get_data(parent, "sigview");
	sigview_zoom(sigview, 0, 0);
}

static const GtkActionEntry action_items[] = {
	/* name, stock-id, label, accel, tooltip, callback */
	{"DevMenu", NULL, "_Device", NULL, NULL, NULL},
	{"DevOpen", GTK_STOCK_OPEN, "_Open", "<control>O",
		"Open Session File", G_CALLBACK(dev_file_open)},
	{"DevSelectMenu", NULL, "Select Device", NULL, NULL, NULL},
	{"DevRescan", GTK_STOCK_REFRESH, "_Rescan", "<control>R",
		"Rescan for LA devices", G_CALLBACK(dev_select_rescan)},
	{"DevProperties", GTK_STOCK_PROPERTIES, "_Properties", "<control>P",
		"Configure LA", G_CALLBACK(dev_set_options)},
	{"DevProbes", GTK_STOCK_COLOR_PICKER, "_Probes", "<control>O",
		"Configure Probes", G_CALLBACK(dev_set_probes)},
	{"DevAcquire", GTK_STOCK_EXECUTE, "_Acquire", "<control>A",
		"Acquire Samples", G_CALLBACK(capture_run)},
	{"Exit", GTK_STOCK_QUIT, "E_xit", "<control>Q",
		"Exit the program", G_CALLBACK(gtk_main_quit) },

	{"ViewMenu", NULL, "_View", NULL, NULL, NULL},
	{"ViewZoomIn", GTK_STOCK_ZOOM_IN, "Zoom _In", "plus", NULL,
		G_CALLBACK(zoom_in)},
	{"ViewZoomOut", GTK_STOCK_ZOOM_OUT, "Zoom _Out", "minus",
		NULL, G_CALLBACK(zoom_out)},
	{"ViewZoomFit", GTK_STOCK_ZOOM_FIT, NULL, NULL,
		NULL, G_CALLBACK(zoom_fit)},

	{"HelpMenu", NULL, "_Help", NULL, NULL, NULL},
	{"HelpWiki", GTK_STOCK_ABOUT, "sigrok _Wiki", NULL, NULL,
		G_CALLBACK(help_wiki)},
	{"HelpAbout", GTK_STOCK_ABOUT, "_About", NULL, NULL,
		G_CALLBACK(help_about)},
};

static const GtkToggleActionEntry toggle_items[] = {
	/* name, stock-id, label, accel, tooltip, callback, isactive */
	{"ViewLog", GTK_STOCK_JUSTIFY_LEFT, "_Log", NULL, NULL,
		G_CALLBACK(toggle_log), FALSE},
};

static const char ui_xml[] =
"<ui>"
"  <menubar>"
"    <menu action='DevMenu'>"
"      <menuitem action='DevOpen'/>"
"      <separator/>"
"      <menu action='DevSelectMenu'>"
"        <separator/>"
"        <menuitem action='DevRescan'/>"
"      </menu>"
"      <menuitem action='DevProperties'/>"
"      <menuitem action='DevProbes'/>"
"      <separator/>"
"      <menuitem action='DevAcquire'/>"
"      <separator/>"
"      <menuitem action='Exit'/>"
"    </menu>"
"    <menu action='ViewMenu'>"
"      <menuitem action='ViewZoomIn'/>"
"      <menuitem action='ViewZoomOut'/>"
"      <menuitem action='ViewZoomFit'/>"
"      <separator/>"
"      <menuitem action='ViewLog'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='HelpWiki'/>"
"      <menuitem action='HelpAbout'/>"
"    </menu>"
"  </menubar>"
"  <toolbar>"
"    <placeholder name='DevSelect'/>"
"    <toolitem action='DevRescan'/>"
"    <toolitem action='DevProperties'/>"
"    <toolitem action='DevProbes'/>"
"    <separator/>"
"    <placeholder name='DevSampleCount' />"
"    <toolitem action='DevAcquire'/>"
"    <separator/>"
"    <toolitem action='ViewZoomIn'/>"
"    <toolitem action='ViewZoomOut'/>"
"    <toolitem action='ViewZoomFit'/>"
"    <separator/>"
"  </toolbar>"
"</ui>";

GtkWidget *toolbar_init(GtkWindow *parent)
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkToolbar *toolbar;
	GtkActionGroup *ag = gtk_action_group_new("Actions");
	gtk_action_group_add_actions(ag, action_items,
					G_N_ELEMENTS(action_items), parent);
	gtk_action_group_add_toggle_actions(ag, toggle_items,
					G_N_ELEMENTS(toggle_items), parent);

	GtkUIManager *ui = gtk_ui_manager_new();
	g_object_set_data(G_OBJECT(parent), "ui_manager", ui);
	gtk_ui_manager_insert_action_group(ui, ag, 0);
	GtkAccelGroup *accel = gtk_ui_manager_get_accel_group(ui);
	gtk_window_add_accel_group(parent, accel);

	GError *error = NULL;
        if (!gtk_ui_manager_add_ui_from_string (ui, ui_xml, -1, &error)) {
                g_message ("building menus failed: %s", error->message);
                g_error_free (error);
                exit (-1);
        }

	GtkWidget *menubar = gtk_ui_manager_get_widget(ui, "/menubar");
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(menubar), FALSE, TRUE, 0);
	toolbar = GTK_TOOLBAR(gtk_ui_manager_get_widget(ui, "/toolbar"));
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(toolbar), FALSE, TRUE, 0);

	/* Time/Samples entry */
	GtkToolItem *toolitem = gtk_tool_item_new();
	GtkWidget *timesamples = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(timesamples), "100");
	gtk_entry_set_alignment(GTK_ENTRY(timesamples), 1.0);
	gtk_widget_set_size_request(timesamples, 100, -1);
	gtk_container_add(GTK_CONTAINER(toolitem), timesamples);
	gtk_toolbar_insert(toolbar, toolitem, 7);

	/* Time unit combo box */
	toolitem = gtk_tool_item_new();
	GtkWidget *align = gtk_alignment_new(0.5, 0.5, 2, 0);
	GtkWidget *timeunit = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(timeunit), "samples");
	gtk_combo_box_append_text(GTK_COMBO_BOX(timeunit), "ms");
	gtk_combo_box_append_text(GTK_COMBO_BOX(timeunit), "s");
	gtk_combo_box_set_active(GTK_COMBO_BOX(timeunit), 0);
	gtk_container_add(GTK_CONTAINER(align), timeunit);
	gtk_container_add(GTK_CONTAINER(toolitem), align);
	gtk_toolbar_insert(toolbar, toolitem, 8);

	g_object_set_data(G_OBJECT(parent), "timesamples", timesamples);
	g_object_set_data(G_OBJECT(parent), "timeunit", timeunit);

	/* Device selection GtkComboBox */
	toolitem = gtk_tool_item_new();
	align = gtk_alignment_new(0.5, 0.5, 2, 0);
	GtkWidget *dev = dev_select_combo_box_new(parent);

	gtk_container_add(GTK_CONTAINER(align), dev);
	gtk_container_add(GTK_CONTAINER(toolitem), align);
	gtk_toolbar_insert(toolbar, toolitem, 0);


	return GTK_WIDGET(vbox);
}

