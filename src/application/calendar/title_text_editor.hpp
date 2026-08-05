#ifndef TITLE_TEXT_EDITOR_HPP
#define TITLE_TEXT_EDITOR_HPP

#include <cstddef>
#include <functional>
#include <glm/vec2.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../../domain/state_topic.hpp"
#include "../../domain/text_edit_buffer.hpp"
#include "../../domain/text_edit_view.hpp"
#include "../../domain/title_config.hpp"
#include "../../domain/title_config_store.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"
#include "../../infrastructure/graphics/utf8_codec.hpp"
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
                  domain::StateTopic<std::optional<TextEditView>>& edit_topic)
      : title_store_(title_store), edit_topic_(edit_topic) {}

  void SetPickSource(PickSource pick_source) {
    pick_source_ = std::move(pick_source);
  }

  void SetCaretIndexSource(CaretIndexSource caret_index_source) {
    caret_index_source_ = std::move(caret_index_source);
  }

  [[nodiscard]] bool IsEditing() const { return buffer_.has_value(); }

  // A keyboard input, already in its meaning. Without a running edit it fizzles
  // out.
  void Handle(const TextInputEvent& event) {
    switch (event.kind) {
      case TextInputEvent::Kind::kInsert:
        Insert(event.text);
        break;
      case TextInputEvent::Kind::kDeleteBefore:
        DeleteBefore();
        break;
      case TextInputEvent::Kind::kDeleteAfter:
        DeleteAfter();
        break;
      case TextInputEvent::Kind::kMove:
        MoveCaret(event.direction, event.selection);
        break;
      case TextInputEvent::Kind::kSelectAll:
        SelectAll();
        break;
      case TextInputEvent::Kind::kCommit:
        Commit();
        break;
      case TextInputEvent::Kind::kCancel:
        Cancel();
        break;
    }
  }

  // A click during the edit: inside the title it sets the cursor, outside it
  // ends the edit like Enter — whatever was typed stays. The return value says
  // whether the click is consumed.
  bool OnPrimaryDown(glm::vec2 page_point,
                     TextEditBuffer::Selection selection) {
    if (!buffer_) {
      return false;
    }
    if (!HitsTitle(page_point)) {
      Commit();
      return false;
    }
    SetCaret(CaretIndexAt(page_point), selection);
    return true;
  }

  // A double click in the running edit selects the word beneath it.
  bool OnDoubleClick(glm::vec2 page_point) {
    if (!buffer_ || !HitsTitle(page_point)) {
      return false;
    }
    SelectWordAt(CaretIndexAt(page_point));
    return true;
  }

  // Starts editing the element that was hit. The title alone is editable;
  // everything else leaves the editor at rest.
  void Begin(const PickId& picked) {
    if (picked.kind != PickId::Kind::kTitle) {
      return;
    }
    TextEditBuffer buffer(ToCodePoints(title_store_.Get().TitleText()));
    buffer.SelectAll();
    buffer_ = buffer;
    Publish(buffer);
  }

  // What gets inserted is UTF-8 — as keyboard and clipboard both deliver it.
  void Insert(const std::string& text) {
    const std::u32string code_points = ToCodePoints(text);
    Apply(
        [&code_points](TextEditBuffer& buffer) { buffer.Insert(code_points); });
  }

  void DeleteBefore() {
    Apply([](TextEditBuffer& buffer) { buffer.DeleteBefore(); });
  }

  void DeleteAfter() {
    Apply([](TextEditBuffer& buffer) { buffer.DeleteAfter(); });
  }

  void MoveCaret(TextEditBuffer::Direction direction,
                 TextEditBuffer::Selection selection) {
    Apply([direction, selection](TextEditBuffer& buffer) {
      buffer.MoveCaret(direction, selection);
    });
  }

  void SetCaret(std::size_t index, TextEditBuffer::Selection selection) {
    Apply([index, selection](TextEditBuffer& buffer) {
      buffer.SetCaret(index, selection);
    });
  }

  void SelectWordAt(std::size_t index) {
    Apply([index](TextEditBuffer& buffer) { buffer.SelectWordAt(index); });
  }

  void SelectAll() {
    Apply([](TextEditBuffer& buffer) { buffer.SelectAll(); });
  }

  // The selected text as UTF-8, for the clipboard in presentation.
  [[nodiscard]] std::string SelectedText() const {
    if (!buffer_) {
      return {};
    }
    return EncodeUtf8(buffer_->SelectedText());
  }

  [[nodiscard]] std::size_t Caret() const {
    return buffer_ ? buffer_->Caret() : 0;
  }

  // Enter: the buffer becomes canonical. The store publishes the fact, the
  // editor ends.
  void Commit() {
    if (!buffer_) {
      return;
    }
    TitleConfig config = title_store_.Get();
    config.SetTitleText(EncodeUtf8(buffer_->Text()));
    End();
    title_store_.ReceiveTitleConfig(config);
  }

  // Esc: the buffer goes, the store stays untouched.
  void Cancel() { End(); }

 private:
  [[nodiscard]] static std::u32string ToCodePoints(const std::string& text) {
    const std::vector<char32_t> decoded = DecodeUtf8(text);
    return {decoded.begin(), decoded.end()};
  }

  // Every change runs the same path: only while editing, and a repaint follows.
  template <typename Operation>
  void Apply(Operation operation) {
    if (!buffer_) {
      return;
    }
    operation(*buffer_);
    Publish(*buffer_);
  }

  void End() {
    if (!buffer_) {
      return;
    }
    buffer_.reset();
    edit_topic_(std::nullopt);
  }

  void Publish(const TextEditBuffer& buffer) {
    edit_topic_(TextEditView{.text = EncodeUtf8(buffer.Text()),
                             .caret = buffer.Caret(),
                             .selection_begin = buffer.SelectionBegin(),
                             .selection_end = buffer.SelectionEnd()});
  }

  [[nodiscard]] bool HitsTitle(glm::vec2 page_point) const {
    if (!pick_source_) {
      return false;
    }
    const std::optional<PickId> hit = pick_source_(page_point);
    return hit.has_value() && hit->kind == PickId::Kind::kTitle;
  }

  [[nodiscard]] std::size_t CaretIndexAt(glm::vec2 page_point) const {
    return caret_index_source_ ? caret_index_source_(page_point) : Caret();
  }

  TitleConfigStore& title_store_;
  domain::StateTopic<std::optional<TextEditView>>& edit_topic_;
  PickSource pick_source_;
  CaretIndexSource caret_index_source_;
  std::optional<TextEditBuffer> buffer_;
};

#endif  // TITLE_TEXT_EDITOR_HPP
