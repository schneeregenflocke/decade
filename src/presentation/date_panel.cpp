#include "date_panel.hpp"

#include <QtCore/qtmetamacros.h>

#include <QtCore/QHashFunctions>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <cstddef>
#include <exception>
#include <ranges>
#include <string>
#include <vector>

#include "../domain/date.hpp"
#include "../domain/date_entry.hpp"
#include "../domain/date_format.hpp"
#include "../domain/date_group.hpp"
#include "../domain/date_period.hpp"
#include "../domain/detail/reentry_guard.hpp"
#include "make_owned.hpp"
#include "table_panel_base.hpp"

DateTablePanel::DateTablePanel(QWidget* parent,
                               LocaleDateFormatter& date_format)
    : TablePanelBase(parent, QAbstractItemView::ExtendedSelection),
      date_format_(date_format) {
  InitColumns({{.label = "From Date", .editable = true},
               {.label = "To Date", .editable = true},
               {.label = "Number", .editable = false},
               {.label = "Group", .editable = false},
               {.label = "Group Number", .editable = false},
               {.label = "Duration", .editable = false},
               {.label = "Duration to next", .editable = false}});

  auto* select_group_control = MakeOwned<QComboBox>(this);
  select_group_control_ = select_group_control;

  BuildTableLayout({select_group_control});

  connect(table(), &QTableWidget::itemChanged, this,
          [this](QTableWidgetItem* item) { OnItemChanged(item); });
  connect(table(), &QTableWidget::itemSelectionChanged, this,
          [this]() { UpdateDeleteButton(); });
  connect(add_button(), &QPushButton::clicked, this, [this]() { OnAdd(); });
  connect(delete_button(), &QPushButton::clicked, this,
          [this]() { OnDelete(); });
  connect(select_group_control, &QComboBox::activated, this,
          [this](int index) { OnGroupChosen(index); });
}

void DateTablePanel::ReceiveDateEntries(
    const std::vector<DateEntry>& date_entries) {
  const domain::detail::ScopedReentryFlag guard(filling_);

  auto valid_rows_list = BuildValidRowsList();

  const auto change_row_number = static_cast<int>(date_entries.size()) -
                                 static_cast<int>(valid_rows_list.size());

  for (int index = 0; index < change_row_number; ++index) {
    const int append_index = table()->rowCount();
    InsertRow(append_index);
    valid_rows_list.push_back(append_index);
  }

  for (int index = change_row_number; index < 0; ++index) {
    RemoveRow(valid_rows_list.back());
    valid_rows_list.pop_back();
  }

  for (std::size_t index = 0; index < valid_rows_list.size(); ++index) {
    const int row = valid_rows_list[index];
    const DateEntry& entry = date_entries[index];

    SetCellText(row, ColumnIndex(Columns::first_date),
                date_format_.Format(entry.GetDateInterval().Begin()));

    // The to-column shows the inclusive last day; a single-day period
    // (length 1) shows an empty to-date.
    std::string second_date;
    if (entry.GetDateInterval().LengthDays() > 1) {
      second_date = date_format_.Format(entry.GetDateInterval().Last());
    }
    SetCellText(row, ColumnIndex(Columns::second_date), second_date);

    SetCellText(row, ColumnIndex(Columns::number),
                std::to_string(entry.GetNumber() + 1));

    // Unknown groups fall back to the default group (0); the store resets
    // them the same way on its side (CheckAndAdjustGroupIntegrity).
    int group = entry.GetGroup();
    if (group > date_groups_.GetGroupMax()) {
      group = 0;
    }
    SetCellText(row, ColumnIndex(Columns::group), date_groups_.GetName(group));

    SetCellText(row, ColumnIndex(Columns::group_number),
                std::to_string(entry.GetGroupNumber() + 1));

    SetCellText(row, ColumnIndex(Columns::duration),
                std::to_string(entry.GetDateInterval().LengthDays()));

    // The inter-interval (end_i, begin_{i+1}) is half-open as well, so its
    // length is exactly the number of free days between the two entries.
    const bool has_next = (index + 1) < date_entries.size();
    SetCellText(row, ColumnIndex(Columns::duration_to_next),
                has_next
                    ? std::to_string(entry.GetDateInterInterval().LengthDays())
                    : std::string{});
  }
}

void DateTablePanel::ReceiveDateGroups(
    const std::vector<DateGroup>& date_groups) {
  date_groups_.Assign(date_groups);

  SendDateEntries();

  const QSignalBlocker blocker(select_group_control_);
  select_group_control_->clear();
  for (const std::string& name : date_groups_.GetDateGroupsNames()) {
    select_group_control_->addItem(QString::fromStdString(name));
  }
  select_group_control_->setCurrentIndex(0);
}

void DateTablePanel::SendDateEntries() {
  std::vector<DateEntry> date_entries;

  for (const int row : BuildValidRowsList()) {
    const auto begin_date =
        GetDateByCell({.row = row, .column = Columns::first_date});
    const auto last_date =
        GetDateByCell({.row = row, .column = Columns::second_date});

    const DatePeriod date_interval =
        PeriodFromInclusiveDates(begin_date, last_date);
    if (date_interval.IsNull()) {
      continue;
    }

    DateEntry date_entry;
    date_entry.SetDateInterval(date_interval);

    int group_number = 0;
    try {
      group_number =
          date_groups_.GetNumber(CellText(row, ColumnIndex(Columns::group)));
    } catch (const std::exception&) {
      group_number = 0;
    }
    date_entry.SetGroup(group_number);

    date_entries.push_back(date_entry);
  }

  emit DateEntriesEdited(date_entries);
}

void DateTablePanel::UpdateDeleteButton() {
  delete_button()->setEnabled(!SelectedRows().empty());
}

std::vector<int> DateTablePanel::BuildValidRowsList() {
  std::vector<int> valid_rows_list;

  for (int row = 0; row < table()->rowCount(); ++row) {
    const auto begin_date =
        GetDateByCell({.row = row, .column = Columns::first_date});
    const auto last_date =
        GetDateByCell({.row = row, .column = Columns::second_date});

    if (!PeriodFromInclusiveDates(begin_date, last_date).IsNull()) {
      valid_rows_list.push_back(row);
      continue;
    }

    SetCellText(row, ColumnIndex(Columns::number), "");
    SetCellText(row, ColumnIndex(Columns::group), "");
    SetCellText(row, ColumnIndex(Columns::group_number), "");
    SetCellText(row, ColumnIndex(Columns::duration), "");
    SetCellText(row, ColumnIndex(Columns::duration_to_next), "");
  }

  return valid_rows_list;
}

std::vector<int> DateTablePanel::SelectedRows() const {
  std::vector<int> rows;
  for (const auto& index : table()->selectionModel()->selectedRows()) {
    rows.push_back(index.row());
  }
  std::ranges::sort(rows);
  return rows;
}

void DateTablePanel::InsertRow(int row) {
  table()->insertRow(row);
  FillEmptyRow(row);
  SetCellText(row, ColumnIndex(Columns::group), date_groups_.GetName(0));
}

void DateTablePanel::RemoveRow(int row) {
  // Guard against an out-of-range row instead of throwing: this runs inside a
  // Qt event handler, where an escaping exception would tear down the app.
  if (row < 0 || row >= table()->rowCount()) {
    return;
  }
  table()->removeRow(row);
}

Date DateTablePanel::GetDateByCell(CellIndex cell) {
  return date_format_.Parse(CellText(cell.row, ColumnIndex(cell.column)));
}

void DateTablePanel::OnItemChanged(const QTableWidgetItem* item) {
  if (filling_ || item == nullptr) {
    return;
  }
  const int column = item->column();
  if (column != ColumnIndex(Columns::first_date) &&
      column != ColumnIndex(Columns::second_date)) {
    return;
  }

  // A parseable date gets written back in the canonical spelling; anything
  // else stays as typed, so the user sees what they wrote.
  const std::string edited_string = item->text().toStdString();
  const Date edited_date = date_format_.Parse(edited_string);
  if (edited_date.IsValid()) {
    const domain::detail::ScopedReentryFlag guard(filling_);
    SetCellText(item->row(), column, date_format_.Format(edited_date));
  }

  SendDateEntries();
}

void DateTablePanel::OnAdd() {
  const std::vector<int> selections = SelectedRows();
  const int insert_row =
      selections.empty() ? table()->rowCount() : selections.back() + 1;

  {
    const domain::detail::ScopedReentryFlag guard(filling_);
    InsertRow(insert_row);
  }
  table()->selectRow(insert_row);
  table()->scrollToItem(
      table()->item(insert_row, ColumnIndex(Columns::first_date)));
  UpdateDeleteButton();
}

void DateTablePanel::OnDelete() {
  const std::vector<int> selections = SelectedRows();
  if (selections.empty()) {
    return;
  }
  const int post_remove_select = selections.front();

  {
    const domain::detail::ScopedReentryFlag guard(filling_);
    for (const int row : std::views::reverse(selections)) {
      RemoveRow(row);
    }
  }

  if (table()->rowCount() > 0) {
    table()->selectRow(post_remove_select < table()->rowCount()
                           ? post_remove_select
                           : post_remove_select - 1);
  }

  UpdateDeleteButton();
  SendDateEntries();
}

void DateTablePanel::OnGroupChosen(int group_number) {
  const std::string group_name = date_groups_.GetName(group_number);
  {
    const domain::detail::ScopedReentryFlag guard(filling_);
    for (const int row : SelectedRows()) {
      SetCellText(row, ColumnIndex(Columns::group), group_name);
    }
  }
  SendDateEntries();
}
