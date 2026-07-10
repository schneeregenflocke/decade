#ifndef RECT_HPP
#define RECT_HPP

#include <array>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
template <typename Ty>
class rect {
 public:
  rect() : edges_({0, 0, 0, 0}) {}

  rect(Ty left, Ty right, Ty bottom, Ty top)
      : edges_({left, right, bottom, top}) {}

  struct Dimension {
    Ty width;
    Ty height;
  };

  static rect from_dimension(Dimension dimension) {
    rect result;
    result.setL(-dimension.width / static_cast<Ty>(2));
    result.setR(dimension.width / static_cast<Ty>(2));
    result.setB(-dimension.height / static_cast<Ty>(2));
    result.setT(dimension.height / static_cast<Ty>(2));
    return result;
  }

  [[nodiscard]] Ty l() const { return edges_[0]; }

  [[nodiscard]] Ty r() const { return edges_[1]; }

  [[nodiscard]] Ty b() const { return edges_[2]; }

  [[nodiscard]] Ty t() const { return edges_[3]; }

  [[nodiscard]] Ty width() const { return edges_[1] - edges_[0]; }

  [[nodiscard]] Ty height() const { return edges_[3] - edges_[2]; }

  [[nodiscard]] rect shift(Ty x_offset, Ty y_offset) const {
    return rect(l() + x_offset, r() + x_offset, b() + y_offset, t() + y_offset);
  }

  [[nodiscard]] rect expand(const rect& value) const {
    return rect(l() - value.l(), r() + value.r(), b() - value.b(),
                t() + value.t());
  }

  [[nodiscard]] rect reduce(const rect& value) const {
    return rect(l() + value.l(), r() - value.r(), b() + value.b(),
                t() - value.t());
  }

  [[nodiscard]] rect scale(Ty factor) const {
    const Ty expand_width_value =
        ((width() * factor) - width()) / static_cast<Ty>(2);
    const Ty expand_height_value =
        ((height() * factor) - height()) / static_cast<Ty>(2);

    rect result = expand(rect(expand_width_value, expand_width_value,
                              expand_height_value, expand_height_value));
    return result;
  }

  [[nodiscard]] rect dimension(Ty width, Ty height) const {
    return rect(l(), l() + width, b(), b() + height);
  }

  [[nodiscard]] glm::vec3 getCenter() const {
    return glm::vec3(edges_[0] + (width() / static_cast<Ty>(2)),
                     edges_[2] + (height() / static_cast<Ty>(2)),
                     static_cast<Ty>(0));
  }

  [[nodiscard]] glm::vec3 getLB() const {
    return glm::vec3(edges_[0], edges_[2], static_cast<Ty>(0));
  }

  [[nodiscard]] glm::vec3 getRB() const {
    return glm::vec3(edges_[1], edges_[2], static_cast<Ty>(0));
  }

  [[nodiscard]] glm::vec3 getLT() const {
    return glm::vec3(edges_[0], edges_[3], static_cast<Ty>(0));
  }

  [[nodiscard]] glm::vec3 getRT() const {
    return glm::vec3(edges_[1], edges_[3], static_cast<Ty>(0));
  }

  void setL(Ty value) { edges_[0] = value; }

  void setR(Ty value) { edges_[1] = value; }

  void setB(Ty value) { edges_[2] = value; }

  void setT(Ty value) { edges_[3] = value; }

 private:
  void addL(Ty value) { edges_[0] += value; }

  void addR(Ty value) { edges_[1] += value; }

  void addB(Ty value) { edges_[2] += value; }

  void addT(Ty value) { edges_[3] += value; }

  std::array<Ty, 4> edges_;
};

using rectf = rect<float>;
#endif  // RECT_HPP
