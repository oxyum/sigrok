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

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "sigrok-gtk.h"

GtkWidget *sigview;

static const char *colours[8] = {
	"black", "brown", "red", "orange",
	"gold", "darkgreen", "blue", "magenta",
};

static void
datafeed_in(struct sr_dev *dev, struct sr_datafeed_packet *packet)
{
	static int logic_probelist[SR_MAX_NUM_PROBES + 1] = { 0 };
	static int unitsize = 0;
	struct sr_probe *probe;
	struct sr_datafeed_logic *logic = NULL;
	struct sr_datafeed_meta_logic *meta_logic;
	int num_enabled_probes, sample_size, i;
	uint64_t filter_out_len;
	uint8_t *filter_out;
	GArray *data;

	switch (packet->type) {
	case SR_DF_HEADER:
		g_message("fe: Received SR_DF_HEADER");
		break;
	case SR_DF_END:
		sigview_zoom(sigview, 1, 0);
		g_message("fe: Received SR_DF_END");
		sr_session_stop();
		break;
	case SR_DF_TRIGGER:
		g_message("fe: received SR_DF_TRIGGER");
		break;
	case SR_DF_META_LOGIC:
		g_message("fe: received SR_DF_META_LOGIC");
		meta_logic = packet->payload;
		num_enabled_probes = 0;
		gtk_list_store_clear(siglist);
		for (i = 0; i < meta_logic->num_probes; i++) {
			probe = g_slist_nth_data(dev->probes, i);
			if (probe->enabled) {
				GtkTreeIter iter;
				logic_probelist[num_enabled_probes++] = probe->index;
				gtk_list_store_append(siglist, &iter);
				gtk_list_store_set(siglist, &iter,
						0, probe->name,
						1, colours[(num_enabled_probes - 1) & 7],
						2, num_enabled_probes - 1,
						-1);
			}
		}
		/* How many bytes we need to store num_enabled_probes bits */
		unitsize = (num_enabled_probes + 7) / 8;
		data = g_array_new(FALSE, FALSE, unitsize);
		g_object_set_data(G_OBJECT(siglist), "sampledata", data);
		break;
	case SR_DF_LOGIC:
		logic = packet->payload;
		sample_size = logic->unitsize;
		g_message("fe: received SR_DF_LOGIC, %"PRIu64" bytes", logic->length);

		if (!logic)
			break;

		if (sr_filter_probes(sample_size, unitsize, logic_probelist,
					   logic->data, logic->length,
					   &filter_out, &filter_out_len) != SR_OK)
			break;

		data = g_object_get_data(G_OBJECT(siglist), "sampledata");
		g_return_if_fail(data != NULL);

		g_array_append_vals(data, filter_out, filter_out_len/unitsize);

		g_free(filter_out);
		break;
	default:
		g_message("fw: received unknown packet type %d", packet->type);
		break;
	}
}

void load_input_file(GtkWindow *parent, const gchar *file)
{
	if (sr_session_load(file) == SR_OK) {
		/* sigrok session file */
		sr_session_datafeed_callback_add(datafeed_in);
		sr_session_start();
		sr_session_run();
		sr_session_stop();
	}

	/* Create a new session and programatically emit changed signal from
	 * the device selection combo box to reselect the device. 
	 */
	sr_session_new();
	sr_session_datafeed_callback_add(datafeed_in);
	g_signal_emit_by_name(g_object_get_data(G_OBJECT(parent), "devcombo"), 
			"changed");
}

int main(int argc, char **argv)
{
	GtkWindow *window;
	GtkWidget *vbox, *vpaned, *log;
	gtk_init(&argc, &argv);
	icons_register();
	sr_init();
	sr_session_new();
	sr_session_datafeed_callback_add(datafeed_in);

	window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	gtk_window_set_icon_name(window, "sigrok-logo");
	gtk_window_set_title(window, "sigrok-gtk");
	gtk_window_set_default_size(window, 600, 400);

	g_signal_connect(window, "destroy", gtk_main_quit, NULL);

	vbox = gtk_vbox_new(FALSE, 0);
	vpaned = gtk_vpaned_new();

	gtk_box_pack_start(GTK_BOX(vbox), toolbar_init(window), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), vpaned, TRUE, TRUE, 0);
	log = log_init();
	gtk_widget_set_no_show_all(log, TRUE);
	g_object_set_data(G_OBJECT(window), "logview", log);
	gtk_paned_add2(GTK_PANED(vpaned), log);
	sigview = sigview_init();
	g_object_set_data(G_OBJECT(window), "sigview", sigview);
	gtk_paned_pack1(GTK_PANED(vpaned), sigview, TRUE, FALSE);

	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show_all(GTK_WIDGET(window));

	sr_log_loglevel_set(SR_LOG_ERR);

	gtk_main();

	sr_session_destroy();
	gtk_exit(0);

	return 0;
}

