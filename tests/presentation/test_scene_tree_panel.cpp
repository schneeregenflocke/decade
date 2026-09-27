#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtCore/Qt>
#include <QtTest/QTest>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "domain/scene_snapshot.hpp"
#include "presentation/scene_tree_panel.hpp"

namespace {

using Paths = std::vector<std::string>;

SceneNodeSnapshot Node(const std::string& name,
                       std::vector<SceneNodeSnapshot> children = {}) {
  SceneNodeSnapshot node;
  node.values.name = name;
  // A leaf draws; the tree marks a node without a shape in its label.
  node.values.has_shape = children.empty();
  node.children = std::move(children);
  return node;
}

// root -> Date Bars -> entry 1, entry 2, shown and listening to what the
// panel reports.
struct ShownTree {
  SceneTreePanel panel{nullptr};
  std::optional<Paths> reported;

  ShownTree() {
    panel.ReceiveSceneSnapshot(
        Node("root", {Node("Date Bars", {Node("entry 1"), Node("entry 2")})}));
    QObject::connect(&panel, &SceneTreePanel::SelectedNodesChanged,
                     [this](const Paths& paths) { reported = paths; });
    constexpr int kWidth = 500;
    constexpr int kHeight = 400;
    panel.resize(kWidth, kHeight);
    panel.show();
  }

  // The panel holds two tree widgets; the node tree is the one without a
  // header, the detail list beside it names its columns.
  [[nodiscard]] QTreeWidget& Tree() const {
    for (QTreeWidget* tree : panel.findChildren<QTreeWidget*>()) {
      if (tree->isHeaderHidden()) {
        return *tree;
      }
    }
    throw std::logic_error("the panel carries no node tree");
  }

  [[nodiscard]] QTreeWidgetItem* ItemNamed(const QString& name) const {
    const auto found =
        Tree().findItems(name, Qt::MatchExactly | Qt::MatchRecursive);
    return found.isEmpty() ? nullptr : found.first();
  }

  void Click(const QString& name, Qt::KeyboardModifiers modifiers = {}) {
    QTreeWidgetItem* item = ItemNamed(name);
    ASSERT_NE(item, nullptr);
    const QRect row = Tree().visualItemRect(item);
    QTest::mouseClick(Tree().viewport(), Qt::LeftButton, modifiers,
                      row.center());
  }
};

}  // namespace

TEST(SceneTreePanelTest, ControlClicksSelectSeveralNodes) {
  ShownTree shown;
  shown.Click("entry 1");
  shown.Click("entry 2", Qt::ControlModifier);

  EXPECT_EQ(shown.reported,
            (Paths{"root/Date Bars/entry 1", "root/Date Bars/entry 2"}));
}

TEST(SceneTreePanelTest, ASelectionFromOutsideIsShownWithoutBeingReported) {
  ShownTree shown;
  shown.panel.ReceiveSelectedNodes(
      {"root/Date Bars/entry 1", "root/Date Bars/entry 2"});

  ASSERT_NE(shown.ItemNamed("entry 1"), nullptr);
  EXPECT_TRUE(shown.ItemNamed("entry 1")->isSelected());
  EXPECT_TRUE(shown.ItemNamed("entry 2")->isSelected());
  EXPECT_FALSE(shown.reported.has_value());
}

TEST(SceneTreePanelTest, ARebuildKeepsTheSelection) {
  ShownTree shown;
  shown.panel.ReceiveSelectedNodes(
      {"root/Date Bars/entry 1", "root/Date Bars/entry 2"});
  shown.panel.ReceiveSceneSnapshot(
      Node("root", {Node("Date Bars", {Node("entry 1"), Node("entry 2")})}));

  ASSERT_NE(shown.ItemNamed("entry 1"), nullptr);
  EXPECT_TRUE(shown.ItemNamed("entry 1")->isSelected());
  EXPECT_TRUE(shown.ItemNamed("entry 2")->isSelected());
  EXPECT_FALSE(shown.reported.has_value());
}
