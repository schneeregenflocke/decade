#ifndef TABLE_PANEL_BASE_HPP
#define TABLE_PANEL_BASE_HPP

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtGui/QFont>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <string>
#include <utility>
#include <vector>

#include "make_owned.hpp"

// Common scaffolding shared by the two data-table panels (date entries and date
// groups): a QTableWidget sitting below an "Add Row" / "Delete Row" button row.
// The base owns the three widgets and lays them out; subclasses choose the
// selection mode and columns, fill the rows and decide what Add/Delete actually
// do (single vs. multi selection, what a new/removed row means) by connecting
// their own handlers to the two buttons.
class TablePanelBase : public QWidget {
 public:
  TablePanelBase(QWidget* parent,
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

 protected:
  // Widgets the base owns, exposed to subclasses through accessors (the data
  // members themselves stay private). Each returns a non-owning pointer that
  // auto-nulls if Qt destroys the widget.
  [[nodiscard]] QTableWidget* table() const { return table_; }
  [[nodiscard]] QPushButton* add_button() const { return add_button_; }
  [[nodiscard]] QPushButton* delete_button() const { return delete_button_; }

  // Lays out the button row (Add, Delete, then any subclass-specific controls)
  // above the table and installs the layout. Call once from the subclass
  // constructor after creating the extra controls.
  void BuildTableLayout(
      const std::vector<QWidget*>& extra_button_controls = {}) {
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

  // The columns, once, with their edit permission. A cell of a read-only column
  // never becomes writable, so no handler has to guard against an edit that
  // cannot happen.
  struct Column {
    const char* label;
    bool editable;
  };

  void InitColumns(const std::vector<Column>& columns) {
    columns_ = columns;
    table_->setColumnCount(static_cast<int>(columns.size()));
    QStringList labels;
    for (const Column& column : columns) {
      labels.append(QString::fromUtf8(column.label));
    }
    table_->setHorizontalHeaderLabels(labels);
  }

  // Creates the cells of a row; without them there is nothing to read or write.
  void FillEmptyRow(int row) {
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

  void SetCellText(int row, int column, const std::string& text) {
    if (QTableWidgetItem* item = table_->item(row, column); item != nullptr) {
      item->setText(QString::fromStdString(text));
    }
  }

  [[nodiscard]] std::string CellText(int row, int column) const {
    const QTableWidgetItem* item = table_->item(row, column);
    return item != nullptr ? item->text().toStdString() : std::string{};
  }

 private:
  QPointer<QTableWidget> table_;
  QPointer<QPushButton> add_button_;
  QPointer<QPushButton> delete_button_;
  std::vector<Column> columns_;
};

#endif  // TABLE_PANEL_BASE_HPP
