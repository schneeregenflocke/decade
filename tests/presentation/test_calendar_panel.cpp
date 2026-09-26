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
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>
#include <utility>
#include <vector>

#include "domain/band_part.hpp"
#include "domain/calendar_config.hpp"
#include "domain/calendar_metrics.hpp"
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

// A field found the way a screen reader finds it: by its accessible name.
template <typename Field>
Field* FieldNamed(const QWidget& form, const QString& name) {
  for (auto* field : form.findChildren<Field*>()) {
    if (field->accessibleName() == name) {
      return field;
    }
  }
  return nullptr;
}

// One segment of an axis' size switch, by the switch's name and its text.
QToolButton* Segment(const QWidget& form, const QString& axis,
                     const QString& text) {
  const auto* mode = FieldNamed<QWidget>(form, axis);
  if (mode == nullptr) {
    return nullptr;
  }
  for (auto* button : mode->findChildren<QToolButton*>()) {
    if (button->text() == text) {
      return button;
    }
  }
  return nullptr;
}

// The sizes of a layout the tests pretend the renderer came back with.
CalendarMetrics FittedMetrics() {
  return {{.day = 0.73F, .row_labels = 19.5F, .legend_entry = 44.0F},
          {.parts = {0.25F, 1.0F, 0.5F, 1.0F, 0.5F, 1.0F, 0.25F},
           .column_labels = 4.5F,
           .legend = 4.5F},
          {.row_labels = 3.5F, .column_labels = 3.5F}};
}

}  // namespace

TEST(CalendarSetupPanelTest, ClickingAnnualCoverageHidesItAndItsBandPart) {
  constexpr int kWidth = 500;
  constexpr int kHeight = 1400;
  CalendarSetupPanel panel(nullptr);
  std::vector<bool> reported;
  QObject::connect(&panel, &CalendarSetupPanel::CalendarConfigEdited,
                   [&reported](const CalendarConfig& config) {
                     reported.push_back(config.ShowsAnnualCoverage());
                   });
  panel.resize(kWidth, kHeight);
  panel.show();
  auto* coverage = FieldLabelled<QCheckBox>(panel, "Annual coverage");
  auto* coverage_share = FieldNamed<QDoubleSpinBox>(panel, "Coverage share");
  ASSERT_NE(coverage, nullptr);
  ASSERT_NE(coverage_share, nullptr);
  ASSERT_TRUE(coverage->isChecked());
  ASSERT_TRUE(coverage_share->isEnabled());

  ClickIndicator(*coverage);

  EXPECT_EQ(reported, std::vector<bool>{false});
  EXPECT_FALSE(coverage_share->isEnabled());
}

TEST(CalendarSetupPanelTest, ChoosingAViewReportsIt) {
  CalendarSetupPanel panel(nullptr);
  std::vector<CalendarView> reported;
  QObject::connect(&panel, &CalendarSetupPanel::CalendarConfigEdited,
                   [&reported](const CalendarConfig& config) {
                     reported.push_back(config.View());
                   });
  panel.show();
  auto* view = FieldLabelled<QComboBox>(panel, "Calendar view");
  ASSERT_NE(view, nullptr);
  ASSERT_EQ(view->count(), static_cast<int>(kCalendarViews.size()));
  ASSERT_EQ(view->currentText(), "One Year per Row");
  view->setFocus();

  QTest::keyClick(view, Qt::Key_Down);

  EXPECT_EQ(view->currentText(), "All Years in One Row");
  EXPECT_EQ(reported, std::vector{CalendarView::kAllYearsInOneRow});
}

// A fitted width shows its millimetres grey; fixing it takes them over, so the
// drawing keeps its size.
TEST(CalendarSetupPanelTest, FixingTheWidthTakesOverTheFittedSizes) {
  constexpr int kWidth = 500;
  constexpr int kHeight = 1400;
  CalendarSetupPanel panel(nullptr);
  std::vector<CalendarConfig> reported;
  QObject::connect(&panel, &CalendarSetupPanel::CalendarConfigEdited,
                   [&reported](const CalendarConfig& config) {
                     reported.push_back(config);
                   });
  panel.resize(kWidth, kHeight);
  panel.show();
  panel.ReceiveCalendarMetrics(FittedMetrics());
  auto* day_width = FieldNamed<QDoubleSpinBox>(panel, "Day width");
  auto* fixed = Segment(panel, "Width size", "Fixed");
  ASSERT_NE(day_width, nullptr);
  ASSERT_NE(fixed, nullptr);
  ASSERT_FALSE(day_width->isEnabled());
  ASSERT_DOUBLE_EQ(day_width->value(), 0.73);

  QTest::mouseClick(fixed, Qt::LeftButton);

  ASSERT_EQ(reported.size(), 1U);
  EXPECT_TRUE(reported.front().Sizing().FixesWidth());
  EXPECT_FLOAT_EQ(reported.front().Sizing().FixedWidths().day, 0.73F);
  EXPECT_FLOAT_EQ(reported.front().Sizing().FixedWidths().row_labels, 19.5F);
  EXPECT_TRUE(day_width->isEnabled());
}

// Fixing the height takes over the fitted part heights; the shares turn grey
// and show what each part takes of the band.
TEST(CalendarSetupPanelTest, FixingTheHeightTakesOverTheFittedParts) {
  constexpr int kWidth = 500;
  constexpr int kHeight = 1400;
  CalendarSetupPanel panel(nullptr);
  std::vector<CalendarConfig> reported;
  QObject::connect(&panel, &CalendarSetupPanel::CalendarConfigEdited,
                   [&reported](const CalendarConfig& config) {
                     reported.push_back(config);
                   });
  panel.resize(kWidth, kHeight);
  panel.show();
  panel.ReceiveCalendarMetrics(FittedMetrics());
  auto* fixed = Segment(panel, "Height size", "Fixed");
  auto* days_share = FieldNamed<QDoubleSpinBox>(panel, "Days share");
  auto* days_height = FieldNamed<QDoubleSpinBox>(panel, "Days height");
  ASSERT_NE(fixed, nullptr);
  ASSERT_NE(days_share, nullptr);
  ASSERT_NE(days_height, nullptr);

  QTest::mouseClick(fixed, Qt::LeftButton);

  ASSERT_EQ(reported.size(), 1U);
  EXPECT_TRUE(reported.front().Sizing().FixesHeight());
  EXPECT_EQ(reported.front().Sizing().FixedHeights().parts,
            FittedMetrics().GetHeights().parts);
  EXPECT_TRUE(days_height->isEnabled());
  EXPECT_FALSE(days_share->isEnabled());
  EXPECT_NEAR(days_share->value(), 22.2, 0.05);  // 1 mm of a 4.5 mm band
}

// The default parts of a fixed band add up to 6 mm; the entry labels take
// 1.4 mm of them, which is 4.0 pt.
TEST(CalendarSetupPanelTest, AFixedHeightShowsWhatItComesTo) {
  CalendarSetupPanel panel(nullptr);
  CalendarConfig config;
  CalendarSizing sizing;
  sizing.SetFixesHeight(true);
  config.SetSizing(sizing);

  panel.ReceiveCalendarConfig(config);

  auto* band = FieldNamed<QDoubleSpinBox>(panel, "Band height");
  auto* entry_text =
      FieldNamed<QDoubleSpinBox>(panel, "Entry labels text size");
  ASSERT_NE(band, nullptr);
  ASSERT_NE(entry_text, nullptr);
  EXPECT_NEAR(band->value(), 6.0, 1.0e-6);
  EXPECT_NEAR(entry_text->value(), 4.0, 0.05);
  EXPECT_FALSE(band->isEnabled());
}

// Back to a fitted height, the shares keep the percentages the fixed parts
// came to, so the band keeps its shape.
TEST(CalendarSetupPanelTest, FittingTheHeightAgainKeepsTheBandsShape) {
  constexpr int kWidth = 500;
  constexpr int kHeight = 1400;
  CalendarSetupPanel panel(nullptr);
  std::vector<CalendarConfig> reported;
  QObject::connect(&panel, &CalendarSetupPanel::CalendarConfigEdited,
                   [&reported](const CalendarConfig& config) {
                     reported.push_back(config);
                   });
  CalendarConfig config;
  CalendarSizing sizing;
  sizing.SetFixesHeight(true);
  sizing.SetFixedHeights({.parts = {0.5F, 1.0F, 0.5F, 1.0F, 0.5F, 1.0F, 0.5F},
                          .column_labels = 5.0F,
                          .legend = 5.0F});
  config.SetSizing(sizing);
  panel.resize(kWidth, kHeight);
  panel.show();
  panel.ReceiveCalendarConfig(config);
  auto* fit = Segment(panel, "Height size", "Fit page");
  ASSERT_NE(fit, nullptr);

  QTest::mouseClick(fit, Qt::LeftButton);

  ASSERT_EQ(reported.size(), 1U);
  EXPECT_FALSE(reported.front().Sizing().FixesHeight());
  const BandProportions& shares = reported.front().GetBandProportions();
  EXPECT_NEAR(shares.at(std::to_underlying(BandPart::kDays)), 20.0F, 0.05F);
  EXPECT_NEAR(shares.at(std::to_underlying(BandPart::kAboveLabels)), 10.0F,
              0.05F);
}
