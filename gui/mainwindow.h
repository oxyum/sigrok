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

#ifndef SIGROK_MAINWINDOW_H
#define SIGROK_MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QLineEdit>
#include <QDockWidget>
#include <QGridLayout>
#include <QScrollBar>
#include "channelform.h"

extern uint8_t *sample_buffer;

namespace Ui
{
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

	void setCurrentLA(int la);
	int getCurrentLA(void);
	void setNumChannels(int ch);
	int getNumChannels(void);
	void setSampleRate(uint64_t s);
	uint64_t getSampleRate(void);
	void setNumSamples(uint64_t s);
	uint64_t getNumSamples(void);

	/* TODO: Don't hardcode maximum number of channels. */
#define NUMCHANNELS 64

	/* FIXME */
	QDockWidget *dockWidgets[NUMCHANNELS];
	ChannelForm *channelForms[NUMCHANNELS];

	void setupDockWidgets(void);

private:
	Ui::MainWindow *ui;
	int currentLA;
	int numChannels;
	uint64_t sampleRate;
	uint64_t numSamples;
	int configChannelTitleBarLayout;

public slots:
	void configChannelTitleBarLayoutChanged(int index);

private slots:
	void on_actionConfigure_triggered();
	void on_action_New_triggered();
	void on_action_Get_samples_triggered();
	void on_action_Save_as_triggered();
	void on_action_Open_triggered();
	void on_actionScan_triggered();
	void on_actionPreferences_triggered();
	void on_actionAbout_Qt_triggered();
	void on_actionAbout_triggered();
	void updateScrollBars(int value);
	void updateZoomFactors(float value);
};

extern MainWindow *w;

#endif
