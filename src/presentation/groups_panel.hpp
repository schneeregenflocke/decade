#ifndef GROUPS_PANEL_HPP
#define GROUPS_PANEL_HPP

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

class DateGroupsTablePanel : public TablePanelBase {
 public:
  explicit DateGroupsTablePanel(QWidget* parent);

  void ReceiveDateGroups(const std::vector<DateGroup>& argument_date_groups);

  sigslot::signal<const std::vector<DateGroup>&>& SignalTableDateGroups();

 private:
  // The default group sits in row 0 and is neither removable nor insertable
  // before: everything the user adds lands behind it.
  static constexpr int kFirstEditableRow = 1;
  static constexpr int kNumberColumn = 0;
  static constexpr int kNameColumn = 1;

  void ResizeRows(int row_count);

  // The single place that reads the selection; -1 means nothing selected.
  [[nodiscard]] int SelectedRow() const;

  void UpdateButtons();

  void CallbackAdd();

  void CallbackDelete();

  void CallbackItemChanged(const QTableWidgetItem* item);

  std::vector<DateGroup> date_groups_;
  sigslot::signal<const std::vector<DateGroup>&> signal_table_date_groups_;

  bool filling_{false};
};
#endif  // GROUPS_PANEL_HPP
