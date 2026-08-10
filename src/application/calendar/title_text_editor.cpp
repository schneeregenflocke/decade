#include "title_text_editor.hpp"

#include <cstddef>
#include <glm/ext/vector_float2.hpp>
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

TitleTextEditor::TitleTextEditor(
    TitleConfigStore& title_store,
    domain::StateTopic<std::optional<TextEditView>>& edit_topic)
    : title_store_(title_store), edit_topic_(edit_topic) {}

void TitleTextEditor::SetPickSource(PickSource pick_source) {
  pick_source_ = std::move(pick_source);
}

void TitleTextEditor::SetCaretIndexSource(CaretIndexSource caret_index_source) {
  caret_index_source_ = std::move(caret_index_source);
}

bool TitleTextEditor::IsEditing() const { return buffer_.has_value(); }

void TitleTextEditor::Handle(const TextInputEvent& event) {
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

bool TitleTextEditor::OnPrimaryDown(glm::vec2 page_point,
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

bool TitleTextEditor::OnDoubleClick(glm::vec2 page_point) {
  if (!buffer_ || !HitsTitle(page_point)) {
    return false;
  }
  SelectWordAt(CaretIndexAt(page_point));
  return true;
}

void TitleTextEditor::Begin(const PickId& picked) {
  if (picked.kind != PickId::Kind::kTitle) {
    return;
  }
  TextEditBuffer buffer(ToCodePoints(title_store_.Get().TitleText()));
  buffer.SelectAll();
  buffer_ = buffer;
  Publish(buffer);
}

void TitleTextEditor::Insert(const std::string& text) {
  const std::u32string code_points = ToCodePoints(text);
  Apply([&code_points](TextEditBuffer& buffer) { buffer.Insert(code_points); });
}

void TitleTextEditor::DeleteBefore() {
  Apply([](TextEditBuffer& buffer) { buffer.DeleteBefore(); });
}

void TitleTextEditor::DeleteAfter() {
  Apply([](TextEditBuffer& buffer) { buffer.DeleteAfter(); });
}

void TitleTextEditor::MoveCaret(TextEditBuffer::Direction direction,
                                TextEditBuffer::Selection selection) {
  Apply([direction, selection](TextEditBuffer& buffer) {
    buffer.MoveCaret(direction, selection);
  });
}

void TitleTextEditor::SetCaret(std::size_t index,
                               TextEditBuffer::Selection selection) {
  Apply([index, selection](TextEditBuffer& buffer) {
    buffer.SetCaret(index, selection);
  });
}

void TitleTextEditor::SelectWordAt(std::size_t index) {
  Apply([index](TextEditBuffer& buffer) { buffer.SelectWordAt(index); });
}

void TitleTextEditor::SelectAll() {
  Apply([](TextEditBuffer& buffer) { buffer.SelectAll(); });
}

std::string TitleTextEditor::SelectedText() const {
  if (!buffer_) {
    return {};
  }
  return EncodeUtf8(buffer_->SelectedText());
}

std::size_t TitleTextEditor::Caret() const {
  return buffer_ ? buffer_->Caret() : 0;
}

void TitleTextEditor::Commit() {
  if (!buffer_) {
    return;
  }
  TitleConfig config = title_store_.Get();
  config.SetTitleText(EncodeUtf8(buffer_->Text()));
  End();
  title_store_.ReceiveTitleConfig(config);
}

void TitleTextEditor::Cancel() { End(); }

std::u32string TitleTextEditor::ToCodePoints(const std::string& text) {
  const std::vector<char32_t> decoded = DecodeUtf8(text);
  return {decoded.begin(), decoded.end()};
}

void TitleTextEditor::End() {
  if (!buffer_) {
    return;
  }
  buffer_.reset();
  edit_topic_(std::nullopt);
}

void TitleTextEditor::Publish(const TextEditBuffer& buffer) {
  edit_topic_(TextEditView{.text = EncodeUtf8(buffer.Text()),
                           .caret = buffer.Caret(),
                           .selection_begin = buffer.SelectionBegin(),
                           .selection_end = buffer.SelectionEnd()});
}

bool TitleTextEditor::HitsTitle(glm::vec2 page_point) const {
  if (!pick_source_) {
    return false;
  }
  const std::optional<PickId> hit = pick_source_(page_point);
  return hit.has_value() && hit->kind == PickId::Kind::kTitle;
}

std::size_t TitleTextEditor::CaretIndexAt(glm::vec2 page_point) const {
  return caret_index_source_ ? caret_index_source_(page_point) : Caret();
}
