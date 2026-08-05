#ifndef TEXT_EDIT_BUFFER_HPP
#define TEXT_EDIT_BUFFER_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

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

  explicit TextEditBuffer(std::u32string text)
      : text_(std::move(text)), caret_(text_.size()), anchor_(text_.size()) {}

  [[nodiscard]] const std::u32string& Text() const { return text_; }
  [[nodiscard]] std::size_t Caret() const { return caret_; }

  [[nodiscard]] bool HasSelection() const { return caret_ != anchor_; }
  [[nodiscard]] std::size_t SelectionBegin() const {
    return std::min(caret_, anchor_);
  }
  [[nodiscard]] std::size_t SelectionEnd() const {
    return std::max(caret_, anchor_);
  }

  [[nodiscard]] std::u32string SelectedText() const {
    return text_.substr(SelectionBegin(), SelectionEnd() - SelectionBegin());
  }

  void Insert(char32_t character) { Insert(std::u32string(1, character)); }

  void Insert(const std::u32string& text) {
    EraseSelection();
    text_.insert(caret_, text);
    SetCaret(caret_ + text.size(), Selection::kReplace);
  }

  // Backspace: it deletes the selection, otherwise the character before the cursor.
  void DeleteBefore() {
    if (HasSelection()) {
      EraseSelection();
      return;
    }
    if (caret_ == 0) {
      return;
    }
    text_.erase(caret_ - 1, 1);
    SetCaret(caret_ - 1, Selection::kReplace);
  }

  // Delete: it removes the selection, otherwise the character after the cursor.
  void DeleteAfter() {
    if (HasSelection()) {
      EraseSelection();
      return;
    }
    if (caret_ >= text_.size()) {
      return;
    }
    text_.erase(caret_, 1);
  }

  void SetCaret(std::size_t index, Selection selection) {
    caret_ = std::min(index, text_.size());
    if (selection == Selection::kReplace) {
      anchor_ = caret_;
    }
  }

  // A movement without Shift cancels an existing selection and puts the cursor
  // at its edge — as in every text field.
  void MoveCaret(Direction direction, Selection selection) {
    if (selection == Selection::kReplace && HasSelection() &&
        (direction == Direction::kLeft || direction == Direction::kRight)) {
      SetCaret(
          direction == Direction::kLeft ? SelectionBegin() : SelectionEnd(),
          Selection::kReplace);
      return;
    }
    SetCaret(TargetIndex(direction), selection);
  }

  void SelectAll() {
    anchor_ = 0;
    caret_ = text_.size();
  }

  // Selects the word around `index`; on whitespace the contiguous gap. Words
  // are runs of non-whitespace here — enough for a single-line title, where ICU
  // word boundaries would be effort without a visible gain.
  void SelectWordAt(std::size_t index) {
    if (text_.empty()) {
      return;
    }
    const std::size_t position = std::min(index, text_.size() - 1);
    const bool space = IsSpace(text_[position]);

    std::size_t begin = position;
    while (begin > 0 && IsSpace(text_[begin - 1]) == space) {
      --begin;
    }
    std::size_t end = position;
    while (end < text_.size() && IsSpace(text_[end]) == space) {
      ++end;
    }
    anchor_ = begin;
    caret_ = end;
  }

 private:
  static bool IsSpace(char32_t character) {
    return character == U' ' || character == U'\t';
  }

  [[nodiscard]] std::size_t TargetIndex(Direction direction) const {
    switch (direction) {
      case Direction::kLeft:
        return caret_ > 0 ? caret_ - 1 : 0;
      case Direction::kRight:
        return caret_ + 1;
      case Direction::kBegin:
        return 0;
      case Direction::kEnd:
        return text_.size();
    }
    return caret_;
  }

  void EraseSelection() {
    if (!HasSelection()) {
      return;
    }
    const std::size_t begin = SelectionBegin();
    text_.erase(begin, SelectionEnd() - begin);
    SetCaret(begin, Selection::kReplace);
  }

  std::u32string text_;
  std::size_t caret_{0};
  std::size_t anchor_{0};
};

#endif  // TEXT_EDIT_BUFFER_HPP
