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

#ifndef SIGROK_SAMPLEIODEVICE_H
#define SIGROK_SAMPLEIODEVICE_H

#include <QIODevice>

class SampleIODevice : public QIODevice
{
public:
	SampleIODevice();
	bool open(OpenMode openMode);

protected:
	qint64 readData(char *data, qint64 maxlen);
	qint64 writeData(const char *data, qint64 len);
};

#endif // SIGROK_SAMPLEIODEVICE_H
