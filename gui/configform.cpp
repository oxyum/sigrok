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

#include "configform.h"
#include "ui_configform.h"
#include "mainwindow.h"

ConfigForm::ConfigForm(QWidget *parent) :
	QWidget(parent),
	m_ui(new Ui::ConfigForm)
{
	m_ui->setupUi(this);
}

ConfigForm::~ConfigForm()
{
	delete m_ui;
}

void ConfigForm::changeEvent(QEvent *e)
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

void ConfigForm::on_closeButton_clicked()
{
	close();
}

void ConfigForm::on_listWidget_currentItemChanged(QListWidgetItem *current,
						  QListWidgetItem *previous)
{
	if (!current)
		current = previous;

	m_ui->stackedWidget->setCurrentIndex(m_ui->listWidget->row(current));
}

void ConfigForm::on_channelTitleBarLayoutComboBox_currentIndexChanged(int index)
{
	// FIXME
	emit w->configChannelTitleBarLayoutChanged(index);
}
