#ifndef RENDER_SURFACE_HPP
#define RENDER_SURFACE_HPP

namespace application {

// The port through which the rendering adapter asks for a repaint, without
// knowing what draws. The implementation is the GL canvas in presentation, so
// the dependency points inwards (#15): presentation implements what the
// application declares.
//
// Two methods rather than one, because the two costs differ by an order of
// magnitude and the caller knows which it needs.
class RenderSurface {
 public:
  RenderSurface() = default;
  RenderSurface(const RenderSurface&) = delete;
  RenderSurface& operator=(const RenderSurface&) = delete;
  RenderSurface(RenderSurface&&) = delete;
  RenderSurface& operator=(RenderSurface&&) = delete;
  // Defaulted in the class body, and this port therefore has no unit of its
  // own: a destructor defined out of line would make the class more than a
  // pure interface, which is what an implementer inherits it as beside its
  // widget base.
  virtual ~RenderSurface() = default;

  // The page geometry or the window size changed: viewport, projection and zoom
  // bounds have to be refitted before the repaint.
  virtual void RefreshView() = 0;

  // Nothing but colours changed (hover, selection): repaint on the geometry
  // that already stands.
  virtual void Repaint() = 0;

  // The adapter is about to build GL objects outside a paint — a state change
  // arriving over the bus rebuilds the scene, and that allocates buffers and
  // textures. The surface makes its context current; without it the calls
  // dispatch into whatever context happens to be current, which is none.
  virtual void MakeGraphicsCurrent() = 0;
};

}  // namespace application

#endif  // RENDER_SURFACE_HPP
