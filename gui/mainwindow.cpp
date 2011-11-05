/*
 * This file is part of the sigrok project.
 *
 * Copyright (C) 2010-2011 Uwe Hermann <uwe@hermann-uwe.de>
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
#include <sigrokdecode.h>
}

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
#include "decodersform.h"
#include "ui_decodersform.h"

extern "C" {
/* __STDC_FORMAT_MACROS is required for PRIu64 and friends (in C++). */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdint.h>
#include <glib.h>
#include <sigrok.h>
}

#define DOCK_VERTICAL   0
#define DOCK_HORIZONTAL 1

uint64_t limit_samples = 0; /* FIXME */

QProgressDialog *progress = NULL;

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent), ui(new Ui::MainWindow)
{
	currentLA = -1;
	numChannels = -1;
	configChannelTitleBarLayout = DOCK_VERTICAL; /* Vertical layout */
	for (int i = 0; i < NUMCHANNELS; ++i)
		dockWidgets[i] = NULL;

	ui->setupUi(this);

	/* FIXME */
	QMainWindow::setCentralWidget(ui->mainWidget);

	// this->setDockOptions(QMainWindow::AllowNestedDocks);
}

MainWindow::~MainWindow()
{
	srd_exit();
	sr_exit();

	delete ui;
}

void MainWindow::setupDockWidgets(void)
{
	/* TODO: Do not create new dockWidgets if we already have them. */

	/* TODO: Kill any old dockWidgets before creating new ones? */

	for (int i = 0; i < getNumChannels(); ++i) {
		channelForms[i] = new ChannelForm(this);
		channelForms[i]->setChannelNumber(i);

		dockWidgets[i] = new QDockWidget(this);
		dockWidgets[i]->setAllowedAreas(Qt::BottomDockWidgetArea);

		QDockWidget::DockWidgetFeatures f;
		f = QDockWidget::DockWidgetClosable |
		    QDockWidget::DockWidgetMovable |
		    QDockWidget::DockWidgetFloatable;
		if (configChannelTitleBarLayout == DOCK_VERTICAL)
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
	GSList *l;
	struct sr_device_plugin *plugin;
	struct sr_input_format **inputs;
	struct sr_output_format **outputs;
	struct srd_decoder *dec;

	QString s = tr("%1 %2<br />%3<br /><a href=\"%4\">%4</a>\n<p>")
		.arg(QApplication::applicationName())
		.arg(QApplication::applicationVersion())
		.arg(tr("GNU GPL, version 2 or later"))
		.arg(QApplication::organizationDomain());

	s.append("<b>" + tr("Supported hardware drivers:") + "</b><table>");
	for (l = sr_list_hwplugins(); l; l = l->next) {
		plugin = (struct sr_device_plugin *)l->data;
		s.append(QString("<tr><td><i>%1</i></td><td>%2</td></tr>")
			 .arg(QString(plugin->name))
			 .arg(QString(plugin->longname)));
	}
	s.append("</table><p>");

	s.append("<b>" + tr("Supported input formats:") + "</b><table>");
	inputs = sr_input_list();
	for (int i = 0; inputs[i]; ++i) {
		s.append(QString("<tr><td><i>%1</i></td><td>%2</td></tr>")
			 .arg(QString(inputs[i]->id))
			 .arg(QString(inputs[i]->description)));
	}
	s.append("</table><p>");

	s.append("<b>" + tr("Supported output formats:") + "</b><table>");
	outputs = sr_output_list();
	for (int i = 0; outputs[i]; ++i) {
		s.append(QString("<tr><td><i>%1</i></td><td>%2</td></tr>")
			.arg(QString(outputs[i]->id))
			.arg(QString(outputs[i]->description)));
	}
	s.append("</table><p>");

	s.append("<b>" + tr("Supported protocol decoders:") + "</b><table>");
	for (l = srd_list_decoders(); l; l = l->next) {
		dec = (struct srd_decoder *)l->data;
		s.append(QString("<tr><td><i>%1</i></td><td>%2</td></tr>")
			 .arg(QString(dec->id))
			 .arg(QString(dec->desc)));
	}
	s.append("</table>");

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
	struct sr_device *device;
	char *di_num_probes, *str;
	struct sr_samplerates *samplerates;
	const static float mult[] = { 2.f, 2.5f, 2.f };

	statusBar()->showMessage(tr("Scanning for logic analyzers..."), 2000);

	sr_device_scan();
	devices = sr_device_list();
	num_devices = g_slist_length(devices);

	ui->comboBoxLA->clear();
	for (int i = 0; i < num_devices; ++i) {
		device = (struct sr_device *)g_slist_nth_data(devices, i);
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
			device = (struct sr_device *)g_slist_nth_data(devices, i);
			s.append(device->plugin->name);
			if (i != num_devices - 1)
				s.append(", ");
		}
		statusBar()->showMessage(s, 2000);
		return;
	}

	device = (struct sr_device *)g_slist_nth_data(devices, 0 /* opt_device */);
	
	setCurrentLA(0 /* TODO */);

	di_num_probes = (char *)device->plugin->get_device_info(
			device->plugin_index, SR_DI_NUM_PROBES);
	if (di_num_probes != NULL) {
		setNumChannels(GPOINTER_TO_INT(di_num_probes));
	} else {
		setNumChannels(8); /* FIXME: Error handling. */
	}

	ui->comboBoxLA->clear();
	ui->comboBoxLA->addItem(device->plugin->name); /* TODO: Full name */

	s = QString(tr("Channels: %1")).arg(getNumChannels());
	ui->labelChannels->setText(s);

	samplerates = (struct sr_samplerates *)device->plugin->get_device_info(
		      device->plugin_index, SR_DI_SAMPLERATES);
	if (!samplerates) {
		/* TODO: Error handling. */
	}

	/* Populate the combobox with supported samplerates. */
	ui->comboBoxSampleRate->clear();
	if (!samplerates) {
		ui->comboBoxSampleRate->addItem("No samplerate");
		ui->comboBoxSampleRate->setEnabled(false);
	} else if (samplerates->list != NULL) {
		for (int i = 0; samplerates->list[i]; ++i) {
			str = sr_samplerate_string(samplerates->list[i]);
			s = QString(str);
			free(str);
			ui->comboBoxSampleRate->insertItem(0, s,
				QVariant::fromValue(samplerates->list[i]));
		}
		ui->comboBoxSampleRate->setEnabled(true);
	} else {
		pos = 0;
		for (uint64_t r = samplerates->low; r <= samplerates->high; ) {
			str = sr_samplerate_string(r);
			s = QString(str);
			free(str);
			ui->comboBoxSampleRate->insertItem(0, s,
						QVariant::fromValue(r));
			r *= mult[pos++];
			pos %= 3;
		}
		ui->comboBoxSampleRate->setEnabled(true);
	}
	ui->comboBoxSampleRate->setCurrentIndex(0);

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

void datafeed_in(struct sr_device *device, struct sr_datafeed_packet *packet)
{
	static int num_probes = 0;
	static int probelist[65] = {0};
	static uint64_t received_samples = 0;
	static int triggered = 0;
	struct sr_probe *probe;
	struct sr_datafeed_header *header;
	struct sr_datafeed_logic *logic;
	int num_enabled_probes, sample_size;
	uint64_t sample;

	/* If the first packet to come in isn't a header, don't even try. */
	// if (packet->type != SR_DF_HEADER && o == NULL)
	//	return;

	/* TODO: Also check elsewhere? */
	/* TODO: Abort acquisition too, if user pressed cancel. */
	if (progress && progress->wasCanceled())
		return;

	sample_size = -1;

	switch (packet->type) {
	case SR_DF_HEADER:
		qDebug("SR_DF_HEADER");
		header = (struct sr_datafeed_header *)packet->payload;
		num_probes = header->num_logic_probes;
		num_enabled_probes = 0;
		for (int i = 0; i < header->num_logic_probes; ++i) {
			probe = (struct sr_probe *)g_slist_nth_data(device->probes, i);
			if (probe->enabled)
				probelist[num_enabled_probes++] = probe->index;
		}

		qDebug() << "Acquisition with" << num_enabled_probes << "/"
			 << num_probes << "probes at"
			 << sr_samplerate_string(header->samplerate)
			 << "starting at" << ctime(&header->starttime.tv_sec)
			 << "(" << limit_samples << "samples)";

		/* TODO: realloc() */
		break;
	case SR_DF_END:
		qDebug("SR_DF_END");
		/* TODO: o */
		sr_session_halt();
		progress->setValue(received_samples); /* FIXME */
		break;
	case SR_DF_TRIGGER:
		qDebug("SR_DF_TRIGGER");
		/* TODO */
		triggered = 1;
		break;
	case SR_DF_LOGIC:
		logic = (sr_datafeed_logic*)packet->payload;
		qDebug() << "SR_DF_LOGIC (length =" << logic->length
			 << ", unitsize = " << logic->unitsize << ")";
		sample_size = logic->unitsize;
		break;
	default:
		qDebug("SR_DF_XXXX, not yet handled");
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
			     && i < logic->length; i += sample_size) {
		sample = 0;
		memcpy(&sample, (char *)packet->payload + i, sample_size);
		sample_buffer[i] = (uint8_t)(sample & 0xff); /* FIXME */
		// qDebug("Sample %" PRIu64 ": 0x%x", i, sample);
		received_samples++;
	}

	progress->setValue(received_samples);
}

void MainWindow::on_action_Get_samples_triggered()
{
	uint64_t samplerate;
	QString s;
	int opt_device;
	struct sr_device *device;
	QComboBox *n = ui->comboBoxNumSamples;

	opt_device = 0; /* FIXME */

	/*
	 * The number of samples to get is a drop-down list, but you can also
	 * manually enter a value. If the latter, we have to get the value from
	 * the lineEdit object, otherwise via itemData() and the list index.
	 */
	if (n->lineEdit() != NULL) {
		limit_samples = n->lineEdit()->text().toLongLong();
	} else {
		limit_samples = n->itemData(n->currentIndex()).toLongLong();
	}

	samplerate = ui->comboBoxSampleRate->itemData(
		ui->comboBoxSampleRate->currentIndex()).toLongLong();

	/* TODO: Sanity checks. */

	/* TODO: Assumes unitsize == 1. */
	if (!(sample_buffer = (uint8_t *)malloc(limit_samples))) {
		/* TODO: Error handling. */
		return;
	}

	sr_session_new();
	sr_session_datafeed_callback_add(datafeed_in);

	device = (struct sr_device *)g_slist_nth_data(devices, opt_device);

	/* Set the number of samples we want to get from the device. */
	if (device->plugin->set_configuration(device->plugin_index,
	    SR_HWCAP_LIMIT_SAMPLES, &limit_samples) != SR_OK) {
		qDebug("Failed to set sample limit.");
		sr_session_destroy();
		return;
	}

	if (sr_session_device_add(device) != SR_OK) {
		qDebug("Failed to use device.");
		sr_session_destroy();
		return;
	}

	/* Set the samplerate. */
	if (device->plugin->set_configuration(device->plugin_index,
	    SR_HWCAP_SAMPLERATE, &samplerate) != SR_OK) {
		qDebug("Failed to set samplerate.");
		sr_session_destroy();
		return;
	};

	if (device->plugin->set_configuration(device->plugin_index,
	    SR_HWCAP_PROBECONFIG, (char *)device->probes) != SR_OK) {
		qDebug("Failed to configure probes.");
		sr_session_destroy();
		return;
	}

	if (sr_session_start() != SR_OK) {
		qDebug("Failed to start session.");
		sr_session_destroy();
		return;
	}

	progress = new QProgressDialog("Getting samples from logic analyzer...",
				       "Abort", 0, limit_samples, this);
	progress->setWindowModality(Qt::WindowModal);
	progress->setMinimumDuration(100);

	sr_session_run();

	sr_session_halt();
	sr_session_destroy();

	for (int i = 0; i < getNumChannels(); ++i) {
		channelForms[i]->setNumSamples(limit_samples);
		// channelForms[i]->setSampleStart(0);
		// channelForms[i]->setSampleEnd(limit_samples);

		QScrollBar *sc = channelForms[i]->m_ui->channelScrollBar;
		sc->setMinimum(0);
		sc->setMaximum(99);

		/* The i-th scrollbar scrolls channel i. */
		connect(sc, SIGNAL(valueChanged(int)),
			channelForms[i], SLOT(setScrollBarValue(int)));

		/* If any of the scrollbars change, update all of them. */
		connect(sc, SIGNAL(valueChanged(int)),
		        w, SLOT(updateScrollBars(int)));

		/* If any of the zoom factors change, update all of them.. */
		connect(channelForms[i], SIGNAL(zoomFactorChanged(float)),
		        w, SLOT(updateZoomFactors(float)));

		channelForms[i]->generatePainterPath();
		// channelForms[i]->update();
	}

	/* Enable the relevant labels/buttons. */
	ui->labelSampleStart->setEnabled(true);
	ui->labelSampleEnd->setEnabled(true);
	ui->labelZoomFactor->setEnabled(true);
	ui->action_Save_as->setEnabled(true);

	// sr_hw_get_samples_shutdown(&ctx, 1000);
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

void MainWindow::configChannelTitleBarLayoutChanged(int index)
{
	QDockWidget::DockWidgetFeatures f =
		QDockWidget::DockWidgetClosable |
		QDockWidget::DockWidgetMovable |
		QDockWidget::DockWidgetFloatable;

	configChannelTitleBarLayout = index;

	if (configChannelTitleBarLayout == DOCK_VERTICAL)
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

void MainWindow::updateZoomFactors(float value)
{
	static int lock = 0;

	/* TODO: There must be a better way to do this. */
	if (lock == 1)
		return;

	lock = 1;
	for (int i = 0; i < getNumChannels(); ++i) {
		// qDebug("updating zoomFactor %d", i);
		channelForms[i]->setZoomFactor(value);
		// qDebug("updating zoomFactor %d (DONE)", i);
	}
	lock = 0;
}

void MainWindow::on_actionConfigure_triggered()
{
	DecodersForm *form = new DecodersForm();
	form->show();
}
