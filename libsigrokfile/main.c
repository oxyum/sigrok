/*
 * This file is part of the sigrok project.
 *
 * Copyright (C) 2012 Uwe Hermann <uwe@hermann-uwe.de>
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

#include "sigrokfile.h"
#include "sigrokfile-internal.h"

/**
 * Initialize libsigrokfile.
 *
 * The caller is responsible for calling the clean-up function srf_exit(),
 * which will properly shut down libsigrokfile and free its allocated memory.
 *
 * Multiple calls to srf_init(), without calling srf_exit() in between,
 * are not allowed.
 *
 * @return SRF_OK upon success, a (negative) error code otherwise.
 */
int srf_init(void)
{
	return SRF_OK;
}

/**
 * Shutdown libsigrokfile.
 *
 * This function should only be called if there was a (successful!) invocation
 * of srf_init() before. Calling this function multiple times in a row, without
 * any successful srf_init() calls in between, is not allowed.
 *
 * @return SRF_OK upon success, a (negative) error code otherwise.
 */
int srf_exit(void)
{
	return SRF_OK;
}
