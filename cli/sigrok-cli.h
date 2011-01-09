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


#ifndef SIGROK_CLI_H_
#define SIGROK_CLI_H_

/* sigrok-cli.c */
int num_real_devices(void);

/* parsers.c */
char **parse_probestring(int max_probes, char *probestring);
char **parse_triggerstring(struct device *device, char *triggerstring);
uint64_t parse_sizestring(char *sizestring);
struct device *parse_devicestring(char *devicestring);


#endif /* SIGROK_CLI_H_ */
