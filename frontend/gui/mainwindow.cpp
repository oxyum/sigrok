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

#include <QMessageBox>
#include <QFileDialog>
#include <QProgressDialog>
#include <QDockWidget>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "configform.h"
#include "ui_configform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* __STDC_FORMAT_MACROS is required for PRIu64 and friends (in C++). */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdint.h>
#include <glib.h>
#include <gmodule.h>
#include "sigrok.h"
#include "backend.h"
#include "device.h"
#include "session.h"

#ifdef __cplusplus
}
#endif

GMainContext *gmaincontext = NULL;
GMainLoop *gmainloop = NULL;

uint64_t limit_samples = 0; /* FIXME */

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::MainWindow)
{
	currentLA = -1;
	numChannels = -1;
	configChannelTitleBarLayout = 0; /* Vertical layout */

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
	QString s;
	QColor color;

	/* TODO: Do not create new dockWidgets if we already have them. */

	/* TODO: Kill any old dockWidgets before creating new ones? */

	for (int i = 0; i < getNumChannels(); ++i) {
		widgets[i] = new QWidget(this);
		gridLayouts[i] = new QGridLayout(widgets[i]);

		lineEdits[i] = new QLineEdit(this);
		lineEdits[i]->setMaximumWidth(150);
		lineEdits[i]->setText(s.sprintf("Channel %d", i));
		/* Use random colors for the channel names for now. */
		QPalette p = QPalette(QApplication::palette());
		color = QColor(2 + qrand() * 16);
		p.setColor(QPalette::Base, color);
		lineEdits[i]->setPalette(p);
		gridLayouts[i]->addWidget(lineEdits[i], i, 1);

		channelRenderAreas[i] = new ChannelRenderArea(this);
		channelRenderAreas[i]->setSizePolicy(QSizePolicy::Minimum,
					QSizePolicy::MinimumExpanding);
		channelRenderAreas[i]->setChannelNumber(i);
		channelRenderAreas[i]->setChannelColor(color);
		gridLayouts[i]->addWidget(channelRenderAreas[i], i, 2);

		dockWidgets[i] = new QDockWidget(this);
		dockWidgets[i]->setAllowedAreas(Qt::RightDockWidgetArea);

		QDockWidget::DockWidgetFeatures f;
		f = QDockWidget::DockWidgetClosable |
		    QDockWidget::DockWidgetMovable |
		    QDockWidget::DockWidgetFloatable;
		if (configChannelTitleBarLayout == 0)
			f |= QDockWidget::DockWidgetVerticalTitleBar;
		dockWidgets[i]->setFeatures(f);
		dockWidgets[i]->setWidget(widgets[i]);
		addDockWidget(Qt::RightDockWidgetArea, dockWidgets[i]);

		/* Update labels upon changes. */
		QObject::connect(channelRenderAreas[i],
			SIGNAL(sampleStartChanged(QString)),
			ui->labelSampleStart, SLOT(setText(QString)));
		QObject::connect(channelRenderAreas[i],
			SIGNAL(sampleEndChanged(QString)),
			ui->labelSampleEnd, SLOT(setText(QString)));
		QObject::connect(channelRenderAreas[i],
			SIGNAL(zoomFactorChanged(QString)),
			ui->labelZoomFactor, SLOT(setText(QString)));

		/* Redraw channels upon changes. */
		QObject::connect(channelRenderAreas[i],
			SIGNAL(sampleStartChanged(QString)),
			channelRenderAreas[i], SLOT(generatePainterPath()));
		QObject::connect(channelRenderAreas[i],
			SIGNAL(sampleEndChanged(QString)),
			channelRenderAreas[i], SLOT(generatePainterPath()));
		QObject::connect(channelRenderAreas[i],
			SIGNAL(zoomFactorChanged(QString)),
			channelRenderAreas[i], SLOT(generatePainterPath()));

		// dockWidgets[i]->show();
#if 0
		/* If the user renames a channel, adapt the dock title. */
		QObject::connect(lineEdits[i], SIGNAL(textChanged(QString)),
				 dockWidgets[i], SLOT(setWindowTitle(QString)));
#endif
	}
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
	QMessageBox::about(this, tr("About"),
		tr("sigrok-gui 0.1<br />\nCopyright (C) 2010 "
		"Uwe Hermann &lt;uwe@hermann-uwe.de&gt;<br />\n"
		"GNU GPL, version 2 or later<br />\n"
		"<a href=\"http://www.sigrok.org\">"
		"http://www.sigrok.org</a>"));
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
	int ret;
	QString s;
	int num_devices;
	struct device *device;
	char *di_num_probes;
	uint64_t *di_samplerates;

	statusBar()->showMessage(tr("Scanning for logic analyzers..."));

	device_scan();
	devices = device_list();
	num_devices = g_slist_length(devices);
	device = (struct device *)g_slist_nth_data(devices, 0 /* opt_device */);

	if (num_devices == 0) {
		s = tr("No supported logic analyzer found.");
		statusBar()->showMessage(s);
		return;
	} else {
		s = tr("Found supported logic analyzer: ");
		s.append(device->plugin->name);
		statusBar()->showMessage(s);
	}

	setCurrentLA(0 /* TODO */);

	di_num_probes = device->plugin->get_device_info(device->plugin_index,
							DI_NUM_PROBES);
	if (di_num_probes != NULL) {
		setNumChannels(GPOINTER_TO_INT(di_num_probes));
	} else {
		setNumChannels(8); /* FIXME: Error handling. */
	}

	ui->comboBoxLA->clear();
	ui->comboBoxLA->addItem(device->plugin->name); /* TODO: Full name */

	ui->labelChannels->setText(s.sprintf("Channels: %d",
			getNumChannels()));

	di_samplerates = (uint64_t *)device->plugin->get_device_info(
			device->plugin_index, DI_SAMPLE_RATES);
	if (!di_samplerates) {
		/* TODO: Error handling. */
	}

	ui->comboBoxSampleRate->clear();
	for (int i = 0; di_samplerates[i]; ++i) {
		if (di_samplerates[i] < 1000000)
			s.sprintf("%"PRIu64" kHz", di_samplerates[i] / 1000);
		else
			s.sprintf("%"PRIu64" MHz", di_samplerates[i] / 1000000);
		ui->comboBoxSampleRate->addItem(s, di_samplerates[i]);
	}

	/* FIXME */
	ui->comboBoxNumSamples->addItem("100", 100); /* For testing... */
	ui->comboBoxNumSamples->addItem("3000000", 3000000);
	ui->comboBoxNumSamples->addItem("2000000", 2000000);
	ui->comboBoxNumSamples->addItem("1000000", 1000000);

	ui->comboBoxNumSamples->setEditable(true);

	/// ret = sigrok_hw_init(getCurrentLA(), &ctx);
	// if (ret < 0)
	// 	statusBar()->showMessage(tr("ERROR: LA init failed."));

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
	s.sprintf("%d", getNumChannels());
	s.prepend(tr("Channels: "));
	ui->labelChannels->setText(s);
	ui->labelChannels->setEnabled(false);

	ui->comboBoxSampleRate->clear();
	ui->comboBoxSampleRate->setEnabled(false); /* FIXME */

	ui->comboBoxNumSamples->clear();
	ui->comboBoxNumSamples->addItem(s.sprintf("%"PRIu64, getNumSamples()),
					getNumSamples());
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
		channelRenderAreas[i]->setChannelNumber(i);
		channelRenderAreas[i]->setNumSamples(file.size());
		channelRenderAreas[i]->setSampleStart(0);
		channelRenderAreas[i]->setSampleEnd(getNumSamples());
		channelRenderAreas[i]->update();
	}

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

void datafeed_callback(struct device *device, struct datafeed_packet *packet)
{
	static int num_probes = 0;
	static int received_samples = 0;
	static int probelist[65] = {0};

	struct probe *probe;
	struct datafeed_header *header;
	int num_enabled_probes, offset, sample_size, unitsize, p;
	int bpl_offset, bpl_cnt;
	uint64_t sample;

	sample_size = -1;

	// if(packet->type != DF_HEADER && linebuf == NULL)
	//	return;

	switch (packet->type) {
	case DF_HEADER:
		printf("DF_HEADER\n");
		header = (struct datafeed_header *) packet->payload;
		num_probes = header->num_probes;
		num_enabled_probes = 0;
		for (int i = 0; i < header->num_probes; ++i) {
			probe = (struct probe *)g_slist_nth_data(device->probes, i);
			if (probe->enabled)
				probelist[num_enabled_probes++] = probe->index;
		}

		printf("Acquisition with %d/%d probes at %"PRIu64"MHz "
		       "starting at %s (%"PRIu64" samples)\n",
		       num_enabled_probes, num_probes, header->rate,
		       ctime(&header->starttime.tv_sec), limit_samples);

		// linebuf = g_malloc0(num_probes * linebuf_len);
		break;
	case DF_END:
		printf("DF_END\n");
		g_main_loop_quit(gmainloop);
		return;
		break;
	case DF_TRIGGER:
		/* TODO */
		break;
	case DF_LOGIC8:
		sample_size = 1;
		printf("DF_LOGIC8\n");
		break;
	/* TODO: DF_LOGIC16 etc. */
	}

	if (sample_size < 0)
		return;

	for (uint64_t i = 0; received_samples < limit_samples
			     && i < packet->length; i += sample_size) {
		sample = 0;
		memcpy(&sample, packet->payload + offset, sample_size);
		sample_buffer[i] = (uint8_t)(sample & 0xff); /* FIXME */
		printf("Sample %d: 0x%x\n", i, sample);
		received_samples++;
	}
}

void MainWindow::on_action_Get_samples_triggered()
{
	uint8_t *buf;
	uint64_t pos = 0;
	uint64_t numSamplesLocal = ui->comboBoxNumSamples->itemData(
			ui->comboBoxNumSamples->currentIndex()).toLongLong();
	uint64_t chunksize = 512;
	QString s;
	int opt_device;
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
	session_output_add_callback(datafeed_callback);
	device = (struct device *)g_slist_nth_data(devices, opt_device);

	snprintf(numBuf, 16, "%"PRIu64"", limit_samples);
	device->plugin->set_configuration(device->plugin_index,
		HWCAP_LIMIT_SAMPLES, (char *)numBuf);


	if (session_device_add(device) != SIGROK_OK) {
		printf("Failed to use device.\n");
		session_destroy();
		return;
	}

	if (session_start() != SIGROK_OK) {
		printf("Failed to start session.\n");
		session_destroy();
		return;
	}

	gmaincontext = g_main_context_default();
	gmainloop = g_main_loop_new(gmaincontext, FALSE);
	g_main_loop_run(gmainloop);

	session_stop();
	session_destroy();

#if 0
	// if (getCurrentLA() < 0) {
	// 	/* TODO */
	// 	return;
	// }

	// buf = sigrok_hw_get_samples_init(&ctx, numSamplesLocal, 1000000, 1000);
	// if (buf == NULL) {
	// 	/* TODO: Error handling. */
	// 	return;
	// }

	QProgressDialog progress("Getting samples from logic analyzer...",
				 "Abort", 0, numSamplesLocal, this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setMinimumDuration(500);

	for (uint64_t i = 0; i < numSamplesLocal; i += chunksize) {
		progress.setValue(i);

		/* TODO: Properly handle an abort. */
		if (progress.wasCanceled())
			break;

		/* Get a small chunk of samples. */
		// ret = sigrok_hw_get_samples_chunk(&ctx, buf, chunksize,
		// 				      pos, 1000);
		pos += chunksize;
		/* TODO: Error handling. */
	}
	progress.setValue(numSamplesLocal);

	sample_buffer = buf;
#endif

	for (int i = 0; i < getNumChannels(); ++i) {
		channelRenderAreas[i]->setChannelNumber(i);
		channelRenderAreas[i]->setNumSamples(numSamplesLocal);
		channelRenderAreas[i]->setSampleStart(0);
		channelRenderAreas[i]->setSampleEnd(numSamplesLocal);
		channelRenderAreas[i]->update();
	}

#if 0
	/* FIXME */
	s.sprintf("%"PRIu64"", (int)channelRenderAreas[0]->getSampleStart());
	s.prepend(tr("Start sample: "));
	ui->labelSampleStart->setText(s);

	/* FIXME */
	s.sprintf("%"PRIu64"", (int)channelRenderAreas[0]->getSampleEnd());
	s.prepend(tr("End sample: "));
	ui->labelSampleEnd->setText(s);
#endif

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
