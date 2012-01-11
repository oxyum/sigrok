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

#ifndef SIGROKFILE_SIGROKFILE_H
#define SIGROKFILE_SIGROKFILE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Status/error codes returned by libsigrokfile functions.
 *
 * All possible return codes of libsigrokfile functions must be listed here.
 * Functions should never return hardcoded numbers as status, but rather
 * use these #defines instead. All error codes are negative numbers.
 *
 * The error codes are globally unique in libsigrokfile, i.e. if one of the
 * libsigrokfile functions returns a "malloc error" it must be exactly the
 * same return value as used by all other functions to indicate "malloc error".
 * There must be no functions which indicate two different errors via the
 * same return code.
 *
 * Also, for compatibility reasons, no defined return codes are ever removed
 * or reused for different #defines later. You can only add new #defines and
 * return codes, but never remove or redefine existing ones.
 */
#define SRF_OK                 0 /**< No error */
#define SRF_ERR               -1 /**< Generic/unspecified error */
#define SRF_ERR_MALLOC        -2 /**< Malloc/calloc/realloc error */
#define SRF_ERR_ARG           -3 /**< Function argument error */
#define SRF_ERR_BUG           -4 /**< Errors hinting at internal bugs */

/* libsigrokfile loglevels. */
#define SRF_LOG_NONE   0 /**< Output no messages at all. */
#define SRF_LOG_ERR    1 /**< Output error messages. */
#define SRF_LOG_WARN   2 /**< Output warnings. */
#define SRF_LOG_INFO   3 /**< Output informational messages. */
#define SRF_LOG_DBG    4 /**< Output debug messages. */
#define SRF_LOG_SPEW   5 /**< Output very noisy debug messages. */

/*--- main.c ----------------------------------------------------------------*/

int srf_init(void);
int srf_exit(void);

/*--- log.c -----------------------------------------------------------------*/

int srf_set_loglevel(int loglevel);
int srf_get_loglevel(void);

#ifdef __cplusplus
}
#endif

#endif
