#include "groups_panel.hpp"

#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QWidget>
#include <cstddef>
#include <sigslot/signal.hpp>
#include <string>
#include <vector>

#include "../domain/date_group.hpp"
#include "../domain/detail/reentry_guard.hpp"
#include "table_panel_base.hpp"

DateGroupsTablePanel::DateGroupsTablePanel(QWidget* parent)
    : TablePanelBase(parent, QAbstractItemView::SingleSelection) {
  InitColumns({{.label = "Group Number", .editable = false},
               {.label = "Group Name", .editable = true}});
  BuildTableLayout();

  connect(table(), &QTableWidget::itemChanged, this,
          [this](QTableWidgetItem* item) { CallbackItemChanged(item); });
  connect(table(), &QTableWidget::itemSelectionChanged, this,
          [this]() { UpdateButtons(); });
  connect(add_button(), &QPushButton::clicked, this,
          [this]() { CallbackAdd(); });
  connect(delete_button(), &QPushButton::clicked, this,
          [this]() { CallbackDelete(); });
}

void DateGroupsTablePanel::ReceiveDateGroups(
    const std::vector<DateGroup>& argument_date_groups) {
  date_groups_ = argument_date_groups;

  // Filling the cells fires itemChanged as an edit would; the guard tells the
  // two apart, so a rebuild does not read itself back in as user input.
  const domain::detail::ScopedReentryFlag guard(filling_);

  const auto row_count = static_cast<int>(date_groups_.size());
  ResizeRows(row_count);

  for (int row = 0; row < row_count; ++row) {
    const auto index = static_cast<std::size_t>(row);
    SetCellText(row, kNumberColumn,
                std::to_string(date_groups_[index].GetNumber()));
    SetCellText(row, kNameColumn, date_groups_[index].GetName());
  }
}

sigslot::signal<const std::vector<DateGroup>&>&
DateGroupsTablePanel::SignalTableDateGroups() {
  return signal_table_date_groups_;
}

void DateGroupsTablePanel::ResizeRows(int row_count) {
  while (table()->rowCount() > row_count) {
    table()->removeRow(table()->rowCount() - 1);
  }
  while (table()->rowCount() < row_count) {
    const int row = table()->rowCount();
    table()->insertRow(row);
    FillEmptyRow(row);
  }
}

int DateGroupsTablePanel::SelectedRow() const {
  const auto rows = table()->selectionModel()->selectedRows();
  return rows.isEmpty() ? -1 : rows.front().row();
}

void DateGroupsTablePanel::UpdateButtons() {
  delete_button()->setEnabled(SelectedRow() >= kFirstEditableRow);
}

void DateGroupsTablePanel::CallbackAdd() {
  const int selected_row = SelectedRow();
  int insert_row = 0;
  if (selected_row < 0) {
    insert_row = table()->rowCount();
  } else if (selected_row < kFirstEditableRow) {
    insert_row = kFirstEditableRow;
  } else {
    insert_row = selected_row + 1;
  }

  date_groups_.insert(
      date_groups_.cbegin() + static_cast<std::ptrdiff_t>(insert_row),
      DateGroup(""));

  signal_table_date_groups_(date_groups_);

  table()->selectRow(insert_row);
  table()->scrollToItem(table()->item(insert_row, kNameColumn));
  UpdateButtons();
}

void DateGroupsTablePanel::CallbackDelete() {
  const int selected_row = SelectedRow();
  if (selected_row < kFirstEditableRow ||
      static_cast<std::size_t>(selected_row) >= date_groups_.size()) {
    return;
  }

  date_groups_.erase(date_groups_.cbegin() +
                     static_cast<std::ptrdiff_t>(selected_row));

  signal_table_date_groups_(date_groups_);

  if (table()->rowCount() > 0) {
    table()->selectRow(selected_row < table()->rowCount() ? selected_row
                                                          : selected_row - 1);
  }
  UpdateButtons();
}

void DateGroupsTablePanel::CallbackItemChanged(const QTableWidgetItem* item) {
  if (filling_ || item == nullptr || item->column() != kNameColumn) {
    return;
  }
  // The row comes from the item, not from the selection: ReceiveDateGroups
  // inserts and deletes rows while an edit can stand open, and the selection
  // can have moved on by the time the edit lands.
  const auto row = static_cast<std::size_t>(item->row());
  if (row >= date_groups_.size()) {
    return;
  }
  date_groups_[row].SetName(item->text().toStdString());
  signal_table_date_groups_(date_groups_);
}
