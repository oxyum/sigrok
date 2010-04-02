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

#include <iostream>
#include <QtGui/QApplication>
#include <QTranslator>
#include <QLocale>
#include "mainwindow.h"

extern "C" {
#include <stdint.h>
#include <sigrok.h>
}

uint8_t *sample_buffer;
MainWindow *w;

int main(int argc, char *argv[])
{
	QString locale = QLocale::system().name();
	QApplication a(argc, argv);
	QTranslator translator;

	translator.load(QString("locale/sigrok-gui_") + locale);
	a.installTranslator(&translator);

	/* Set some application metadata. */
	QApplication::setApplicationVersion(APP_VERSION);
	QApplication::setApplicationName("sigrok-gui");
	QApplication::setOrganizationDomain("http://www.sigrok.org");

	w = new MainWindow;

	if (sigrok_init() != SIGROK_OK) {
		std::cerr << "ERROR: Failed to init sigrok." << std::endl;
		return 1;
	}

	w->show();
	return a.exec();
}
