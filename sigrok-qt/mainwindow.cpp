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
#include "decoderstackform.h"
#include "ui_decoderstackform.h"

extern "C" {
/* __STDC_FORMAT_MACROS is required for PRIu64 and friends (in C++). */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdint.h>
#include <stdarg.h>
#include <glib.h>
#include <libsigrok/libsigrok.h>
#include <sigrokdecode.h>
}

#define DOCK_VERTICAL   0
#define DOCK_HORIZONTAL 1

uint64_t limit_samples = 0; /* FIXME */

QProgressDialog *progress = NULL;

/* TODO: Documentation. */
extern "C" {
static int logger(void *cb_data, int loglevel, const char *format, va_list args)
{
	QString s;

	if (loglevel > srd_log_loglevel_get())
		return SRD_OK;

	s.vsprintf(format, args);

	MainWindow *mw = (MainWindow *)cb_data;
	mw->ui->plainTextEdit->appendPlainText(QString("srd: ").append(s));

	return SRD_OK;
}
}

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

	srd_log_loglevel_set(SRD_LOG_SPEW);

	if (srd_log_callback_set(logger, (void *)this) != SRD_OK) {
		qDebug() << "ERROR: srd_log_handler_set() failed.";
		return; /* TODO? */
	}
	qDebug() << "srd_log_handler_set() call successful.";

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
			SIGNAL(scaleFactorChanged(QString)),
			ui->labelScaleFactor, SLOT(setText(QString)));

		/* Redraw channels upon changes. */
		QObject::connect(channelForms[i],
			SIGNAL(sampleStartChanged(QString)),
			channelForms[i], SLOT(generatePainterPath()));
		QObject::connect(channelForms[i],
			SIGNAL(sampleEndChanged(QString)),
			channelForms[i], SLOT(generatePainterPath()));
		QObject::connect(channelForms[i],
			SIGNAL(scaleFactorChanged(QString)),
			channelForms[i], SLOT(generatePainterPath()));

		// dockWidgets[i]->show();

#if 0
		/* If the user renames a channel, adapt the dock title. */
		QObject::connect(lineEdits[i], SIGNAL(textChanged(QString)),
				 dockWidgets[i], SLOT(setWindowTitle(QString)));
#endif
	}

	/* For now, display only one scrollbar which scrolls all channels. */
	QDockWidget* scrollWidget = new QDockWidget(this);
	scrollWidget->setAllowedAreas(Qt::BottomDockWidgetArea);
	horizontalScrollBar = new QScrollBar(this);
	horizontalScrollBar->setOrientation(Qt::Horizontal);
	
	QDockWidget::DockWidgetFeatures f;
	if (configChannelTitleBarLayout == DOCK_VERTICAL)
		f |= QDockWidget::DockWidgetVerticalTitleBar;
	scrollWidget->setFeatures(f);
	scrollWidget->setWidget(horizontalScrollBar);
	addDockWidget(Qt::BottomDockWidgetArea, scrollWidget,
			      Qt::Vertical);
	
	for (int i = 0; i < getNumChannels(); ++i) {
		/* The scrollbar scrolls all channels. */
		connect(horizontalScrollBar, SIGNAL(valueChanged(int)),
				channelForms[i], SLOT(setScrollBarValue(int)));
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
	GSList *l;
	struct sr_dev_driver **drivers;
	struct sr_input_format **inputs;
	struct sr_output_format **outputs;
	struct srd_decoder *dec;

	QString s = tr("%1 %2<br />%3<br /><a href=\"%4\">%4</a>\n<p>")
		.arg(QApplication::applicationName())
		.arg(QApplication::applicationVersion())
		.arg(tr("GNU GPL, version 2 or later"))
		.arg(QApplication::organizationDomain());

	s.append("<b>" + tr("Supported hardware drivers:") + "</b><table>");
	drivers = sr_driver_list();
	for (int i = 0; drivers[i]; ++i) {
		s.append(QString("<tr><td><i>%1</i></td><td>%2</td></tr>")
			 .arg(QString(drivers[i]->name))
			 .arg(QString(drivers[i]->longname)));
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
	for (l = srd_decoder_list(); l; l = l->next) {
		dec = (struct srd_decoder *)l->data;
		s.append(QString("<tr><td><i>%1</i></td><td>%2</td></tr>")
			 .arg(QString(dec->id))
			 .arg(QString(dec->longname)));
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
	GSList *devs = NULL;
	int num_devs, pos;
	struct sr_dev *dev;
	char *di_num_probes, *str;
	struct sr_samplerates *samplerates;
	const static float mult[] = { 2.f, 2.5f, 2.f };

	statusBar()->showMessage(tr("Scanning for logic analyzers..."), 2000);

	sr_dev_scan();
	devs = sr_dev_list();
	num_devs = g_slist_length(devs);

	ui->comboBoxLA->clear();
	for (int i = 0; i < num_devs; ++i) {
		dev = (struct sr_dev *)g_slist_nth_data(devs, i);
		ui->comboBoxLA->addItem(dev->driver->name); /* TODO: Full name */
	}

	if (num_devs == 0) {
		s = tr("No supported logic analyzer found.");
		statusBar()->showMessage(s, 2000);
		return;
	} else if (num_devs == 1) {
		s = tr("Found supported logic analyzer: ");
		s.append(dev->driver->name);
		statusBar()->showMessage(s, 2000);
	} else {
		/* TODO: Allow user to select one of the devices. */
		s = tr("Found multiple logic analyzers: ");
		for (int i = 0; i < num_devs; ++i) {
			dev = (struct sr_dev *)g_slist_nth_data(devs, i);
			s.append(dev->driver->name);
			if (i != num_devs - 1)
				s.append(", ");
		}
		statusBar()->showMessage(s, 2000);
		return;
	}

	dev = (struct sr_dev *)g_slist_nth_data(devs, 0 /* opt_dev */);

	setCurrentLA(0 /* TODO */);

	di_num_probes = (char *)dev->driver->dev_info_get(
			dev->driver_index, SR_DI_NUM_PROBES);
	if (di_num_probes != NULL) {
		setNumChannels(GPOINTER_TO_INT(di_num_probes));
	} else {
		setNumChannels(8); /* FIXME: Error handling. */
	}

	ui->comboBoxLA->clear();
	ui->comboBoxLA->addItem(dev->driver->name); /* TODO: Full name */

	s = QString(tr("Channels: %1")).arg(getNumChannels());
	ui->labelChannels->setText(s);

	samplerates = (struct sr_samplerates *)dev->driver->dev_info_get(
		      dev->driver_index, SR_DI_SAMPLERATES);
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

	ui->labelScaleFactor->setText(tr("Scale factor: "));
	ui->labelScaleFactor->setEnabled(true);

	ui->action_Save_as->setEnabled(true);
	ui->action_Get_samples->setEnabled(false);

	for (int i = 0; i < getNumChannels(); ++i) {
		channelForms[i]->setNumSamples(file.size());

		channelForms[i]->update();
	}
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

void datafeed_in(struct sr_dev *dev, struct sr_datafeed_packet *packet)
{
	static int num_probes = 0;
	static int logic_probelist[SR_MAX_NUM_PROBES + 1] = { 0 };
	static uint64_t received_samples = 0;
	static int triggered = 0;
	struct sr_probe *probe;
	struct sr_datafeed_header *header;
	struct sr_datafeed_meta_logic *meta_logic;
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
	case SR_DF_END:
		qDebug("SR_DF_END");
		/* TODO: o */
		sr_session_stop();
		progress->setValue(received_samples); /* FIXME */
		break;
	case SR_DF_TRIGGER:
		qDebug("SR_DF_TRIGGER");
		/* TODO */
		triggered = 1;
		break;
	case SR_DF_META_LOGIC:
		qDebug("SR_DF_META_LOGIC");
		meta_logic = (struct sr_datafeed_meta_logic *)packet->payload;
		num_probes = meta_logic->num_probes;
		num_enabled_probes = 0;
		for (int i = 0; i < meta_logic->num_probes; ++i) {
			probe = (struct sr_probe *)g_slist_nth_data(dev->probes, i);
			if (probe->enabled)
				logic_probelist[num_enabled_probes++] = probe->index;
		}

		qDebug() << "Acquisition with" << num_enabled_probes << "/"
			 << num_probes << "probes at"
			 << sr_samplerate_string(meta_logic->samplerate)
			 << "starting at" << ctime(&header->starttime.tv_sec)
			 << "(" << limit_samples << "samples)";

		/* TODO: realloc() */
		break;
	case SR_DF_LOGIC:
		logic = (sr_datafeed_logic *)packet->payload;
		qDebug() << "SR_DF_LOGIC (length =" << logic->length
			 << ", unitsize = " << logic->unitsize << ")";
		sample_size = logic->unitsize;

		if (sample_size == -1)
			break;
		
		/* Don't store any samples until triggered. */
		// if (opt_wait_trigger && !triggered)
		// 	return;
	
		if (received_samples >= limit_samples)
			break;
	
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
		break;
	default:
		qDebug("SR_DF_XXXX, not yet handled");
		break;
	}
}

void MainWindow::on_action_Get_samples_triggered()
{
	uint64_t samplerate;
	QString s;
	GSList *devs = NULL;
	int opt_dev;
	struct sr_dev *dev;
	QComboBox *n = ui->comboBoxNumSamples;

	opt_dev = 0; /* FIXME */

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

	devs = sr_dev_list();

	dev = (struct sr_dev *)g_slist_nth_data(devs, opt_dev);

	/* Set the number of samples we want to get from the device. */
	if (dev->driver->dev_config_set(dev->driver_index,
	    SR_HWCAP_LIMIT_SAMPLES, &limit_samples) != SR_OK) {
		qDebug("Failed to set sample limit.");
		sr_session_destroy();
		return;
	}

	if (sr_session_dev_add(dev) != SR_OK) {
		qDebug("Failed to use device.");
		sr_session_destroy();
		return;
	}

	/* Set the samplerate. */
	if (dev->driver->dev_config_set(dev->driver_index,
	    SR_HWCAP_SAMPLERATE, &samplerate) != SR_OK) {
		qDebug("Failed to set samplerate.");
		sr_session_destroy();
		return;
	};

	if (dev->driver->dev_config_set(dev->driver_index,
	    SR_HWCAP_PROBECONFIG, (char *)dev->probes) != SR_OK) {
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

	sr_session_stop();
	sr_session_destroy();

	for (int i = 0; i < getNumChannels(); ++i) {
		channelForms[i]->setNumSamples(limit_samples);
		// channelForms[i]->setSampleStart(0);
		// channelForms[i]->setSampleEnd(limit_samples);

		/* If any of the scale factors change, update all of them.. */
		connect(channelForms[i], SIGNAL(scaleFactorChanged(float)),
		        w, SLOT(updateScaleFactors(float)));

		channelForms[i]->generatePainterPath();
		// channelForms[i]->update();
	}

	setNumSamples(limit_samples);
	
	/* Enable the relevant labels/buttons. */
	ui->labelSampleStart->setEnabled(true);
	ui->labelSampleEnd->setEnabled(true);
	ui->labelScaleFactor->setEnabled(true);
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
	updateScrollBar();
}

void MainWindow::updateScrollBar(void)
{
	int stepSize = channelForms[0]->getStepSize();
	float scaleFactor = channelForms[0]->getScaleFactor();

	uint64_t viewport = channelForms[0]->getNumSamplesVisible() * stepSize / scaleFactor;
	uint64_t length = numSamples * stepSize / scaleFactor;

	horizontalScrollBar->setMinimum(0);
	horizontalScrollBar->setPageStep(viewport);
	if (viewport < length)
	{
		horizontalScrollBar->setMaximum(length - viewport / 2);
	} else {
		horizontalScrollBar->setMaximum(0);
	}

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

	ui->labelScaleFactor->setText(tr("Scale factor: "));
	ui->labelScaleFactor->setEnabled(false);

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


void MainWindow::updateScaleFactors(float value)
{
	static int lock = 0;

	/* TODO: There must be a better way to do this. */
	if (lock == 1)
		return;

	lock = 1;
	for (int i = 0; i < getNumChannels(); ++i) {
		// qDebug("updating scaleFactor %d", i);
		channelForms[i]->setScaleFactor(value);
		// qDebug("updating scaleFactor %d (DONE)", i);
	}
	updateScrollBar();
	lock = 0;
}

void MainWindow::on_actionConfigure_triggered()
{
	DecodersForm *form = new DecodersForm();
	form->show();
}

void MainWindow::on_actionProtocol_decoder_stacks_triggered()
{
	DecoderStackForm *form = new DecoderStackForm();
	form->show();
}

extern "C" void show_pd_annotation(struct srd_proto_data *pdata, void *cb_data)
{
	char **annotations;

	annotations = (char **)pdata->data;

	MainWindow *mw = (MainWindow *)cb_data;

	mw->ui->plainTextEdit->appendPlainText(
		QString("%1-%2: %3: %4").arg(pdata->start_sample)
			.arg(pdata->end_sample).arg(pdata->pdo->proto_id)
			.arg((char *)annotations[0]));
}

void MainWindow::on_actionQUICK_HACK_PD_TEST_triggered()
{
#define N 500000

	struct srd_decoder_inst *di;
	GHashTable *pd_opthash;
	uint8_t *buf = (uint8_t *)malloc(N + 1);

	pd_opthash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
					   g_free);

	/* Hardcode a specific I2C probe mapping. */
	g_hash_table_insert(pd_opthash, g_strdup("scl"), g_strdup("5"));
	g_hash_table_insert(pd_opthash, g_strdup("sda"), g_strdup("7"));

	/*
	 * Get data from a hardcoded binary file.
	 * (converted to binary from melexis_mlx90614_5s_24deg.sr.
	 */
	QFile file("foo.bin");
	int ret = file.open(QIODevice::ReadOnly);
	ret = file.read((char *)buf, N);

	// sr_log_loglevel_set(SR_LOG_NONE);
	// srd_log_loglevel_set(SRD_LOG_NONE);

	if (!(di = srd_inst_new("i2c", pd_opthash))) {
		ui->plainTextEdit->appendPlainText("ERROR: srd_inst_new");
		return;
	}

	if (srd_inst_probe_set_all(di, pd_opthash) != SRD_OK) {
		ui->plainTextEdit->appendPlainText("ERROR: srd_inst_set_probes");
		return;
	}

	if (srd_pd_output_callback_add(SRD_OUTPUT_ANN,
	    (srd_pd_output_callback_t)show_pd_annotation, (void *)this) != SRD_OK) {
		ui->plainTextEdit->appendPlainText("ERROR: srd_pd_output_callback_add");
		return;
	}

	if (srd_session_start(8, 1, 1000000) != SRD_OK) {
		ui->plainTextEdit->appendPlainText("ERROR: srd_session_start");
		return;
	}

	if (srd_session_send(0, buf, N) != SRD_OK) {
		ui->plainTextEdit->appendPlainText("ERROR: srd_session_send");
		return;
	}
}
