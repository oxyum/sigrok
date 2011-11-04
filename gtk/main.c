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

GtkWidget *log_init(void);
GtkWidget *toolbar_init(GtkWindow *parent);

static void datafeed_in(struct sr_device *device, struct sr_datafeed_packet *packet)
{
	static int probelist[65] = { 0 };
	static int unitsize = 0;
	struct sr_probe *probe;
	struct sr_datafeed_header *header;
	struct sr_datafeed_logic *logic;
	int num_enabled_probes, sample_size, i;

	switch (packet->type) {
	case SR_DF_HEADER:
		g_message("cli: Received SR_DF_HEADER");
		header = packet->payload;
		num_enabled_probes = 0;
		for (i = 0; i < header->num_logic_probes; i++) {
			probe = g_slist_nth_data(device->probes, i);
			if (probe->enabled)
				probelist[num_enabled_probes++] = probe->index;
		}
		/* How many bytes we need to store num_enabled_probes bits */
		unitsize = (num_enabled_probes + 7) / 8;

		break;
	case SR_DF_END:
		g_message("cli: Received SR_DF_END");
		sr_session_halt();
		break;
	case SR_DF_TRIGGER:
		g_message("cli: received SR_DF_TRIGGER at %"PRIu64" ms",
				packet->timeoffset / 1000000);
		break;
	case SR_DF_LOGIC:
		logic = packet->payload;
		sample_size = logic->unitsize;
		g_message("cli: received SR_DF_LOGIC at %f ms duration %f ms, %"PRIu64" bytes",
				packet->timeoffset / 1000000.0, packet->duration / 1000000.0,
				logic->length);
		break;
	case SR_DF_ANALOG:
		break;
	}
}


int main(int argc, char **argv)
{
	GtkWindow *window;
	GtkWidget *vbox;
	gtk_init(&argc, &argv);
	sr_init();
	sr_session_new();
	sr_session_datafeed_callback_add(datafeed_in);

	window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	gtk_window_set_title(window, "Sigrok-GTK");
	gtk_window_set_default_size(window, 600, 400);

	g_signal_connect(window, "destroy", gtk_main_quit, NULL);

	vbox = gtk_vbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), toolbar_init(window), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), log_init(), TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show_all(GTK_WIDGET(window));

	sr_set_loglevel(5);

	gtk_main();

	sr_session_destroy();
	gtk_exit(0);

	return 0;
}

