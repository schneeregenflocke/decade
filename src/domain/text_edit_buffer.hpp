#ifndef TEXT_EDIT_BUFFER_HPP
#define TEXT_EDIT_BUFFER_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

// Pure domain value: ein einzeiliger Textpuffer mit Cursor und Auswahl — die
// Editierlogik hinter dem Bearbeiten im Canvas, ohne wx, GL oder Schriftmasse.
// Positionen zählen Codepoints, nicht Bytes, darum `std::u32string`: so ist ein
// Umlaut ein Schritt, und der Renderer misst dieselben Einheiten.
//
// Cursor und Anker spannen die Auswahl auf. Ohne Auswahl liegen beide
// aufeinander; jedes Einfügen und Löschen ersetzt zuerst die Auswahl.
class TextEditBuffer {
 public:
  // Ob eine Bewegung die Auswahl aufspannt (Shift gedrückt) oder aufhebt.
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

  // Rücktaste: löscht die Auswahl, sonst das Zeichen vor dem Cursor.
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

  // Entf-Taste: löscht die Auswahl, sonst das Zeichen hinter dem Cursor.
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

  // Eine Bewegung ohne Shift hebt eine bestehende Auswahl auf und setzt den
  // Cursor an deren Rand — wie in jedem Textfeld.
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

  // Wählt das Wort um `index`; auf Leerraum die zusammenhängende Lücke. Wörter
  // sind hier Folgen von Nicht-Leerraum — für einen einzeiligen Titel genügt
  // das, ICU-Wortgrenzen wären hier Aufwand ohne sichtbaren Gewinn.
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
