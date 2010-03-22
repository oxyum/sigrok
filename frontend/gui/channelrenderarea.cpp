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

#include <QPainter>
#include <QPen>
#include <QColor>
#include "channelrenderarea.h"
#include "mainwindow.h"

#ifdef __cplusplus
extern "C" {
#endif

/* __STDC_FORMAT_MACROS is required for PRIu64 and friends (in C++). */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdint.h>
#include <sigrok.h>

#ifdef __cplusplus
}
#endif

/* TODO: Should move elsewhere. */
int getbit(uint8_t *buf, int numbyte, int chan)
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

/* WHEEL_DELTA was introduced in Qt 4.6, earlier versions don't have it. */
#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif

extern uint8_t *sample_buffer;

ChannelRenderArea::ChannelRenderArea(QWidget *parent) : QWidget(parent)
{
	channelNumber = -1;
	channelColor = Qt::black;
	numSamples = 0;
	sampleStart = 0;
	sampleEnd = 0;
	zoomFactor = 1.0;
	painterPath = new QPainterPath();
}

QSize ChannelRenderArea::minimumSizeHint() const
{
	return QSize(300, 10);
	// return QSize(Qt::PreferredSize, Qt::PreferredSize);
}

QSize ChannelRenderArea::sizeHint() const
{
	return QSize(300, 10);
	// return QSize(Qt::PreferredSize, Qt::PreferredSize);
}

void ChannelRenderArea::generatePainterPath(void)
{
	int current_x, current_y, oldval, newval;
	int low = this->height() - 2, high = 2, ch = getChannelNumber();
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

void ChannelRenderArea::resizeEvent(QResizeEvent *event)
{
	/* Quick hack to prevent compiler warning. */
	event = event;

	/* Quick hack to force a redraw upon resize. */
	generatePainterPath();
}

void ChannelRenderArea::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);

	/* Quick hack to prevent compiler warning. */
	event = event;

	if (sample_buffer == NULL)
		return;

	/* TODO: Use Qt::black etc. */
	QPen pen(this->getChannelColor(), 1, Qt::SolidLine, Qt::SquareCap,
		 Qt::BevelJoin);
	painter.setPen(pen);

	// painter.fillRect(0, 0, this->width(), this->height(), QColor(Qt::gray));
	// painter.setRenderHint(QPainter::Antialiasing, false);

	painter.drawPath(*painterPath);

	// painter.scale(getZoomFactor(), getZoomFactor());
}

void ChannelRenderArea::wheelEvent(QWheelEvent *event)
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
	uint64_t sampleStartNew, sampleEndNew;

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

void ChannelRenderArea::setChannelColor(QColor color)
{
	channelColor = color;
}

QColor ChannelRenderArea::getChannelColor(void)
{
	return channelColor;
}

void ChannelRenderArea::setChannelNumber(int ch)
{
	channelNumber = ch;
}

int ChannelRenderArea::getChannelNumber(void)
{
	return channelNumber;
}

void ChannelRenderArea::setNumSamples(uint64_t s)
{
	numSamples = s;
}

uint64_t ChannelRenderArea::getNumSamples(void)
{
	return numSamples;
}

void ChannelRenderArea::setSampleStart(uint64_t s)
{
	QString str;

	sampleStart = s;

	emit(sampleStartChanged(sampleStart));
	emit(sampleStartChanged(QString::number(sampleStart)));
}

uint64_t ChannelRenderArea::getSampleStart(void)
{
	return sampleStart;
}

void ChannelRenderArea::setSampleEnd(uint64_t e)
{
	QString str;

	sampleEnd = e;

	emit(sampleEndChanged(sampleEnd));
	emit(sampleEndChanged(QString::number(sampleEnd)));
}

uint64_t ChannelRenderArea::getSampleEnd(void)
{
	return sampleEnd;
}

void ChannelRenderArea::setZoomFactor(float z)
{
	QString str;

	zoomFactor = z;

	emit(zoomFactorChanged(zoomFactor));
	emit(zoomFactorChanged(QString::number(zoomFactor)));
}

float ChannelRenderArea::getZoomFactor(void)
{
	return zoomFactor;
}
