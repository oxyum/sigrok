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

extern "C" {
#include <sigrokdecode.h> /* First, so we avoid a _POSIX_C_SOURCE warning. */
#include <stdint.h>
#include <sigrok.h>
}

#include <iostream>
#include <QtGui/QApplication>
#include <QTranslator>
#include <QLocale>
#include <QDebug>
#include "mainwindow.h"

uint8_t *sample_buffer;
MainWindow *w;

int main(int argc, char *argv[])
{
	QString locale = QLocale::system().name();
	QApplication a(argc, argv);
	QTranslator translator;

	translator.load(QString("locale/sigrok-qt_") + locale);
	a.installTranslator(&translator);

	/* Set some application metadata. */
	QApplication::setApplicationVersion(APP_VERSION);
	QApplication::setApplicationName("sigrok-qt");
	QApplication::setOrganizationDomain("http://www.sigrok.org");

	/* Disable this to actually allow running the (pre-alpha!) Qt GUI. */
	qDebug() << "The Qt GUI is not yet usable, aborting.";
	// return 1;

	if (sr_init() != SR_OK) {
		qDebug() << "ERROR: libsigrok init failed.";
		return 1;
	}
	qDebug() << "libsigrok initialized successfully.";

	if (srd_init(NULL) != SRD_OK) {
		qDebug() << "ERROR: libsigrokdecode init failed.";
		return 1;
	}
	qDebug() << "libsigrokdecode initialized successfully.";

	w = new MainWindow;
	w->show();

	return a.exec();
}
