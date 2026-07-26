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

// Application: das Bearbeiten des Titels direkt im Canvas. Der Editor hält den
// Puffer, solange bearbeitet wird, und veröffentlicht bei jeder Änderung, was
// zu sehen ist (Text, Cursor, Auswahl) — der Renderer zeichnet daraus, ohne den
// Editor zu kennen.
//
// Kanonisch wird der Text erst mit Enter: dann geht er als ein Befehl an den
// Store, der ihn als Tatsache auf seinem Topic bekanntgibt. Esc verwirft den
// Puffer, der Store bleibt unberührt. Damit ist eine Bearbeitung genau ein
// Zustandswechsel — nicht einer je Tastenanschlag.
class TitleTextEditor {
 public:
  // Was am Zeigerpunkt liegt und welcher Cursor-Index dort gemeint ist. Beides
  // weiss der Rendering-Adapter; eingesetzt bleibt der Editor von ihm frei.
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

  // Eine Tastatureingabe, schon in ihrer Bedeutung. Ohne laufende Bearbeitung
  // verpufft sie.
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

  // Ein Klick während der Bearbeitung: im Titel setzt er den Cursor, ausserhalb
  // beendet er die Bearbeitung wie Enter — was getippt war, bleibt erhalten.
  // Der Rückgabewert sagt, ob der Klick verbraucht ist.
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

  // Ein Doppelklick in der laufenden Bearbeitung wählt das Wort darunter.
  bool OnDoubleClick(glm::vec2 page_point) {
    if (!buffer_ || !HitsTitle(page_point)) {
      return false;
    }
    SelectWordAt(CaretIndexAt(page_point));
    return true;
  }

  // Startet das Bearbeiten des getroffenen Elements. Nur der Titel ist
  // bearbeitbar; alles andere lässt den Editor ruhen.
  void Begin(const PickId& picked) {
    if (picked.kind != PickId::Kind::kTitle) {
      return;
    }
    TextEditBuffer buffer(ToCodePoints(title_store_.Get().TitleText()));
    buffer.SelectAll();
    buffer_ = buffer;
    Publish(buffer);
  }

  // Eingefügt wird UTF-8 — so liefert es die Tastatur wie die Zwischenablage.
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

  // Der ausgewählte Text als UTF-8, für die Zwischenablage der Presentation.
  [[nodiscard]] std::string SelectedText() const {
    if (!buffer_) {
      return {};
    }
    return EncodeUtf8(buffer_->SelectedText());
  }

  [[nodiscard]] std::size_t Caret() const {
    return buffer_ ? buffer_->Caret() : 0;
  }

  // Enter: der Puffer wird kanonisch. Der Store veröffentlicht die Tatsache,
  // der Editor endet.
  void Commit() {
    if (!buffer_) {
      return;
    }
    TitleConfig config = title_store_.Get();
    config.SetTitleText(EncodeUtf8(buffer_->Text()));
    End();
    title_store_.ReceiveTitleConfig(config);
  }

  // Esc: Puffer weg, Store unberührt.
  void Cancel() { End(); }

 private:
  [[nodiscard]] static std::u32string ToCodePoints(const std::string& text) {
    const std::vector<char32_t> decoded = DecodeUtf8(text);
    return {decoded.begin(), decoded.end()};
  }

  // Jede Änderung läuft über denselben Pfad: nur wenn bearbeitet wird, und
  // danach ist neu zu zeichnen.
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
