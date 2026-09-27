#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtTest/QTest>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

#include "domain/date.hpp"
#include "domain/date_category.hpp"
#include "domain/date_entry.hpp"
#include "domain/date_format.hpp"
#include "domain/date_period.hpp"
#include "presentation/date_panel.hpp"

namespace {

using Entries = std::vector<std::size_t>;

constexpr int kNumberColumn = 2;

DateEntry EntryInJanuary(int year, int number) {
  DateEntry entry;
  entry.SetDateInterval(
      DatePeriod(Date::FromYmd(year, 1, 1), Date::FromYmd(year, 1, 10)));
  entry.SetNumber(number);
  return entry;
}

// Three entries in three rows, shown, and what the panel reports.
class ShownEntries {
 public:
  ShownEntries() {
    panel_.ReceiveDateCategories({DateCategory("Default")});
    panel_.ReceiveDateEntries({EntryInJanuary(2001, 0), EntryInJanuary(2002, 1),
                               EntryInJanuary(2003, 2)});
    QObject::connect(&panel_, &DateTablePanel::SelectedEntriesChanged,
                     [this](const Entries& entries) { reported = entries; });
    constexpr int kWidth = 700;
    constexpr int kHeight = 400;
    panel_.resize(kWidth, kHeight);
    panel_.show();
  }

  DateTablePanel& Panel() { return panel_; }

  void ClickRow(int row, Qt::KeyboardModifiers modifiers = {}) {
    QTableWidget& table = Table();
    const QRect cell =
        table.visualRect(table.model()->index(row, kNumberColumn));
    QTest::mouseClick(table.viewport(), Qt::LeftButton, modifiers,
                      cell.center());
  }

  void Click(const QString& label) {
    for (QAbstractButton* button : panel_.findChildren<QAbstractButton*>()) {
      if (button->text() == label) {
        QTest::mouseClick(button, Qt::LeftButton);
        return;
      }
    }
    FAIL() << "no button labelled " << label.toStdString();
  }

  [[nodiscard]] std::vector<int> SelectedRows() {
    std::vector<int> rows;
    for (const QModelIndex& index : Table().selectionModel()->selectedRows()) {
      rows.push_back(index.row());
    }
    std::ranges::sort(rows);
    return rows;
  }

  std::optional<Entries> reported;

 private:
  QTableWidget& Table() { return *panel_.findChild<QTableWidget*>(); }

  LocaleDateFormatter date_format_{"de_CH"};
  DateTablePanel panel_{nullptr, date_format_};
};

}  // namespace

TEST(DateTablePanelSelectionTest, SelectedRowsReportTheirEntries) {
  ShownEntries shown;
  shown.ClickRow(0);
  shown.ClickRow(2, Qt::ControlModifier);

  EXPECT_EQ(shown.reported, (Entries{0, 2}));
}

TEST(DateTablePanelSelectionTest, AHalfTypedRowIsNoEntry) {
  ShownEntries shown;
  shown.ClickRow(0);
  shown.Click("Add Row");
  EXPECT_EQ(shown.reported, Entries{});

  shown.ClickRow(0);
  shown.ClickRow(2, Qt::ControlModifier);
  EXPECT_EQ(shown.reported, (Entries{0, 1}));
}

TEST(DateTablePanelSelectionTest, ReceivedEntriesSelectTheirRowsQuietly) {
  ShownEntries shown;
  shown.reported.reset();
  shown.Panel().ReceiveSelectedEntries({1, 2});

  EXPECT_EQ(shown.SelectedRows(), (std::vector<int>{1, 2}));
  EXPECT_FALSE(shown.reported.has_value());
}

// The bus hands the panel's own selection straight back, translated by a
// scene that may not know a new entry yet; the panel must not take it.
TEST(DateTablePanelSelectionTest, ThePanelIgnoresTheEchoOfItsOwnSelection) {
  ShownEntries shown;
  QObject::connect(
      &shown.Panel(), &DateTablePanel::SelectedEntriesChanged,
      [&shown](const Entries&) { shown.Panel().ReceiveSelectedEntries({}); });
  shown.ClickRow(1);

  EXPECT_EQ(shown.SelectedRows(), std::vector<int>{1});
}

TEST(DateTablePanelSelectionTest, AnUntickedSyncBoxNeitherReportsNorFollows) {
  ShownEntries shown;
  shown.Click("Sync with calendar");
  shown.reported.reset();

  shown.ClickRow(1);
  shown.Panel().ReceiveSelectedEntries({2});

  EXPECT_FALSE(shown.reported.has_value());
  EXPECT_EQ(shown.SelectedRows(), std::vector<int>{1});
}

TEST(DateTablePanelSelectionTest, TickingTheSyncBoxReportsTheSelection) {
  ShownEntries shown;
  shown.Click("Sync with calendar");
  shown.ClickRow(2);

  shown.Click("Sync with calendar");

  EXPECT_EQ(shown.reported, Entries{2});
}
