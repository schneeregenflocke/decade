#ifndef DATE_PANEL_HPP
#define DATE_PANEL_HPP

#include <QtCore/QPointer>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <cstddef>
#include <cstdint>
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

// The panel is the user-facing boundary for date intervals: the "To Date"
// column shows and accepts the *inclusive* end date, while every DateEntry
// sent or received carries the internal half-open period [begin, end). The
// conversion happens exactly here (PeriodFromInclusiveDates on input, Last()
// on display) and nowhere else.
class DateTablePanel : public TablePanelBase {
  Q_OBJECT

 public:
  // `date_format` belongs to the composition root, so the whole application
  // shares one locale configuration.
  DateTablePanel(QWidget* parent, LocaleDateFormatter& date_format);

  void ReceiveDateEntries(const std::vector<DateEntry>& date_entries);

  void ReceiveDateGroups(const std::vector<DateGroup>& date_groups);

 signals:
  void DateEntriesEdited(const std::vector<DateEntry>& date_entries);

 private:
  enum class Columns : std::uint8_t {
    first_date,
    second_date,
    number,
    group,
    group_number,
    duration,
    duration_to_next
  };

  struct CellIndex {
    int row{0};
    Columns column{Columns::first_date};
  };

  static constexpr int ColumnIndex(Columns column) {
    return static_cast<int>(column);
  }

  void SendDateEntries();

  void UpdateDeleteButton();

  // The rows holding a usable period, in ascending order. A row that does not
  // parse keeps its two date cells and loses its derived columns — it is a
  // half-typed entry, not an error.
  std::vector<int> BuildValidRowsList();

  [[nodiscard]] std::vector<int> SelectedRows() const;

  void InsertRow(int row);

  void RemoveRow(int row);

  Date GetDateByCell(CellIndex cell);

  void OnItemChanged(const QTableWidgetItem* item);

  void OnAdd();

  void OnDelete();

  void OnGroupChosen(int group_number);

  QPointer<QComboBox> select_group_control_;

  LocaleDateFormatter& date_format_;
  DateGroups date_groups_;

  bool filling_{false};
};
#endif  // DATE_PANEL_HPP
