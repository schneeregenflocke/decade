#include "title_section.hpp"

#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <vector>

#include "../../domain/shape_configuration.hpp"
#include "../../domain/text_edit_view.hpp"
#include "../../infrastructure/graphics/font.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"
#include "../../infrastructure/graphics/rect.hpp"
#include "../../infrastructure/graphics/shapes.hpp"
#include "../../infrastructure/graphics/utf8_codec.hpp"
#include "section_context.hpp"

namespace calendar_sections {

namespace title_edit {

TextLine Layout(const SectionContext& ctx) {
  TextLine line;
  line.text = ctx.text_edit.has_value() ? ctx.text_edit->text
                                        : ctx.title_config.TitleText();
  const std::vector<char32_t> decoded = DecodeUtf8(line.text);
  line.code_points.assign(decoded.begin(), decoded.end());
  line.font_size = ctx.title_config.FontSizeMillimetres();
  line.left = ctx.layout.TitleArea().Center().x -
              (ctx.font->TextWidth(line.text, line.font_size) * detail::kHalf);
  return line;
}

void FillCaretAndSelection(const SectionContext& ctx, const TextLine& line) {
  FillShape& caret_shape = ctx.nodes.title_caret.Shape();
  FillShape& selection_shape = ctx.nodes.title_selection.Shape();
  if (!ctx.text_edit.has_value()) {
    caret_shape.Hide();
    selection_shape.Hide();
    return;
  }

  const float text_height = ctx.font->TextHeight(line.font_size);
  const float center_y = ctx.layout.TitleArea().Center().y;
  const float bottom = center_y - (text_height * detail::kHalf);
  const float top = center_y + (text_height * detail::kHalf);
  const auto offset = [&](std::size_t index) {
    return line.left +
           ctx.font->TextWidth(line.code_points, line.font_size, index);
  };

  const float caret_x = offset(ctx.text_edit->caret);
  const float caret_width = line.font_size * kCaretWidthRatio;
  caret_shape.SetShape(RectF(caret_x, caret_x + caret_width, bottom, top));
  caret_shape.SetColor(ctx.title_config.TextColor());

  if (HasSelection(*ctx.text_edit)) {
    selection_shape.SetShape(RectF(offset(ctx.text_edit->selection_begin),
                                   offset(ctx.text_edit->selection_end), bottom,
                                   top));
    selection_shape.SetColor(glm::vec4(kSelectionRed, kSelectionGreen,
                                       kSelectionBlue, kSelectionAlpha));
  } else {
    selection_shape.Hide();
  }
}

std::size_t CaretIndexAt(const SectionContext& ctx, const TextLine& line,
                         glm::vec2 page_point) {
  const float local_x = page_point.x - ctx.layout.PrintAreaOrigin().x;
  return ctx.font->IndexAtOffset(line.code_points, line.font_size,
                                 local_x - line.left);
}

}  // namespace title_edit

void BuildPrintArea(const SectionContext& ctx) {
  detail::FillRectangles(
      ctx.nodes.print_area, ctx.layout.PrintArea(),
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kPageMargin));
}

PickBox BuildTitle(const SectionContext& ctx) {
  detail::FillRectangles(
      ctx.nodes.title_area, ctx.layout.TitleArea(),
      ctx.shape_config.GetShapeConfiguration(ShapeConfigSet::kTitleFrame));

  const title_edit::TextLine line = title_edit::Layout(ctx);
  FontShape& title_shape = ctx.nodes.title_text.Shape();
  title_shape.SetFont(ctx.font);
  title_shape.SetColor(ctx.title_config.TextColor());
  title_shape.SetShapeCentered(line.text, ctx.layout.TitleArea().Center(),
                               line.font_size);
  title_edit::FillCaretAndSelection(ctx, line);

  return PickBox{
      .id = PickId{.kind = PickId::Kind::kTitle, .index = 0},
      .rect = ctx.layout.TitleArea().Shift(ctx.layout.PrintAreaOrigin().x,
                                           ctx.layout.PrintAreaOrigin().y)};
}

}  // namespace calendar_sections
