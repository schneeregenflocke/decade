#include "text_edit_view.hpp"

bool HasSelection(const TextEditView& view) {
  return view.selection_end > view.selection_begin;
}
