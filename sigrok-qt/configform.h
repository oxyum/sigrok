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

#ifndef SIGROK_QT_CONFIGFORM_H
#define SIGROK_QT_CONFIGFORM_H

#include <QtGui/QWidget>
#include <QListWidgetItem>

namespace Ui {
	class ConfigForm;
}

class ConfigForm : public QWidget {
	Q_OBJECT
public:
	ConfigForm(QWidget *parent = 0);
	~ConfigForm();

protected:
	void changeEvent(QEvent *e);

private:
	Ui::ConfigForm *m_ui;

private slots:
	void on_channelTitleBarLayoutComboBox_currentIndexChanged(int index);
	void on_listWidget_currentItemChanged(QListWidgetItem *current,
					      QListWidgetItem *previous);
	void on_closeButton_clicked();
};

#endif
