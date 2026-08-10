#ifndef PICK_ID_HPP
#define PICK_ID_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "rect.hpp"

// Identity of a pickable scene element. Kept as a small, dependency-free value
// so both the scene graph (SceneNode carries one) and the physics/picking layer
// (the collision world maps a hit back to one) can use it without coupling.
// `index` is the element's index within its kind (e.g. the bar index); kinds
// that exist only once (the title) leave it at 0.
struct PickId {
  enum class Kind : std::uint8_t { kBar, kTitle };

  Kind kind{Kind::kBar};
  std::size_t index{0};

  friend bool operator==(const PickId&, const PickId&) = default;
};

// The plain name of the kind, for diagnostic output alone.
[[nodiscard]] std::string_view PickKindName(PickId::Kind kind);

// A pickable element's identity paired with its page-space rectangle. The scene
// builder produces these (Bullet-free); the physics layer turns them into
// collision geometry. Lives here so neither side depends on the other.
struct PickBox {
  PickId id;
  RectF rect;
};

#endif  // PICK_ID_HPP
