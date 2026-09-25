#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtTest/QTest>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QWidget>
#include <optional>
#include <string>
#include <string_view>

#include "domain/date_category.hpp"
#include "domain/shape_configuration.hpp"
#include "presentation/shape_panel.hpp"

namespace {

QString ToQString(std::string_view text) {
  return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

// The shapes panel with its edit signal caught, so a test can read what an
// edit actually publishes.
class WiredShapePanel {
 public:
  WiredShapePanel() {
    QObject::connect(
        &panel_, &ShapeSetupPanel::ShapeConfigSetEdited, &scope_,
        [this](const ShapeConfigSet& edited) { edited_ = edited; });

    panel_.ReceiveShapeConfigSet(ShapeConfigSet{});
    panel_.resize(kPanelWidth, kPanelHeight);
    panel_.show();
  }

  [[nodiscard]] int RowKeyed(const QString& key) const {
    QListWidget* list = List();
    if (list == nullptr) {
      return -1;
    }
    for (int row = 0; row < list->count(); ++row) {
      if (list->item(row)->data(Qt::UserRole).toString() == key) {
        return row;
      }
    }
    return -1;
  }

  void ClickRow(int row) {
    QListWidget* list = List();
    ASSERT_NE(list, nullptr);
    QListWidgetItem* item = list->item(row);
    ASSERT_NE(item, nullptr);
    list->scrollToItem(item);
    QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
                      list->visualItemRect(item).center());
  }

  // One step up on the line width — a keystroke, so the edit travels the path
  // a user's does rather than being written into the widget.
  void TypeLineWidthUp() {
    QDoubleSpinBox* line_width = panel_.findChild<QDoubleSpinBox*>();
    ASSERT_NE(line_width, nullptr);
    line_width->setFocus();
    QTest::keyClick(line_width, Qt::Key_Up);
  }

  [[nodiscard]] const std::optional<ShapeConfigSet>& Edited() const {
    return edited_;
  }

  [[nodiscard]] QString LabelOf(int row) const {
    QListWidget* list = List();
    return list != nullptr && list->item(row) != nullptr
               ? list->item(row)->text()
               : QString{};
  }

  // One category configuration named `name`, delivered the way the binder
  // delivers it: the set first, then the categories.
  void ReceiveOneCategory(const std::string& name) {
    ShapeConfigSet set;
    set.SyncToDateCategories(1);
    panel_.ReceiveShapeConfigSet(set);
    panel_.ReceiveDateCategories({DateCategory(name)});
  }

 private:
  static constexpr int kPanelWidth = 600;
  static constexpr int kPanelHeight = 400;

  [[nodiscard]] QListWidget* List() const {
    return panel_.findChild<QListWidget*>();
  }

  ShapeSetupPanel panel_{nullptr};
  std::optional<ShapeConfigSet> edited_;

  // Last member, so it dies first and takes the connection with it.
  QObject scope_;
};

}  // namespace

TEST(ShapeSetupPanelTest, TheListNamesEveryConfigurationByItsKey) {
  const WiredShapePanel wired;

  EXPECT_GE(wired.RowKeyed(ToQString(ShapeConfigSet::kAnnualCoverageKey)), 0);
  EXPECT_GE(wired.RowKeyed(ToQString(ShapeConfigSet::kPageMarginKey)), 0);
  EXPECT_EQ(wired.RowKeyed(ToQString("Does Not Exist")), -1);
}

TEST(ShapeSetupPanelTest, ClickingARowEditsTheConfigurationUnderThatKey) {
  WiredShapePanel wired;
  const int row = wired.RowKeyed(ToQString(ShapeConfigSet::kAnnualCoverageKey));
  ASSERT_GE(row, 0);

  wired.ClickRow(row);
  wired.TypeLineWidthUp();

  ASSERT_TRUE(wired.Edited().has_value());
  const ShapeConfiguration edited =
      wired.Edited()->GetShapeConfiguration(ShapeConfigSet::kAnnualCoverageKey);
  EXPECT_EQ(edited.Key(), ShapeConfigSet::kAnnualCoverageKey);
  EXPECT_GT(edited.LineWidthDisabled(),
            ShapeConfigSet{}
                .GetShapeConfiguration(ShapeConfigSet::kAnnualCoverageKey)
                .LineWidthDisabled());
}

TEST(ShapeSetupPanelTest, AFixedRowReadsAsItsLabelNotItsKey) {
  const WiredShapePanel wired;

  const int row = wired.RowKeyed(ToQString(ShapeConfigSet::kPageMarginKey));

  ASSERT_GE(row, 0);
  EXPECT_EQ(wired.LabelOf(row), "Page Margin");
}

TEST(ShapeSetupPanelTest, ACategoryRowReadsAsItsCategoryName) {
  WiredShapePanel wired;

  wired.ReceiveOneCategory("PBL");

  const int row =
      wired.RowKeyed(ToQString(ShapeConfigSet::DynamicConfigurationKey(0)));
  ASSERT_GE(row, 0);
  EXPECT_EQ(wired.LabelOf(row), "PBL");
}
