#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtGui/QColor>
#include <QtTest/QTest>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>
#include <cstddef>
#include <glm/vec4.hpp>
#include <optional>
#include <vector>

#include "domain/date_category.hpp"
#include "domain/date_category_store.hpp"
#include "domain/shape_configuration.hpp"
#include "domain/shape_configuration_store.hpp"
#include "domain/state_topics.hpp"
#include "presentation/categories_panel.hpp"

namespace {

// Panel, topics and stores wired the way app_binder does for the date
// categories. A simulated click therefore travels the path it travels in the
// running program: button -> panel -> DateCategoriesEdited -> store -> topic ->
// consumers.
class WiredCategoriesPanel {
 public:
  WiredCategoriesPanel() {
    // The stores are no QObjects, so their connections carry `scope_` as the
    // context object and reach the store through a lambda — the same shape
    // app_binder::Connect gives every store connection.
    QObject::connect(&panel_, &DateCategoriesTablePanel::DateCategoriesEdited,
                     &scope_,
                     [this](const std::vector<DateCategory>& date_categories) {
                       categories_store_.ReceiveDateCategories(date_categories);
                     });
    QObject::connect(&categories_topic_,
                     &domain::DateCategoriesTopic::Published, &panel_,
                     &DateCategoriesTablePanel::ReceiveDateCategories);
    QObject::connect(&categories_topic_,
                     &domain::DateCategoriesTopic::Published, &scope_,
                     [this](const std::vector<DateCategory>& date_categories) {
                       shape_store_.ReceiveDateCategories(date_categories);
                     });

    categories_store_.SendDefaultValues();

    panel_.resize(kPanelWidth, kPanelHeight);
    panel_.show();
  }

  // QTest sends the click through QWindowSystemInterface, so it takes the same
  // route a real one does — hit test, grab, focus — instead of being handed to
  // the button directly.
  void ClickAddRow() {
    QPushButton* add_row = ButtonLabelled("Add Row");
    ASSERT_NE(add_row, nullptr);
    QTest::mouseClick(add_row, Qt::LeftButton);
  }

  [[nodiscard]] std::size_t CategoryCount() const {
    return categories_store_.Get().Items().size();
  }

  [[nodiscard]] int TableRowCount() const {
    const QTableWidget* table = panel_.findChild<QTableWidget*>();
    return table == nullptr ? -1 : table->rowCount();
  }

  [[nodiscard]] glm::vec4 AnnualCoverageFillColor() const {
    return shape_store_.Get()
        .GetShapeConfiguration(ShapeConfigSet::kAnnualCoverageKey)
        .FillColorDisabled();
  }

  [[nodiscard]] glm::vec4 CategoryFillColor(std::size_t category_index) const {
    return shape_store_.Get()
        .GetDynamicConfiguration(category_index)
        .FillColorDisabled();
  }

 private:
  static constexpr int kPanelWidth = 400;
  static constexpr int kPanelHeight = 300;

  [[nodiscard]] QPushButton* ButtonLabelled(const QString& label) const {
    for (QPushButton* button : panel_.findChildren<QPushButton*>()) {
      if (button->text() == label) {
        return button;
      }
    }
    return nullptr;
  }

  domain::DateCategoriesTopic categories_topic_;
  domain::ShapeConfigSetTopic shape_topic_;
  DateCategoryStore categories_store_{categories_topic_};
  ShapeConfigurationStore shape_store_{shape_topic_};
  DateCategoriesTablePanel panel_{nullptr};

  // Last member, so it dies first and takes every connection with it while the
  // receivers above are still alive.
  QObject scope_;
};

void ExpectSameColor(const glm::vec4& actual, const glm::vec4& expected) {
  EXPECT_FLOAT_EQ(actual[0], expected[0]);
  EXPECT_FLOAT_EQ(actual[1], expected[1]);
  EXPECT_FLOAT_EQ(actual[2], expected[2]);
  EXPECT_FLOAT_EQ(actual[3], expected[3]);
}

}  // namespace

TEST(DateCategoriesPanelTest, AddRowClickAddsOneCategoryPerClick) {
  WiredCategoriesPanel wired;
  ASSERT_EQ(wired.CategoryCount(), 1U);

  wired.ClickAddRow();
  EXPECT_EQ(wired.CategoryCount(), 2U);
  EXPECT_EQ(wired.TableRowCount(), 2);

  wired.ClickAddRow();
  wired.ClickAddRow();
  EXPECT_EQ(wired.CategoryCount(), 4U);
  EXPECT_EQ(wired.TableRowCount(), 4);
}

TEST(DateCategoriesPanelTest, AddRowClicksLeaveTheAnnualCoverageColorAlone) {
  WiredCategoriesPanel wired;
  const glm::vec4 before = wired.AnnualCoverageFillColor();

  for (int click = 0; click < 3; ++click) {
    wired.ClickAddRow();
    ExpectSameColor(wired.AnnualCoverageFillColor(), before);
  }
}

TEST(DateCategoriesPanelTest, AddRowClicksLeaveExistingCategoryColorsAlone) {
  WiredCategoriesPanel wired;
  const glm::vec4 first_category = wired.CategoryFillColor(0);

  wired.ClickAddRow();
  const glm::vec4 second_category = wired.CategoryFillColor(1);

  wired.ClickAddRow();
  ExpectSameColor(wired.CategoryFillColor(0), first_category);
  ExpectSameColor(wired.CategoryFillColor(1), second_category);
}

// The Color column is a delegate-drawn button: a click on it opens the colour
// dialogue, and the colour chosen there comes out as an edit of the set.
TEST(DateCategoriesPanelTest, AColorPickedInTheTableEditsThatCategorysColor) {
  DateCategoriesTablePanel panel(nullptr);
  ShapeConfigSet set;
  set.SyncToDateCategories(1);
  panel.ReceiveDateCategories({DateCategory("PBL")});
  panel.ReceiveShapeConfigSet(set);
  std::optional<ShapeConfigSet> edited;
  QObject::connect(&panel, &DateCategoriesTablePanel::ShapeConfigSetEdited,
                   [&edited](const ShapeConfigSet& value) { edited = value; });
  constexpr int kWidth = 400;
  constexpr int kHeight = 300;
  panel.resize(kWidth, kHeight);
  panel.show();

  auto* table = panel.findChild<QTableWidget*>();
  ASSERT_NE(table, nullptr);
  const QRect cell = table->visualRect(table->model()->index(0, 1));
  QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::KeyboardModifiers(),
                    cell.center());
  auto* dialog = panel.findChild<QColorDialog*>();
  ASSERT_NE(dialog, nullptr);
  dialog->setCurrentColor(QColor(0, 128, 128));
  dialog->accept();

  ASSERT_TRUE(edited.has_value());
  const glm::vec4 fill = edited->GetDynamicConfiguration(0).FillColorDisabled();
  EXPECT_NEAR(fill[0], 0.0F, 1e-3F);
  EXPECT_NEAR(fill[1], 128.0F / 255.0F, 1e-3F);
  EXPECT_NEAR(fill[2], 128.0F / 255.0F, 1e-3F);
}
