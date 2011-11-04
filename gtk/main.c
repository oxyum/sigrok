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

GtkListStore *siglist;
static const char *colours[8] = {
	"black", "brown", "red", "orange",
	"gold", "darkgreen", "blue", "magenta",
};

static void datafeed_in(struct sr_device *device, struct sr_datafeed_packet *packet)
{
	static int probelist[65] = { 0 };
	static int unitsize = 0;
	struct sr_probe *probe;
	struct sr_datafeed_header *header;
	struct sr_datafeed_logic *logic = NULL;
	int num_enabled_probes, sample_size, i;
	uint64_t filter_out_len;
	char *filter_out;
	GArray *data;

	switch (packet->type) {
	case SR_DF_HEADER:
		g_message("cli: Received SR_DF_HEADER");
		header = packet->payload;
		num_enabled_probes = 0;
		gtk_list_store_clear(siglist);
		for (i = 0; i < header->num_logic_probes; i++) {
			probe = g_slist_nth_data(device->probes, i);
			if (probe->enabled) {
				GtkTreeIter iter;
				probelist[num_enabled_probes++] = probe->index;
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

	if(!logic)
		return;

	if(sr_filter_probes(sample_size, unitsize, probelist,
				   logic->data, logic->length,
				   &filter_out, &filter_out_len) != SR_OK)
		return;

	data = g_object_get_data(G_OBJECT(siglist), "sampledata");
	g_return_if_fail(data != NULL);

	g_array_append_vals(data, logic->data, logic->length);
}

void format_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
		GtkTreeModel *siglist, GtkTreeIter *iter, gpointer h)
{
	GArray *data = g_object_get_data(G_OBJECT(siglist), "sampledata");
	guint nsamples = data->len / g_array_get_element_size(data);
	char str[nsamples + 1];
	guint i;
	int probe;
	char *colour;
	uint16_t *tmp16;
	uint32_t *tmp32;

	gtk_tree_model_get(siglist, iter, 1, &colour, 2, &probe, -1);

	str[nsamples] = 0;
	switch(g_array_get_element_size(data)) {
	case 1:
		for(i = 0; i < nsamples; i++)
			str[i] = data->data[i] & (1 << probe) ? '1' : '0';
		break;
	case 2:
		tmp16 = (uint16_t*)data->data;
		for(i = 0; i < nsamples; i++)
			str[i] = tmp16[i] & (1 << probe) ? '1' : '0';
		break;
	case 4:
		tmp32 = (uint32_t*)data->data;
		for(i = 0; i < nsamples; i++)
			str[i] = tmp32[i] & (1 << probe) ? '1' : '0';
		break;
	default:
		g_critical("Sample size not 8, 16 or 32 bits!");
	}
	
	g_object_set(G_OBJECT(cell), "foreground", colour, "text", str, NULL);
}


GtkWidget *sigview_init(void)
{
	GtkWidget *sw, *tv;
	GtkTreeViewColumn *col;
	GtkCellRenderer *cel;

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	tv = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(sw), tv);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), FALSE);

	col = gtk_tree_view_column_new_with_attributes(NULL,
					gtk_cell_renderer_text_new(),
					"text", 0, "foreground", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

	cel = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(col, cel, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, cel, format_func,
					NULL, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

	siglist = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING,
					G_TYPE_INT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(siglist));

	return sw;
}


int main(int argc, char **argv)
{
	GtkWindow *window;
	GtkWidget *vbox, *vpaned;
	gtk_init(&argc, &argv);
	sr_init();
	sr_session_new();
	sr_session_datafeed_callback_add(datafeed_in);

	window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	gtk_window_set_title(window, "Sigrok-GTK");
	gtk_window_set_default_size(window, 600, 400);

	g_signal_connect(window, "destroy", gtk_main_quit, NULL);

	vbox = gtk_vbox_new(FALSE, 0);
	vpaned = gtk_vpaned_new();

	gtk_box_pack_start(GTK_BOX(vbox), toolbar_init(window), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), vpaned, TRUE, TRUE, 0);
	gtk_paned_add2(GTK_PANED(vpaned), log_init());
	gtk_paned_pack1(GTK_PANED(vpaned), sigview_init(), TRUE, FALSE);

	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show_all(GTK_WIDGET(window));

	sr_set_loglevel(5);

	gtk_main();

	sr_session_destroy();
	gtk_exit(0);

	return 0;
}

