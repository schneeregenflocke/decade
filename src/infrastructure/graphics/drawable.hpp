#ifndef DRAWABLE_HPP
#define DRAWABLE_HPP

#include <glm/mat4x4.hpp>

#include "rect.hpp"

// What a scene node needs of the thing it draws, and nothing more: paint
// yourself under this world transform, and say how big you are in your own
// space.
//
// The point of the interface is what it does **not** pull in. `Shape`, the one
// implementation that matters at runtime, allocates GL objects in the
// constructors of its members — so a scene graph holding a `Shape` could not be
// built without a live context, and nothing downstream of it could be tested
// (#77). Holding a `Drawable` instead keeps `scene_graph.hpp` free of
// `epoxy/gl.h`: a test builds a node tree over its own trivial implementation
// and pins the traversal, the draw order and the bounds.
//
// It stays this small on purpose. Anything a caller needs beyond these two —
// colours, geometry, glyphs — it asks the concrete type for through a
// `dynamic_cast`, as it did before.
class Drawable {
 public:
  virtual ~Drawable() = default;

  Drawable(const Drawable&) = delete;
  Drawable& operator=(const Drawable&) = delete;
  Drawable(Drawable&&) = delete;
  Drawable& operator=(Drawable&&) = delete;

  virtual void Draw(const glm::mat4& model) const = 0;

  // The axis-aligned box of the geometry in the drawable's own space. A
  // drawable without geometry reports a zero-extent box.
  [[nodiscard]] virtual const rectf& LocalBounds() const = 0;

 protected:
  Drawable() = default;
};

#endif  // DRAWABLE_HPP
