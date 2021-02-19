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
#include <QAction>
#include <QVector>
#include <QVariant>
#include <QString>

#include <string>


class TableModel : public QAbstractTableModel
{
	Q_OBJECT

public:

	explicit TableModel(const int column_count, QObject* parent = nullptr) :
		QAbstractTableModel(parent),
		row_count(0),
		column_count(column_count),
		column_labels(column_count)
	{
	}

	int rowCount(const QModelIndex& parent = QModelIndex()) const override
	{
		return row_count;
	}

	int columnCount(const QModelIndex& parent = QModelIndex()) const override
	{
		return column_count;
	}

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
	{
		if (role == Qt::DisplayRole)
		{
			return QString("Row%1, Column%2")
				.arg(index.row() + 1)
				.arg(index.column() + 1);
		}
			
		return QVariant();
	}

	QVariant headerData(int section, Qt::Orientation orientation, int role) const override
	{
		QVariant header_data = QVariant();
		

		if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
		{
			auto test = column_labels[section];
			header_data = column_labels[section];
		}

		return header_data;
	}

	void SetColumnLabels(const QVector<QString>& column_labels)
	{
		this->column_labels = column_labels;
	}

public slots:

	void InsertRowAtSelection()
	{
		insertRow(0);
	}

	

	bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override
	{
		beginInsertRows(parent, row, row + (count - 1));

		for (int count_index = 0; count_index < count; ++count_index)
		{
			data_store.insert(row + count_index, column_count, QVariant());
			row_count += 1;
		}

		endInsertRows();

		return true;
	}

	bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override
	{
		beginRemoveRows(parent, row, row + (count - 1));

		for (int count_index = 0; count_index < count; ++count_index)
		{
			data_store.remove(row + count_index, column_count);
			row_count -= 1;
		}

		endRemoveRows();

		return true;
	}

private:

	int row_count;
	const int column_count;
	
	QVector<QVariant> data_store;
	QVector<QString> column_labels;
};



class DateTable : public QWidget
{
	Q_OBJECT

public:

	explicit DateTable(QWidget* parent = nullptr) :
		QWidget(parent)
	{

		tool_bar = new QToolBar(tr("tool_bar"), this);

		add_row = tool_bar->addAction(tr("Add Row"));
		delete_row = tool_bar->addAction(tr("Delete Row"));


		table_view = new QTableView(this);

		table_model = new TableModel(5, this);
		
		QStringList column_labels;
		column_labels << tr("Hallo") << tr("die") << tr("Tabelle") << tr("ist") << tr("unzufrieden");
		
		table_model->SetColumnLabels(column_labels);
		
		table_view->setModel(table_model);

		connect(add_row, &QAction::triggered, table_model, &TableModel::InsertRowAtSelection);

		QVBoxLayout* vbox_layout = new QVBoxLayout(this);

		vbox_layout->addWidget(tool_bar);
		vbox_layout->addWidget(table_view);

		setLayout(vbox_layout);
		layout()->update();
	}

private:

	QToolBar* tool_bar;
	QTableView* table_view;
	TableModel* table_model;

	QAction* add_row;
	QAction* delete_row;
};




class DateTableDock : public QDockWidget
{
	Q_OBJECT

public:

	explicit DateTableDock(QWidget* parent = nullptr) :
		QDockWidget(parent)
	{
		setMinimumWidth(300);


		date_table = new DateTable(this);
		
		setWidget(date_table);
		
	}

private:

	DateTable* date_table;
	
};

