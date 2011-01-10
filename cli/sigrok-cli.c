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


#define SIGROK_CLI_VERSION "0.1pre2"
#define DEFAULT_OUTPUT_FORMAT "bits64"

extern struct hwcap_option hwcap_options[];

gboolean debug = 0;
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
static gboolean opt_list_hwdrivers = FALSE;
static gboolean opt_list_devices = FALSE;
static gboolean opt_list_analyzers = FALSE;
static gboolean opt_list_output = FALSE;
static gboolean opt_wait_trigger = FALSE;
static gchar *opt_load_filename = NULL;
static gchar *opt_save_filename = NULL;
static gchar *opt_device = NULL;
static gchar *opt_probes = NULL;
static gchar *opt_triggers = NULL;
static gchar **opt_devoption = NULL;
static gchar *opt_pds = NULL;
static gchar *opt_format = NULL;
static gchar *opt_time = NULL;
static gchar *opt_samples = NULL;

static GOptionEntry optargs[] = {
	{"version", 'V', 0, G_OPTION_ARG_NONE, &opt_version, "Show version", NULL},
	{"list-hardware-drivers", 'H', 0, G_OPTION_ARG_NONE, &opt_list_hwdrivers, "List hardware drivers", NULL},
	{"list-devices", 'D', 0, G_OPTION_ARG_NONE, &opt_list_devices, "List devices", NULL},
	{"list-analyzer-plugins", 'A', 0, G_OPTION_ARG_NONE, &opt_list_analyzers, "List analyzer plugins", NULL},
	{"list-output-modules", 0, 0, G_OPTION_ARG_NONE, &opt_list_output, "list output modules", NULL},
	{"load-file", 'L', 0, G_OPTION_ARG_FILENAME, &opt_load_filename, "Load session from file", NULL},
	{"save-file", 'S', 0, G_OPTION_ARG_FILENAME, &opt_save_filename, "Save session to file", NULL},
	{"device", 'd', 0, G_OPTION_ARG_STRING, &opt_device, "Use device id", NULL},
	{"probes", 'p', 0, G_OPTION_ARG_STRING, &opt_probes, "Probes to use", NULL},
	{"triggers", 't', 0, G_OPTION_ARG_STRING, &opt_triggers, "Trigger configuration", NULL},
	{"wait-trigger", 'w', 0, G_OPTION_ARG_NONE, &opt_wait_trigger, "Wait for trigger", NULL},
	{"device-option", 'o', 0, G_OPTION_ARG_STRING_ARRAY, &opt_devoption, "Device-specific option", NULL},
	{"analyzers", 'a', 0, G_OPTION_ARG_STRING, &opt_pds, "Protocol analyzer sequence", NULL},
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

void show_hwdriver_list(void)
{
	GSList *plugins, *p;
	struct device_plugin *plugin;

	printf("The following drivers are installed:\n");
	plugins = list_hwplugins();
	for (p = plugins; p; p = p->next) {
		plugin = p->data;
		printf(" %s\n", plugin->name);
	}
}

void print_device_line(struct device *device)
{
	struct sigrok_device_instance *sdi;

	sdi = device->plugin->get_device_info(device->plugin_index, DI_INSTANCE);
	printf("%s %s", sdi->vendor, sdi->model);
	if (sdi->version && sdi->version[0])
		printf(" %s", sdi->version);
	if (device->probes)
		printf(" with %d probes", g_slist_length(device->probes));
	printf("\n");
}

void show_device_list(void)
{
	struct device *device, *demo_device;
	GSList *devices, *l;
	int devcnt;

	devcnt = 0;
	device_scan();
	devices = device_list();

	if (g_slist_length(devices) == 0)
		return;

	printf("The following devices were found:\nID  Device\n");
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

void show_device_detail(void)
{
	struct device *device;
	struct hwcap_option *hwo;
	struct samplerates *samplerates;
	int cap, *capabilities, i;
	char *title, *triggers;

	device_scan();
	device = parse_devicestring(opt_device);
	if (!device) {
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
		if (!(hwo = find_hwcap_option(capabilities[cap])))
			continue;

		if (title) {
			printf("%s", title);
			title = NULL;
		}

		if (hwo->capability == HWCAP_SAMPLERATE) {
			printf("    %s", hwo->shortname);
			/* Supported samplerates */
			samplerates = device->plugin->get_device_info(
				device->plugin_index, DI_SAMPLERATES);
			if (!samplerates) {
				printf("\n");
				break;
			}

			/* TODO: Add missing free()s. */
			if (samplerates->step) {
				printf(" (%s - %s in steps of %s)\n",
				  sigrok_samplerate_string(samplerates->low),
				  sigrok_samplerate_string(samplerates->high),
				  sigrok_samplerate_string(samplerates->step));
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

void show_analyzer_list(void)
{
	/* TODO: Implement. */
}

void show_output_list(void)
{
	struct output_format **outputs;
	int i;

	printf("Supported output formats:\n");
	outputs = output_list();
	for (i = 0; outputs[i]; i++) {
		printf("%-12s %s\n", outputs[i]->extension, outputs[i]->description);
	}

}

void datafeed_in(struct device *device, struct datafeed_packet *packet)
{
	static struct output *o = NULL;
	static int probelist[65] = { 0 };
	static uint64_t received_samples = 0;
	static int unitsize = 0;
	static int triggered = 0;
	struct probe *probe;
	struct datafeed_header *header;
	int num_enabled_probes, sample_size, ret, i;
	uint64_t output_len, filter_out_len, len;
	char *output_buf, *filter_out;

	/* If the first packet to come in isn't a header, don't even try. */
	if (packet->type != DF_HEADER && o == NULL)
		return;

	sample_size = -1;

	switch (packet->type) {
	case DF_HEADER:
		/* initialize the output module. */
		if (!(o = malloc(sizeof(struct output))))
			return; /* FIXME: How to report errors? */
		o->format = output_format;
		o->device = device;
		o->param = output_format_param;

		/* TODO: Error handling. */
		if (o->format->init) {
			if ((ret = o->format->init(o)) != SIGROK_OK) {
				g_error("Output format init failed.");
				// return ret;
				return; /* FIXME */
			}
		}

		header = (struct datafeed_header *)packet->payload;
		num_enabled_probes = 0;
		for (i = 0; i < header->num_logic_probes; i++) {
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
		if (opt_save_filename) {
			ret = datastore_new(unitsize, &(device->datastore));
			if (ret != SIGROK_OK) {
				g_error("Couldn't create datastore.");
				/* TODO: free()? */
				// return SIGROK_ERR_MALLOC;
				return; /* FIXME */
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
				fwrite(output_buf, 1, output_len, stdout);
				free(output_buf);
				output_len = 0;
			}
		}
		if (limit_samples && received_samples < limit_samples)
			printf("Device only sent %" PRIu64 " samples.\n",
			       received_samples);
		end_acquisition = TRUE;
		free(o);
		o = NULL;
		break;
	case DF_TRIGGER:
		o->format->event(o, DF_TRIGGER, 0, 0);
		triggered = 1;
		break;
	case DF_LOGIC:
	case DF_ANALOG:
		sample_size = packet->unitsize;
		break;
	}

	if (sample_size == -1)
		return;

	/* Don't store any samples until triggered. */
	if (opt_wait_trigger && !triggered)
		return;

	if (received_samples >= limit_samples)
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
		filter_out_len = packet->length;
	}

	if (device->datastore)
		datastore_put(device->datastore, filter_out,
			      filter_out_len, sample_size, probelist);

	/* Don't dump samples on stdout when also saving the session. */
	output_len = 0;
	if (!opt_save_filename) {
		if (o->format->data && packet->type == o->format->df_type) {
			if (received_samples + packet->length / sample_size > limit_samples * sample_size)
				len = limit_samples * sample_size - received_samples;
			else
				len = filter_out_len;
			o->format->data(o, filter_out, len, &output_buf, &output_len);
		}
		if (output_len)
			fwrite(output_buf, 1, output_len, stdout);
	}
	free(filter_out);
	if (output_len)
		free(output_buf);
	received_samples += packet->length / sample_size;

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

/* Register the given PDs for this session. */
/* TODO: Support both serial PDs and nested PDs. Parallel PDs even? */
/* TODO: Only register here, run in streaming fashion later/elsewhere. */
static int register_pds(struct device *device, const char *pdstring)
{
	int i, ret;
	char **tokens;
	uint8_t *inbuf = NULL, *outbuf = NULL;
	uint64_t outbuflen = 0;
	struct sigrokdecode_decoder *dec;

	/* Avoid compiler warnings. */
	device = device;

	/* FIXME: Just for testing... */
#define BUFLEN 50
	inbuf = calloc(BUFLEN, 1);
	for (i = 0; i < BUFLEN; i++)	/* Fill array with some values. */
		inbuf[i] = (uint8_t) (rand() % 256);

	/* TODO: Error handling. */
	tokens = g_strsplit(pdstring, ",", 10 /* FIXME */);

	/* Run specified PDs serially on the data, in the given order. */
	for (i = 0; tokens[i]; i++) {
		printf("Running protocol decoder %d (%s).\n", i, tokens[i]);
		sigrokdecode_init();
		ret = sigrokdecode_load_decoder(tokens[i], &dec);
		ret = sigrokdecode_run_decoder(dec, inbuf, BUFLEN, &outbuf,
					       &outbuflen);
		printf("outbuf (%" PRIu64 " bytes):\n%s\n", outbuflen, outbuf);
		sigrokdecode_shutdown();
	}

	return 0;
}

void load_file(void)
{
	struct stat st;
	struct input_format **inputs, *input_format;
	int i;

	inputs = input_list();
	for (i = 0; inputs[i]; i++) {
		if (inputs[i]->format_match(opt_load_filename))
			break;
	}

	if (!inputs[i])
		/* no input module wanted to touch this */
		return;
	input_format = inputs[i];

	if (stat(opt_load_filename, &st) == -1) {
		printf("unable to load %s: %s", opt_load_filename, strerror(errno));
		return;
	}
	limit_samples = st.st_size;

	session_new();
	session_datafeed_callback_add(datafeed_in);
	input_format->in_loadfile(opt_load_filename);
	session_stop();

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

void run_session(void)
{
	struct device *device;
	struct probe *probe;
	GPollFD *fds;
	int num_devices, max_probes, *capabilities, ret, found, i, j;
	unsigned int time_msec;
	uint64_t tmp_u64;
	char **probelist, *val;

	/* FIXME: Wrong location, just for testing... */
	if (opt_pds)
		register_pds(NULL, opt_pds);

	device_scan();
	num_devices = num_real_devices();

	if (num_devices == 0) {
		g_warning("No devices found.");
		return;
	}

	if (!opt_device) {
		if (num_devices == 1)
			/* No device specified, but there is only one. */
			device = parse_devicestring("0");
		else {
			g_warning("%d devices found, please select one.", num_devices);
			return;
		}
	} else {
		device = parse_devicestring(opt_device);
		if (!device) {
			g_warning("Device not found.");
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

	if (opt_devoption) {
		for (i = 0; opt_devoption[i]; i++) {
			if (!(val = strchr(opt_devoption[i], '='))) {
				printf("No value given for device option '%s'.\n", opt_devoption[i]);
				session_destroy();
				return;
			}

			found = FALSE;
			*val++ = 0;
			for (j = 0; hwcap_options[j].capability; j++) {
				if (!strcmp(hwcap_options[j].shortname, opt_devoption[i])) {
					found = TRUE;
					switch (hwcap_options[j].type) {
					case T_UINT64:
						tmp_u64 = parse_sizestring(val);
						ret = device->plugin-> set_configuration(device-> plugin_index,
								hwcap_options[j]. capability, &tmp_u64);
						break;
					case T_CHAR:
						ret = device->plugin-> set_configuration(device-> plugin_index,
								hwcap_options[j]. capability, val);
						break;
					default:
						ret = SIGROK_ERR;
					}

					if (ret != SIGROK_OK) {
						printf("Failed to set device option '%s'.\n", opt_devoption[i]);
						session_destroy();
						return;
					}
					else
						break;
				}
			}
			if(!found)
			{
				printf("Unknown device option '%s'.\n", opt_devoption[i]);
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
		device->plugin->set_configuration(device->plugin_index,
					  HWCAP_LIMIT_SAMPLES, &limit_samples);
	}

	if (opt_samples) {
		limit_samples = parse_sizestring(opt_samples);
		device->plugin->set_configuration(device->plugin_index,
					  HWCAP_LIMIT_SAMPLES, &limit_samples);
	}

	if (opt_pds)
		register_pds(NULL, opt_pds);

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
	if (opt_save_filename)
		if (session_save(opt_save_filename) != SIGROK_OK)
			printf("Failed to save session.\n");
	session_destroy();
}

void logger(const gchar *log_domain, GLogLevelFlags log_level,
	    const gchar *message, gpointer user_data)
{
	/* QUICK FIX */
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
	int i;
	GOptionContext *context;
	GError *error;
#if 0
	int ret;
	uint8_t *inbuf = NULL, *outbuf = NULL;
	uint64_t outbuflen = 0;
	struct sigrokdecode_decoder *dec;
#endif

	g_log_set_default_handler(logger, NULL);
	if (getenv("SIGROK_DEBUG"))
		debug = strtol(getenv("SIGROK_DEBUG"), NULL, 10);

#if 0
#define BUFLEN 50
	sigrokdecode_init();

	inbuf = calloc(BUFLEN, 1);
	for (i = 0; i < BUFLEN; i++)	/* Fill array with some values. */
		// inbuf[i] = i % 256;
		inbuf[i] = (uint8_t) (rand() % 256);

	ret = sigrokdecode_load_decoder("i2c", &dec);
	ret = sigrokdecode_run_decoder(dec, inbuf, BUFLEN, &outbuf, &outbuflen);
	printf("outbuf (%" PRIu64 " bytes):\n%s\n", outbuflen, outbuf);

	ret = sigrokdecode_load_decoder("transitioncounter", &dec);
	ret = sigrokdecode_run_decoder(dec, inbuf, BUFLEN, &outbuf, &outbuflen);
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

	if (!opt_format)
		opt_format = DEFAULT_OUTPUT_FORMAT;
	outputs = output_list();
	for (i = 0; outputs[i]; i++) {
		if (!strncasecmp(outputs[i]->extension, opt_format,
		     strlen(outputs[i]->extension))) {
			output_format = outputs[i];
			output_format_param =
			    opt_format + strlen(outputs[i]->extension);
			break;
		}
	}
	if (!output_format) {
		printf("invalid output format %s\n", opt_format);
		return 1;
	}

	if (opt_version)
		show_version();
	else if (opt_list_hwdrivers)
		show_hwdriver_list();
	else if (opt_list_devices)
		show_device_list();
	else if (opt_list_analyzers)
		show_analyzer_list();
	else if (opt_list_output)
		show_output_list();
	else if (opt_load_filename)
		load_file();
	else if (opt_samples || opt_time)
		run_session();
	else if (opt_device)
		show_device_detail();
	else
		printf("%s", g_option_context_get_help(context, TRUE, NULL));

	g_option_context_free(context);
	sigrok_cleanup();

	return 0;
}
