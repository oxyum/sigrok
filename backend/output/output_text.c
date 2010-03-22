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

#include "sigrok.h"


static void init_text_binary(struct output *o)
{

}


static int data_text_binary(struct output *o, char *data_in, uint64_t length_in, char **data_out, uint64_t *length_out)
{

	return SIGROK_OK;
}


static int event_text_binary(struct output *o, int event_type, char **data_out, uint64_t *length_out)
{

	return SIGROK_OK;
}


static void init_text_hex(struct output *o)
{

}


static int data_text_hex(struct output *o, char *data_in, uint64_t length_in, char **data_out, uint64_t *length_out)
{

	return SIGROK_OK;
}


static int event_text_hex(struct output *o, int event_type, char **data_out, uint64_t *length_out)
{

	return SIGROK_OK;
}



struct output_format output_text_binary = {
	"bin",
	"Text (binary)",
	init_text_binary,
	data_text_binary,
	event_text_binary
};


struct output_format output_text_hex = {
	"hex",
	"Text (hexadecimal)",
	init_text_hex,
	data_text_hex,
	event_text_hex
};

