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

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/time.h>
#include <inttypes.h>
#include <glib.h>
#include <libusb.h>
#include "config.h"
#include "sigrok.h"

#define USB_VENDOR				0x0c12
#define USB_PRODUCT			0x700e
#define USB_VENDOR_NAME		"Zeroplus"
#define USB_MODEL_NAME			"Logic Cube"
#define USB_MODEL_VERSION		""

#define USB_INTERFACE			0
#define USB_CONFIGURATION		1
#define NUM_PROBES				16
#define NUM_TRIGGER_STAGES		4
#define TRIGGER_TYPES			"01"

/* delay in ms */
#define NUM_SIMUL_TRANSFERS	10
#define MAX_EMPTY_TRANSFERS	NUM_SIMUL_TRANSFERS * 2


extern GMainContext *gmaincontext;


/* there is only one model Saleae Logic, and this is what it supports */
int capabilities[] = {
	HWCAP_LOGIC_ANALYZER,
	HWCAP_SAMPLERATE,

	/* these are really implemented in the driver, not the hardware */
	HWCAP_LIMIT_SAMPLES,
	0
};

/* list of struct sigrok_device_instance, maintained by opendev() and closedev() */
GSList *device_instances = NULL;

libusb_context *usb_context = NULL;

struct samplerates samplerates = {
	100,
	MHZ(100),
	1,
	NULL
};

/* TODO: all of these should go in a device-specific struct */
uint64_t cur_samplerate = 0;
uint64_t limit_samples = 0;
uint8_t probe_mask = 0, \
		trigger_mask[NUM_TRIGGER_STAGES] = {0}, \
		trigger_value[NUM_TRIGGER_STAGES] = {0}, \
		trigger_buffer[NUM_TRIGGER_STAGES] = {0};;


int hw_set_configuration(int device_index, int capability, void *value);


struct sigrok_device_instance *zp_open_device(int device_index)
{
	struct sigrok_device_instance *sdi;
	libusb_device **devlist;
	struct libusb_device_descriptor des;
	int err, i;

	if(!(sdi = get_sigrok_device_instance(device_instances, device_index)))
		return NULL;

	libusb_get_device_list(usb_context, &devlist);
	if(sdi->status == ST_INACTIVE) {
		/* find the device by vendor, product, bus and address */
		libusb_get_device_list(usb_context, &devlist);
		for(i = 0; devlist[i]; i++) {
			if( (err = libusb_get_device_descriptor(devlist[i], &des)) ) {
				g_warning("failed to get device descriptor: %d", err);
				continue;
			}

			if(des.idVendor == USB_VENDOR && des.idProduct == USB_PRODUCT) {
				if(libusb_get_bus_number(devlist[i]) == sdi->usb->bus &&
						libusb_get_device_address(devlist[i]) == sdi->usb->address) {
					/* found it */
					if( !(err = libusb_open(devlist[i], &(sdi->usb->devhdl))) ) {
						sdi->status = ST_ACTIVE;
						g_message("opened device %d on %d.%d interface %d", sdi->index,
								sdi->usb->bus, sdi->usb->address, USB_INTERFACE);
					}
					else {
						g_warning("failed to open device: %d", err);
						sdi = NULL;
					}
				}
			}
		}
	}
	else {
		/* status must be ST_ACTIVE, i.e. already in use... */
		sdi = NULL;
	}
	libusb_free_device_list(devlist, 1);

	if(sdi && sdi->status != ST_ACTIVE)
		sdi = NULL;

	return sdi;
}


void close_device(struct sigrok_device_instance *sdi)
{

	if(sdi->usb->devhdl)
	{
		g_message("closing device %d on %d.%d interface %d", sdi->index, sdi->usb->bus,
				sdi->usb->address, USB_INTERFACE);
		libusb_release_interface(sdi->usb->devhdl, USB_INTERFACE);
		libusb_close(sdi->usb->devhdl);
		sdi->usb->devhdl = NULL;
		sdi->status = ST_INACTIVE;
	}

}


int configure_probes(GSList *probes)
{
	struct probe *probe;
	GSList *l;
	int probe_bit, stage, i;
	char *tc;

	probe_mask = 0;
	for(i = 0; i < NUM_TRIGGER_STAGES; i++)
	{
		trigger_mask[i] = 0;
		trigger_value[i] = 0;
	}

	stage = -1;
	for(l = probes; l; l = l->next)
	{
		probe = (struct probe *) l->data;
		if(probe->enabled == FALSE)
			continue;
		probe_bit = 1 << (probe->index - 1);
		probe_mask |= probe_bit;
		if(probe->trigger)
		{
			stage = 0;
			for(tc = probe->trigger; *tc; tc++)
			{
				trigger_mask[stage] |= probe_bit;
				if(*tc == '1')
					trigger_value[stage] |= probe_bit;
				stage++;
				if(stage > NUM_TRIGGER_STAGES)
					return SIGROK_NOK;
			}
		}
	}

	return SIGROK_OK;
}



/*
 * API callbacks
 */

int hw_init(char *deviceinfo)
{
	struct sigrok_device_instance *sdi;
	struct libusb_device_descriptor des;
	libusb_device **devlist;
	int err, devcnt, i;

	if(libusb_init(&usb_context) != 0) {
		g_warning("Failed to initialize USB.");
		return 0;
	}

	/* find all Saleae Logic devices and upload firmware to all of them */
	devcnt = 0;
	libusb_get_device_list(usb_context, &devlist);
	for(i = 0; devlist[i]; i++) {
		err = libusb_get_device_descriptor(devlist[i], &des);
		if(err != 0) {
			g_warning("failed to get device descriptor: %d", err);
			continue;
		}

		if(des.idVendor == USB_VENDOR && des.idProduct == USB_PRODUCT) {
			/* definitely a Zeroplus */
			/* TODO: any way to detect specific model/version in the zeroplus range? */
			sdi = sigrok_device_instance_new(devcnt, ST_INACTIVE,
					USB_VENDOR_NAME, USB_MODEL_NAME, USB_MODEL_VERSION);
			if(!sdi)
				return 0;
			device_instances = g_slist_append(device_instances, sdi);
			sdi->usb = usb_device_instance_new(libusb_get_bus_number(devlist[i]),
					libusb_get_device_address(devlist[i]), NULL);
			devcnt++;
		}
	}
	libusb_free_device_list(devlist, 1);

	return devcnt;
}


int hw_opendev(int device_index)
{
	struct sigrok_device_instance *sdi;
	int err;

	if( !(sdi = zp_open_device(device_index)) ) {
		g_warning("unable to open device");
		return SIGROK_NOK;
	}

	err = libusb_claim_interface(sdi->usb->devhdl, USB_INTERFACE);
	if(err != 0) {
		g_warning("Unable to claim interface: %d", err);
		return SIGROK_NOK;
	}

	if(cur_samplerate == 0) {
		/* sample rate hasn't been set; default to the slowest it has */
		if(hw_set_configuration(device_index, HWCAP_SAMPLERATE, &samplerates.low) == SIGROK_NOK)
			return SIGROK_NOK;
	}

	return SIGROK_OK;
}


void hw_closedev(int device_index)
{
	struct sigrok_device_instance *sdi;

	if( (sdi = get_sigrok_device_instance(device_instances, device_index)) )
		close_device(sdi);

}


void hw_cleanup(void)
{
	GSList *l;

	/* properly close all devices */
	for(l = device_instances; l; l = l->next)
		close_device( (struct sigrok_device_instance *) l->data);

	/* and free all their memory */
	for(l = device_instances; l; l = l->next)
		g_free(l->data);
	g_slist_free(device_instances);
	device_instances = NULL;

	if(usb_context)
		libusb_exit(usb_context);
	usb_context = NULL;

}


void *hw_get_device_info(int device_index, int device_info_id)
{
	struct sigrok_device_instance *sdi;
	void *info;

	if( !(sdi = get_sigrok_device_instance(device_instances, device_index)) )
		return NULL;

	info = NULL;
	switch(device_info_id)
	{
	case DI_INSTANCE:
		info = sdi;
		break;
	case DI_NUM_PROBES:
		info = GINT_TO_POINTER(NUM_PROBES);
		break;
	case DI_SAMPLERATES:
		info = &samplerates;
		break;
	case DI_TRIGGER_TYPES:
		info = TRIGGER_TYPES;
		break;
	case DI_CUR_SAMPLE_RATE:
		info = &cur_samplerate;
		break;
	}

	return info;
}


int hw_get_status(int device_index)
{
	struct sigrok_device_instance *sdi;

	sdi = get_sigrok_device_instance(device_instances, device_index);
	if(sdi)
		return sdi->status;
	else
		return ST_NOT_FOUND;
}


int *hw_get_capabilities(void)
{

	return capabilities;
}


/* TODO: this is very specific to the saleae logic */
int set_configuration_samplerate(struct sigrok_device_instance *sdi, uint64_t samplerate)
{
	uint8_t divider;
	int ret, result;
	unsigned char buf[2];

	if(samplerate < samplerates.low || samplerate < samplerates.high)
		return SIGROK_ERR_BADVALUE;

	divider = (uint8_t) (48 / (float) (samplerate/1000000)) - 1;

	g_message("setting sample rate to %"PRIu64" Hz (divider %d)", samplerate, divider);
	buf[0] = 0x01;
	buf[1] = divider;
	ret = libusb_bulk_transfer(sdi->usb->devhdl, 1 | LIBUSB_ENDPOINT_OUT, buf, 2, &result, 500);
	if(ret != 0) {
		g_warning("failed to set samplerate: %d", ret);
		return SIGROK_NOK;
	}
	cur_samplerate = samplerate;

	return SIGROK_OK;
}


int hw_set_configuration(int device_index, int capability, void *value)
{
	struct sigrok_device_instance *sdi;
	int ret;
	uint64_t *tmp_u64;

	if( !(sdi = get_sigrok_device_instance(device_instances, device_index)) )
		return SIGROK_NOK;

	if(capability == HWCAP_SAMPLERATE) {
		tmp_u64 = value;
		ret = set_configuration_samplerate(sdi, *tmp_u64);
	}
	else if(capability == HWCAP_PROBECONFIG)
		ret = configure_probes( (GSList *) value);
	else if(capability == HWCAP_LIMIT_SAMPLES) {
		limit_samples = strtoull(value, NULL, 10);
		ret = SIGROK_OK;
	}
	else
		ret = SIGROK_NOK;

	return ret;
}


int receive_data(GSource *source, gpointer data)
{
	struct timeval tv;

	tv.tv_sec = tv.tv_usec = 0;
	libusb_handle_events_timeout(usb_context, &tv);

	return TRUE;
}


void receive_transfer(struct libusb_transfer *transfer)
{
	static int num_samples = 0;
	static int empty_transfer_count = 0;

	struct datafeed_packet packet;
	void *user_data;
	int cur_buflen;
	unsigned char *cur_buf, *new_buf;

	if(transfer == NULL) {
		/* hw_stop_acquisition() telling us to stop */
		num_samples = -1;
	}

	if(num_samples == -1) {
		/* acquisition has already ended, just free any queued up transfer that come in */
		libusb_free_transfer(transfer);
	}
	else {
		g_message("receive_transfer(): status %d received %d bytes", transfer->status, transfer->actual_length);

		/* save the incoming transfer before reusing the transfer struct */
		cur_buf = transfer->buffer;
		cur_buflen = transfer->actual_length;
		user_data = transfer->user_data;

		/* fire off a new request */
		new_buf = g_malloc(4096);
		transfer->buffer = new_buf;
		transfer->length = 4096;
		if(libusb_submit_transfer(transfer) != 0) {
			/* TODO: stop session? */
			g_warning("eek");
		}

		if(cur_buflen == 0) {
			empty_transfer_count++;
			if(empty_transfer_count > MAX_EMPTY_TRANSFERS) {
				/* the FX2 gave up... end the acquisition, the frontend will work
				 * out that the samplecount is short
				 */
				packet.type = DF_END;
				session_bus(user_data, &packet);
				num_samples = -1;
			}
			return;
		}
		else
			empty_transfer_count = 0;

		/* send the incoming transfer to the session bus */
		packet.type = DF_LOGIC8;
		packet.length = cur_buflen;
		packet.payload = cur_buf;
		session_bus(user_data, &packet);
		g_free(cur_buf);

		num_samples += cur_buflen;
		if(num_samples > limit_samples)
		{
			/* end the acquisition */
			packet.type = DF_END;
			session_bus(user_data, &packet);
			num_samples = -1;
		}
	}

}


int hw_start_acquisition(int device_index, gpointer session_device_id)
{
	struct sigrok_device_instance *sdi;
	struct datafeed_packet *packet;
	struct datafeed_header *header;
	struct libusb_transfer *transfer;
	const struct libusb_pollfd **lupfd;
	int size, i;
	unsigned char *buf;

	if( !(sdi = get_sigrok_device_instance(device_instances, device_index)))
		return SIGROK_NOK;

	packet = g_malloc(sizeof(struct datafeed_packet));
	header = g_malloc(sizeof(struct datafeed_header));
	if(!packet || !header)
		return SIGROK_NOK;

	/* start with 2K transfer, subsequently increased to 4K */
	size = 2048;
	for(i = 0; i < NUM_SIMUL_TRANSFERS; i++) {
		buf = g_malloc(size);
		transfer = libusb_alloc_transfer(0);
		libusb_fill_bulk_transfer(transfer, sdi->usb->devhdl, 2 | LIBUSB_ENDPOINT_IN, buf, size,
				receive_transfer, session_device_id, 40);
		if(libusb_submit_transfer(transfer) != 0) {
			/* TODO: free them all */
			libusb_free_transfer(transfer);
			g_free(buf);
			return SIGROK_NOK;
		}
		size = 4096;
	}

	lupfd = libusb_get_pollfds(usb_context);
	for(i = 0; lupfd[i]; i++)
		add_source_fd(lupfd[i]->fd, lupfd[i]->events, receive_data, NULL);
	free(lupfd);

	packet->type = DF_HEADER;
	packet->length = sizeof(struct datafeed_header);
	packet->payload = (unsigned char *) header;
	header->feed_version = 1;
	gettimeofday(&header->starttime, NULL);
	header->rate = cur_samplerate;
	header->protocol_id = PROTO_RAW;
	header->num_probes = NUM_PROBES;
	session_bus(session_device_id, packet);
	g_free(header);
	g_free(packet);

	return SIGROK_OK;
}


/* this stops acquisition on ALL devices, ignoring device_index */
void hw_stop_acquisition(int device_index, gpointer session_device_id)
{
	struct datafeed_packet packet;

	packet.type = DF_END;
	session_bus(session_device_id, &packet);

	receive_transfer(NULL);

	/* TODO: need to cancel and free any queued up transfers */

}



struct device_plugin plugin_info = {
	"zeroplus-logic-cube",
	1,
	hw_init,
	hw_cleanup,

	hw_opendev,
	hw_closedev,
	hw_get_device_info,
	hw_get_status,
	hw_get_capabilities,
	hw_set_configuration,
	hw_start_acquisition,
	hw_stop_acquisition
};

