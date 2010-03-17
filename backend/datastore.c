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
#include <stdint.h>
#include <string.h>
#include <glib.h>

#include "datastore.h"


static gpointer new_chunk(struct datastore **ds);



struct datastore *datastore_new(int unitsize)
{
	struct datastore *ds;

	ds = g_malloc(sizeof(struct datastore));
	ds->ds_unitsize = unitsize;
	ds->num_units = 0;
	ds->chunklist = NULL;

	return ds;
}


void datastore_destroy(struct datastore *ds)
{
	GSList *chunk;

	for(chunk = ds->chunklist; chunk; chunk = chunk->next)
		g_free(chunk->data);
	g_slist_free(ds->chunklist);
	g_free(ds);

}


void datastore_put(struct datastore *ds, void *data, unsigned int length, int in_unitsize, int *probelist)
{
	int num_enabled_probes, out_bit, in_offset, out_offset, capacity, stored, size, i;
	int num_chunks, chunk_bytes_free, chunk_offset;
	uint64_t sample_in, sample_out;
	gpointer chunk;
	char *buf;

	num_enabled_probes = 0;
	for(i = 0; probelist[i]; i++)
		num_enabled_probes++;

	if(num_enabled_probes != in_unitsize * 8) {
		/* convert sample from maximum probes -- the way the hardware driver sent
		 * it -- to a sample taking up only as much space as required, with
		 * unused probes removed.
		 */
		buf = malloc(length);
		in_offset = out_offset = 0;
		while(in_offset < length) {
			memcpy(&sample_in, data + in_offset, in_unitsize);
			sample_out = 0;
			out_bit = 0;
			for(i = 0; probelist[i]; i++) {
				if(sample_in & (1 << (probelist[i]-1)))
					sample_out |= 1 << out_bit;
				out_bit++;
			}
			memcpy(buf + out_offset, &sample_out, ds->ds_unitsize);
			in_offset += in_unitsize;
			out_offset += ds->ds_unitsize;
		}
	}
	else {
		/* all probes are used -- no need to compress anything */
		buf = data;
		out_offset = length;
	}

	if(ds->chunklist == NULL)
		chunk = new_chunk(&ds);
	else
		chunk = g_slist_last(ds->chunklist)->data;
	num_chunks = g_slist_length(ds->chunklist);
	capacity = (num_chunks * DATASTORE_CHUNKSIZE);
	chunk_bytes_free = capacity - (ds->ds_unitsize * ds->num_units);
	chunk_offset = capacity - (DATASTORE_CHUNKSIZE * (num_chunks - 1)) - chunk_bytes_free;
	stored = 0;
	while(stored < out_offset) {
		if(chunk_bytes_free == 0) {
			chunk = new_chunk(&ds);
			chunk_bytes_free = DATASTORE_CHUNKSIZE;
			chunk_offset = 0;
		}

		if(out_offset - stored > chunk_bytes_free)
			size = chunk_bytes_free;
		else
			/* last part, won't fill up this chunk */
			size = out_offset - stored;
		memcpy(chunk + chunk_offset, buf + stored, size);
		stored += size;
	}
	ds->num_units += stored / ds->ds_unitsize;

	if(buf != data)
		free(buf);

}


static gpointer new_chunk(struct datastore **ds)
{
	gpointer chunk;

	chunk = g_malloc(DATASTORE_CHUNKSIZE * (*ds)->ds_unitsize);
	(*ds)->chunklist = g_slist_append((*ds)->chunklist, chunk);

	return chunk;
}


