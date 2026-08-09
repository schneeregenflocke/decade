#include "text_edit_buffer.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

TextEditBuffer::TextEditBuffer(std::u32string text)
    : text_(std::move(text)), caret_(text_.size()), anchor_(text_.size()) {}

const std::u32string& TextEditBuffer::Text() const { return text_; }

std::size_t TextEditBuffer::Caret() const { return caret_; }

bool TextEditBuffer::HasSelection() const { return caret_ != anchor_; }

std::size_t TextEditBuffer::SelectionBegin() const {
  return std::min(caret_, anchor_);
}

std::size_t TextEditBuffer::SelectionEnd() const {
  return std::max(caret_, anchor_);
}

std::u32string TextEditBuffer::SelectedText() const {
  return text_.substr(SelectionBegin(), SelectionEnd() - SelectionBegin());
}

void TextEditBuffer::Insert(char32_t character) {
  Insert(std::u32string(1, character));
}

void TextEditBuffer::Insert(const std::u32string& text) {
  EraseSelection();
  text_.insert(caret_, text);
  SetCaret(caret_ + text.size(), Selection::kReplace);
}

void TextEditBuffer::DeleteBefore() {
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

void TextEditBuffer::DeleteAfter() {
  if (HasSelection()) {
    EraseSelection();
    return;
  }
  if (caret_ >= text_.size()) {
    return;
  }
  text_.erase(caret_, 1);
}

void TextEditBuffer::SetCaret(std::size_t index, Selection selection) {
  caret_ = std::min(index, text_.size());
  if (selection == Selection::kReplace) {
    anchor_ = caret_;
  }
}

void TextEditBuffer::MoveCaret(Direction direction, Selection selection) {
  if (selection == Selection::kReplace && HasSelection() &&
      (direction == Direction::kLeft || direction == Direction::kRight)) {
    SetCaret(direction == Direction::kLeft ? SelectionBegin() : SelectionEnd(),
             Selection::kReplace);
    return;
  }
  SetCaret(TargetIndex(direction), selection);
}

void TextEditBuffer::SelectAll() {
  anchor_ = 0;
  caret_ = text_.size();
}

void TextEditBuffer::SelectWordAt(std::size_t index) {
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

bool TextEditBuffer::IsSpace(char32_t character) {
  return character == U' ' || character == U'\t';
}

std::size_t TextEditBuffer::TargetIndex(Direction direction) const {
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

void TextEditBuffer::EraseSelection() {
  if (!HasSelection()) {
    return;
  }
  const std::size_t begin = SelectionBegin();
  text_.erase(begin, SelectionEnd() - begin);
  SetCaret(begin, Selection::kReplace);
}
