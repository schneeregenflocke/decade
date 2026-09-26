#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtTest/QTest>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOptionButton>
#include <QtWidgets/QWidget>
#include <vector>

#include "domain/band_part.hpp"
#include "domain/calendar_config.hpp"
#include "domain/calendar_sizing.hpp"
#include "domain/calendar_view.hpp"
#include "presentation/calendar_panel.hpp"

namespace {

// The field a form row labels, found the way a user finds it: by its label.
template <typename Field>
Field* FieldLabelled(const QWidget& form, const QString& text) {
  for (const auto* label : form.findChildren<QLabel*>()) {
    if (label->text() == text) {
      return qobject_cast<Field*>(label->buddy());
    }
  }
  return nullptr;
}

// A check box without text grows to its row's width, but only the indicator
// takes a click — so the click aims there, not at the widget's centre.
void ClickIndicator(QCheckBox& check_box) {
  QStyleOptionButton option;
  option.initFrom(&check_box);
  const QRect indicator = check_box.style()->subElementRect(
      QStyle::SE_CheckBoxIndicator, &option, &check_box);
  QTest::mouseClick(&check_box, Qt::LeftButton, {}, indicator.center());
}

}  // namespace

TEST(CalendarSetupPanelTest, ClickingAnnualCoverageHidesItAndItsBandPart) {
  constexpr int kWidth = 400;
  constexpr int kHeight = 800;
  CalendarSetupPanel panel(nullptr);
  std::vector<bool> reported;
  QObject::connect(&panel, &CalendarSetupPanel::CalendarConfigEdited,
                   [&reported](const CalendarConfig& config) {
                     reported.push_back(config.ShowsAnnualCoverage());
                   });
  panel.resize(kWidth, kHeight);
  panel.show();
  auto* coverage = FieldLabelled<QCheckBox>(panel, "Annual Coverage");
  auto* coverage_proportion = FieldLabelled<QDoubleSpinBox>(panel, "Coverage");
  ASSERT_NE(coverage, nullptr);
  ASSERT_NE(coverage_proportion, nullptr);
  ASSERT_TRUE(coverage->isChecked());
  ASSERT_TRUE(coverage_proportion->isEnabled());

  ClickIndicator(*coverage);

  EXPECT_EQ(reported, std::vector<bool>{false});
  EXPECT_FALSE(coverage_proportion->isEnabled());
}

TEST(CalendarSetupPanelTest, ChoosingAViewReportsIt) {
  CalendarSetupPanel panel(nullptr);
  std::vector<CalendarView> reported;
  QObject::connect(&panel, &CalendarSetupPanel::CalendarConfigEdited,
                   [&reported](const CalendarConfig& config) {
                     reported.push_back(config.View());
                   });
  panel.show();
  auto* view = FieldLabelled<QComboBox>(panel, "Calendar View");
  ASSERT_NE(view, nullptr);
  ASSERT_EQ(view->count(), static_cast<int>(kCalendarViews.size()));
  ASSERT_EQ(view->currentText(), "One Year per Row");
  view->setFocus();

  QTest::keyClick(view, Qt::Key_Down);

  EXPECT_EQ(view->currentText(), "All Years in One Row");
  EXPECT_EQ(reported, std::vector{CalendarView::kAllYearsInOneRow});
}

TEST(CalendarSetupPanelTest, FixingTheWidthReportsItAndOpensItsMillimetres) {
  constexpr int kWidth = 400;
  constexpr int kHeight = 1200;
  CalendarSetupPanel panel(nullptr);
  std::vector<bool> reported;
  QObject::connect(&panel, &CalendarSetupPanel::CalendarConfigEdited,
                   [&reported](const CalendarConfig& config) {
                     reported.push_back(config.Sizing().FixesWidth());
                   });
  panel.resize(kWidth, kHeight);
  panel.show();
  auto* fixes_width = FieldLabelled<QCheckBox>(panel, "Fixed Width");
  auto* day_width = FieldLabelled<QDoubleSpinBox>(panel, "Day Width (mm)");
  ASSERT_NE(fixes_width, nullptr);
  ASSERT_NE(day_width, nullptr);
  ASSERT_FALSE(day_width->isEnabled());

  ClickIndicator(*fixes_width);

  EXPECT_EQ(reported, std::vector<bool>{true});
  EXPECT_TRUE(day_width->isEnabled());
}

// The default parts give the days 100 of 450, so a band of 9 mm leaves them
// 2 mm, and the entry labels 2 mm, which is 5.7 pt.
TEST(CalendarSetupPanelTest, AFixedBandHeightShowsWhatItComesTo) {
  CalendarSetupPanel panel(nullptr);
  CalendarConfig config;
  CalendarSizing sizing;
  sizing.SetFixesHeight(true);
  sizing.SetFixedHeights({.band = 9.0F, .column_labels = 5.0F, .legend = 5.0F});
  config.SetSizing(sizing);

  panel.ReceiveCalendarConfig(config);

  auto* day_height = FieldLabelled<QLabel>(panel, "Day Height (mm)");
  auto* entry_label_size =
      FieldLabelled<QLabel>(panel, "Entry Label Size (pt)");
  ASSERT_NE(day_height, nullptr);
  ASSERT_NE(entry_label_size, nullptr);
  EXPECT_EQ(day_height->text(), "2.00");
  EXPECT_EQ(entry_label_size->text(), "5.7");
}
