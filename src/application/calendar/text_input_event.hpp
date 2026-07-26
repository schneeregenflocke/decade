#ifndef TEXT_INPUT_EVENT_HPP
#define TEXT_INPUT_EVENT_HPP

#include <cstdint>
#include <string>
#include <utility>

#include "../../domain/text_edit_buffer.hpp"

// Application: eine Tastatureingabe, bereits von wx-Tastencodes in ihre
// Bedeutung übersetzt. Das Canvas kennt die Tasten, der Editor die Bedeutung —
// dieses Ereignis ist die Naht dazwischen, damit weder wx-Codes in die
// Application wandern noch Editierlogik in die Presentation.
//
// Die Zwischenablage kommt hier nicht vor: Kopieren ist Auswahl lesen (der
// Editor gibt sie als UTF-8 heraus), Einfügen ist ein kInsert. Der wx-Teil
// bleibt damit in der Presentation.
struct TextInputEvent {
  enum class Kind : std::uint8_t {
    kInsert,
    kDeleteBefore,
    kDeleteAfter,
    kMove,
    kSelectAll,
    kCommit,
    kCancel
  };

  Kind kind{Kind::kInsert};
  std::string text;  // UTF-8, nur bei kInsert
  TextEditBuffer::Direction direction{TextEditBuffer::Direction::kLeft};
  TextEditBuffer::Selection selection{TextEditBuffer::Selection::kReplace};

  // Benannte Fabriken statt Feld-für-Feld-Initialisierung: sie sagen, was
  // gemeint ist, und lassen die unbeteiligten Felder auf ihrem Vorgabewert.
  [[nodiscard]] static TextInputEvent Insert(std::string text) {
    return {.kind = Kind::kInsert, .text = std::move(text)};
  }

  [[nodiscard]] static TextInputEvent Move(
      TextEditBuffer::Direction direction,
      TextEditBuffer::Selection selection) {
    TextInputEvent event{.kind = Kind::kMove, .text = {}};
    event.direction = direction;
    event.selection = selection;
    return event;
  }

  [[nodiscard]] static TextInputEvent Command(Kind kind) {
    return {.kind = kind, .text = {}};
  }
};

#endif  // TEXT_INPUT_EVENT_HPP
