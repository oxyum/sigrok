/*
 * This file is part of the sigrok project.
 *
 * Copyright (C) 2010 Bert Vermeulen <bert@biot.com>
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <glib.h>
#include <libusb.h>
#include <sigrok.h>
#include "sigrok-cli.h"
#include "config.h"

#define DEFAULT_OUTPUT_FORMAT "bits:width=64"

extern struct hwcap_option hwcap_options[];

/* demo.c */
extern GIOChannel channels[2];

gboolean debug = 0;
int end_acquisition = FALSE;
uint64_t limit_samples = 0;
struct output_format *output_format = NULL;
int default_output_format = FALSE;
char *output_format_param = NULL;
char *input_format_param = NULL;

/* Protocol decoder */
struct sigrokdecode_decoder *dec;
char *current_decoder;

/* These live in hwplugin.c, for the frontend to override. */
extern source_callback_add source_cb_add;
extern source_callback_remove source_cb_remove;

struct source {
	int fd;
	int events;
	int timeout;
	receive_data_callback cb;
	void *user_data;
};

struct source *sources = NULL;
int num_sources = 0;
int source_timeout = -1;

static gboolean opt_version = FALSE;
static gboolean opt_list_devices = FALSE;
static gboolean opt_wait_trigger = FALSE;
static gchar *opt_input_file = NULL;
static gchar *opt_output_file = NULL;
static gchar *opt_device = NULL;
static gchar *opt_probes = NULL;
static gchar *opt_triggers = NULL;
static gchar *opt_pds = NULL;
static gchar *opt_format = NULL;
static gchar *opt_time = NULL;
static gchar *opt_samples = NULL;
static gchar *opt_continuous = NULL;

static GOptionEntry optargs[] = {
	{"version", 'V', 0, G_OPTION_ARG_NONE, &opt_version, "Show version and support list", NULL},
	{"list-devices", 'D', 0, G_OPTION_ARG_NONE, &opt_list_devices, "List devices", NULL},
	{"input-file", 'i', 0, G_OPTION_ARG_FILENAME, &opt_input_file, "Load input from file", NULL},
	{"output-file", 'o', 0, G_OPTION_ARG_FILENAME, &opt_output_file, "Save output to file", NULL},
	{"device", 'd', 0, G_OPTION_ARG_STRING, &opt_device, "Use device ID", NULL},
	{"probes", 'p', 0, G_OPTION_ARG_STRING, &opt_probes, "Probes to use", NULL},
	{"triggers", 't', 0, G_OPTION_ARG_STRING, &opt_triggers, "Trigger configuration", NULL},
	{"wait-trigger", 'w', 0, G_OPTION_ARG_NONE, &opt_wait_trigger, "Wait for trigger", NULL},
	{"protocol-decoders", 'a', 0, G_OPTION_ARG_STRING, &opt_pds, "Protocol decoder sequence", NULL},
	{"format", 'f', 0, G_OPTION_ARG_STRING, &opt_format, "Output format", NULL},
	{"time", 0, 0, G_OPTION_ARG_STRING, &opt_time, "How long to sample (ms)", NULL},
	{"samples", 0, 0, G_OPTION_ARG_STRING, &opt_samples, "Number of samples to acquire", NULL},
	{"continuous", 0, 0, G_OPTION_ARG_NONE, &opt_continuous, "Sample continuously", NULL},
	{NULL, 0, 0, 0, NULL, NULL, NULL}
};

static void show_version(void)
{
	GSList *plugins, *p, *l;
	struct device_plugin *plugin;
	struct input_format **inputs;
	struct output_format **outputs;
	int i;

	printf("sigrok-cli %s\n\n", VERSION);
	printf("Supported hardware drivers:\n");
	plugins = list_hwplugins();
	for (p = plugins; p; p = p->next) {
		plugin = p->data;
		printf("  %s\n", plugin->name);
	}
	printf("\n");

	printf("Supported input formats:\n");
	inputs = input_list();
	for (i = 0; inputs[i]; i++) {
		printf("  %-20s %s\n", inputs[i]->extension,
		       inputs[i]->description);
	}
	printf("\n");

	printf("Supported output formats:\n");
	outputs = output_list();
	for (i = 0; outputs[i]; i++) {
		printf("  %-20s %s\n", outputs[i]->extension,
		       outputs[i]->description);
	}
	printf("\n");

	/* TODO: Error handling. */
	sigrokdecode_init();

	printf("Supported protocol decoders:\n");
	for (l = sigrokdecode_list_decoders(); l; l = l->next)
		printf("  %s\n", (const char *)l->data);
	printf("\n");

	sigrokdecode_shutdown();
}

static void print_device_line(struct device *device)
{
	struct sigrok_device_instance *sdi;

	sdi = device->plugin->get_device_info(device->plugin_index, DI_INSTANCE);
	printf("%s", sdi->vendor);
	if (sdi->model && sdi->model[0])
		printf(" %s", sdi->model);
	if (sdi->version && sdi->version[0])
		printf(" %s", sdi->version);
	if (device->probes)
		printf(" with %d probes", g_slist_length(device->probes));
	printf("\n");
}

static void show_device_list(void)
{
	struct device *device, *demo_device;
	GSList *devices, *l;
	int devcnt;

	devcnt = 0;
	devices = device_list();

	if (g_slist_length(devices) == 0)
		return;

	printf("The following devices were found:\nID    Device\n");
	demo_device = NULL;
	for (l = devices; l; l = l->next) {
		device = l->data;
		if (strstr(device->plugin->name, "demo")) {
			demo_device = device;
			continue;
		}
		printf("%-3d   ", devcnt++);
		print_device_line(device);
	}
	if (demo_device) {
		printf("demo  ");
		print_device_line(demo_device);
	}
}

static void show_device_detail(void)
{
	struct device *device;
	struct hwcap_option *hwo;
	struct samplerates *samplerates;
	int cap, *capabilities, i;
	char *s, *title, *charopts, **stropts;

	device = parse_devicestring(opt_device);
	if (!device) {
		printf("No such device. Use -D to list all devices.\n");
		return;
	}

	print_device_line(device);

	if ((charopts = (char *)device->plugin->get_device_info(
			device->plugin_index, DI_TRIGGER_TYPES))) {
		printf("Supported triggers: ");
		while (*charopts) {
			printf("%c ", *charopts);
			charopts++;
		}
		printf("\n");
	}

	title = "Supported options:\n";
	capabilities = device->plugin->get_capabilities();
	for (cap = 0; capabilities[cap]; cap++) {
		if (!(hwo = find_hwcap_option(capabilities[cap])))
			continue;

		if (title) {
			printf("%s", title);
			title = NULL;
		}

		if (hwo->capability == HWCAP_PATTERN_MODE) {
			printf("    %s", hwo->shortname);
			stropts = (char **)device->plugin->get_device_info(
					device->plugin_index, DI_PATTERNMODES);
			if (!stropts) {
				printf("\n");
				continue;
			}
			printf(" - supported modes:\n");
			for (i = 0; stropts[i]; i++)
				printf("      %s\n", stropts[i]);
		} else if (hwo->capability == HWCAP_SAMPLERATE) {
			printf("    %s", hwo->shortname);
			/* Supported samplerates */
			samplerates = device->plugin->get_device_info(
				device->plugin_index, DI_SAMPLERATES);
			if (!samplerates) {
				printf("\n");
				continue;
			}

			if (samplerates->step) {
				/* low */
				if (!(s = sigrok_samplerate_string(samplerates->low)))
					continue;
				printf(" (%s", s);
				free(s);
				/* high */
				if (!(s = sigrok_samplerate_string(samplerates->high)))
					continue;
				printf(" - %s", s);
				free(s);
				/* step */
				if (!(s = sigrok_samplerate_string(samplerates->step)))
					continue;
				printf(" in steps of %s)\n", s);
				free(s);
			} else {
				printf(" - supported samplerates:\n");
				for (i = 0; samplerates->list[i]; i++) {
					printf("      %7s\n", sigrok_samplerate_string(samplerates->list[i]));
				}
			}
		} else {
			printf("    %s\n", hwo->shortname);
		}
	}
}

static void datafeed_in(struct device *device, struct datafeed_packet *packet)
{
	static struct output *o = NULL;
	static int probelist[65] = { 0 };
	static uint64_t received_samples = 0;
	static int unitsize = 0;
	static int triggered = 0;
	static FILE *outfile = NULL;
	struct probe *probe;
	struct datafeed_header *header;
	int num_enabled_probes, sample_size, ret, i;
	uint64_t output_len, filter_out_len, len, dec_out_size;
	char *output_buf, *filter_out;
	uint8_t *dec_out;

	/* If the first packet to come in isn't a header, don't even try. */
	if (packet->type != DF_HEADER && o == NULL)
		return;

	sample_size = -1;

	switch (packet->type) {
	case DF_HEADER:
		/* Initialize the output module. */
		if (!(o = malloc(sizeof(struct output)))) {
			printf("Output module malloc failed.\n");
			exit(1);
		}
		o->format = output_format;
		o->device = device;
		o->param = output_format_param;
		if (o->format->init) {
			if (o->format->init(o) != SIGROK_OK) {
				printf("Output format initialization failed.\n");
				exit(1);
			}
		}

		header = (struct datafeed_header *)packet->payload;
		num_enabled_probes = 0;
		for (i = 0; i < header->num_logic_probes; i++) {
			probe = g_slist_nth_data(device->probes, i);
			if (probe->enabled)
				probelist[num_enabled_probes++] = probe->index;
		}
		/* How many bytes we need to store num_enabled_probes bits */
		unitsize = (num_enabled_probes + 7) / 8;

		outfile = stdout;
		if (opt_output_file) {
			if (default_output_format) {
				/* output file is in session format, which means we'll
				 * dump everything in the datastore as it comes in,
				 * and save from there after the session. */
				outfile = NULL;
				ret = datastore_new(unitsize, &(device->datastore));
				if (ret != SIGROK_OK) {
					printf("Failed to create datastore.\n");
					exit(1);
				}
			} else {
				/* saving to a file in whatever format was set
				 * with --format, so all we need is a filehandle */
				outfile = fopen(opt_output_file, "w");
			}
		}
		break;
	case DF_END:
		g_message("Received DF_END");
		if (end_acquisition) {
			g_message("double end!");
			break;
		}
		if (o->format->event) {
			o->format->event(o, DF_END, &output_buf, &output_len);
			if (output_len) {
				if (outfile)
					fwrite(output_buf, 1, output_len, outfile);
				free(output_buf);
				output_len = 0;
			}
		}
		if (limit_samples && received_samples < limit_samples)
			printf("Device only sent %" PRIu64 " samples.\n",
			       received_samples);
		if (opt_continuous)
			printf("Device stopped after %" PRIu64 " samples.\n",
			       received_samples);
		end_acquisition = TRUE;
		if (outfile && outfile != stdout)
			fclose(outfile);
		free(o);
		o = NULL;
		break;
	case DF_TRIGGER:
		if (o->format->event)
			o->format->event(o, DF_TRIGGER, &output_buf, &output_len);
		triggered = 1;
		break;
	case DF_LOGIC:
	case DF_ANALOG:
		sample_size = packet->unitsize;
		break;
	}

	if (sample_size == -1 || packet->length == 0)
		return;

	/* Don't store any samples until triggered. */
	if (opt_wait_trigger && !triggered)
		return;

	if (limit_samples && received_samples >= limit_samples)
		return;

	if (packet->type == DF_LOGIC) {
		/* filters only support DF_LOGIC */
		ret = filter_probes(sample_size, unitsize, probelist,
				    packet->payload, packet->length,
				    &filter_out, &filter_out_len);
		if (ret != SIGROK_OK)
			return;
	} else {
		if (!(filter_out = malloc(packet->length)))
			return;
		memcpy(filter_out, packet->payload, packet->length);
		filter_out_len = packet->length;
	}

	if (device->datastore)
		datastore_put(device->datastore, filter_out,
			      filter_out_len, sample_size, probelist);

	if (opt_output_file && default_output_format)
		/* saving to a session file, don't need to do anything else
		 * to this data for now. */
		goto cleanup;

	if (current_decoder) {
		/* TODO: Handle errors. */
		sigrokdecode_run_decoder(dec, packet->payload, packet->length,
					 &dec_out, &dec_out_size);
		printf("Protocol decoder output:\n%s\n", dec_out);
	}

	/* Don't dump samples on stdout when also saving the session. */
	output_len = 0;
	if (o->format->data && packet->type == o->format->df_type) {
		if (limit_samples && (received_samples + packet->length
			/ sample_size > limit_samples * sample_size))
			len = limit_samples * sample_size
				- received_samples;
		else
			len = filter_out_len;
		o->format->data(o, filter_out, len, &output_buf, &output_len);
	}
	if (output_len) {
		fwrite(output_buf, 1, output_len, outfile);
		free(output_buf);
	}

	cleanup:
	free(filter_out);
	received_samples += packet->length / sample_size;

}

static void remove_source(int fd)
{
	struct source *new_sources;
	int old, new;

	if (!sources)
		return;

	new_sources = calloc(1, sizeof(struct source) * num_sources);
	for (old = 0; old < num_sources; old++)
		if (sources[old].fd != fd)
			memcpy(&new_sources[new++], &sources[old],
			       sizeof(struct source));

	if (old != new) {
		free(sources);
		sources = new_sources;
		num_sources--;
	} else {
		/* Target fd was not found. */
		free(new_sources);
	}
}

void add_source(int fd, int events, int timeout,
	        receive_data_callback callback, void *user_data)
{
	struct source *new_sources, *s;

	new_sources = calloc(1, sizeof(struct source) * (num_sources + 1));

	if (sources) {
		memcpy(new_sources, sources,
		       sizeof(struct source) * num_sources);
		free(sources);
	}

	s = &new_sources[num_sources++];
	s->fd = fd;
	s->events = events;
	s->timeout = timeout;
	s->cb = callback;
	s->user_data = user_data;
	sources = new_sources;

	if (timeout != source_timeout && timeout > 0
	    && (source_timeout == -1 || timeout < source_timeout))
		source_timeout = timeout;
}

/* Register the given PDs for this session. */
/* TODO: Support both serial PDs and nested PDs. Parallel PDs even? */
/* TODO: Only register here, run in streaming fashion later/elsewhere. */
static int register_pds(struct device *device, const char *pdstring)
{
	char **tokens;

	/* Avoid compiler warnings. */
	device = device;

	tokens = g_strsplit(pdstring, ",", 10 /* FIXME */);

	if (tokens[0] && strlen(tokens[0]) > 0) {
		current_decoder = tokens[0];
		/* TODO: Handle errors. */
		sigrokdecode_load_decoder(current_decoder, &dec);
	}

	return 0;
}

static int select_probes(struct device *device)
{
	struct probe *probe;
	char **probelist;
	int max_probes, i;

	if (!opt_probes)
		return SIGROK_OK;

	/*
	 * This only works because a device by default initializes
	 * and enables all its probes.
	 */
	max_probes = g_slist_length(device->probes);
	probelist = parse_probestring(max_probes, opt_probes);
	if (!probelist) {
		return SIGROK_ERR;
	}

	for (i = 0; i < max_probes; i++) {
		if (probelist[i]) {
			device_probe_name(device, i + 1, probelist[i]);
			g_free(probelist[i]);
		} else {
			probe = probe_find(device, i + 1);
			probe->enabled = FALSE;
		}
	}
	g_free(probelist);

	return SIGROK_OK;
}

static void load_input_file(void)
{
	struct stat st;
	struct input *in;
	struct input_format **inputs, *input_format;
	int i;

	/* Find an input module that can handle this file. */
	inputs = input_list();
	for (i = 0; inputs[i]; i++) {
		if (inputs[i]->format_match(opt_input_file))
			break;
	}

	/* Abort if no input module wanted to touch this. */
	if (!inputs[i])
		return;

	input_format = inputs[i];

	if (stat(opt_input_file, &st) == -1) {
		printf("Failed to load %s: %s\n", opt_input_file,
			strerror(errno));
		exit(1);
	}

	/* Initialize the input module. */
	if (!(in = malloc(sizeof(struct input)))) {
		printf("Failed to allocate input module.\n");
		exit(1);
	}
	in->format = input_format;
	in->param = input_format_param;
	if (in->format->init) {
		if (in->format->init(in) != SIGROK_OK) {
			printf("Input format init failed.\n");
			exit(1);
		}
	}

	if (select_probes(in->vdevice) > 0)
            return;

	session_new();
	session_datafeed_callback_add(datafeed_in);
	if (session_device_add(in->vdevice) != SIGROK_OK) {
		printf("Failed to use device.\n");
		session_destroy();
		return;
	}

	input_format->loadfile(in, opt_input_file);
	session_stop();
	if (opt_output_file && default_output_format) {
		if (session_save(opt_output_file) != SIGROK_OK)
			printf("Failed to save session.\n");
	}
	session_destroy();

}

int num_real_devices(void)
{
	struct device *device;
	GSList *devices, *l;
	int num_devices;

	num_devices = 0;
	devices = device_list();
	for (l = devices; l; l = l->next) {
		device = l->data;
		if (!strstr(device->plugin->name, "demo"))
			num_devices++;
	}

	return num_devices;
}

int set_device_options(struct device *device, GHashTable *args)
{
	GHashTableIter iter;
	gpointer key, value;
	int ret, i;
	uint64_t tmp_u64;
	gboolean found;

	g_hash_table_iter_init(&iter, args);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		found = FALSE;
		for (i = 0; hwcap_options[i].capability; i++) {
			if (strcmp(hwcap_options[i].shortname, key))
				continue;
			if (value == NULL && hwcap_options[i].type != T_NULL) {
				printf("Option '%s' needs a value.\n", (char *)key);
				return SIGROK_ERR;
			}
			found = TRUE;
			switch (hwcap_options[i].type) {
			case T_UINT64:
				tmp_u64 = parse_sizestring(value);
				ret = device->plugin-> set_configuration(device-> plugin_index,
						hwcap_options[i]. capability, &tmp_u64);
				break;
			case T_CHAR:
				ret = device->plugin-> set_configuration(device-> plugin_index,
						hwcap_options[i]. capability, value);
				break;
			case T_NULL:
				ret = device->plugin-> set_configuration(device-> plugin_index,
						hwcap_options[i]. capability, NULL);
				break;
			default:
				ret = SIGROK_ERR;
			}

			if (ret != SIGROK_OK) {
				printf("Failed to set device option '%s'.\n", (char *)key);
				return ret;
			}
			else
				break;
		}
		if (!found) {
			printf("Unknown device option '%s'.\n", (char *) key);
			return SIGROK_ERR;
		}
	}

	return SIGROK_OK;
}

static void run_session(void)
{
	struct device *device;
	GPollFD *fds, my_gpollfd;
	GHashTable *devargs;
	int num_devices, max_probes, *capabilities, ret, i;
	uint64_t tmp_u64, time_msec;
	char **probelist, *devspec;

	devargs = NULL;
	if (opt_device) {
		devargs = parse_generic_arg(opt_device);
		devspec = g_hash_table_lookup(devargs, "sigrok_key");
		device = parse_devicestring(devspec);
		if (!device) {
			g_warning("Device not found.");
			return;
		}
		g_hash_table_remove(devargs, "sigrok_key");
	} else {
		num_devices = num_real_devices();
		if (num_devices == 1) {
			/* No device specified, but there is only one. */
			devargs = NULL;
			device = parse_devicestring("0");
		} else if (num_devices == 0) {
			printf("No devices found.\n");
			return;
		} else {
			printf("%d devices found, please select one.\n", num_devices);
			return;
		}
	}

	session_new();
	session_datafeed_callback_add(datafeed_in);
	source_cb_remove = remove_source;
	source_cb_add = add_source;

	if (session_device_add(device) != SIGROK_OK) {
		printf("Failed to use device.\n");
		session_destroy();
		return;
	}

	if (devargs) {
		if (set_device_options(device, devargs) != SIGROK_OK) {
			session_destroy();
			return;
		}
		g_hash_table_destroy(devargs);
	}

	if (select_probes(device) != SIGROK_OK)
            return;

	if (opt_continuous) {
		capabilities = device->plugin->get_capabilities();
		if (!find_hwcap(capabilities, HWCAP_CONTINUOUS)) {
			printf("This device does not support continuous sampling.");
			session_destroy();
			return;
		}
	}

	if (opt_triggers) {
		probelist = parse_triggerstring(device, opt_triggers);
		if (!probelist) {
			session_destroy();
			return;
		}

		max_probes = g_slist_length(device->probes);
		for (i = 0; i < max_probes; i++) {
			if (probelist[i]) {
				device_trigger_set(device, i + 1, probelist[i]);
				g_free(probelist[i]);
			}
		}
		g_free(probelist);
	}

	if (opt_time) {
		time_msec = parse_timestring(opt_time);
		if (time_msec == 0) {
			printf("Invalid time '%s'\n", opt_time);
			session_destroy();
			return;
		}

		capabilities = device->plugin->get_capabilities();
		if (find_hwcap(capabilities, HWCAP_LIMIT_MSEC)) {
			if (device->plugin->set_configuration(device->plugin_index,
							  HWCAP_LIMIT_MSEC, &time_msec) != SIGROK_OK) {
				printf("Failed to configure time limit.\n");
				session_destroy();
				return;
			}
		}
		else {
			/* time limit set, but device doesn't support this...
			 * convert to samples based on the samplerate.
			 */
			tmp_u64 = *((uint64_t *) device->plugin->get_device_info(
					device->plugin_index, DI_CUR_SAMPLERATE));
			limit_samples = tmp_u64 * time_msec / (uint64_t) 1000;
			if (limit_samples == 0) {
				printf("Not enough time at this samplerate.\n");
				session_destroy();
				return;
			}

			if (device->plugin->set_configuration(device->plugin_index,
						  HWCAP_LIMIT_SAMPLES, &limit_samples) != SIGROK_OK) {
				printf("Failed to configure time-based sample limit.\n");
				session_destroy();
				return;
			}
		}
	}

	if (opt_samples) {
		limit_samples = parse_sizestring(opt_samples);
		if (device->plugin->set_configuration(device->plugin_index,
					  HWCAP_LIMIT_SAMPLES, &limit_samples) != SIGROK_OK) {
			printf("Failed to configure sample limit.\n");
			session_destroy();
			return;
		}
	}

	if (device->plugin->set_configuration(device->plugin_index,
		  HWCAP_PROBECONFIG, (char *)device->probes) != SIGROK_OK) {
		printf("Failed to configure probes.\n");
		session_destroy();
		return;
	}

	if (session_start() != SIGROK_OK) {
		printf("Failed to start session.\n");
		session_destroy();
		return;
	}

	if (opt_continuous)
		add_anykey();

	fds = NULL;
	while (!end_acquisition) {
		if (fds)
			free(fds);

		/* Construct g_poll()'s array. */
		fds = malloc(sizeof(GPollFD) * num_sources);
		for (i = 0; i < num_sources; i++) {
#ifdef _WIN32
			g_io_channel_win32_make_pollfd(&channels[0],
					sources[i].events, &my_gpollfd);
#else
			my_gpollfd.fd = sources[i].fd;
			my_gpollfd.events = sources[i].events;
			fds[i] = my_gpollfd;
#endif
		}

		ret = g_poll(fds, num_sources, source_timeout);

		for (i = 0; i < num_sources; i++) {
			if (fds[i].revents > 0 || (ret == 0
				&& source_timeout == sources[i].timeout)) {
				/*
				 * Invoke the source's callback on an event,
				 * or if the poll timeout out and this source
				 * asked for that timeout.
				 */
				sources[i].cb(fds[i].fd, fds[i].revents,
					      sources[i].user_data);
			}
		}
	}
	free(fds);

	if (opt_continuous)
		clear_anykey();

	session_stop();
	if (opt_output_file && default_output_format) {
		if (session_save(opt_output_file) != SIGROK_OK)
			printf("Failed to save session.\n");
	}
	session_destroy();

}

static void logger(const gchar *log_domain, GLogLevelFlags log_level,
		   const gchar *message, gpointer user_data)
{
	/* Avoid compiler warnings. */
	log_domain = log_domain;
	user_data = user_data;

	/*
	 * All messages, warnings, errors etc. go to stderr (not stdout) in
	 * order to not mess up the CLI tool data output, e.g. VCD output.
	 */
	if (log_level & (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_WARNING)) {
		fprintf(stderr, "%s\n", message);
		fflush(stderr);
	} else {
		if ((log_level & G_LOG_LEVEL_MESSAGE && debug == 1)
		    || debug == 2) {
			printf("%s\n", message);
			fflush(stderr);
		}
	}
}

int main(int argc, char **argv)
{
	struct output_format **outputs;
	GOptionContext *context;
	GError *error;
	GHashTable *fmtargs;
	GHashTableIter iter;
	gpointer key, value;
	int i;
	char *fmtspec;

	/* No decoder at the moment. */
	current_decoder = NULL;

	g_log_set_default_handler(logger, NULL);
	if (getenv("SIGROK_DEBUG"))
		debug = strtol(getenv("SIGROK_DEBUG"), NULL, 10);

	error = NULL;
	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, optargs, NULL);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_warning("%s", error->message);
		return 1;
	}

	if (sigrok_init() != SIGROK_OK)
		return 1;

	if (opt_pds) {
		/* TODO: Error handling. */
		sigrokdecode_init();
		register_pds(NULL, opt_pds);
	}

	if (!opt_format) {
		opt_format = DEFAULT_OUTPUT_FORMAT;
		/* we'll need to remember this, so when saving to an file
		 * later, sigrok session format will be used.
		 */
		default_output_format = TRUE;
	}
	fmtargs = parse_generic_arg(opt_format);
	fmtspec = g_hash_table_lookup(fmtargs, "sigrok_key");
	if (!fmtspec) {
		printf("Invalid output format.\n");
		return 1;
	}
	outputs = output_list();
	for (i = 0; outputs[i]; i++) {
		if (strcmp(outputs[i]->extension, fmtspec))
			continue;
		g_hash_table_remove(fmtargs, "sigrok_key");
		output_format = outputs[i];
		g_hash_table_iter_init(&iter, fmtargs);
		while (g_hash_table_iter_next(&iter, &key, &value)) {
			/* only supporting one parameter per output module
			 * for now, and only its value */
			output_format_param = g_strdup(value);
			break;
		}
		break;
	}
	if (!output_format) {
		printf("invalid output format %s\n", opt_format);
		return 1;
	}

	if (opt_version)
		show_version();
	else if (opt_list_devices)
		show_device_list();
	else if (opt_input_file)
		load_input_file();
	else if (opt_samples || opt_time || opt_continuous)
		run_session();
	else if (opt_device)
		show_device_detail();
	else
		printf("%s", g_option_context_get_help(context, TRUE, NULL));

	if (opt_pds)
		sigrokdecode_shutdown();

	g_option_context_free(context);
	g_hash_table_destroy(fmtargs);
	sigrok_cleanup();

	return 0;
}
