/*
 *   sigrok - datastore.h
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

#ifndef DATASTORE_H
#define DATASTORE_H

#include <glib.h>

/* size of a chunk in units */
#define DATASTORE_CHUNKSIZE 512000

struct datastore {
	/* size in bytes of the number of units stored in this datastore */
	int unitsize;
	unsigned int num_units;
	GSList *chunklist;
};


struct datastore *datastore_new(int unitsize);
void datastore_destroy(struct datastore *ds);
void datastore_put(struct datastore *ds, unsigned int num_units, int unitsize, gpointer data);

#endif
