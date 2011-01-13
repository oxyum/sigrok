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
	zoomFactor = 32.0;
	scrollBarValue = 0;
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
	double old_x, current_x, step;
	int current_y, oldval, newval, x_change_visible;
	int low = m_ui->renderAreaWidget->height() - 2, high = 20;
	int ch = getChannelNumber();
	uint64_t ss, se;

	if (sample_buffer == NULL)
		return;

	delete painterPath;
	painterPath = new QPainterPath();

	ss = getNumSamples() * ((double)getScrollBarValue() / (double)100);
	se = ss + getNumSamples() / getZoomFactor();
	step = (double)width() / (double)(se - ss);
	if (se > getNumSamples()) /* Do this _after_ calculating 'step'! */
		se = getNumSamples();

	old_x = current_x = 0;
	oldval = getbit(sample_buffer, 0, ch);
	current_y = (oldval) ? high : low;
	painterPath->moveTo(current_x, current_y);

	// qDebug() << "generatePainterPath() for ch" << getChannelNumber()
	// 	 << "(" << ss << " - " << se << ")";

	for (uint64_t i = ss + 1; i < se; ++i) {
		current_x += step;
		newval = getbit(sample_buffer, i, ch);
		x_change_visible = (uint64_t)current_x > (uint64_t)old_x;
		if (oldval != newval && x_change_visible) {
			painterPath->lineTo(current_x, current_y);
			current_y = (newval) ? high : low;
			painterPath->lineTo(current_x, current_y);
			old_x = current_x;
			oldval = newval;
		}
	}
	painterPath->lineTo(current_x, current_y);

	/* Force a redraw. */
	update();
}

void ChannelForm::resizeEvent(QResizeEvent *event)
{
	/* Avoid compiler warnings. */
	event = event;

	/* Quick hack to force a redraw upon resize. */
	generatePainterPath();
}

void ChannelForm::paintEvent(QPaintEvent *event)
{
	// qDebug() << "Paint event on ch" << getChannelNumber();

	// QPainter p(m_ui->renderAreaWidget);
	QPainter p(this);

	/* Avoid compiler warnings. */
	event = event;

	if (sample_buffer == NULL)
		return;

	QPen pen(getChannelColor(), 1, Qt::SolidLine, Qt::SquareCap,
		 Qt::BevelJoin);
	p.setPen(pen);

	// p.fillRect(0, 0, this->width(), this->height(), QColor(Qt::gray));
	p.setRenderHint(QPainter::Antialiasing, false);

	// p.scale(getZoomFactor(), 1.0);
	p.drawPath(*painterPath);

	for (int i = 0; i < width(); i += width() / 100)
		p.drawLine(i, 12, i, 15);

	for (int i = width() / 10; i < width(); i += width() / 10) {
		p.drawText(i, 10, QString::number(i));
		p.drawLine(i, 11, i, 17);
	}
}

void ChannelForm::wheelEvent(QWheelEvent *event)
{
	float zoomFactorNew;

	if ((event->delta() / WHEEL_DELTA) == 1)
		zoomFactorNew = getZoomFactor() * 2;
	else if ((event->delta() / WHEEL_DELTA) == -1)
		zoomFactorNew = getZoomFactor() / 2;
	else
		zoomFactorNew = getZoomFactor();

	if (zoomFactorNew < 1)
		zoomFactorNew = 1;

	setZoomFactor(zoomFactorNew);

	/* TODO: Config option to scroll (instead of zoom) via the wheel. */
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
