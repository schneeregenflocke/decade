#ifndef TABLE_PANEL_BASE_HPP
#define TABLE_PANEL_BASE_HPP

#include <QtCore/QPointer>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QWidget>
#include <string>
#include <vector>

class QPushButton;
class QTableWidget;

// Common scaffolding shared by the two data-table panels (date entries and date
// groups): a QTableWidget sitting below an "Add Row" / "Delete Row" button row.
// The base owns the three widgets and lays them out; subclasses choose the
// selection mode and columns, fill the rows and decide what Add/Delete actually
// do (single vs. multi selection, what a new/removed row means) by connecting
// their own handlers to the two buttons.
class TablePanelBase : public QWidget {
 public:
  TablePanelBase(QWidget* parent,
                 QAbstractItemView::SelectionMode selection_mode);

 protected:
  // Widgets the base owns, exposed to subclasses through accessors (the data
  // members themselves stay private). Each returns a non-owning pointer that
  // auto-nulls if Qt destroys the widget.
  [[nodiscard]] QTableWidget* table() const;
  [[nodiscard]] QPushButton* add_button() const;
  [[nodiscard]] QPushButton* delete_button() const;

  // Lays out the button row (Add, Delete, then any subclass-specific controls)
  // above the table and installs the layout. Call once from the subclass
  // constructor after creating the extra controls.
  void BuildTableLayout(
      const std::vector<QWidget*>& extra_button_controls = {});

  // The columns, once, with their edit permission. A cell of a read-only column
  // never becomes writable, so no handler has to guard against an edit that
  // cannot happen.
  struct Column {
    const char* label;
    bool editable;
  };

  void InitColumns(const std::vector<Column>& columns);

  // Creates the cells of a row; without them there is nothing to read or write.
  void FillEmptyRow(int row);

  void SetCellText(int row, int column, const std::string& text);

  [[nodiscard]] std::string CellText(int row, int column) const;

 private:
  QPointer<QTableWidget> table_;
  QPointer<QPushButton> add_button_;
  QPointer<QPushButton> delete_button_;
  std::vector<Column> columns_;
};

#endif  // TABLE_PANEL_BASE_HPP
