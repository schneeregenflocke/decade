#include "table_panel_base.hpp"

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/Qt>
#include <QtGui/QFont>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "make_owned.hpp"

TablePanelBase::TablePanelBase(QWidget* parent,
                               QAbstractItemView::SelectionMode selection_mode)
    : QWidget(parent) {
  table_ = MakeOwned<QTableWidget>(this);
  table_->setSelectionMode(selection_mode);
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  // Editing on a double click alone, as the wxDataViewCtrl did: a single
  // click selects, and the delete button follows the selection.
  table_->setEditTriggers(QAbstractItemView::DoubleClicked |
                          QAbstractItemView::EditKeyPressed);
  table_->verticalHeader()->setVisible(false);
  table_->horizontalHeader()->setStretchLastSection(true);

  // More compact than the system font; the row height follows it by itself.
  constexpr double kTableFontPointSize = 11.0;
  QFont table_font = table_->font();
  table_font.setPointSizeF(kTableFontPointSize);
  table_->setFont(table_font);

  add_button_ = MakeOwned<QPushButton>("Add Row", this);
  delete_button_ = MakeOwned<QPushButton>("Delete Row", this);
  delete_button_->setEnabled(false);
}

QTableWidget* TablePanelBase::table() const { return table_; }

QPushButton* TablePanelBase::add_button() const { return add_button_; }

QPushButton* TablePanelBase::delete_button() const { return delete_button_; }

void TablePanelBase::BuildTableLayout(
    const std::vector<QWidget*>& extra_button_controls) {
  constexpr int kBorderPx = 5;

  auto* buttons_layout = MakeOwned<QHBoxLayout>();
  buttons_layout->addWidget(add_button_);
  buttons_layout->addWidget(delete_button_);
  for (QWidget* control : extra_button_controls) {
    buttons_layout->addWidget(control);
  }
  buttons_layout->addStretch(1);

  auto* main_layout = MakeOwned<QVBoxLayout>();
  main_layout->setContentsMargins(kBorderPx, kBorderPx, kBorderPx, kBorderPx);
  main_layout->setSpacing(kBorderPx);
  main_layout->addLayout(buttons_layout);
  main_layout->addWidget(table_, 1);
  setLayout(main_layout);
}

void TablePanelBase::InitColumns(const std::vector<Column>& columns) {
  columns_ = columns;
  table_->setColumnCount(static_cast<int>(columns.size()));
  QStringList labels;
  for (const Column& column : columns) {
    labels.append(QString::fromUtf8(column.label));
  }
  table_->setHorizontalHeaderLabels(labels);
}

void TablePanelBase::FillEmptyRow(int row) {
  for (int column = 0; std::cmp_less(column, columns_.size()); ++column) {
    auto* item = MakeOwned<QTableWidgetItem>();
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (columns_[static_cast<std::size_t>(column)].editable) {
      flags |= Qt::ItemIsEditable;
    }
    item->setFlags(flags);
    table_->setItem(row, column, item);
  }
}

void TablePanelBase::SetCellText(int row, int column, const std::string& text) {
  if (QTableWidgetItem* item = table_->item(row, column); item != nullptr) {
    item->setText(QString::fromStdString(text));
  }
}

std::string TablePanelBase::CellText(int row, int column) const {
  const QTableWidgetItem* item = table_->item(row, column);
  return item != nullptr ? item->text().toStdString() : std::string{};
}
