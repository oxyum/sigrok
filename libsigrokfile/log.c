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
#include <stdarg.h>
#include <stdio.h>

static int srf_loglevel = SRF_LOG_WARN; /* Show errors+warnings per default. */

/**
 * Set the libsigrokfile loglevel.
 *
 * This influences the amount of log messages (debug messages, error messages,
 * and so on) libsigrokfile will output. Using SRF_LOG_NONE disables all
 * messages.
 *
 * @param loglevel The loglevel to set (SRF_LOG_NONE, SRF_LOG_ERR,
 *                 SRF_LOG_WARN, SRF_LOG_INFO, SRF_LOG_DBG, or SRF_LOG_SPEW).
 * @return SRF_OK upon success, SRF_ERR_ARG upon invalid loglevel.
 */
int srf_set_loglevel(int loglevel)
{
	if (loglevel < SRF_LOG_NONE || loglevel > SRF_LOG_SPEW) {
		srf_err("log: %s: invalid loglevel %d", __func__, loglevel);
		return SRF_ERR_ARG;
	}

	srf_loglevel = loglevel;

	srf_dbg("log: %s: libsigrokfile loglevel set to %d",
		__func__, loglevel);

	return SRF_OK;
}

/**
 * Get the libsigrokfile loglevel.
 *
 * @return The currently configured libsigrokfile loglevel.
 */
int srf_get_loglevel(void)
{
	return srf_loglevel;
}

static int srf_logv(int loglevel, const char *format, va_list args)
{
	int ret;

	/* Only output messages of at least the selected loglevel(s). */
	if (loglevel > srf_loglevel)
		return SRF_OK; /* TODO? */

	ret = vfprintf(stderr, format, args);
	fprintf(stderr, "\n");

	return ret;
}

int srf_log(int loglevel, const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = srf_logv(loglevel, format, args);
	va_end(args);

	return ret;
}

int srf_spew(const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = srf_logv(SRF_LOG_SPEW, format, args);
	va_end(args);

	return ret;
}

int srf_dbg(const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = srf_logv(SRF_LOG_DBG, format, args);
	va_end(args);

	return ret;
}

int srf_info(const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = srf_logv(SRF_LOG_INFO, format, args);
	va_end(args);

	return ret;
}

int srf_warn(const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = srf_logv(SRF_LOG_WARN, format, args);
	va_end(args);

	return ret;
}

int srf_err(const char *format, ...)
{
	int ret;
	va_list args;

	va_start(args, format);
	ret = srf_logv(SRF_LOG_ERR, format, args);
	va_end(args);

	return ret;
}
