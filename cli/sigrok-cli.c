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
#include <glib.h>
#include <libusb.h>
#include <sigrok.h>
#include "config.h"

#define SIGROK_CLI_VERSION "0.1"
#define DEFAULT_OUTPUT_FORMAT "bits64"

extern struct hwcap_option hwcap_options[];

gboolean debug = FALSE;
int end_acquisition = FALSE;
uint64_t limit_samples = 0;
struct output_format *output_format = NULL;
char *output_format_param = NULL;

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
static gboolean opt_list_hwplugins = FALSE;
static gboolean opt_list_devices = FALSE;
static gboolean opt_list_analyzers = FALSE;
static gchar *opt_load_session_filename = NULL;
static gchar *opt_save_session_filename = NULL;
static int opt_device = -1;
static gchar *opt_probes = NULL;
static gchar *opt_triggers = NULL;
static gchar **opt_devoption = NULL;
static gchar *opt_analyzers = NULL;
static gchar *opt_format = NULL;
static gchar *opt_time = NULL;
static gchar *opt_samples = NULL;

static GOptionEntry optargs[] = {
	{"version", 'V', 0, G_OPTION_ARG_NONE, &opt_version, "Show version", NULL},
	{"list-hardware-plugins", 'H', 0, G_OPTION_ARG_NONE, &opt_list_hwplugins, "List hardware plugins", NULL},
	{"list-devices", 'D', 0, G_OPTION_ARG_NONE, &opt_list_devices, "List devices", NULL},
	{"list-analyzer-plugins", 'A', 0, G_OPTION_ARG_NONE, &opt_list_analyzers, "List analyzer plugins", NULL},
	{"load-session-file", 'L', 0, G_OPTION_ARG_FILENAME, &opt_load_session_filename, "Load session from file", NULL},
	{"save-session-file", 'S', 0, G_OPTION_ARG_FILENAME, &opt_save_session_filename, "Save session to file", NULL},
	{"device", 'd', 0, G_OPTION_ARG_INT, &opt_device, "Use device id", NULL},
	{"probes", 'p', 0, G_OPTION_ARG_STRING, &opt_probes, "Probes to use", NULL},
	{"triggers", 't', 0, G_OPTION_ARG_STRING, &opt_triggers, "Trigger configuration", NULL},
	{"device-option", 'o', 0, G_OPTION_ARG_STRING_ARRAY, &opt_devoption, "Device-specific option", NULL},
	{"analyzers", 'a', 0, G_OPTION_ARG_STRING, &opt_analyzers, "Protocol analyzer sequence", NULL},
	{"format", 'f', 0, G_OPTION_ARG_STRING, &opt_format, "Output format", NULL},
	{"time", 0, 0, G_OPTION_ARG_STRING, &opt_time, "How long to sample (ms)", NULL},
	{"samples", 0, 0, G_OPTION_ARG_STRING, &opt_samples, "Number of samples to acquire", NULL},
	{NULL, 0, 0, 0, NULL, NULL, NULL}
};

void show_version(void)
{
	printf("sigrok version %s\nCLI version %s\n", VERSION,
	       SIGROK_CLI_VERSION);
}

void show_hwplugin_list(void)
{
	GSList *plugins, *p;
	struct device_plugin *plugin;

	printf("Plugins for the following devices are installed:\n");
	plugins = list_hwplugins();
	for (p = plugins; p; p = p->next) {
		plugin = p->data;
		printf(" %s\n", plugin->name);
	}
}

void print_device_line(struct device *device)
{
	struct sigrok_device_instance *sdi;

	sdi =
	    device->plugin->get_device_info(device->plugin_index, DI_INSTANCE);
	printf("%s %s", sdi->vendor, sdi->model);
	if (sdi->version && sdi->version[0])
		printf(" %s", sdi->version);
	if (device->probes)
		printf(" with %d probes", g_slist_length(device->probes));
	printf("\n");
}

void show_device_list(void)
{
	struct device *device;
	GSList *devices, *l;
	int devcnt;

	devcnt = 0;
	device_scan();
	devices = device_list();

	if (g_slist_length(devices) > 0) {
		printf("The following devices were found:\nID  Device\n");
		for (l = devices; l; l = l->next) {
			device = l->data;
			printf("%-3d ", devcnt);
			print_device_line(device);
			devcnt++;
		}
	}
}

void show_device_detail(void)
{
	struct device *device;
	struct hwcap_option *hwo;
	struct samplerates *samplerates;
	GSList *devices;
	int cap, *capabilities, i;
	char *title, *triggers;

	device_scan();
	devices = device_list();
	device = g_slist_nth_data(devices, opt_device);
	if (device == NULL) {
		printf("No such device. Use -D to list all devices.\n");
		return;
	}

	print_device_line(device);

	if ((triggers = (char *)device->plugin->get_device_info(
			device->plugin_index, DI_TRIGGER_TYPES))) {
		printf("Supported triggers: ");
		while (*triggers) {
			printf("%c ", *triggers);
			triggers++;
		}
		printf("\n");
	}

	title = "Supported options:\n";
	capabilities = device->plugin->get_capabilities();
	for (cap = 0; capabilities[cap]; cap++) {
		hwo = find_hwcap_option(capabilities[cap]);
		if (hwo) {
			if (title) {
				printf("%s", title);
				title = NULL;
			}
			if (hwo->capability == HWCAP_SAMPLERATE) {
				printf("    %s", hwo->shortname);
				/* Supported samplerates */
				samplerates = device->plugin->get_device_info(
					device->plugin_index, DI_SAMPLERATES);
				if (samplerates) {
					if (samplerates->step) {
						printf(" (%s - %s in steps of %s)\n",
						     sigrok_samplerate_string
						     (samplerates->low),
						     sigrok_samplerate_string
						     (samplerates->high),
						     sigrok_samplerate_string
						     (samplerates->step));
					} else {
						printf(" - supported samplerates:\n");
						for (i = 0; samplerates->list[i]; i++) {
							printf("    %7s\n", sigrok_samplerate_string(samplerates->list[i]));
						}
					}
				} else
					printf("\n");
			} else
				printf("      %s\n", hwo->shortname);
		}
	}
}

void show_analyzer_list(void)
{
	/* TODO: Implement. */
}

void datafeed_in(struct device *device, struct datafeed_packet *packet)
{
	static struct output *o = NULL;
	static int probelist[65] = { 0 };
	static uint64_t received_samples = 0;
	static int unitsize = 0;
	struct probe *probe;
	struct datafeed_header *header;
	int num_enabled_probes, sample_size, i;
	uint64_t output_len, filter_out_len;
	char *output_buf, *filter_out;

	/* If the first packet to come in isn't a header, don't even try. */
	if (packet->type != DF_HEADER && o == NULL)
		return;

	sample_size = 0;

	switch (packet->type) {
	case DF_HEADER:
		/* initialize the output module. */
		o = malloc(sizeof(struct output));
		o->format = output_format;
		o->device = device;
		o->param = output_format_param;
		o->format->init(o);

		header = (struct datafeed_header *)packet->payload;
		num_enabled_probes = 0;
		for (i = 0; i < header->num_probes; i++) {
			probe = g_slist_nth_data(device->probes, i);
			if (probe->enabled)
				probelist[num_enabled_probes++] = probe->index;
		}
		/* How many bytes we need to store num_enabled_probes bits? */
		unitsize = (num_enabled_probes + 7) / 8;

		/*
		 * Saving sessions will need a datastore to dump into
		 * the session file.
		 */
		if (opt_save_session_filename)
			device->datastore = datastore_new(unitsize);
		break;
	case DF_END:
		g_message("received DF_END");
		o->format->event(o, DF_END, &output_buf, &output_len);
		printf("%s", output_buf);
		if (limit_samples && received_samples < limit_samples)
			printf("Device only sent %" PRIu64 " samples.\n",
			       received_samples);
		end_acquisition = TRUE;
		free(o);
		o = NULL;
		break;
	case DF_TRIGGER:
		/*
		 * TODO: If pre-trigger capture is set, display ! here.
		 * Otherwise the capture just always begins with the trigger,
		 * which is fine: no need to mark the trigger.
		 */
		break;
	case DF_LOGIC8:
		sample_size = 1;
		break;
	case DF_LOGIC16:
		sample_size = 2;
		break;
	case DF_LOGIC24:
		sample_size = 3;
		break;
	case DF_LOGIC32:
		sample_size = 4;
		break;
	case DF_LOGIC48:
		sample_size = 6;
		break;
	case DF_LOGIC64:
		sample_size = 8;
		break;
	}

	if (sample_size > 0) {
		if (received_samples < limit_samples) {
			filter_probes(sample_size, unitsize, probelist,
				      packet->payload, packet->length,
				      &filter_out, &filter_out_len);
			if (device->datastore)
				datastore_put(device->datastore, filter_out,
					      filter_out_len, sample_size,
					      probelist);

			/*
			 * Don't dump samples on stdout when also saving the
			 * session.
			 */
			if (!opt_save_session_filename) {
				if (received_samples +
				    packet->length / sample_size >
				    limit_samples * sample_size) {
					o->format->data(o, filter_out,
							limit_samples *
							sample_size -
							received_samples,
							&output_buf,
							&output_len);
				} else {
					o->format->data(o, filter_out,
							filter_out_len,
							&output_buf,
							&output_len);
				}
				printf("%s", output_buf);
			}
			if (filter_out)
				free(filter_out);
			received_samples += packet->length / sample_size;
		}
	}
}

char **parse_probestring(int max_probes, char *probestring)
{
	int tmp, b, e, i;
	char **tokens, **range, **probelist, *name, str[8];
	gboolean error;

	error = FALSE;
	range = NULL;
	probelist = g_malloc0(max_probes * sizeof(char *));
	tokens = g_strsplit(probestring, ",", max_probes);

	for (i = 0; tokens[i]; i++) {
		if (strchr(tokens[i], '-')) {
			/* A range of probes in the form 1-5. */
			range = g_strsplit(tokens[i], "-", 2);
			if (!range[0] || !range[1] || range[2]) {
				/* Need exactly two arguments. */
				printf("Invalid probe syntax '%s'.\n",
				       tokens[i]);
				error = TRUE;
				break;
			}

			b = strtol(range[0], NULL, 10);
			e = strtol(range[1], NULL, 10);
			if (b < 1 || e > max_probes || b >= e) {
				printf("Invalid probe range '%s'.\n",
				       tokens[i]);
				error = TRUE;
				break;
			}

			while (b <= e) {
				snprintf(str, 7, "%d", b);
				probelist[b - 1] = g_strdup(str);
				b++;
			}
		} else {
			tmp = strtol(tokens[i], NULL, 10);
			if (tmp < 1 || tmp > max_probes) {
				printf("Invalid probe %d.\n", tmp);
				error = TRUE;
				break;
			}

			if ((name = strchr(tokens[i], '='))) {
				probelist[tmp - 1] = g_strdup(++name);
			} else {
				snprintf(str, 7, "%d", tmp);
				probelist[tmp - 1] = g_strdup(str);
			}
		}
	}

	if (error) {
		for (i = 0; i < max_probes; i++)
			if (probelist[i])
				g_free(probelist[i]);
		g_free(probelist);
		probelist = NULL;
	}

	g_strfreev(tokens);
	if (range)
		g_strfreev(range);

	return probelist;
}

char **parse_triggerstring(struct device *device, char *triggerstring)
{
	GSList *l;
	struct probe *probe;
	int max_probes, probenum, i;
	char **tokens, **triggerlist, *trigger, *tc, *trigger_types;
	gboolean error;

	max_probes = g_slist_length(device->probes);
	error = FALSE;
	triggerlist = g_malloc0(max_probes * sizeof(char *));
	tokens = g_strsplit(triggerstring, ",", max_probes);
	trigger_types = device->plugin->get_device_info(0, DI_TRIGGER_TYPES);
	if (trigger_types == NULL)
		return NULL;

	for (i = 0; tokens[i]; i++) {
		if (tokens[i][0] < '0' || tokens[i][0] > '9') {
			/* Named probe */
			probenum = 0;
			for (l = device->probes; l; l = l->next) {
				probe = (struct probe *)l->data;
				if (probe->enabled
				    && !strncmp(probe->name, tokens[i],
						strlen(probe->name))) {
					probenum = probe->index;
					break;
				}
			}
		} else {
			probenum = strtol(tokens[i], NULL, 10);
		}

		if (probenum < 1 || probenum > max_probes) {
			printf("Invalid probe.\n");
			error = TRUE;
			break;
		}

		if ((trigger = strchr(tokens[i], '='))) {
			for (tc = ++trigger; *tc; tc++) {
				if (strchr(trigger_types, *tc) == NULL) {
					printf("Unsupported trigger type "
					       "'%c'\n", *tc);
					error = TRUE;
					break;
				}
			}
			if (!error)
				triggerlist[probenum - 1] = g_strdup(trigger);
		}
	}
	g_strfreev(tokens);

	if (error) {
		for (i = 0; i < max_probes; i++)
			if (triggerlist[i])
				g_free(triggerlist[i]);
		g_free(triggerlist);
		triggerlist = NULL;
	}

	return triggerlist;
}

void remove_source(int fd)
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

void add_source(int fd, int events, int timeout, receive_data_callback callback,
		void *user_data)
{
	struct source *new_sources, *s;

	// add_source_fd(fd, events, timeout, callback, user_data);

	new_sources = calloc(1, sizeof(struct source) * (num_sources + 1));

	if (sources) {
		memcpy(new_sources, sources, sizeof(GPollFD) * num_sources);
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

void run_session(void)
{
	struct device *device;
	struct probe *probe;
	struct output_format **formats;
	GPollFD *fds;
	GSList *devices;
	int num_devices, max_probes, *capabilities, ret, i, j;
	unsigned int time_msec;
	uint64_t tmp_u64;
	char **probelist, *val;

	device_scan();
	devices = device_list();
	num_devices = g_slist_length(devices);
	if (num_devices == 0) {
		g_warning("No devices found.");
		return;
	}

	if (opt_device == -1) {
		if (num_devices == 1)
			/* No device specified, but there is only one. */
			opt_device = 0;
		else {
			g_warning("%d devices found, please select one.",
				  num_devices);
			return;
		}
	} else {
		if (opt_device >= num_devices) {
			g_warning("Device not found.");
			return;
		}
	}

	session_new();
	session_datafeed_callback_add(datafeed_in);
	source_cb_remove = remove_source;
	source_cb_add = add_source;

	device = g_slist_nth_data(devices, opt_device);
	if (session_device_add(device) != SIGROK_OK) {
		printf("Failed to use device.\n");
		session_destroy();
		return;
	}

	if (opt_probes) {
		/*
		 * This only works because a device by default initializes
		 * and enables all its probes.
		 */
		max_probes = g_slist_length(device->probes);
		probelist = parse_probestring(max_probes, opt_probes);
		if (!probelist) {
			session_destroy();
			return;
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

	if (!opt_format)
		opt_format = DEFAULT_OUTPUT_FORMAT;
	formats = output_list();
	for (i = 0; formats[i]; i++) {
		if (!strncasecmp(formats[i]->extension, opt_format,
		     strlen(formats[i]->extension))) {
			output_format = formats[i];
			output_format_param =
			    opt_format + strlen(formats[i]->extension);
			break;
		}
	}

	if (opt_devoption) {
		for (i = 0; opt_devoption[i]; i++) {
			if ((val = strchr(opt_devoption[i], '='))) {
				*val++ = 0;
				for (j = 0; hwcap_options[j].capability; j++) {
					if (!strcmp(hwcap_options[i].shortname,
						    opt_devoption[i])) {
						ret = SIGROK_ERR;
						if (hwcap_options[i].type == T_UINT64) {
							tmp_u64 = strtoull(val, NULL, 10);
							ret =
							    device->plugin->
							    set_configuration
							    (device->
							     plugin_index,
							     hwcap_options[j].
							     capability,
							     &tmp_u64);
						} else if (hwcap_options[i].
							   type == T_CHAR) {
							ret = device->plugin->
							    set_configuration
							    (device->
							     plugin_index,
							     hwcap_options[j].
							     capability, val);
						}

						if (ret != SIGROK_OK) {
							printf("Failed to set device option '%s'.\n", opt_devoption[i]);
							session_destroy();
							return;
						}
					}
				}
			} else {
				printf("Invalid device option '%s'.\n",
				       opt_devoption[i]);
				session_destroy();
				return;
			}
		}
	}

	if (opt_time) {
		time_msec = strtoul(opt_time, &val, 10);
		capabilities = device->plugin->get_capabilities();
		if (find_hwcap(capabilities, HWCAP_LIMIT_MSEC))
			device->plugin->set_configuration(device->plugin_index,
							  HWCAP_LIMIT_MSEC,
							  opt_time);
		else {
			if (val && !strncasecmp(val, "s", 1))
				time_msec *= 1000;
			tmp_u64 = *((uint64_t *) device->plugin->
			      get_device_info(device->plugin_index,
					      DI_CUR_SAMPLERATE));
			limit_samples = tmp_u64 * time_msec / (uint64_t) 1000;
		}
	}

	if (opt_samples) {
		limit_samples = strtoull(opt_samples, NULL, 10);
		device->plugin->set_configuration(device->plugin_index,
					  HWCAP_LIMIT_SAMPLES, opt_samples);
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

	fds = NULL;
	while (!end_acquisition) {
		if (fds)
			free(fds);

		/* Construct g_poll()'s array. */
		fds = malloc(sizeof(GPollFD) * num_sources);
		for (i = 0; i < num_sources; i++) {
			fds[i].fd = sources[i].fd;
			fds[i].events = sources[i].events;
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

	session_stop();
	if (opt_save_session_filename)
		if (session_save(opt_save_session_filename) != SIGROK_OK)
			printf("Failed to save session.\n");
	session_destroy();
}

void logger(const gchar *log_domain, GLogLevelFlags log_level,
	    const gchar *message, gpointer user_data)
{
	/* QUICK FIX */
	log_domain = log_domain;
	user_data = user_data;

	if (log_level & (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_WARNING)) {
		fprintf(stderr, "Warning: %s\n", message);
		fflush(stderr);
	} else {
		if (debug) {
			printf("* %s\n", message);
			fflush(stdout);
		}
	}

}

int main(int argc, char **argv)
{
	int ret, i;
	uint8_t *inbuf = NULL, *outbuf = NULL;
	uint64_t outbuflen = 0;
	GOptionContext *context;
	GError *error;

	g_log_set_default_handler(logger, NULL);
	if (getenv("SIGROK_DEBUG"))
		debug = TRUE;

#if 0
#define BUFLEN 5000
	sigrokdecode_init();

	inbuf = calloc(BUFLEN, 1);
	for (i = 0; i < BUFLEN; i++)	/* Fill array with some values. */
		// inbuf[i] = i % 256;
		inbuf[i] = (uint8_t) (rand() % 256);

	// ret = sigrokdecode_run_decoder("sigrokdecode_count_transitions",
	ret = sigrokdecode_run_decoder("sigrokdecode_i2c",
				       inbuf, BUFLEN, &outbuf, &outbuflen);

	printf("outbuf (%" PRIu64 " bytes):\n%s\n", outbuflen, outbuf);

	sigrokdecode_shutdown();
#endif

	error = NULL;
	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, optargs, NULL);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_warning("%s", error->message);
		return 1;
	}

	if (sigrok_init() != SIGROK_OK)
		return 1;

	if (opt_version)
		show_version();
	else if (opt_list_hwplugins)
		show_hwplugin_list();
	else if (opt_list_devices)
		show_device_list();
	else if (opt_list_analyzers)
		show_analyzer_list();
	else if (opt_samples || opt_time)
		run_session();
	else if (opt_device != -1)
		show_device_detail();
	else
		printf("%s", g_option_context_get_help(context, TRUE, NULL));

	g_option_context_free(context);
	sigrok_cleanup();

	return 0;
}
