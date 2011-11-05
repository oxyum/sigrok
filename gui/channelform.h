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

#ifndef SIGROK_CHANNELFORM_H
#define SIGROK_CHANNELFORM_H

#include <QtGui/QWidget>
#include <QPainter>
#include <QPen>
#include <QWheelEvent>
#include <stdint.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

namespace Ui {
	class ChannelForm;
}

class ChannelForm : public QWidget {
	Q_OBJECT
public:
	ChannelForm(QWidget *parent = 0);
	~ChannelForm();
	Ui::ChannelForm *m_ui;
	void setChannelColor(QColor color);
	QColor getChannelColor(void);
	void setChannelNumber(int c);
	int getChannelNumber(void);
	void setNumSamples(uint64_t s);
	uint64_t getNumSamples(void);
	uint64_t getSampleStart(void);
	uint64_t getSampleEnd(void);
	float getScaleFactor(void);
	int getScrollBarValue(void);

public slots:
	void setSampleStart(uint64_t s);
	void setSampleEnd(uint64_t s);
	void setScaleFactor(float z);
	void generatePainterPath(void);
	void setScrollBarValue(int value);

signals:
	void sampleStartChanged(uint64_t);
	void sampleStartChanged(QString);
	void sampleEndChanged(uint64_t);
	void sampleEndChanged(QString);
	void scaleFactorChanged(float);
	void scaleFactorChanged(QString);

protected:
	void changeEvent(QEvent *e);
	void resizeEvent(QResizeEvent *event);
	void paintEvent(QPaintEvent *event);
	void wheelEvent(QWheelEvent *event);

private:
	QColor channelColor;
	int channelNumber;
	uint64_t sampleStart;
	uint64_t sampleEnd;
	uint64_t numSamples;
	float scaleFactor;
	QPainterPath *painterPath;
	// static int numTotalChannels;
	int scrollBarValue;
	int stepSize;
};

#endif
