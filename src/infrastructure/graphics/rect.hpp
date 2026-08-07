#ifndef RECT_HPP
#define RECT_HPP

#include <array>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
template <typename Ty>
class Rect {
 public:
  Rect() : edges_({0, 0, 0, 0}) {}

  Rect(Ty left, Ty right, Ty bottom, Ty top)
      : edges_({left, right, bottom, top}) {}

  struct Dimension {
    Ty width;
    Ty height;
  };

  static Rect FromDimension(Dimension dimension) {
    Rect result;
    result.SetLeft(-dimension.width / static_cast<Ty>(2));
    result.SetRight(dimension.width / static_cast<Ty>(2));
    result.SetBottom(-dimension.height / static_cast<Ty>(2));
    result.SetTop(dimension.height / static_cast<Ty>(2));
    return result;
  }

  [[nodiscard]] Ty Left() const { return edges_[0]; }

  [[nodiscard]] Ty Right() const { return edges_[1]; }

  [[nodiscard]] Ty Bottom() const { return edges_[2]; }

  [[nodiscard]] Ty Top() const { return edges_[3]; }

  [[nodiscard]] Ty Width() const { return edges_[1] - edges_[0]; }

  [[nodiscard]] Ty Height() const { return edges_[3] - edges_[2]; }

  [[nodiscard]] Rect Shift(Ty x_offset, Ty y_offset) const {
    return Rect(Left() + x_offset, Right() + x_offset, Bottom() + y_offset,
                Top() + y_offset);
  }

  [[nodiscard]] Rect Expand(const Rect& value) const {
    return Rect(Left() - value.Left(), Right() + value.Right(),
                Bottom() - value.Bottom(), Top() + value.Top());
  }

  [[nodiscard]] Rect Reduce(const Rect& value) const {
    return Rect(Left() + value.Left(), Right() - value.Right(),
                Bottom() + value.Bottom(), Top() - value.Top());
  }

  [[nodiscard]] Rect Scale(Ty factor) const {
    const Ty expand_width_value =
        ((Width() * factor) - Width()) / static_cast<Ty>(2);
    const Ty expand_height_value =
        ((Height() * factor) - Height()) / static_cast<Ty>(2);

    Rect result = Expand(Rect(expand_width_value, expand_width_value,
                              expand_height_value, expand_height_value));
    return result;
  }

  [[nodiscard]] glm::vec3 Center() const {
    return glm::vec3(edges_[0] + (Width() / static_cast<Ty>(2)),
                     edges_[2] + (Height() / static_cast<Ty>(2)),
                     static_cast<Ty>(0));
  }

  [[nodiscard]] glm::vec3 LeftBottom() const {
    return glm::vec3(edges_[0], edges_[2], static_cast<Ty>(0));
  }

  [[nodiscard]] glm::vec3 RightBottom() const {
    return glm::vec3(edges_[1], edges_[2], static_cast<Ty>(0));
  }

  [[nodiscard]] glm::vec3 LeftTop() const {
    return glm::vec3(edges_[0], edges_[3], static_cast<Ty>(0));
  }

  [[nodiscard]] glm::vec3 RightTop() const {
    return glm::vec3(edges_[1], edges_[3], static_cast<Ty>(0));
  }

  void SetLeft(Ty value) { edges_[0] = value; }

  void SetRight(Ty value) { edges_[1] = value; }

  void SetBottom(Ty value) { edges_[2] = value; }

  void SetTop(Ty value) { edges_[3] = value; }

 private:
  std::array<Ty, 4> edges_;
};

using RectF = Rect<float>;
#endif  // RECT_HPP
