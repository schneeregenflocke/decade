#ifndef CATEGORIES_PANEL_HPP
#define CATEGORIES_PANEL_HPP

#include <QtGui/QColor>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QWidget>
#include <cstddef>
#include <string>
#include <vector>

#include "../domain/date_category.hpp"
#include "../domain/detail/reentry_guard.hpp"
#include "../domain/shape_configuration.hpp"
#include "table_panel_base.hpp"

class DateCategoriesTablePanel : public TablePanelBase {
  Q_OBJECT

 public:
  explicit DateCategoriesTablePanel(QWidget* parent);

  void ReceiveDateCategories(
      const std::vector<DateCategory>& argument_date_categories);

  // The Color column shows each category's colour out of this set, and a
  // colour picked there comes back as an edit of the set.
  void ReceiveShapeConfigSet(const ShapeConfigSet& shape_config_set);

 signals:
  void DateCategoriesEdited(const std::vector<DateCategory>& date_categories);

  void ShapeConfigSetEdited(const ShapeConfigSet& shape_config_set);

 private:
  // The default category sits in row 0 and is neither removable nor insertable
  // before: everything the user adds lands behind it.
  static constexpr int kFirstEditableRow = 1;
  static constexpr int kNumberColumn = 0;
  static constexpr int kColorColumn = 1;
  static constexpr int kNameColumn = 2;

  void ResizeRows(int row_count);

  // The single place that reads the selection; -1 means nothing selected.
  [[nodiscard]] int SelectedRow() const;

  void UpdateButtons();

  void CallbackAdd();

  void CallbackDelete();

  void CallbackItemChanged(const QTableWidgetItem* item);

  void RefreshColors();

  // Non-modal, so a script driving the GUI keeps running while it is open.
  void OpenColorDialog(int row);

  void CallbackColorChosen(int row, const QColor& color);

  std::vector<DateCategory> date_categories_;
  ShapeConfigSet shape_config_set_;

  bool filling_{false};
};
#endif  // CATEGORIES_PANEL_HPP
