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

#include "channelform.h"
#include "ui_channelform.h"
#include <stdint.h>

extern uint8_t *sample_buffer;

/* WHEEL_DELTA was introduced in Qt 4.6, earlier versions don't have it. */
#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif

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
	zoomFactor = 1.0;
	painterPath = new QPainterPath();

	/* Set random colors for the channel names (for now). */
	/* TODO: Channel graph color vs. label color? */
	channelColor = QColor(2 + qrand() * 16);

	/* Set title and color of the channel name QLineEdit. */
	QLineEdit *l = m_ui->channelLineEdit;
	l->setText(QString(tr("Channel %1")).arg(0 /* i */)); // FIXME
	QPalette p = QPalette(QApplication::palette());
	p.setColor(QPalette::Base, channelColor);
	l->setPalette(p);
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
	int current_x, current_y, oldval, newval;
	int low = m_ui->renderAreaWidget->height() - 2, high = 2;
	int ch = getChannelNumber();
	uint64_t ss = getSampleStart(), se = getSampleEnd();

	if (sample_buffer == NULL)
		return;

	delete painterPath;
	painterPath = new QPainterPath();

	current_x = 0;
	oldval = getbit(sample_buffer, 0, ch);
	current_y = (oldval) ? high : low;
	painterPath->moveTo(current_x, current_y);

	for (uint64_t i = ss + 1; i < se; ++i) {
		current_x += 2;
		newval = getbit(sample_buffer, i, ch);
		if (oldval != newval) {
			painterPath->lineTo(current_x, current_y);
			current_y = (newval) ? high : low;
			painterPath->lineTo(current_x, current_y);
			oldval = newval;
		}
	}
	painterPath->lineTo(current_x, current_y);

	/* Force a redraw. */
	this->update();
}

void ChannelForm::resizeEvent(QResizeEvent *event)
{
	/* Quick hack to prevent compiler warning. */
	event = event;

	/* Quick hack to force a redraw upon resize. */
	generatePainterPath();
}

void ChannelForm::paintEvent(QPaintEvent *event)
{
	// QPainter p(m_ui->renderAreaWidget);
	QPainter p(this);

	/* Quick hack to prevent compiler warning. */
	event = event;

	if (sample_buffer == NULL)
		return;

	QPen pen(getChannelColor(), 1, Qt::SolidLine, Qt::SquareCap,
		 Qt::BevelJoin);
	p.setPen(pen);

	// p.fillRect(0, 0, this->width(), this->height(), QColor(Qt::gray));
	// p.setRenderHint(QPainter::Antialiasing, false);

	// p.scale(getZoomFactor(), 1.0);
	p.drawPath(*painterPath);
}

void ChannelForm::wheelEvent(QWheelEvent *event)
{
	uint64_t sampleStartNew, sampleEndNew;
	float zoomFactorNew;

	/* FIXME: Make this constant user-configurable. */
	zoomFactorNew = getZoomFactor()
			+ 0.01 * (event->delta() / WHEEL_DELTA);
	if (zoomFactorNew < 0)
		zoomFactorNew = 0;
	if (zoomFactorNew > 2)
		zoomFactorNew = 2; /* FIXME: Don't hardcode. */
	setZoomFactor(zoomFactorNew);

	sampleStartNew = 0; /* FIXME */
	sampleEndNew = getNumSamples() * zoomFactorNew;
	if (sampleEndNew > getNumSamples())
		sampleEndNew = getNumSamples();

	setSampleStart(sampleStartNew);
	setSampleEnd(sampleEndNew);

#if 0
	sampleStartNew = getSampleStart() + event->delta() / WHEEL_DELTA;
	sampleEndNew = getSampleEnd() + event->delta() / WHEEL_DELTA;

	/* TODO: More checks. */

#if 1
	if (sampleStartNew < 0 || sampleEndNew < 0)
		return;
	if (sampleStartNew > 512 * 1000 || sampleEndNew > 512 * 1000 /* FIXME */)
		return;
#endif

	setSampleStart(sampleStartNew);
	setSampleEnd(sampleEndNew); /* FIXME: Use len? */
#endif

	repaint();
}

void ChannelForm::setChannelColor(QColor color)
{
	channelColor = color;
}

QColor ChannelForm::getChannelColor(void)
{
	return channelColor;
}

void ChannelForm::setChannelNumber(int c)
{
	channelNumber = c;
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

void ChannelForm::setZoomFactor(float z)
{
	zoomFactor = z;

	emit(zoomFactorChanged(zoomFactor));
	emit(zoomFactorChanged(QString::number(zoomFactor)));
}

float ChannelForm::getZoomFactor(void)
{
	return zoomFactor;
}

void ChannelForm::setScrollBarValue(int value)
{
	double mul = 0;
	uint64_t newSampleStart, newSampleEnd;

	mul = (double)value / (double)99;

	newSampleStart = getNumSamples() * mul;
	if (newSampleStart >= 100)
		newSampleStart -= 100;
	newSampleEnd = newSampleStart + 99; /* FIXME */
	if (newSampleEnd > getNumSamples())
		newSampleEnd = getNumSamples();

	setSampleStart(newSampleStart);
	setSampleEnd(newSampleEnd); /* FIXME */
	repaint();
}
