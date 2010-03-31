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

#include <stdio.h>
#include <sigrokdecode.h>

int sigrokdecode_init(void)
{
	/* Py_Initialize() returns void and usually cannot fail. */
	Py_Initialize();

	return 0;
}

int sigrokdecode_run_decoder(const char *filename, uint8_t *inbuf,
			     uint8_t *outbuf)
{
	FILE *f;
	PyObject *py_file;

	/* TODO: Use #defines for the return codes. */

	if ((py_file = PyFile_FromString((char *)filename, "r")) == NULL)
		return -1;

	if ((f = PyFile_AsFile(py_file)) == NULL)
		return -2;

	if (PyRun_SimpleFile(f, filename) != 0)
		return -3;

	/* TODO: Use inbuf, outbuf. */

	return 0;
}

int sigrokdecode_shutdown(void)
{
	/* Py_Finalize() returns void, any finalization errors are ignored. */
	Py_Finalize();

	return 0;
}
