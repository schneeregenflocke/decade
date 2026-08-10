#ifndef TITLE_TEXT_EDITOR_HPP
#define TITLE_TEXT_EDITOR_HPP

#include <cstddef>
#include <functional>
#include <glm/vec2.hpp>
#include <optional>
#include <string>

#include "../../domain/state_topics.hpp"
#include "../../domain/text_edit_buffer.hpp"
#include "../../domain/text_edit_view.hpp"
#include "../../domain/title_config_store.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"
#include "text_input_event.hpp"

// Application: editing the title straight in the canvas. The editor holds the
// buffer while the edit runs and publishes what is to be seen on every change
// (text, cursor, selection) — the renderer draws from that without knowing the
// editor.
//
// The text becomes canonical with Enter alone: then it goes to the store as one
// command, which announces it as a fact on its topic. Esc discards the buffer
// and leaves the store untouched. An edit is thereby exactly one state change —
// not one per keystroke.
class TitleTextEditor {
 public:
  // What lies at the pointer and which cursor index is meant there. The
  // rendering adapter knows both; injected, the editor stays free of it.
  using PickSource = std::function<std::optional<PickId>(glm::vec2)>;
  using CaretIndexSource = std::function<std::size_t(glm::vec2)>;

  TitleTextEditor(TitleConfigStore& title_store,
                  domain::TextEditTopic& edit_topic);

  void SetPickSource(PickSource pick_source);

  void SetCaretIndexSource(CaretIndexSource caret_index_source);

  [[nodiscard]] bool IsEditing() const;

  // A keyboard input, already in its meaning. Without a running edit it fizzles
  // out.
  void Handle(const TextInputEvent& event);

  // A click during the edit: inside the title it sets the cursor, outside it
  // ends the edit like Enter — whatever was typed stays. The return value says
  // whether the click is consumed.
  bool OnPrimaryDown(glm::vec2 page_point, TextEditBuffer::Selection selection);

  // A double click in the running edit selects the word beneath it.
  bool OnDoubleClick(glm::vec2 page_point);

  // Starts editing the element that was hit. The title alone is editable;
  // everything else leaves the editor at rest.
  void Begin(const PickId& picked);

  // What gets inserted is UTF-8 — as keyboard and clipboard both deliver it.
  void Insert(const std::string& text);

  void DeleteBefore();

  void DeleteAfter();

  void MoveCaret(TextEditBuffer::Direction direction,
                 TextEditBuffer::Selection selection);

  void SetCaret(std::size_t index, TextEditBuffer::Selection selection);

  void SelectWordAt(std::size_t index);

  void SelectAll();

  // The selected text as UTF-8, for the clipboard in presentation.
  [[nodiscard]] std::string SelectedText() const;

  [[nodiscard]] std::size_t Caret() const;

  // Enter: the buffer becomes canonical. The store publishes the fact, the
  // editor ends.
  void Commit();

  // Esc: the buffer goes, the store stays untouched.
  void Cancel();

 private:
  [[nodiscard]] static std::u32string ToCodePoints(const std::string& text);

  // Every change runs the same path: only while editing, and a repaint follows.
  template <typename Operation>
  void Apply(Operation operation) {
    if (!buffer_) {
      return;
    }
    operation(*buffer_);
    Publish(*buffer_);
  }

  void End();

  void Publish(const TextEditBuffer& buffer);

  [[nodiscard]] bool HitsTitle(glm::vec2 page_point) const;

  [[nodiscard]] std::size_t CaretIndexAt(glm::vec2 page_point) const;

  TitleConfigStore& title_store_;
  domain::TextEditTopic& edit_topic_;
  PickSource pick_source_;
  CaretIndexSource caret_index_source_;
  std::optional<TextEditBuffer> buffer_;
};

#endif  // TITLE_TEXT_EDITOR_HPP
