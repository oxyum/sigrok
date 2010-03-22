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

#ifndef SIGROK_SIGROK_H
#define SIGROK_SIGROK_H

#include <glib.h>
#include <sys/time.h>
#include <stdint.h>
#include <inttypes.h>


/* returned status/error codes */
#define SIGROK_STATUS_DISABLED		0
#define SIGROK_OK					1
#define SIGROK_NOK					2
#define SIGROK_ERR_BADVALUE		20

/* handy little macros */
#define KHZ(n) (n*1000)
#define MHZ(n) (n*1000000)
#define GHZ(n) (n*1000000000)

/* data types, used by hardware plugins for set_configuration() */
enum {
	T_UINT64,
	T_CHAR,
};

enum {
	PROTO_RAW,
};

/* (unused) protocol decoder stack entry */
struct protocol {
	char *name;
	int id;
	int stackindex;
};


/*****************************
 * datafeed
 */

/* datafeed_packet.type values */
enum {
	DF_HEADER,
	DF_END,
	DF_TRIGGER,
	DF_LOGIC8,
	DF_LOGIC16,
	DF_LOGIC24,
	DF_LOGIC32,
	DF_LOGIC48,
	DF_LOGIC64
};

struct datafeed_packet {
	uint16_t type;
	uint16_t length;
	unsigned char *payload;
};

struct datafeed_header {
	int feed_version;
	struct timeval starttime;
	uint64_t rate;
	int protocol_id;
	int num_probes;
};


/*****************************
 * output
 */

struct output {
	struct output_format *format;
	struct device *device;
	int fd;
	char *param;
	char *internal;
};

struct output_format {
	char *extension;
	char *description;
	void (*init) (struct output *o);
	int (*data) (struct output *o, char *data_in, uint64_t length_in, char **data_out, uint64_t *length_out);
	int (*event) (struct output *o, int event_type, char **data_out, uint64_t *length_out);
};

#endif
