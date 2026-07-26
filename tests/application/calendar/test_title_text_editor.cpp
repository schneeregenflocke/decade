#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "application/calendar/title_text_editor.hpp"
#include "domain/state_topic.hpp"
#include "domain/text_edit_view.hpp"
#include "domain/title_config.hpp"
#include "domain/title_config_store.hpp"

namespace {

constexpr PickId kTitlePick{.kind = PickId::Kind::kTitle, .index = 0};
constexpr PickId kBarPick{.kind = PickId::Kind::kBar, .index = 0};

// Store plus Topics plus Editor, wie sie die Composition Root verbindet — nur
// ohne wx und ohne GL.
struct EditorFixture {
  domain::StateTopic<TitleConfig> title_topic;
  domain::StateTopic<std::optional<TextEditView>> edit_topic;
  TitleConfigStore store{title_topic};
  TitleTextEditor editor{store, edit_topic};
  std::optional<TextEditView> last_view;

  EditorFixture() {
    edit_topic.connect(
        [this](const std::optional<TextEditView>& view) { last_view = view; });
    TitleConfig config;
    config.SetTitleText("Titel");
    store.ReceiveTitleConfig(config);
  }
};

}  // namespace

TEST(TitleTextEditorTest, BeginSelectsTheWholeTitle) {
  EditorFixture fixture;
  fixture.editor.Begin(kTitlePick);

  ASSERT_TRUE(fixture.last_view.has_value());
  EXPECT_EQ(fixture.last_view->text, "Titel");
  EXPECT_EQ(fixture.last_view->selection_begin, 0U);
  EXPECT_EQ(fixture.last_view->selection_end, 5U);
  EXPECT_TRUE(fixture.editor.IsEditing());
}

TEST(TitleTextEditorTest, OnlyTheTitleIsEditable) {
  EditorFixture fixture;
  fixture.editor.Begin(kBarPick);
  EXPECT_FALSE(fixture.editor.IsEditing());
}

TEST(TitleTextEditorTest, TypingReplacesTheSelectedTitle) {
  EditorFixture fixture;
  fixture.editor.Begin(kTitlePick);
  fixture.editor.Insert("Neu");

  ASSERT_TRUE(fixture.last_view.has_value());
  EXPECT_EQ(fixture.last_view->text, "Neu");
  EXPECT_EQ(fixture.last_view->caret, 3U);
}

TEST(TitleTextEditorTest, UmlautsSurviveTheRoundTrip) {
  EditorFixture fixture;
  fixture.editor.Begin(kTitlePick);
  fixture.editor.Insert("Zehn Jahre Änderung");
  fixture.editor.Commit();

  EXPECT_EQ(fixture.store.Get().TitleText(), "Zehn Jahre Änderung");
}

TEST(TitleTextEditorTest, TheStoreLearnsTheTextOnlyOnCommit) {
  EditorFixture fixture;
  fixture.editor.Begin(kTitlePick);
  fixture.editor.Insert("Neu");
  EXPECT_EQ(fixture.store.Get().TitleText(), "Titel");

  fixture.editor.Commit();
  EXPECT_EQ(fixture.store.Get().TitleText(), "Neu");
  EXPECT_FALSE(fixture.editor.IsEditing());
  EXPECT_FALSE(fixture.last_view.has_value());
}

TEST(TitleTextEditorTest, CancelLeavesTheStoreUntouched) {
  EditorFixture fixture;
  fixture.editor.Begin(kTitlePick);
  fixture.editor.Insert("Verworfen");
  fixture.editor.Cancel();

  EXPECT_EQ(fixture.store.Get().TitleText(), "Titel");
  EXPECT_FALSE(fixture.editor.IsEditing());
}

TEST(TitleTextEditorTest, CommitKeepsTheOtherTitleSettings) {
  EditorFixture fixture;
  TitleConfig config = fixture.store.Get();
  config.SetFrameHeight(42.0F);
  fixture.store.ReceiveTitleConfig(config);

  fixture.editor.Begin(kTitlePick);
  fixture.editor.Insert("Neu");
  fixture.editor.Commit();

  EXPECT_FLOAT_EQ(fixture.store.Get().FrameHeight(), 42.0F);
}

TEST(TitleTextEditorTest, InputEventsReachTheBuffer) {
  EditorFixture fixture;
  fixture.editor.Begin(kTitlePick);
  fixture.editor.Handle(
      TextInputEvent::Command(TextInputEvent::Kind::kDeleteAfter));
  fixture.editor.Handle(TextInputEvent::Insert("Neu"));
  fixture.editor.Handle(TextInputEvent::Move(
      TextEditBuffer::Direction::kBegin, TextEditBuffer::Selection::kReplace));
  fixture.editor.Handle(TextInputEvent::Insert("A"));

  ASSERT_TRUE(fixture.last_view.has_value());
  EXPECT_EQ(fixture.last_view->text, "ANeu");
}

TEST(TitleTextEditorTest, SelectedTextIsUtf8ForTheClipboard) {
  EditorFixture fixture;
  TitleConfig config = fixture.store.Get();
  config.SetTitleText("Öl");
  fixture.store.ReceiveTitleConfig(config);

  fixture.editor.Begin(kTitlePick);
  EXPECT_EQ(fixture.editor.SelectedText(), "Öl");
}

TEST(TitleTextEditorTest, InputWithoutAnOpenEditorDoesNothing) {
  EditorFixture fixture;
  fixture.editor.Handle(TextInputEvent::Insert("Neu"));

  EXPECT_FALSE(fixture.editor.IsEditing());
  EXPECT_FALSE(fixture.last_view.has_value());
  EXPECT_EQ(fixture.store.Get().TitleText(), "Titel");
}
