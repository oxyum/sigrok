/*
 *   sigrok - sigrok-cli.c
 *
 *   Copyright (C) 2010 Bert Vermeulen <bert@biot.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <glib.h>

#include "config.h"
#include "sigrok.h"
#include "backend.h"
#include "hwplugin.h"
#include "device.h"
#include "session.h"

#include <libusb.h>
#include <sys/time.h>

#define SIGROK_CLI_VERSION "0.1"

GMainContext *gmaincontext = NULL;
GMainLoop *gmainloop = NULL;
struct termios term_orig;
static gboolean opt_version = FALSE;
static gboolean opt_list_hwplugins = FALSE;
static gboolean opt_list_devices = FALSE;
static gboolean opt_list_analyzers = FALSE;
static gchar *session_filename = NULL;
static int opt_device = -1;
static gchar *analyzers = NULL;
static GOptionEntry optargs[] =
{
	{ "version", 'V', 0, G_OPTION_ARG_NONE, &opt_version, "Show version", NULL },
	{ "list-hardware-plugins", 'H', 0, G_OPTION_ARG_NONE, &opt_list_hwplugins, "List hardware plugins", NULL },
	{ "list-devices", 'D', 0, G_OPTION_ARG_NONE, &opt_list_devices, "List devices", NULL },
	{ "list-analyzer-plugins", 'A', 0, G_OPTION_ARG_NONE, &opt_list_analyzers, "List analyzer plugins", NULL },

	{ "session-file", 'F', 0, G_OPTION_ARG_FILENAME, &session_filename, "Load session from file", NULL },
	{ "device", 'd', 0, G_OPTION_ARG_INT, &opt_device, "Use device id", NULL },
	{ "analyzers", 'a', 0, G_OPTION_ARG_STRING, &analyzers, "Protocol analyzer sequence", NULL },
	{ NULL }

};

void show_version(void)
{

	printf("Sigrok version %s\nCLI version %s\n", VERSION, SIGROK_CLI_VERSION);

}


void show_hwplugin_list(void)
{
	GSList *plugins, *p;
	struct device_plugin *plugin;

	printf("Plugins for the following devices are installed:\n");
	plugins = list_hwplugins();
	for(p = plugins; p; p = p->next)
	{
		plugin = p->data;
		printf(" %s\n", plugin->name);
	}

}


void show_device_list()
{
	struct device *device;
	struct probe *probe;
	GSList *devices, *l, *p;
	int devcnt;
	char *idstring;

	devcnt = 0;
	device_scan();
	devices = device_list();
	if(g_slist_length(devices) > 0)
	{
		printf("The following devices were found:\nID  Device\n");
		for(l = devices; l; l = l->next)
		{
			device = l->data;
			idstring = device->plugin->get_device_info(device->plugin_index, DI_IDENTIFIER);
			printf("%-3d %s %s\n", devcnt, device->plugin->name, idstring);
			g_free(idstring);
			if(device->probes)
			{
				printf("  %d probes:\n", g_slist_length(device->probes));
				for(p = device->probes; p; p = p->next)
				{
					probe = (struct probe *) p->data;
					printf("    %2d '%s'", probe->index, probe->name);
					if(probe->trigger)
						printf(" trigger %s", probe->trigger);
					printf("\n");
				}
			}
		}
	}

}


void show_analyzer_list()
{

	/* TODO: implement */

}


void datafeed_callback(struct device *device, struct datafeed_packet *packet)
{
	struct datafeed_header *header;

	if(packet->type == DF_HEADER)
	{
		header = packet->payload;
		printf("Acquisition with %d probes at %.3f Mhz starting at %s",
				header->num_probes, header->rate, ctime(&header->starttime.tv_sec));
	}
	else if(packet->type == DF_LOGIC8)
	{
		printf("received %d samples\n", packet->length);
	}

}


gboolean stdin_prepare(GSource *source, int *timeout)
{

	*timeout = -1;

	return FALSE;
}


gboolean stdin_check(GSource *source)
{
	struct pollfd pfd;

	pfd.fd = STDIN_FILENO;
	pfd.events = POLLIN;
	if(poll(&pfd, 1, 0))
		return TRUE;

	return FALSE;
}


gboolean stdin_dispatch(GSource *source, GSourceFunc callback, gpointer data)
{

	tcflush(STDIN_FILENO, TCIFLUSH);
	tcsetattr(STDIN_FILENO, TCSANOW, &term_orig);
	g_main_loop_quit(gmainloop);

	return TRUE;
}


GSourceFuncs stdin_funcs = {
	stdin_prepare,
	stdin_check,
	stdin_dispatch,
	NULL
};


void run_session(void)
{
	struct device *device;
	struct termios term;
	GSource *stdin_source;
	GPollFD stdin_pfd;
	GSList *devices;
	float rate;
	int num_devices, ret;

	device_scan();
	devices = device_list();
	num_devices = g_slist_length(devices);
	if(num_devices == 0)
	{
		g_warning("No devices found.");
		return;
	}
	if(opt_device == -1)
	{
		if(num_devices == 1)
			/* no device specified, but there is only one */
			opt_device = 0;
		else
		{
			g_warning("%d devices found, please select one.", num_devices);
			return;
		}
	}
	else
	{
		if(opt_device >= num_devices)
		{
			g_warning("Device not found.");
			return;
		}
	}

	session_new();
	device = g_slist_nth_data(devices, opt_device);

	if(session_device_add(device) == SIGROK_OK)
	{
		session_output_add_callback(datafeed_callback);

		rate = 4;
		ret = device->plugin->set_configuration(device->plugin_index, HWCAP_SAMPLERATE, &rate);
		if(ret == SIGROK_OK)
		{
//			gmaincontext = g_main_context_new();
			if(session_start() == SIGROK_OK)
			{
				/* monitor stdin along with the device's I/O */
				tcgetattr(STDIN_FILENO, &term);
				memcpy(&term_orig, &term, sizeof(struct termios));
				term.c_lflag &= ~(ECHO | ICANON | ISIG);
				tcsetattr(STDIN_FILENO, TCSADRAIN, &term);

				stdin_source = g_source_new(&stdin_funcs, sizeof(GSource));
				stdin_pfd.fd = STDIN_FILENO;
				stdin_pfd.events =  G_IO_IN | G_IO_HUP | G_IO_ERR;
				g_main_context_add_poll(gmaincontext, &stdin_pfd, G_PRIORITY_DEFAULT);
				g_source_add_poll(stdin_source, &stdin_pfd);
				ret = g_source_attach(stdin_source, gmaincontext);

				printf("press any key to stop\n");

				gmainloop = g_main_loop_new(gmaincontext, FALSE);
				g_main_loop_run(gmainloop);
				session_stop();
			}
		}
	}

	session_destroy();

}


void logger(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{

	if(log_level & (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_WARNING))
	{
		fprintf(stderr, "Warning: %s\n", message);
		fflush(stderr);
	}
	else
	{
		printf("* %s\n", message);
		fflush(stdout);
	}

}


int main(int argc, char **argv)
{
	GOptionContext *context;
	GError *error;
	int cli_status;

	cli_status = 0;
	error = NULL;
	g_log_set_default_handler(logger, NULL);

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, optargs, NULL);
	if(!g_option_context_parse(context, &argc, &argv, &error))
	{
		g_warning("invalid option: %s", error->message);
		exit(1);
	}

	sigrok_init();

	if(opt_version)
		show_version();
	else if(opt_list_hwplugins)
		show_hwplugin_list();
	else if(opt_list_devices)
		show_device_list();
	else if(opt_list_analyzers)
		show_analyzer_list();
	else
		run_session();

	sigrok_cleanup();

	return cli_status;
}


