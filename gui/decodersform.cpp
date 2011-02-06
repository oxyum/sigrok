/*
 * This file is part of the sigrok project.
 *
 * Copyright (C) 2011 Uwe Hermann <uwe@hermann-uwe.de>
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

extern "C" {
#include <sigrokdecode.h>
#include <glib.h>
}

#include <QLabel>
#include "decodersform.h"
#include "ui_decodersform.h"

DecodersForm::DecodersForm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DecodersForm)
{
	int i;
	GSList *l;
	struct srd_decoder *dec;
	QWidget *pages[MAX_NUM_DECODERS];

	ui->setupUi(this);

	for (l = srd_list_decoders(), i = 0; l; l = l->next, ++i) {
		dec = (struct srd_decoder *)l->data;

		/* Add the decoder to the list. */
		new QListWidgetItem(QString(dec->id), ui->listWidget);

		/* Add a page for the decoder details. */
		pages[i] = new QLabel(QString("Test %1").arg(i));

		/* Add the widget as "Details" page for the current decoder. */
		ui->stackedWidget->addWidget(pages[i]);
	}
}

DecodersForm::~DecodersForm()
{
	delete ui;
}

void DecodersForm::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void DecodersForm::on_closeButton_clicked()
{
	close();
}

void DecodersForm::on_listWidget_currentItemChanged(QListWidgetItem *current,
						    QListWidgetItem *previous)
{
	if (!current)
		current = previous;

	ui->stackedWidget->setCurrentIndex(ui->listWidget->row(current));
}
