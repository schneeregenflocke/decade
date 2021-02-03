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


#include <QWidget>
#include <QDockWidget>
#include <QTableView>
#include <QAbstractTableModel>
#include <QToolBar>
#include <QVBoxLayout>



class DateTable : public QWidget
{
	Q_OBJECT

public:

	explicit DateTable(QWidget* parent = nullptr) :
		QWidget(parent)
	{
		tool_bar = new QToolBar(tr("tool_bar"), this);

		tool_bar->addAction(tr("Add Row"));
		tool_bar->addAction(tr("Delete Row"));

		//layout()->addWidget(tool_bar);


		//layout()->update();
	}

private:

	QToolBar* tool_bar;
};




class DateTableDock : public QDockWidget
{
	Q_OBJECT

public:

	explicit DateTableDock(QWidget* parent = nullptr) :
		QDockWidget(parent)
	{
		date_table = new DateTable(this);
		
		setWidget(date_table);
		
	}

private:

	DateTable* date_table;
	
};

