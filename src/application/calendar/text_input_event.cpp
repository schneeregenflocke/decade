#include "text_input_event.hpp"

#include <string>
#include <utility>

#include "../../domain/text_edit_buffer.hpp"

TextInputEvent TextInputEvent::Insert(std::string text) {
  return {.kind = Kind::kInsert, .text = std::move(text)};
}

TextInputEvent TextInputEvent::Move(TextEditBuffer::Direction direction,
                                    TextEditBuffer::Selection selection) {
  TextInputEvent event{.kind = Kind::kMove, .text = {}};
  event.direction = direction;
  event.selection = selection;
  return event;
}

TextInputEvent TextInputEvent::Command(Kind kind) {
  return {.kind = kind, .text = {}};
}
