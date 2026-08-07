#include <gtest/gtest.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <memory>
#include <string>
#include <vector>

#include "infrastructure/graphics/drawable.hpp"
#include "infrastructure/graphics/rect.hpp"
#include "infrastructure/graphics/scene_graph.hpp"

// Characterisation test over SceneNode. It exists because of the seam that
// `Drawable` opened (#77): the scene graph used to hold a `Shape`, whose
// members allocate GL objects in their constructors, so no node could be built
// without a live context and none of this was reachable.
//
// It pins the traversal: the accumulated transforms, the painter's layer, the
// sibling order (#29) and the node path.

namespace {

// The whole reason a test can exist here: a drawable that draws nothing and
// touches no GL. It records the world matrix it was painted with, in call
// order, so the draw sequence becomes observable.
class RecordingDrawable : public Drawable {
 public:
  RecordingDrawable(std::string name, const rectf& bounds,
                    std::vector<std::string>& log)
      : name_(std::move(name)), bounds_(bounds), log_(log) {}

  void Draw(const glm::mat4& model) const override {
    log_.push_back(name_ + "@" + std::to_string(static_cast<int>(model[3][0])));
  }

  [[nodiscard]] const rectf& LocalBounds() const override { return bounds_; }

 private:
  std::string name_;
  rectf bounds_;
  std::vector<std::string>& log_;
};

std::shared_ptr<SceneNode> MakeNode(const std::string& name,
                                    std::vector<std::string>& log,
                                    const rectf& bounds = rectf(0.0F, 1.0F,
                                                                0.0F, 1.0F)) {
  return std::make_shared<SceneNode>(
      name, std::make_unique<RecordingDrawable>(name, bounds, log));
}

glm::mat4 ShiftedBy(float x_offset) {
  return glm::translate(glm::mat4(1.0F), glm::vec3(x_offset, 0.0F, 0.0F));
}

}  // namespace

// --- The draw order ---

// Siblings of one layer are painted in the order they were added, so the first
// child sits furthest back and the last one on top (#29). The stack pushes them
// back to front for exactly this reason.
TEST(SceneNodeCharacterisation, SiblingsOfOneLayerKeepTheirOrder) {
  std::vector<std::string> log;
  SceneNode root("root");
  root.AddChild(MakeNode("first", log));
  root.AddChild(MakeNode("second", log));
  root.AddChild(MakeNode("third", log));

  root.Draw();

  ASSERT_EQ(log.size(), 3U);
  EXPECT_EQ(log[0], "first@0");
  EXPECT_EQ(log[1], "second@0");
  EXPECT_EQ(log[2], "third@0");
}

// Depth first: a subtree gets painted whole before the next sibling starts, so
// a later sibling covers an earlier one's children too.
TEST(SceneNodeCharacterisation, ASubtreeIsPaintedBeforeTheNextSibling) {
  std::vector<std::string> log;
  SceneNode root("root");
  auto branch = MakeNode("branch", log);
  branch->AddChild(MakeNode("branch_child", log));
  root.AddChild(branch);
  root.AddChild(MakeNode("later", log));

  root.Draw();

  ASSERT_EQ(log.size(), 3U);
  EXPECT_EQ(log[0], "branch@0");
  EXPECT_EQ(log[1], "branch_child@0");
  EXPECT_EQ(log[2], "later@0");
}

// The layer beats the tree position, and that part does hold: a lower layer is
// painted first however deep in the hierarchy it sits.
TEST(SceneNodeCharacterisation, LowerLayersAreDrawnFirst) {
  std::vector<std::string> log;
  SceneNode root("root");
  auto high = MakeNode("high", log);
  high->SetDrawLayer(10);
  auto low = MakeNode("low", log);
  low->SetDrawLayer(-5);
  root.AddChild(high);
  root.AddChild(low);

  root.Draw();

  ASSERT_EQ(log.size(), 2U);
  EXPECT_EQ(log[0], "low@0");
  EXPECT_EQ(log[1], "high@0");
}

// A node without geometry takes part in no draw call, but its transform still
// reaches its children.
TEST(SceneNodeCharacterisation, ContainerNodesTransformButDoNotDraw) {
  std::vector<std::string> log;
  SceneNode root("root");
  auto container = std::make_shared<SceneNode>("container");
  container->SetModelMatrix(ShiftedBy(7.0F));
  container->AddChild(MakeNode("child", log));
  root.AddChild(container);

  root.Draw();

  ASSERT_EQ(log.size(), 1U);
  EXPECT_EQ(log[0], "child@7");
}

// The transforms accumulate down the chain, parent before child.
TEST(SceneNodeCharacterisation, TransformsAccumulateAlongTheChain) {
  std::vector<std::string> log;
  SceneNode root("root");
  root.SetModelMatrix(ShiftedBy(1.0F));
  auto middle = std::make_shared<SceneNode>("middle");
  middle->SetModelMatrix(ShiftedBy(10.0F));
  middle->AddChild(MakeNode("leaf", log));
  root.AddChild(middle);

  root.Draw();

  ASSERT_EQ(log.size(), 1U);
  EXPECT_EQ(log[0], "leaf@11");
}

// --- The bounding box ---

TEST(SceneNodeCharacterisation, WorldBoundsSpanTheWholeSubtree) {
  std::vector<std::string> log;
  SceneNode root("root");
  root.AddChild(MakeNode("left", log, rectf(0.0F, 2.0F, 0.0F, 2.0F)));
  auto shifted = MakeNode("right", log, rectf(0.0F, 2.0F, 0.0F, 2.0F));
  shifted->SetModelMatrix(ShiftedBy(10.0F));
  root.AddChild(shifted);

  const auto bounds = root.WorldBounds();

  ASSERT_TRUE(bounds.has_value());
  EXPECT_FLOAT_EQ(bounds->Left(), 0.0F);
  EXPECT_FLOAT_EQ(bounds->Right(), 12.0F);
  EXPECT_FLOAT_EQ(bounds->Bottom(), 0.0F);
  EXPECT_FLOAT_EQ(bounds->Top(), 2.0F);
}

// A zero-extent box counts as no geometry, so a tree of such nodes reports
// nothing rather than a box at the origin.
TEST(SceneNodeCharacterisation, EmptyBoxesContributeNoBounds) {
  std::vector<std::string> log;
  SceneNode root("root");
  root.AddChild(MakeNode("flat", log, rectf(0.0F, 0.0F, 0.0F, 0.0F)));

  EXPECT_FALSE(root.WorldBounds().has_value());
}

TEST(SceneNodeCharacterisation, ANodeWithoutGeometryReportsNoBounds) {
  SceneNode root("root");
  root.AddChild(std::make_shared<SceneNode>("container"));

  EXPECT_FALSE(root.WorldBounds().has_value());
}

// --- The node path ---

TEST(SceneNodeCharacterisation, NodePathJoinsTheNamesFromTheRoot) {
  std::vector<std::string> log;
  SceneNode root("root");
  auto branch = std::make_shared<SceneNode>("branch");
  auto leaf = MakeNode("leaf", log);
  branch->AddChild(leaf);
  root.AddChild(branch);

  const auto path = FindNodePath(root, *leaf);

  ASSERT_TRUE(path.has_value());
  EXPECT_EQ(*path, "root/branch/leaf");
}

TEST(SceneNodeCharacterisation, NodePathIsEmptyForAStranger) {
  SceneNode root("root");
  const SceneNode stranger("stranger");

  EXPECT_FALSE(FindNodePath(root, stranger).has_value());
}
