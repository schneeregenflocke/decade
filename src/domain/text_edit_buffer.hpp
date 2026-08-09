#ifndef TEXT_EDIT_BUFFER_HPP
#define TEXT_EDIT_BUFFER_HPP

#include <cstddef>
#include <cstdint>
#include <string>

// A pure domain value: a single-line text buffer with cursor and selection —
// the editing logic behind editing in the canvas, without wx, GL or font
// metrics. Positions count code points, not bytes, hence `std::u32string`: that
// way an umlaut is one step, and the renderer measures the same units.
//
// Cursor and anchor span the selection. Without a selection both sit on each
// other; every insert and delete replaces the selection first.
class TextEditBuffer {
 public:
  // Whether a movement spans the selection (Shift held) or cancels it.
  enum class Selection : std::uint8_t { kReplace, kExtend };

  enum class Direction : std::uint8_t { kLeft, kRight, kBegin, kEnd };

  TextEditBuffer() = default;

  explicit TextEditBuffer(std::u32string text);

  [[nodiscard]] const std::u32string& Text() const;
  [[nodiscard]] std::size_t Caret() const;

  [[nodiscard]] bool HasSelection() const;
  [[nodiscard]] std::size_t SelectionBegin() const;
  [[nodiscard]] std::size_t SelectionEnd() const;

  [[nodiscard]] std::u32string SelectedText() const;

  void Insert(char32_t character);

  void Insert(const std::u32string& text);

  // Backspace: it deletes the selection, otherwise the character before the
  // cursor.
  void DeleteBefore();

  // Delete: it removes the selection, otherwise the character after the cursor.
  void DeleteAfter();

  void SetCaret(std::size_t index, Selection selection);

  // A movement without Shift cancels an existing selection and puts the cursor
  // at its edge — as in every text field.
  void MoveCaret(Direction direction, Selection selection);

  void SelectAll();

  // Selects the word around `index`; on whitespace the contiguous gap. Words
  // are runs of non-whitespace here — enough for a single-line title, where ICU
  // word boundaries would be effort without a visible gain.
  void SelectWordAt(std::size_t index);

 private:
  static bool IsSpace(char32_t character);

  [[nodiscard]] std::size_t TargetIndex(Direction direction) const;

  void EraseSelection();

  std::u32string text_;
  std::size_t caret_{0};
  std::size_t anchor_{0};
};

#endif  // TEXT_EDIT_BUFFER_HPP
