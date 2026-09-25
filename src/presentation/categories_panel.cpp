#include "categories_panel.hpp"

#include <QtCore/qtmetamacros.h>

#include <QtCore/QModelIndex>
#include <QtCore/Qt>
#include <QtGui/QColor>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QWidget>
#include <cstddef>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <vector>

#include "../domain/date_category.hpp"
#include "../domain/detail/reentry_guard.hpp"
#include "../domain/shape_configuration.hpp"
#include "casts.hpp"
#include "color_button_delegate.hpp"
#include "make_owned.hpp"
#include "table_panel_base.hpp"

DateCategoriesTablePanel::DateCategoriesTablePanel(QWidget* parent)
    : TablePanelBase(parent, QAbstractItemView::SingleSelection) {
  InitColumns({{.label = "Number", .editable = false},
               {.label = "Color", .editable = false},
               {.label = "Name", .editable = true}});
  BuildTableLayout();

  auto* color_delegate = MakeOwned<ColorButtonDelegate>(this);
  table()->setItemDelegateForColumn(kColorColumn, color_delegate);
  connect(color_delegate, &ColorButtonDelegate::Clicked, this,
          [this](const QModelIndex& index) { OpenColorDialog(index.row()); });

  connect(table(), &QTableWidget::itemChanged, this,
          [this](QTableWidgetItem* item) { CallbackItemChanged(item); });
  connect(table(), &QTableWidget::itemSelectionChanged, this,
          [this]() { UpdateButtons(); });
  connect(add_button(), &QPushButton::clicked, this,
          [this]() { CallbackAdd(); });
  connect(delete_button(), &QPushButton::clicked, this,
          [this]() { CallbackDelete(); });
}

void DateCategoriesTablePanel::ReceiveDateCategories(
    const std::vector<DateCategory>& argument_date_categories) {
  date_categories_ = argument_date_categories;

  // Filling the cells fires itemChanged as an edit would; the guard tells the
  // two apart, so a rebuild does not read itself back in as user input.
  const domain::detail::ScopedReentryFlag guard(filling_);

  const auto row_count = static_cast<int>(date_categories_.size());
  ResizeRows(row_count);

  for (int row = 0; row < row_count; ++row) {
    const auto index = static_cast<std::size_t>(row);
    SetCellText(row, kNumberColumn,
                std::to_string(date_categories_[index].GetNumber()));
    SetCellText(row, kNameColumn, date_categories_[index].GetName());
  }
  RefreshColors();
}

void DateCategoriesTablePanel::ReceiveShapeConfigSet(
    const ShapeConfigSet& shape_config_set) {
  shape_config_set_ = shape_config_set;
  RefreshColors();
}

void DateCategoriesTablePanel::RefreshColors() {
  const domain::detail::ScopedReentryFlag guard(filling_);
  for (int row = 0; row < table()->rowCount(); ++row) {
    QTableWidgetItem* item = table()->item(row, kColorColumn);
    if (item == nullptr) {
      continue;
    }
    QColor color = ToQColor(
        shape_config_set_.GetDynamicConfiguration(static_cast<std::size_t>(row))
            .FillColorDisabled());
    // The swatch shows the category's colour itself, not its translucent fill.
    color.setAlphaF(1.0F);
    item->setData(Qt::UserRole, color);
  }
}

void DateCategoriesTablePanel::OpenColorDialog(int row) {
  const auto current =
      table()->item(row, kColorColumn)->data(Qt::UserRole).value<QColor>();
  auto* dialog = MakeOwned<QColorDialog>(current, this);
  dialog->setWindowTitle("Choose Category Colour");
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  connect(
      dialog, &QColorDialog::colorSelected, this,
      [this, row](const QColor& color) { CallbackColorChosen(row, color); });
  dialog->open();
}

void DateCategoriesTablePanel::CallbackColorChosen(int row,
                                                   const QColor& color) {
  const glm::vec4 chosen = ToGlmVec4(color);
  if (shape_config_set_.SetCategoryColor(static_cast<std::size_t>(row),
                                         glm::vec3(chosen))) {
    emit ShapeConfigSetEdited(shape_config_set_);
  }
}

void DateCategoriesTablePanel::ResizeRows(int row_count) {
  while (table()->rowCount() > row_count) {
    table()->removeRow(table()->rowCount() - 1);
  }
  while (table()->rowCount() < row_count) {
    const int row = table()->rowCount();
    table()->insertRow(row);
    FillEmptyRow(row);
  }
}

int DateCategoriesTablePanel::SelectedRow() const {
  const auto rows = table()->selectionModel()->selectedRows();
  return rows.isEmpty() ? -1 : rows.front().row();
}

void DateCategoriesTablePanel::UpdateButtons() {
  delete_button()->setEnabled(SelectedRow() >= kFirstEditableRow);
}

void DateCategoriesTablePanel::CallbackAdd() {
  const int selected_row = SelectedRow();
  int insert_row = 0;
  if (selected_row < 0) {
    insert_row = table()->rowCount();
  } else if (selected_row < kFirstEditableRow) {
    insert_row = kFirstEditableRow;
  } else {
    insert_row = selected_row + 1;
  }

  date_categories_.insert(
      date_categories_.cbegin() + static_cast<std::ptrdiff_t>(insert_row),
      DateCategory(""));

  emit DateCategoriesEdited(date_categories_);

  table()->selectRow(insert_row);
  table()->scrollToItem(table()->item(insert_row, kNameColumn));
  UpdateButtons();
}

void DateCategoriesTablePanel::CallbackDelete() {
  const int selected_row = SelectedRow();
  if (selected_row < kFirstEditableRow ||
      static_cast<std::size_t>(selected_row) >= date_categories_.size()) {
    return;
  }

  date_categories_.erase(date_categories_.cbegin() +
                         static_cast<std::ptrdiff_t>(selected_row));

  emit DateCategoriesEdited(date_categories_);

  if (table()->rowCount() > 0) {
    table()->selectRow(selected_row < table()->rowCount() ? selected_row
                                                          : selected_row - 1);
  }
  UpdateButtons();
}

void DateCategoriesTablePanel::CallbackItemChanged(
    const QTableWidgetItem* item) {
  if (filling_ || item == nullptr || item->column() != kNameColumn) {
    return;
  }
  // The row comes from the item, not from the selection: ReceiveDateCategories
  // inserts and deletes rows while an edit can stand open, and the selection
  // can have moved on by the time the edit lands.
  const auto row = static_cast<std::size_t>(item->row());
  if (row >= date_categories_.size()) {
    return;
  }
  date_categories_[row].SetName(item->text().toStdString());
  emit DateCategoriesEdited(date_categories_);
}
