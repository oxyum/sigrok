/*
 * This file is part of the sigrok project.
 *
 * Copyright (C) 2011 Gareth McMullin <gareth@blacksphere.co.nz>
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

#include <sigrok.h>
#include <gtk/gtk.h>

void help_wiki(void)
{
	gtk_show_uri(NULL, "http://www.sigrok.org/", GDK_CURRENT_TIME, NULL);
}

void help_about(GtkAction *action, GtkWindow *parent)
{
	(void)action;

	gtk_show_about_dialog(parent,
		"program_name", "sigrok-gtk",
		"version", "0.1",
		"logo-icon-name", "sigrok-logo",
		"comments", "Portable, cross-platform, "
			"Free/Libre/Open-Source logic analyzer software.\n\nGTK+ Front-end",
		"license",
			"This program is free software: you can redistribute it and/or modify\n"
			"it under the terms of the GNU General Public License as published by\n"
			"the Free Software Foundation, either version 3 of the License, or\n"
			"(at your option) any later version.\n\n"

			"This program is distributed in the hope that it will be useful,\n"
			"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
			"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
			"GNU General Public License for more details.\n\n"

			"You should have received a copy of the GNU General Public License\n"
			"along with this program.  If not, see <http://www.gnu.org/licenses/>.\n",
		"website", "http://www.sigrok.org/",
		NULL);
}

