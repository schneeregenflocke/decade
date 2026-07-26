#ifndef TEXT_EDIT_VIEW_HPP
#define TEXT_EDIT_VIEW_HPP

#include <cstddef>
#include <string>

// Pure domain value: was von einer laufenden Textbearbeitung zu sehen ist.
// Der Renderer zeichnet daraus Text, Cursor und Auswahl, ohne den Editor selbst
// zu kennen; ein leeres `optional` beim Konsumenten heisst «niemand editiert».
// Der Text ist UTF-8 (so zeichnet ihn die Schrift), die Positionen zählen
// Codepoints (so rechnet der TextEditBuffer).
struct TextEditView {
  std::string text;
  std::size_t caret{0};
  std::size_t selection_begin{0};
  std::size_t selection_end{0};
};

[[nodiscard]] inline bool HasSelection(const TextEditView& view) {
  return view.selection_end > view.selection_begin;
}

#endif  // TEXT_EDIT_VIEW_HPP
