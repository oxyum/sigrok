/*
 * This file is part of the sigrok project.
 *
 * Copyright (C) 2010 Uwe Hermann <uwe@hermann-uwe.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "sampleiodevice.h"
#include <QIODevice>

extern "C" {
#include <sigrok.h>
#include <glib.h>
}

SampleIODevice::SampleIODevice()
{
	/* TODO */
}

SampleIODevice::~SampleIODevice()
{
	/* TODO */
}

bool SampleIODevice::open(OpenMode openMode)
{
	/* TODO: Handle openMode parameter. */

	/* TODO: Open sigrok backend stuff. */

	/* Call the parent's constructor. */
	QIODevice::open(openMode);

	return true;
}

qint64 SampleIODevice::readData(char *data, qint64 maxlen)
{
	/* FIXME */
	(void)data;
	(void)maxlen;
	return 0;

	/* TODO: Read a chunk of data. */
}

qint64 SampleIODevice::writeData(const char *data, qint64 len)
{
	/* FIXME */
	(void)data;
	(void)len;
	return 0;

	/* TODO: Write a chunk of data. */
}

bool SampleIODevice::isSequential() const
{
	/* The sample stream is sequential (no seeking possible). */
	return true;
}

qint64 SampleIODevice::bytesAvailable() const
{
	// return 100 /* FIXME */ + QIODevice::bytesAvailable();
	return QIODevice::bytesAvailable();
}
