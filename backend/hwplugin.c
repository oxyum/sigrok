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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <glib.h>
#include <gmodule.h>
#include "sigrok.h"

extern GMainContext *gmaincontext;

GSList *plugins;

/* this enumerates which plugin capabilities correspond to user-settable options */
struct hwcap_option hwcap_options[] = {
	{ HWCAP_SAMPLERATE, T_UINT64, "Sample rate", "samplerate" },
	{ 0, 0, NULL, NULL }
};


int load_hwplugins(void)
{
	struct device_plugin *plugin;
	GModule *module;
	DIR *plugindir;
	struct dirent *de;
	int l;
	gchar *module_path;

	if(!g_module_supported())
		return SIGROK_NOK;

	plugindir = opendir(HWPLUGIN_DIR);
	if(plugindir == NULL)
	{
		g_warning("Hardware plugin directory %s not found.", HWPLUGIN_DIR);
		return SIGROK_NOK;
	}

	while( (de = readdir(plugindir)) )
	{
		l = strlen(de->d_name);
		if(l > 3 && !strncmp(de->d_name + l - 3, ".la", 3))
		{
			/* it's a libtool archive */
			l = strlen(HWPLUGIN_DIR) + strlen(de->d_name) + 2;
			module_path = g_malloc(l);
			snprintf(module_path, l, "%s/%s", HWPLUGIN_DIR, de->d_name);
			g_debug("loading %s", module_path);
			module = g_module_open(module_path, G_MODULE_BIND_LOCAL);
			if(module)
			{
				if(g_module_symbol(module, "plugin_info", (gpointer *) &plugin))
					plugins = g_slist_append(plugins, plugin);
				else
				{
					g_warning("failed to find plugin info: %s", g_module_error());
					g_module_close(module);
				}
			}
			else
			{
				g_warning("failed to load module: %s", g_module_error());
			}
			g_free(module_path);
		}
	}

	return SIGROK_OK;
}


GSList *list_hwplugins(void)
{

	return plugins;
}


struct sigrok_device_instance *sigrok_device_instance_new(int index, int status,
		char *vendor, char *model, char *version)
{
	struct sigrok_device_instance *sdi;

	sdi = g_malloc(sizeof(struct sigrok_device_instance));
	if(!sdi)
		return NULL;

	sdi->index = index;
	sdi->status = status;
	sdi->instance_type = -1;
	sdi->vendor = strdup(vendor);
	sdi->model = strdup(model);
	sdi->version = strdup(version);
	sdi->usb = NULL;

	return sdi;
}


struct sigrok_device_instance *get_sigrok_device_instance(GSList *device_instances, int device_index)
{
	struct sigrok_device_instance *sdi;
	GSList *l;

	sdi = NULL;
	for(l = device_instances; l; l = l->next) {
		sdi = (struct sigrok_device_instance *) (l->data);
		if(sdi->index == device_index)
			return sdi;
	}
	g_warning("could not find device index %d instance", device_index);

	return NULL;
}


void sigrok_device_instance_free(struct sigrok_device_instance *sdi)
{

	switch(sdi->instance_type) {
	case USB_INSTANCE:
		usb_device_instance_free(sdi->usb);
		break;
	case SERIAL_INSTANCE:
		serial_device_instance_free(sdi->serial);
		break;
		/* no specific type, nothing extra to free */
	}

	free(sdi->vendor);
	free(sdi->model);
	free(sdi->version);
	free(sdi);

}


struct usb_device_instance *usb_device_instance_new(uint8_t bus, uint8_t address,
		struct libusb_device_handle *hdl)
{
	struct usb_device_instance *udi;

	udi = g_malloc(sizeof(struct usb_device_instance));
	if(!udi)
		return NULL;

	udi->bus = bus;
	udi->address = address;
	udi->devhdl = hdl;

	return udi;
}


void usb_device_instance_free(struct usb_device_instance *usb)
{

	/* nothing to do for this device instance type */

}


// obsolete
struct serial_device_instance *get_serial_device_instance(GSList *serial_devices, int device_index)
{
	struct serial_device_instance *sdi;
	GSList *l;

	sdi = NULL;
	for(l = serial_devices; l; l = l->next)
	{
		sdi = (struct serial_device_instance *) (l->data);
		if(sdi->index == device_index)
			return sdi;
	}
	g_warning("could not find device index %d instance", device_index);

	return NULL;
}


void serial_device_instance_free(struct serial_device_instance *serial)
{

	free(serial->port);

}


int find_hwcap(int *capabilities, int hwcap)
{
	int i;

	for(i = 0; capabilities[i]; i++)
		if(capabilities[i] == hwcap)
			return TRUE;

	return FALSE;
}


struct hwcap_option *find_hwcap_option(int hwcap)
{
	struct hwcap_option *hwo;
	int i;

	hwo = NULL;
	for(i = 0; hwcap_options[i].capability; i++)
	{
		if(hwcap_options[i].capability == hwcap)
		{
			hwo = &hwcap_options[i];
			break;
		}
	}

	return hwo;
}


gboolean gsource_fd_prepare(GSource *source, int *timeout)
{

	return FALSE;
}


gboolean gsource_fd_check(GSource *source)
{
	struct gsource_fd *sfd;

	sfd = (struct gsource_fd *) source;
	if(sfd->gpfd.revents)
		return TRUE;

	return FALSE;
}


gboolean gsource_fd_dispatch(GSource *source, GSourceFunc callback, gpointer data)
{
	int ret;

	ret = ( (receive_data_callback) callback)(source, data);

	return ret;
}


GSourceFuncs gsource_fd_funcs = {
	gsource_fd_prepare,
	gsource_fd_check,
	gsource_fd_dispatch,
	NULL
};


void add_source_fd(int fd, int events, receive_data_callback callback, gpointer user_data)
{
	struct gsource_fd *source;

	source = (struct gsource_fd *) g_source_new(&gsource_fd_funcs, sizeof(struct gsource_fd));
	g_source_set_callback((GSource *) source, (GSourceFunc) callback, user_data, NULL);
	source->gpfd.fd = fd;
	source->gpfd.events = events;
	g_source_add_poll((GSource *) source, &(source->gpfd));
	g_source_attach((GSource *) source, gmaincontext);

}



