#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtTest/QTest>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>
#include <cstddef>
#include <glm/vec4.hpp>
#include <vector>

#include "domain/date_group.hpp"
#include "domain/date_group_store.hpp"
#include "domain/shape_configuration.hpp"
#include "domain/shape_configuration_store.hpp"
#include "domain/state_topics.hpp"
#include "presentation/groups_panel.hpp"

namespace {

// Panel, topics and stores wired the way app_binder does for the date groups.
// A simulated click therefore travels the path it travels in the running
// program: button -> panel -> DateGroupsEdited -> store -> topic -> consumers.
class WiredGroupsPanel {
 public:
  WiredGroupsPanel() {
    // The stores are no QObjects, so their connections carry `scope_` as the
    // context object and reach the store through a lambda — the same shape
    // app_binder::Connect gives every store connection.
    QObject::connect(&panel_, &DateGroupsTablePanel::DateGroupsEdited, &scope_,
                     [this](const std::vector<DateGroup>& date_groups) {
                       groups_store_.ReceiveDateGroups(date_groups);
                     });
    QObject::connect(&groups_topic_, &domain::DateGroupsTopic::Published,
                     &panel_, &DateGroupsTablePanel::ReceiveDateGroups);
    QObject::connect(&groups_topic_, &domain::DateGroupsTopic::Published,
                     &scope_,
                     [this](const std::vector<DateGroup>& date_groups) {
                       shape_store_.ReceiveDateGroups(date_groups);
                     });

    groups_store_.SendDefaultValues();

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

  [[nodiscard]] std::size_t GroupCount() const {
    return groups_store_.Get().Items().size();
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

  [[nodiscard]] glm::vec4 GroupFillColor(std::size_t group_index) const {
    return shape_store_.Get()
        .GetDynamicConfiguration(group_index)
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

  domain::DateGroupsTopic groups_topic_;
  domain::ShapeConfigSetTopic shape_topic_;
  DateGroupStore groups_store_{groups_topic_};
  ShapeConfigurationStore shape_store_{shape_topic_};
  DateGroupsTablePanel panel_{nullptr};

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

TEST(DateGroupsPanelTest, AddRowClickAddsOneGroupPerClick) {
  WiredGroupsPanel wired;
  ASSERT_EQ(wired.GroupCount(), 1U);

  wired.ClickAddRow();
  EXPECT_EQ(wired.GroupCount(), 2U);
  EXPECT_EQ(wired.TableRowCount(), 2);

  wired.ClickAddRow();
  wired.ClickAddRow();
  EXPECT_EQ(wired.GroupCount(), 4U);
  EXPECT_EQ(wired.TableRowCount(), 4);
}

TEST(DateGroupsPanelTest, AddRowClicksLeaveTheAnnualCoverageColorAlone) {
  WiredGroupsPanel wired;
  const glm::vec4 before = wired.AnnualCoverageFillColor();

  for (int click = 0; click < 3; ++click) {
    wired.ClickAddRow();
    ExpectSameColor(wired.AnnualCoverageFillColor(), before);
  }
}

TEST(DateGroupsPanelTest, AddRowClicksLeaveExistingGroupColorsAlone) {
  WiredGroupsPanel wired;
  const glm::vec4 first_group = wired.GroupFillColor(0);

  wired.ClickAddRow();
  const glm::vec4 second_group = wired.GroupFillColor(1);

  wired.ClickAddRow();
  ExpectSameColor(wired.GroupFillColor(0), first_group);
  ExpectSameColor(wired.GroupFillColor(1), second_group);
}
