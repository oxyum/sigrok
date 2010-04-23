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
#include "mainwindow.h"

uint8_t *sample_buffer;
MainWindow *w;

int main(int argc, char *argv[])
{
	QString locale = QLocale::system().name();
	QApplication a(argc, argv);
	QTranslator translator;
	uint8_t *inbuf = NULL, *outbuf = NULL;
	uint64_t outbuflen = 0;
	int ret;

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

#if 1
	if (sigrokdecode_init() != SIGROKDECODE_OK) {
		std::cerr << "ERROR: Failed to init sigrokdecode." << std::endl;
		return 1;
	}
	inbuf = (uint8_t *)calloc(1000, 1);
	inbuf[0] = 'X'; /* Just a quick test. */
	inbuf[1] = 'Y';
	ret = sigrokdecode_run_decoder("transitioncounter",
				       "sigrokdecode_count_transitions",
				       inbuf, 1000, &outbuf, &outbuflen);
	if (outbuf != NULL) {
		std::cout << "outbuflen: " << outbuflen << std::endl;
		std::cout << "outbuf[0]: " << outbuf[0] << std::endl;
		std::cout << "outbuf[1]: " << outbuf[1] << std::endl;
	}
	sigrokdecode_shutdown();
#endif

	w->show();
	return a.exec();
}
