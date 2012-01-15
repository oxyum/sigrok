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
	GSList *ll, *ll2;
	struct srd_decoder *dec;
	QWidget *pages[MAX_NUM_DECODERS];
	char **ann, *doc;
	QString *s;

	ui->setupUi(this);

	for (ll = srd_list_decoders(), i = 0; ll; ll = ll->next, ++i) {
		dec = (struct srd_decoder *)ll->data;

		/* Add the decoder to the list. */
		new QListWidgetItem(QString(dec->name), ui->listWidget);

		/* Add a page for the decoder details. */
		pages[i] = new QWidget;

		/* Add some decoder data to that page. */
		QVBoxLayout *l = new QVBoxLayout;
		l->addWidget(new QLabel("ID: " + QString(dec->id)));
		l->addWidget(new QLabel("Name: " + QString(dec->name)));
		l->addWidget(new QLabel("Long name: " + QString(dec->longname)));
		l->addWidget(new QLabel("Description: " + QString(dec->desc)));
		l->addWidget(new QLabel("License: " + QString(dec->license)));
		s = new QString("Annotations:\n");
		for (ll2 = dec->annotations; ll2; ll2 = ll2->next) {
			ann = (char **)ll2->data;
			s->append(QString(" - %1: %2\n").arg(ann[0]).arg(ann[1]));
		}
		l->addWidget(new QLabel(*s));
		s = new QString("Protocol documentation:\n");
		if (doc = srd_decoder_doc(dec)) {
			s->append(QString("%1\n")
			          .arg(doc[0] == '\n' ? doc + 1 : doc));
			g_free(doc);
		} /* else: Error. */
		l->addWidget(new QLabel(*s));

		l->insertStretch(-1);

		pages[i]->setLayout(l);

		/* Add the decoder's page to the stackedWidget. */
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
