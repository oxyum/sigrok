/*
 *   sigrok - session.h
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

#ifndef SESSION_H
#define SESSION_H

#include <stdio.h>

#include "sigrok.h"
#include "datastore.h"
#include "device.h"
#include "analyzer.h"

struct session {
	/* list of struct device* */
	GSList *devices;
	/* list of struct analyzer* */
	GSList *analyzers;
	/* output file and file handle */
	char *output_filename;
	FILE *output_file;
	/* callback to process output */
	GSList *output_callbacks;
	GTimeVal starttime;
	/* sample frequency expressed as 1s / freqdiv */
	unsigned int freqdiv;
	struct datastore *datastore;
};

typedef void (*output_callback) (struct device *device, struct datafeed_packet *packet);

/* session setup */
struct session *session_load(char *filename);
struct session *session_new(void);
void session_destroy(void);
void session_device_clear(void);
int session_device_add(struct device *device);

/* protocol analyzers setup */
void session_pa_clear(void);
void session_pa_add(struct analyzer *pa);

/* output setup */
void session_output_clear(void);
void session_output_add_file(char *filename);
void session_output_add_callback(output_callback callback);

/* session control */
int session_start(void);
void session_stop(void);
void session_bus(struct device *device, struct datafeed_packet *packet);
void session_save(char *filename);

#endif

