/*
 *   sigrok - hwplugin.h
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

#ifndef SIGROK_HWPLUGIN_H
#define SIGROK_HWPLUGIN_H

#include <stdint.h>
#include <glib.h>
#include "device.h"

/* hardware plugin capabilities */
enum {
	HWCAP_DUMMY,
	HWCAP_LOGIC_ANALYZER,
	HWCAP_SAMPLERATE,
	HWCAP_PROBECONFIG,
	HWCAP_CAPTURE_RATIO,
	HWCAP_LIMIT_SECONDS,
	HWCAP_LIMIT_SAMPLES
};

struct hwcap_option {
	int capability;
	char *description;
	char *shortname;
};

struct usb_device_instance {
	int index;
	int status;
	uint8_t bus;
	uint8_t address;
	struct libusb_device_handle *devhdl;
};

struct serial_device_instance {
	int index;
	int status;
	char *model;
	char *port;
	int fd;
};

/* device instance status */
enum {
	ST_NOT_FOUND,
	/* found, but still booting */
	ST_INITIALIZING,
	/* live, but not in use */
	ST_INACTIVE,
	/* actively in use in a session */
	ST_ACTIVE
};

/* TODO: this sucks, you just kinda have to "know" the returned type */
/* TODO: need a DI to return the number of trigger stages supported */
/* device info IDs */
enum {
	/* string identifying this specific device in the system */
	DI_IDENTIFIER,
	/* the number of probes connected to this device */
	DI_NUM_PROBES,
	/* the samples rates this device supports, as a 0-terminated array of float */
	DI_SAMPLE_RATES,
	/* types of trigger supported, out of "01crf" */
	DI_TRIGGER_TYPES,
};

struct device_plugin {
	/* plugin-specific */
	char *name;
	int api_version;
	int (*init) (char *deviceinfo);
	void (*cleanup) (void);

	/* device-specific */
	int (*open) (int device_index);
	void (*close) (int device_index);
	char *(*get_device_info) (int device_index, int device_info_id);
	int (*get_status) (int device_index);
	int *(*get_capabilities) (void);
	int (*set_configuration) (int device_index, int capability, char *value);
	int (*start_acquisition) (int device_index, gpointer session_device_id);
	void (*stop_acquisition) (int device_index, gpointer session_device_id);
};

struct gsource_fd {
	GSource source;
	GPollFD gpfd;
	/* not really using this */
	GSource *timeout_source;
};

typedef int (*receive_data_callback) (GSource *source, gpointer data);

int load_hwplugins(void);
GSList *list_hwplugins(void);
struct usb_device_instance *usb_device_instance_new(int index, int status, uint8_t bus,
		uint8_t address, struct libusb_device_handle *hdl);
struct usb_device_instance *get_usb_device_instance(GSList *usb_devices, int device_index);
struct serial_device_instance *get_serial_device_instance(GSList *serial_devices, int device_index);
struct hwcap_option *find_hwcap_option(int hwcap);
void add_source_fd(int fd, int events, receive_data_callback callback, gpointer user_data);

#endif
