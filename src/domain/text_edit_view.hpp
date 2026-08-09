#ifndef TEXT_EDIT_VIEW_HPP
#define TEXT_EDIT_VIEW_HPP

#include <cstddef>
#include <string>

// A pure domain value: what is to be seen of a running text edit. The renderer
// draws text, cursor and selection from it without knowing the editor itself;
// an empty `optional` at the consumer means "nobody is editing". The text is
// UTF-8 (that is how the font draws it), the positions count code points (that
// is how TextEditBuffer computes).
struct TextEditView {
  std::string text;
  std::size_t caret{0};
  std::size_t selection_begin{0};
  std::size_t selection_end{0};
};

[[nodiscard]] bool HasSelection(const TextEditView& view);

#endif  // TEXT_EDIT_VIEW_HPP
