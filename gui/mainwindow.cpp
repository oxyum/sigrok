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

#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QProgressDialog>
#include <QDockWidget>
#include <QScrollBar>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "configform.h"
#include "ui_configform.h"
#include "channelform.h"
#include "ui_channelform.h"

extern "C" {
/* __STDC_FORMAT_MACROS is required for PRIu64 and friends (in C++). */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdint.h>
#include <glib.h>
#include <sigrok.h>
}

struct source {
	int fd;
	int events;
	int timeout;
	receive_data_callback cb;
	void *user_data;
};
struct source *sources = NULL;
int num_sources = 0;
int source_timeout = -1;

/* These live in hwplugin.c, for the frontend to override. */
extern source_callback_add source_cb_add;
extern source_callback_remove source_cb_remove;

int end_acquisition = 0;

GPollFD *fds;

uint64_t limit_samples = 0; /* FIXME */

QProgressDialog *progress = NULL;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::MainWindow)
{
	currentLA = -1;
	numChannels = -1;
	configChannelTitleBarLayout = 1; /* Horizontal layout */
	for (int i = 0; i < NUMCHANNELS; ++i)
		dockWidgets[i] = NULL;

	ui->setupUi(this);

	/* FIXME */
	QMainWindow::setCentralWidget(ui->mainWidget);

	// this->setDockOptions(QMainWindow::AllowNestedDocks);
}

MainWindow::~MainWindow()
{
	sigrok_cleanup();

	delete ui;
}

void MainWindow::setupDockWidgets(void)
{
	/* TODO: Do not create new dockWidgets if we already have them. */

	/* TODO: Kill any old dockWidgets before creating new ones? */

	for (int i = 0; i < getNumChannels(); ++i) {
		channelForms[i] = new ChannelForm(this);

		dockWidgets[i] = new QDockWidget(this);
		dockWidgets[i]->setAllowedAreas(Qt::BottomDockWidgetArea);

		QDockWidget::DockWidgetFeatures f;
		f = QDockWidget::DockWidgetClosable |
		    QDockWidget::DockWidgetMovable |
		    QDockWidget::DockWidgetFloatable;
		if (configChannelTitleBarLayout == 0)
			f |= QDockWidget::DockWidgetVerticalTitleBar;
		dockWidgets[i]->setFeatures(f);
		dockWidgets[i]->setWidget(channelForms[i]);
		addDockWidget(Qt::BottomDockWidgetArea, dockWidgets[i],
			      Qt::Vertical);

		/* Update labels upon changes. */
		QObject::connect(channelForms[i],
			SIGNAL(sampleStartChanged(QString)),
			ui->labelSampleStart, SLOT(setText(QString)));
		QObject::connect(channelForms[i],
			SIGNAL(sampleEndChanged(QString)),
			ui->labelSampleEnd, SLOT(setText(QString)));
		QObject::connect(channelForms[i],
			SIGNAL(zoomFactorChanged(QString)),
			ui->labelZoomFactor, SLOT(setText(QString)));

		/* Redraw channels upon changes. */
		QObject::connect(channelForms[i],
			SIGNAL(sampleStartChanged(QString)),
			channelForms[i], SLOT(generatePainterPath()));
		QObject::connect(channelForms[i],
			SIGNAL(sampleEndChanged(QString)),
			channelForms[i], SLOT(generatePainterPath()));
		QObject::connect(channelForms[i],
			SIGNAL(zoomFactorChanged(QString)),
			channelForms[i], SLOT(generatePainterPath()));

		// dockWidgets[i]->show();

		/* Set a default channel name. */
		QLineEdit *l = channelForms[i]->m_ui->channelLineEdit;
		l->setText(QString(tr("Channel %1")).arg(i));

#if 0
		/* If the user renames a channel, adapt the dock title. */
		QObject::connect(lineEdits[i], SIGNAL(textChanged(QString)),
				 dockWidgets[i], SLOT(setWindowTitle(QString)));
#endif
	}

	/* For now, display only one scrollbar which scrolls all channels. */
	for (int i = 0; i < getNumChannels() - 1; ++i)
		channelForms[i]->m_ui->channelScrollBar->setVisible(false);
}

void MainWindow::setCurrentLA(int la)
{
	currentLA = la;
}

int MainWindow::getCurrentLA(void)
{
	return currentLA;
}

void MainWindow::setNumChannels(int ch)
{
	numChannels = ch;
}

int MainWindow::getNumChannels(void)
{
	return numChannels;
}

void MainWindow::on_actionAbout_triggered()
{
	QString s = tr("%1 %2<br />\nCopyright (C) 2010 "
		"Uwe Hermann &lt;uwe@hermann-uwe.de&gt;<br />\n"
		"GNU GPL, version 2 or later<br /><a href=\"%3\">%3</a>\n")
		.arg(QApplication::applicationName())
		.arg(QApplication::applicationVersion())
		.arg(QApplication::organizationDomain());

	QMessageBox::about(this, tr("About"), s);
}

void MainWindow::on_actionAbout_Qt_triggered()
{
	QMessageBox::aboutQt(this, tr("About Qt"));
}

void MainWindow::on_actionPreferences_triggered()
{
	ConfigForm *form = new ConfigForm();
	form->show();
}

void MainWindow::on_actionScan_triggered()
{
	QString s;
	int num_devices, pos;
	struct device *device;
	char *di_num_probes, *str;
	struct samplerates *samplerates;
	const static float mult[] = { 2.f, 2.5f, 2.f };

	statusBar()->showMessage(tr("Scanning for logic analyzers..."), 2000);

	device_scan();
	devices = device_list();
	num_devices = g_slist_length(devices);

	ui->comboBoxLA->clear();
	for (int i = 0; i < num_devices; ++i) {
		device = (struct device *)g_slist_nth_data(devices, i);
		ui->comboBoxLA->addItem(device->plugin->name); /* TODO: Full name */
	}

	if (num_devices == 0) {
		s = tr("No supported logic analyzer found.");
		statusBar()->showMessage(s, 2000);
		return;
	} else if (num_devices == 1) {
		s = tr("Found supported logic analyzer: ");
		s.append(device->plugin->name);
		statusBar()->showMessage(s, 2000);
	} else {
		/* TODO: Allow user to select one of the devices. */
		s = tr("Found multiple logic analyzers: ");
		for (int i = 0; i < num_devices; ++i) {
			device = (struct device *)g_slist_nth_data(devices, i);
			s.append(device->plugin->name);
			if (i != num_devices - 1)
				s.append(", ");
		}
		statusBar()->showMessage(s, 2000);
		device_close_all();
		return;
	}

	device = (struct device *)g_slist_nth_data(devices, 0 /* opt_device */);
	
	setCurrentLA(0 /* TODO */);

	di_num_probes = (char *)device->plugin->get_device_info(
			device->plugin_index, DI_NUM_PROBES);
	if (di_num_probes != NULL) {
		setNumChannels(GPOINTER_TO_INT(di_num_probes));
	} else {
		setNumChannels(8); /* FIXME: Error handling. */
	}

	ui->comboBoxLA->clear();
	ui->comboBoxLA->addItem(device->plugin->name); /* TODO: Full name */

	s = QString(tr("Channels: %1")).arg(getNumChannels());
	ui->labelChannels->setText(s);

	samplerates = (struct samplerates *)device->plugin->get_device_info(
		      device->plugin_index, DI_SAMPLERATES);
	if (!samplerates) {
		/* TODO: Error handling. */
	}

	/* Populate the combobox with supported samplerates. */
	ui->comboBoxSampleRate->clear();
	if (samplerates->list != NULL) {
		for (int i = 0; samplerates->list[i]; ++i) {
			str = sigrok_samplerate_string(samplerates->list[i]);
			s = QString(str);
			free(str);
			ui->comboBoxSampleRate->addItem(s,
				QVariant::fromValue(samplerates->list[i]));
		}
	} else {
		pos = 0;
		for (uint64_t r = samplerates->low; r <= samplerates->high; ) {
			str = sigrok_samplerate_string(r);
			s = QString(str);
			free(str);
			ui->comboBoxSampleRate->addItem(s,
						QVariant::fromValue(r));
			r *= mult[pos++];
			pos %= 3;
		}
	}

	/* FIXME */
	ui->comboBoxNumSamples->clear();
	ui->comboBoxNumSamples->addItem("100", 100); /* For testing... */
	ui->comboBoxNumSamples->addItem("3000000", 3000000);
	ui->comboBoxNumSamples->addItem("2000000", 2000000);
	ui->comboBoxNumSamples->addItem("1000000", 1000000);

	ui->comboBoxNumSamples->setEditable(true);

	if (getCurrentLA() >= 0)
		setupDockWidgets();

	/* Enable all relevant fields now (i.e. make them non-gray). */
	ui->comboBoxSampleRate->setEnabled(true);
	ui->comboBoxNumSamples->setEnabled(true);
	ui->labelChannels->setEnabled(true);
	ui->action_Get_samples->setEnabled(true);
}

void MainWindow::on_action_Open_triggered()
{
	QString s;
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Open sample file"), ".",
		tr("Raw sample files (*.raw *.bin);;"
		   "Gnuplot data files (*.dat);;"
		   "VCD files (*.vcd);;"
		   "All files (*)"));

	if (fileName == NULL)
		return;

	QFile file(fileName);
	file.open(QIODevice::ReadOnly);
	QDataStream in(&file);

	/* TODO: Implement support for loading different input formats. */

	sample_buffer = (uint8_t *)malloc(file.size());
	if (sample_buffer == NULL) {
		/* TODO: Error handling. */
	}

	in.readRawData((char *)sample_buffer, file.size());

	setNumSamples(file.size());
	setNumChannels(8); /* FIXME */

	file.close();

	setupDockWidgets();

	ui->comboBoxLA->clear();
	ui->comboBoxLA->addItem(tr("File"));

	/* FIXME: Store number of channels in the file or allow user config. */
	s = QString(tr("Channels: %1")).arg(getNumChannels());
	ui->labelChannels->setText(s);
	ui->labelChannels->setEnabled(false);

	ui->comboBoxSampleRate->clear();
	ui->comboBoxSampleRate->setEnabled(false); /* FIXME */

	ui->comboBoxNumSamples->clear();
	ui->comboBoxNumSamples->addItem(QString::number(getNumSamples()),
					QVariant::fromValue(getNumSamples()));
	ui->comboBoxNumSamples->setEnabled(true);

	ui->labelSampleStart->setText(tr("Start sample: "));
	ui->labelSampleStart->setEnabled(true);

	ui->labelSampleEnd->setText(tr("End sample: "));
	ui->labelSampleEnd->setEnabled(true);

	ui->labelZoomFactor->setText(tr("Zoom factor: "));
	ui->labelZoomFactor->setEnabled(true);

	ui->action_Save_as->setEnabled(true);
	ui->action_Get_samples->setEnabled(false);

	for (int i = 0; i < getNumChannels(); ++i) {
		channelForms[i]->setChannelNumber(i);
		channelForms[i]->setNumSamples(file.size());

		QScrollBar *sc = channelForms[i]->m_ui->channelScrollBar;
		sc->setMinimum(0);
		sc->setMaximum(99);

		/* The i-th scrollbar scrolls channel i. */
		connect(sc, SIGNAL(valueChanged(int)),
			channelForms[i], SLOT(setScrollBarValue(int)));

		/* If any of the scrollbars change, update all of them. */
		connect(sc, SIGNAL(valueChanged(int)),
			w, SLOT(updateScrollBars(int)));

		/* Set a default channel name. */
		QLineEdit *l = channelForms[i]->m_ui->channelLineEdit;
		l->setText(QString(tr("Channel %1")).arg(i));
		
		channelForms[i]->update();
	}

	/* For now, display only one scrollbar which scrolls all channels. */
	for (int i = 0; i < getNumChannels() - 1; ++i)
		channelForms[i]->m_ui->channelScrollBar->setVisible(false);

	/* FIXME */
}

void MainWindow::on_action_Save_as_triggered()
{
	QString fileName = QFileDialog::getSaveFileName(this,
		tr("Save sample file"), ".",
		tr("Raw sample files (*.raw *.bin);;All files (*)"));

	if (fileName == NULL)
		return;

	QFile file(fileName);
	file.open(QIODevice::WriteOnly);
	QDataStream out(&file);

	/* TODO: Implement support for saving to different output formats. */

	out.writeRawData((const char *)sample_buffer,
			 getNumSamples() * (getNumChannels() / 8));
	file.close();
}

void datafeed_in(struct device *device, struct datafeed_packet *packet)
{
	static int num_probes = 0;
	static int probelist[65] = {0};
	static uint64_t received_samples = 0;
	static int triggered = 0;
	struct probe *probe;
	struct datafeed_header *header;
	int num_enabled_probes, sample_size;
	uint64_t sample;

	/* If the first packet to come in isn't a header, don't even try. */
	// if (packet->type != DF_HEADER && o == NULL)
	//	return;

	/* TODO: Also check elsewhere? */
	/* TODO: Abort acquisition too, if user pressed cancel. */
	if (progress && progress->wasCanceled())
		return;

	sample_size = -1;

	switch (packet->type) {
	case DF_HEADER:
		qDebug("DF_HEADER");
		header = (struct datafeed_header *) packet->payload;
		num_probes = header->num_logic_probes;
		num_enabled_probes = 0;
		for (int i = 0; i < header->num_logic_probes; ++i) {
			probe = (struct probe *)g_slist_nth_data(device->probes, i);
			if (probe->enabled)
				probelist[num_enabled_probes++] = probe->index;
		}

		qDebug() << "Acquisition with" << num_enabled_probes << "/"
			 << num_probes << "probes at"
			 << sigrok_samplerate_string(header->samplerate)
			 << "starting at" << ctime(&header->starttime.tv_sec)
			 << "(" << limit_samples << "samples)";

		/* TODO: realloc() */
		break;
	case DF_END:
		qDebug("DF_END");
		/* TODO: o */
		end_acquisition = 1;
		progress->setValue(received_samples); /* FIXME */
		break;
	case DF_TRIGGER:
		qDebug("DF_TRIGGER");
		/* TODO */
		triggered = 1;
		break;
	case DF_LOGIC:
		qDebug("DF_LOGIC");
		sample_size = packet->unitsize;
		break;
	default:
		qDebug("DF_XXXX, not yet handled");
		break;
	}

	if (sample_size == -1)
		return;
	
	/* Don't store any samples until triggered. */
	// if (opt_wait_trigger && !triggered)
	// 	return;

	if (received_samples >= limit_samples)
		return;

	/* TODO */

	for (uint64_t i = 0; received_samples < limit_samples
			     && i < packet->length; i += sample_size) {
		sample = 0;
		memcpy(&sample, (char *)packet->payload + i, sample_size);
		sample_buffer[i] = (uint8_t)(sample & 0xff); /* FIXME */
		// qDebug("Sample %" PRIu64 ": 0x%x", i, sample);
		received_samples++;
	}

	progress->setValue(received_samples);
}

void remove_source(int fd)
{
	struct source *new_sources;
	int oldsource, newsource = 0;

	if (!sources)
		return;

	new_sources = (struct source *)calloc(1, sizeof(struct source)
					      * num_sources);
	for (oldsource = 0; oldsource < num_sources; oldsource++) {
		if (sources[oldsource].fd != fd)
			memcpy(&new_sources[newsource++], &sources[oldsource],
			       sizeof(struct source));
	}

	if (oldsource != newsource) {
		free(sources);
		sources = new_sources;
		num_sources--;
	} else {
		/* Target fd was not found. */
		free(new_sources);
	}
}

void add_source(int fd, int events, int timeout,
		receive_data_callback callback, void *user_data)
{
	struct source *new_sources, *s;

	// add_source_fd(fd, events, timeout, callback, user_data);

	new_sources = (struct source *)calloc(1, sizeof(struct source)
					      * (num_sources + 1));

	if (sources) {
		memcpy(new_sources, sources, sizeof(GPollFD) * num_sources);
		free(sources);
	}

	s = &new_sources[num_sources++];
	s->fd = fd;
	s->events = events;
	s->timeout = timeout;
	s->cb = callback;
	s->user_data = user_data;
	sources = new_sources;

	if (timeout != source_timeout && timeout > 0
	    && (source_timeout == -1 || timeout < source_timeout))
		source_timeout = timeout;
}

void MainWindow::on_action_Get_samples_triggered()
{
	uint64_t numSamplesLocal = ui->comboBoxNumSamples->itemData(
			ui->comboBoxNumSamples->currentIndex()).toLongLong();
	uint64_t samplerate = ui->comboBoxSampleRate->itemData(
			ui->comboBoxSampleRate->currentIndex()).toLongLong();
	QString s;
	int opt_device, ret, i;
	struct device *device;
	char numBuf[16];

	opt_device = 0; /* FIXME */

	limit_samples = numSamplesLocal;

	sample_buffer = (uint8_t *)malloc(limit_samples);
	if (sample_buffer == NULL) {
		/* TODO: Error handling. */
		return;
	}

	session_new();
	session_datafeed_callback_add(datafeed_in);

	source_cb_remove = remove_source;
	source_cb_add = add_source;

	device = (struct device *)g_slist_nth_data(devices, opt_device);

	/* Set the number of samples we want to get from the device. */
	snprintf(numBuf, 16, "%" PRIu64 "", limit_samples);
	device->plugin->set_configuration(device->plugin_index,
		HWCAP_LIMIT_SAMPLES, (char *)numBuf);

	if (session_device_add(device) != SIGROK_OK) {
		qDebug("Failed to use device.");
		session_destroy();
		return;
	}

	/* Set the samplerate. */
	if (device->plugin->set_configuration(device->plugin_index,
	    HWCAP_SAMPLERATE, &samplerate) != SIGROK_OK) {
		qDebug("Failed to set sample rate.");
		session_destroy();
		return;
	};

	if (device->plugin->set_configuration(device->plugin_index,
	    HWCAP_PROBECONFIG, (char *)device->probes) != SIGROK_OK) {
		qDebug("Failed to configure probes.");
		session_destroy();
		return;
	}

	if (session_start() != SIGROK_OK) {
		qDebug("Failed to start session.");
		session_destroy();
		return;
	}

	progress = new QProgressDialog("Getting samples from logic analyzer...",
				   "Abort", 0, numSamplesLocal, this);
	progress->setWindowModality(Qt::WindowModal);
	progress->setMinimumDuration(100);

	fds = NULL;
	while (!end_acquisition) {
		if (fds)
			free(fds);

		/* Construct g_poll()'s array. */
		fds = (GPollFD *)malloc(sizeof(GPollFD) * num_sources);
		for (i = 0; i < num_sources; ++i) {
			fds[i].fd = sources[i].fd;
			fds[i].events = sources[i].events;
		}

		ret = g_poll(fds, num_sources, source_timeout);

		for (i = 0; i < num_sources; ++i) {
			if (fds[i].revents > 0 || (ret == 0
			    && source_timeout == sources[i].timeout)) {
				/*
				 * Invoke the source's callback on an event,
				 * or if the poll timeout out and this source
				 * asked for that timeout.
				 */
				sources[i].cb(fds[i].fd, fds[i].revents,
					      sources[i].user_data);
			}
		}
	}
	free(fds);

	session_stop();
	session_destroy();

	for (int i = 0; i < getNumChannels(); ++i) {
		channelForms[i]->setChannelNumber(i);
		channelForms[i]->setNumSamples(numSamplesLocal);
		// channelForms[i]->setSampleStart(0);
		// channelForms[i]->setSampleEnd(numSamplesLocal);

		QScrollBar *sc = channelForms[i]->m_ui->channelScrollBar;
		sc->setMinimum(0);
		sc->setMaximum(99);

		/* The i-th scrollbar scrolls channel i. */
		connect(sc, SIGNAL(valueChanged(int)),
			channelForms[i], SLOT(setScrollBarValue(int)));

		/* If any of the scrollbars change, update all of them. */
		connect(sc, SIGNAL(valueChanged(int)),
		        w, SLOT(updateScrollBars(int)));

		channelForms[i]->update();
	}

	/* Enable the relevant labels/buttons. */
	ui->labelSampleStart->setEnabled(true);
	ui->labelSampleEnd->setEnabled(true);
	ui->labelZoomFactor->setEnabled(true);
	ui->action_Save_as->setEnabled(true);

	// sigrok_hw_get_samples_shutdown(&ctx, 1000);
}

void MainWindow::setSampleRate(uint64_t s)
{
	sampleRate = s;
}

uint64_t MainWindow::getSampleRate(void)
{
	return sampleRate;
}

void MainWindow::setNumSamples(uint64_t s)
{
	numSamples = s;
}

uint64_t MainWindow::getNumSamples(void)
{
	return numSamples;
}

void MainWindow::on_action_New_triggered()
{
	for (int i = 0; i < NUMCHANNELS; ++i) {
		if (dockWidgets[i]) {
			/* TODO: Check if all childs are also killed. */
			delete dockWidgets[i];
			dockWidgets[i] = NULL;
		}
	}

	ui->comboBoxLA->clear();
	ui->comboBoxLA->addItem(tr("No LA detected"));

	ui->labelChannels->setText(tr("Channels: "));
	ui->labelChannels->setEnabled(false);

	ui->comboBoxSampleRate->clear();
	ui->comboBoxSampleRate->setEnabled(false);

	ui->comboBoxNumSamples->clear();
	ui->comboBoxNumSamples->setEnabled(false);

	ui->labelSampleStart->setText(tr("Start sample: "));
	ui->labelSampleStart->setEnabled(false);

	ui->labelSampleEnd->setText(tr("End sample: "));
	ui->labelSampleEnd->setEnabled(false);

	ui->labelZoomFactor->setText(tr("Zoom factor: "));
	ui->labelZoomFactor->setEnabled(false);

	ui->action_Save_as->setEnabled(false);
	ui->action_Get_samples->setEnabled(false);

	setNumChannels(0);

	/* TODO: More cleanups. */
	/* TODO: Free sample buffer(s). */
}

uint8_t *MainWindow::getDemoSampleBuffer(void)
{
	/* TODO */
	return NULL;
}

void MainWindow::configChannelTitleBarLayoutChanged(int index)
{
	QDockWidget::DockWidgetFeatures f =
		QDockWidget::DockWidgetClosable |
		QDockWidget::DockWidgetMovable |
		QDockWidget::DockWidgetFloatable;

	configChannelTitleBarLayout = index;

	if (configChannelTitleBarLayout == 0)
		f |= QDockWidget::DockWidgetVerticalTitleBar;

	for (int i = 0; i < getNumChannels(); ++i)
		dockWidgets[i]->setFeatures(f);
}

void MainWindow::updateScrollBars(int value)
{
	static int lock = 0;

	/* TODO: There must be a better way to do this. */
	if (lock == 1)
		return;

	lock = 1;
	for (int i = 0; i < getNumChannels(); ++i) {
		// qDebug("updating scrollbar %d", i);
		channelForms[i]->m_ui->channelScrollBar->setValue(value);
		// qDebug("updating scrollbar %d (DONE)", i);
	}
	lock = 0;
}

