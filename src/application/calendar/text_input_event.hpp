#ifndef TEXT_INPUT_EVENT_HPP
#define TEXT_INPUT_EVENT_HPP

#include <cstdint>
#include <string>
#include <utility>

#include "../../domain/text_edit_buffer.hpp"

// Application: a keyboard input, already translated from Qt key codes into its
// meaning. The canvas knows the keys, the editor the meaning — this event is
// the seam between them, so that neither Qt codes travel into the application
// nor editing logic into presentation.
//
// The clipboard does not turn up here: copying is reading the selection (the
// editor hands it out as UTF-8), pasting is a kInsert. The toolkit part
// thereby stays in presentation.
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
  std::string text;  // UTF-8, on kInsert alone
  TextEditBuffer::Direction direction{TextEditBuffer::Direction::kLeft};
  TextEditBuffer::Selection selection{TextEditBuffer::Selection::kReplace};

  // Named factories instead of field-by-field initialisation: they say what is
  // meant and leave the uninvolved fields at their default.
  [[nodiscard]] static TextInputEvent Insert(std::string text);

  [[nodiscard]] static TextInputEvent Move(TextEditBuffer::Direction direction,
                                           TextEditBuffer::Selection selection);

  [[nodiscard]] static TextInputEvent Command(Kind kind);
};

#endif  // TEXT_INPUT_EVENT_HPP
