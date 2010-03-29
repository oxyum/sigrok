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

ChannelForm::ChannelForm(QWidget *parent) :
	QWidget(parent),
	m_ui(new Ui::ChannelForm)
{
	m_ui->setupUi(this);

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
