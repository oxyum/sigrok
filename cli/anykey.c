/*
 * This file is part of the sigrok project.
 *
 * Copyright (C) 2011 Bert Vermeulen <bert@biot.com>
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

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <glib.h>
#include <sigrok.h>
#include "sigrok-cli.h"


extern int end_acquisition;

struct termios term_orig;


static int received_anykey(int fd, int revents, void *user_data)
{

	/* avoid compiler warning */
	fd = fd;
	revents = revents;
	user_data = user_data;

	end_acquisition = TRUE;

	return TRUE;
}


void add_anykey(void)
{
	struct termios term;

	/* turn off buffering on stdin */
	tcgetattr(STDIN_FILENO, &term);
	memcpy(&term_orig, &term, sizeof(struct termios));
	term.c_lflag &= ~(ECHO | ICANON | ISIG);
	tcsetattr(STDIN_FILENO, TCSADRAIN, &term);

	add_source(STDIN_FILENO, G_IO_IN, -1, received_anykey, NULL);

	printf("Press any key to stop acquisition.\n");

}


void clear_anykey(void)
{

	/* restore stdin */
	tcflush(STDIN_FILENO, TCIFLUSH);
	tcsetattr(STDIN_FILENO, TCSANOW, &term_orig);

}


