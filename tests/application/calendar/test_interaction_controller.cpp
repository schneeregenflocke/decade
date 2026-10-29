#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <glm/vec2.hpp>
#include <optional>
#include <string>
#include <vector>

#include "application/calendar/interaction_controller.hpp"
#include "application/interaction_topics.hpp"
#include "domain/state_topics.hpp"
#include "graphics/pick_id.hpp"

namespace {

using Gesture = InteractionController::Gesture;

// Two bars side by side on the x axis, nothing elsewhere. A bar's path is
// "entry <index>", which is all the controller passes on.
constexpr glm::vec2 kFirstBar{0.5F, 0.0F};
constexpr glm::vec2 kSecondBar{1.5F, 0.0F};
constexpr glm::vec2 kNothing{5.0F, 0.0F};

// The controller wired to its topics the way app_binder does it, the selection
// topic fed back into the controller included.
struct ControllerFixture {
  application::HoveredTopic hovered_topic;
  domain::NodePathsTopic selected_topic;
  application::EditRequestTopic edit_topic;
  InteractionController controller{hovered_topic, selected_topic, edit_topic};
  std::optional<std::vector<std::string>> published;

  ControllerFixture() {
    controller.SetPickSource([](glm::vec2 point) -> std::optional<PickId> {
      if (point.x >= 0.0F && point.x < 2.0F) {
        return PickId{.kind = PickId::Kind::kBar,
                      .index = static_cast<std::size_t>(point.x)};
      }
      return std::nullopt;
    });
    controller.SetPathSource([](PickId picked) -> std::optional<std::string> {
      return "entry " + std::to_string(picked.index);
    });
    QObject::connect(&selected_topic, &domain::NodePathsTopic::Published,
                     [this](const std::vector<std::string>& paths) {
                       published = paths;
                       controller.ReceiveSelectedNodes(paths);
                     });
  }
};

using Paths = std::vector<std::string>;

}  // namespace

TEST(InteractionControllerTest, AClickSelectsTheHitBarAlone) {
  ControllerFixture fixture;
  fixture.controller.OnPrimaryDown(kFirstBar, Gesture::kReplace);
  fixture.controller.OnPrimaryDown(kSecondBar, Gesture::kReplace);

  EXPECT_EQ(fixture.published, Paths{"entry 1"});
}

TEST(InteractionControllerTest, AToggleAddsABarToTheSelection) {
  ControllerFixture fixture;
  fixture.controller.OnPrimaryDown(kFirstBar, Gesture::kReplace);
  fixture.controller.OnPrimaryDown(kSecondBar, Gesture::kToggle);

  EXPECT_EQ(fixture.published, (Paths{"entry 0", "entry 1"}));
}

TEST(InteractionControllerTest, AToggleOnASelectedBarTakesItOut) {
  ControllerFixture fixture;
  fixture.controller.OnPrimaryDown(kFirstBar, Gesture::kReplace);
  fixture.controller.OnPrimaryDown(kSecondBar, Gesture::kToggle);
  fixture.controller.OnPrimaryDown(kFirstBar, Gesture::kToggle);

  EXPECT_EQ(fixture.published, Paths{"entry 1"});
}

TEST(InteractionControllerTest, AToggleStartsFromASelectionMadeElsewhere) {
  ControllerFixture fixture;
  fixture.selected_topic.Publish({"entry 0"});
  fixture.controller.OnPrimaryDown(kSecondBar, Gesture::kToggle);

  EXPECT_EQ(fixture.published, (Paths{"entry 0", "entry 1"}));
}

TEST(InteractionControllerTest, AClickOnNothingClearsTheSelection) {
  ControllerFixture fixture;
  fixture.controller.OnPrimaryDown(kFirstBar, Gesture::kReplace);
  fixture.controller.OnPrimaryDown(kNothing, Gesture::kReplace);

  EXPECT_EQ(fixture.published, Paths{});
}

TEST(InteractionControllerTest, AToggleOnNothingKeepsTheSelection) {
  ControllerFixture fixture;
  fixture.controller.OnPrimaryDown(kFirstBar, Gesture::kReplace);
  fixture.controller.OnPrimaryDown(kNothing, Gesture::kToggle);

  EXPECT_EQ(fixture.published, Paths{"entry 0"});
}
