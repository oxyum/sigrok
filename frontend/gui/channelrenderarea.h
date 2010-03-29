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

#ifndef SIGROK_CHANNELRENDERAREA_H
#define SIGROK_CHANNELRENDERAREA_H

#include <QWidget>
#include <QSize>
#include <QColor>
#include <QWheelEvent>
#include <stdint.h>

class ChannelRenderArea : public QWidget
{
Q_OBJECT

public:
	explicit ChannelRenderArea(QWidget *parent = 0);

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	void setChannelColor(QColor color);
	QColor getChannelColor(void);
	void setChannelNumber(int ch);
	int getChannelNumber(void);

	void setNumSamples(uint64_t s);
	uint64_t getNumSamples(void);
	uint64_t getSampleStart(void);
	uint64_t getSampleEnd(void);
	float getZoomFactor(void);

protected:
	void resizeEvent(QResizeEvent *event);
	void paintEvent(QPaintEvent *event);
	void wheelEvent(QWheelEvent *event);

signals:
	void sampleStartChanged(uint64_t);
	void sampleStartChanged(QString);
	void sampleEndChanged(uint64_t);
	void sampleEndChanged(QString);
	void zoomFactorChanged(float);
	void zoomFactorChanged(QString);

public slots:
	void setSampleStart(uint64_t s);
	void setSampleEnd(uint64_t s);
	void setZoomFactor(float z);
	void generatePainterPath(void);
	void setScrollBarValue(int value);

private:
	int channelNumber;
	QColor channelColor;
	uint64_t sampleStart;
	uint64_t sampleEnd;
	uint64_t numSamples;
	float zoomFactor;
	QPainterPath *painterPath;

};

#endif
