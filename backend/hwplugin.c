/*
 *   sigrok - hwplugin.c
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#include <glib.h>
#include <gmodule.h>

#include "sigrok.h"
#include "hwplugin.h"


GSList *plugins;

/* this enumerates which plugin capabilities correspond to user-settable options */
struct hwcap_option hwcap_options[] = {
	{ HWCAP_SAMPLERATE, "Sample rate", "samplerate" },
	{ 0, NULL, NULL }
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
			module_path = g_malloc(strlen(HWPLUGIN_DIR) + strlen(de->d_name) + 1);
			sprintf(module_path, "%s/%s", HWPLUGIN_DIR, de->d_name);
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


struct usb_device_instance *usb_device_instance_new(int index, int status, uint8_t bus,
		uint8_t address, struct libusb_device_handle *hdl)
{
	struct usb_device_instance *udi;

	udi = g_malloc(sizeof(struct usb_device_instance));
	udi->index = index;
	udi->status = status;
	udi->bus = bus;
	udi->address = address;
	udi->devhdl = hdl;

	return udi;
}


struct usb_device_instance *get_usb_device_instance(GSList *usb_devices, int device_index)
{
	struct usb_device_instance *udi;
	GSList *l;

	udi = NULL;
	for(l = usb_devices; l; l = l->next)
	{
		udi = (struct usb_device_instance *) (l->data);
		if(udi->index == device_index)
			return udi;
	}
	g_warning("could not find device index %d instance", device_index);

	return NULL;
}


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




