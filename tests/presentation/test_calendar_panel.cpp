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

#include "domain/calendar_config.hpp"
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
