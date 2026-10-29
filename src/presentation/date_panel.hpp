#ifndef DATE_PANEL_HPP
#define DATE_PANEL_HPP

#include <QtCore/QPointer>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QCheckBox>
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
#include "../domain/date_category.hpp"
#include "../domain/date_entry.hpp"
#include "../domain/date_format.hpp"
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

  void ReceiveDateCategories(const std::vector<DateCategory>& date_categories);

  // The calendar's selection, as the date entries in it. The rows follow while
  // the sync box is ticked, without reporting back.
  void ReceiveSelectedEntries(const std::vector<std::size_t>& entries);

 signals:
  void DateEntriesEdited(const std::vector<DateEntry>& date_entries);

  // The date entries of the selected rows, while the sync box is ticked; a
  // half-typed row is no entry and drops out.
  void SelectedEntriesChanged(const std::vector<std::size_t>& entries);

 private:
  enum class Columns : std::uint8_t {
    first_date,
    second_date,
    number,
    category,
    duration,
    duration_to_next,
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

  [[nodiscard]] bool RowHoldsPeriod(int row);

  // The rows holding a usable period, in ascending order: the row of entry i
  // is the i-th of them.
  [[nodiscard]] std::vector<int> ValidRows();

  // ValidRows, and a row that does not parse keeps its two date cells and
  // loses its derived columns — it is a half-typed entry, not an error.
  std::vector<int> BuildValidRowsList();

  [[nodiscard]] std::vector<int> SelectedRows() const;

  [[nodiscard]] bool SyncsSelection() const;

  void SendSelectedEntries();

  void SelectRows(const std::vector<int>& rows);

  void InsertRow(int row);

  void RemoveRow(int row);

  Date GetDateByCell(CellIndex cell);

  void OnItemChanged(const QTableWidgetItem* item);

  void OnAdd();

  void OnDelete();

  void OnCategoryChosen(int category_number);

  QPointer<QComboBox> select_category_control_;
  QPointer<QCheckBox> sync_selection_control_;

  LocaleDateFormatter& date_format_;
  DateCategories date_categories_;

  bool filling_{false};
  // The selection this panel sends comes straight back over the bus; it is
  // its own, and by then the scene may not yet know the entries it names.
  bool sending_selection_{false};
};
#endif  // DATE_PANEL_HPP
