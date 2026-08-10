#ifndef TITLE_SECTION_HPP
#define TITLE_SECTION_HPP

#include <cstddef>
#include <glm/vec2.hpp>
#include <string>

#include "../../infrastructure/graphics/pick_id.hpp"
#include "section_context.hpp"

// The print area frame and the title: its frame, its text and — while somebody
// is editing in the canvas — cursor and selection.

namespace calendar_sections {

// The geometry of the title line and, while editing, of cursor and selection.
// Both hang on the same computation — text width in code points, measured from
// the left of the centred text — which is why they sit together here.
namespace title_edit {

inline constexpr float kCaretWidthRatio = 0.06F;
inline constexpr float kSelectionRed = 0.25F;
inline constexpr float kSelectionGreen = 0.5F;
inline constexpr float kSelectionBlue = 1.0F;
inline constexpr float kSelectionAlpha = 0.35F;

// The text to draw right now, with its font size and left edge.
struct TextLine {
  std::string text;
  std::u32string code_points;
  float font_size{0.0F};
  float left{0.0F};
};

[[nodiscard]] TextLine Layout(const SectionContext& ctx);

// Sets the cursor and selection areas, or hides both (a null area) when nobody
// is editing.
void FillCaretAndSelection(const SectionContext& ctx, const TextLine& line);

// The inverse for the pointer: the cursor index a click in page space means.
// The point arrives in page space while the title geometry sits local to the
// print area — hence subtracting its origin.
[[nodiscard]] std::size_t CaretIndexAt(const SectionContext& ctx,
                                       const TextLine& line,
                                       glm::vec2 page_point);

}  // namespace title_edit

void BuildPrintArea(const SectionContext& ctx);

// The title is a pickable element: its hit area is the frame the text fills. As
// with the bars, the returned box lies in page space, so shifted by the origin
// of the print area.
//
// During an edit the frame shows the buffer instead of the stored title — that
// becomes canonical with Enter alone — and cursor and selection alongside.
[[nodiscard]] PickBox BuildTitle(const SectionContext& ctx);

}  // namespace calendar_sections

#endif  // TITLE_SECTION_HPP
