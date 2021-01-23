/*
Decade
Copyright (c) 2019-2021 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Resource.h"

#include <QWidget>
#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLabel>

#include <string>
#include <vector>





class LicenseDialog : public QDialog
{
	Q_OBJECT

public:
	explicit LicenseDialog(QWidget* parent = nullptr) :
		QDialog(parent)
	{
		setMinimumSize(600, 400);
		//setContentsMargins(10, 10, 10, 10);

		QVBoxLayout* vbox_layout = new QVBoxLayout(this);
		//vbox_layout->setContentsMargins(20, 20, 20, 20);
		setLayout(vbox_layout);


		QLabel* license_list_label = new QLabel(tr("Licenses:"), this);
	
		license_list = new QListWidget(this);
		license_list->setFrameStyle(QFrame::NoFrame | QFrame::Plain);

		QLabel* license_text_label = new QLabel(tr("License detail:"), this);
		
		license_text = new QTextEdit(this);
		license_text->setReadOnly(true);
		license_text->setFrameStyle(QFrame::NoFrame | QFrame::Plain);
		

		layout()->addWidget(license_list_label);
		layout()->addWidget(license_list);
		layout()->addWidget(license_text_label);
		layout()->addWidget(license_text);

		layout()->update();

		license_text_array.resize(9);

		license_text_array[0] = LOAD_RESOURCE(decade_LICENSE).toString();
		license_text_array[1] = LOAD_RESOURCE(boost_LICENSE_1_0).toString();
		license_text_array[2] = LOAD_RESOURCE(qt_LICENSE).toString();
		license_text_array[3] = LOAD_RESOURCE(glad_LICENSE).toString();
		license_text_array[4] = LOAD_RESOURCE(glm_copying).toString();
		license_text_array[5] = LOAD_RESOURCE(embed_resource_LICENSE).toString();
		license_text_array[6] = LOAD_RESOURCE(freetype2_LICENSE).toString();
		license_text_array[7] = LOAD_RESOURCE(lodepng_LICENSE).toString();
		license_text_array[8] = LOAD_RESOURCE(sigslot_LICENSE).toString();

		license_list->addItem(tr("Decade"));
		license_list->addItem(tr("Boost"));
		license_list->addItem(tr("Qt"));
		license_list->addItem(tr("glad"));
		license_list->addItem(tr("glm"));
		license_list->addItem(tr("embed-resource"));
		license_list->addItem(tr("freetype2"));
		license_list->addItem(tr("lodepng"));
		license_list->addItem(tr("sigslot"));

		connect(license_list, &QListWidget::currentRowChanged, this, &LicenseDialog::SelectTextSlot);
		
	}

private slots:

	void SelectTextSlot(int current_row)
	{
		license_text->setText(license_text_array[current_row].c_str());
	}

private:

	QListWidget* license_list;
	QTextEdit* license_text;

	std::vector<std::string> license_text_array;
};
