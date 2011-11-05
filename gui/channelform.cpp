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
#include "channelform.h"
#include "ui_channelform.h"
#include <stdint.h>

extern uint8_t *sample_buffer;

/* WHEEL_DELTA was introduced in Qt 4.6, earlier versions don't have it. */
#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif

static QColor channelColors[] = {
	QColor(0x00, 0x00, 0x00), /* Black */
	QColor(0x96, 0x4B, 0x00), /* Brown */
	QColor(0xFF, 0x00, 0x00), /* Red */
	QColor(0xFF, 0xA5, 0x00), /* Orange */
	QColor(0xFF, 0xFF, 0x00), /* Yellow */
	QColor(0x9A, 0xCD, 0x32), /* Green */
	QColor(0x64, 0x95, 0xED), /* Blue */
	QColor(0xEE, 0x82, 0xEE), /* Violet */
	QColor(0xA0, 0xA0, 0xA0), /* Gray */
	QColor(0xFF, 0xFF, 0xFF), /* White */
};

/* TODO: Should move elsewhere. */
static int getbit(uint8_t *buf, int numbyte, int chan)
{
	if (chan < 8) {
		return ((buf[numbyte] & (1 << chan))) >> chan;
	} else if (chan >= 8 && chan < 16) {
		return ((buf[numbyte + 1] & (1 << (chan - 8)))) >> (chan - 8);
	} else {
		/* Error. Currently only 8bit and 16bit LAs are supported. */
		return -1;
	}
}

ChannelForm::ChannelForm(QWidget *parent) :
	QWidget(parent),
	m_ui(new Ui::ChannelForm)
{
	m_ui->setupUi(this);

	channelNumber = -1;
	numSamples = 0;
	sampleStart = 0;
	sampleEnd = 0;
	scaleFactor = 2.0;
	scrollBarValue = 0;
	painterPath = new QPainterPath();
}

ChannelForm::~ChannelForm()
{
	delete m_ui;
}

void ChannelForm::changeEvent(QEvent *e)
{
	QWidget::changeEvent(e);

	switch (e->type()) {
	case QEvent::LanguageChange:
		m_ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void ChannelForm::generatePainterPath(void)
{
	int scaleFactor;
	double old_x, current_x;
	int current_y, oldval, newval, x_change_visible;
	int low = m_ui->renderAreaWidget->height() - 2, high = 20;
	int ch = getChannelNumber();
	uint64_t ss, se;

	if (sample_buffer == NULL)
		return;

	scaleFactor = getScaleFactor();

	delete painterPath;
	painterPath = new QPainterPath();

	old_x = current_x = (-getScrollBarValue() % stepSize);

	ss = (getScrollBarValue() + current_x) * scaleFactor / stepSize;
	se = ss + (getScaleFactor() * width()) * stepSize;
	if (se > getNumSamples()) /* Do this _after_ calculating 'step'! */
		se = getNumSamples();

	oldval = getbit(sample_buffer, ss, ch);
	current_y = (oldval) ? high : low;
	painterPath->moveTo(current_x, current_y);

	// qDebug() << "generatePainterPath() for ch" << getChannelNumber()
	// 	 << "(" << ss << " - " << se << ")";

	for (uint64_t i = ss; i < se; i += scaleFactor) {
		/* Process the samples shown in this step. */
		for(uint64_t j = 0; (j<scaleFactor) && (i+j < se); j++) {
			newval = getbit(sample_buffer, i + j, ch);
			x_change_visible = current_x > old_x;
			if (oldval != newval && x_change_visible) {
				painterPath->lineTo(current_x, current_y);
				current_y = (newval) ? high : low;
				painterPath->lineTo(current_x, current_y);
				old_x = current_x;
				oldval = newval;
			}
			current_x += (double)stepSize / (double)scaleFactor;
		}
	}
	current_x += stepSize;
	painterPath->lineTo(current_x, current_y);

	/* Force a redraw. */
	update();
}

void ChannelForm::resizeEvent(QResizeEvent *event)
{
	/* Avoid compiler warnings. */
	event = event;

	stepSize = width() / 100;

	if (stepSize <= 1)
		stepSize = width() / 50;

	if (stepSize <= 1)
		stepSize = width() / 20;

	/* Quick hack to force a redraw upon resize. */
	generatePainterPath();
}

void ChannelForm::paintEvent(QPaintEvent *event)
{
	int tickStart;
	// qDebug() << "Paint event on ch" << getChannelNumber();

	// QPainter p(m_ui->renderAreaWidget);
	QPainter p(this);

	/* Avoid compiler warnings. */
	event = event;

	if (sample_buffer == NULL)
		return;

	QPen penChannel(getChannelColor(), 1, Qt::SolidLine, Qt::SquareCap,
			Qt::BevelJoin);
	p.setPen(penChannel);
	p.fillRect(0, 0, this->width(), 5, getChannelColor());
	p.fillRect(0, 5, 5, this->height(), getChannelColor());
	p.translate(0, 5);

	QPen penGraph(QColor(0, 0, 0), 1, Qt::SolidLine, Qt::SquareCap,
		 Qt::BevelJoin);
	p.setPen(penGraph);

	// p.fillRect(0, 0, this->width(), this->height(), QColor(Qt::gray));
	p.setRenderHint(QPainter::Antialiasing, false);

	// p.scale(getZoomFactor(), 1.0);
	p.drawPath(*painterPath);

	if (stepSize > 0) {
		if (stepSize > 1) {
			/* Draw minor ticks. */
			tickStart = -getScrollBarValue() % stepSize;
			for (int i = tickStart; i < width(); i += stepSize)
				p.drawLine(i, 12, i, 15);
		}

		/* Draw major ticks every 10 minor tick. */
		tickStart = -getScrollBarValue() % (stepSize*10);
		for (int i = tickStart; i < width(); i += stepSize * 10) {
			p.drawText(i, 10, QString::number((i+getScrollBarValue())*getScaleFactor()));
			p.drawLine(i, 11, i, 17);
		}
	}
}

void ChannelForm::wheelEvent(QWheelEvent *event)
{
	float scaleFactorNew;

	if ((event->delta() / WHEEL_DELTA) == 1)
		scaleFactorNew = getScaleFactor() * 2;
	else if ((event->delta() / WHEEL_DELTA) == -1)
		scaleFactorNew = getScaleFactor() / 2;
	else
		scaleFactorNew = getScaleFactor();

	if (scaleFactorNew < 1)
		scaleFactorNew = 1;

	setScaleFactor(scaleFactorNew);

	/* TODO: Config option to scroll (instead of scale) via the wheel. */
}

void ChannelForm::setChannelColor(QColor color)
{
	channelColor = color;

	/* Set color of the channel name QLineEdit. */
	QLineEdit *l = m_ui->channelLineEdit;
	QPalette p = QPalette(QApplication::palette());
	p.setColor(QPalette::Base, channelColor);
	l->setPalette(p);
}

QColor ChannelForm::getChannelColor(void)
{
	return channelColor;
}

void ChannelForm::setChannelNumber(int c)
{
	if (channelNumber < 0) {
		/* Set a default color for this channel. */
		/* FIXME: Channel color should be dependent on the selected LA. */
		setChannelColor(channelColors[c % (sizeof(channelColors) / sizeof(channelColors[0]))]);
	}
	channelNumber = c;

	/* Set title of the channel name QLineEdit. */
	QLineEdit *l = m_ui->channelLineEdit;
	l->setText(QString(tr("Channel %1")).arg(channelNumber));
}

int ChannelForm::getChannelNumber(void)
{
	return channelNumber;
}

void ChannelForm::setNumSamples(uint64_t s)
{
	numSamples = s;
}

uint64_t ChannelForm::getNumSamples(void)
{
	return numSamples;
}

void ChannelForm::setSampleStart(uint64_t s)
{
	sampleStart = s;

	emit(sampleStartChanged(sampleStart));
	emit(sampleStartChanged(QString::number(sampleStart)));
}

uint64_t ChannelForm::getSampleStart(void)
{
	return sampleStart;
}

void ChannelForm::setSampleEnd(uint64_t e)
{
	sampleEnd = e;

	emit(sampleEndChanged(sampleEnd));
	emit(sampleEndChanged(QString::number(sampleEnd)));
}

uint64_t ChannelForm::getSampleEnd(void)
{
	return sampleEnd;
}

void ChannelForm::setScaleFactor(float z)
{
	scaleFactor = z;

	emit(scaleFactorChanged(scaleFactor));
	emit(scaleFactorChanged(QString::number(scaleFactor)));
}

float ChannelForm::getScaleFactor(void)
{
	return scaleFactor;
}

int ChannelForm::getScrollBarValue(void)
{
	return scrollBarValue;
}

void ChannelForm::setScrollBarValue(int value)
{
	if (scrollBarValue == value)
		return;

	// qDebug("Re-generating ch%d (value = %d)", getChannelNumber(), value);

	scrollBarValue = value;
	generatePainterPath();
}
