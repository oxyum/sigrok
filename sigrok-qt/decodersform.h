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

#ifndef DECODERSFORM_H
#define DECODERSFORM_H

#include <QDialog>
#include <QListWidgetItem>

/* TODO: Don't hardcode maximum number of decoders. */
#define MAX_NUM_DECODERS 64

namespace Ui {
	class DecodersForm;
}

class DecodersForm : public QDialog {
	Q_OBJECT
public:
	DecodersForm(QWidget *parent = 0);
	~DecodersForm();

protected:
	void changeEvent(QEvent *e);

private:
	Ui::DecodersForm *ui;

private slots:
	void on_listWidget_currentItemChanged(QListWidgetItem *current,
					      QListWidgetItem *previous);
	void on_closeButton_clicked();
};

#endif
