#ifndef PAN_ZOOM_CAMERA_HPP
#define PAN_ZOOM_CAMERA_HPP

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>

// 2D-Kamera der Seitenansicht: hält den Pan-/Zoom-Zustand und leitet daraus
// die View-Matrix ab. GL- und UI-frei, damit unit-testbar; die Maus-Seite
// (Pixel → Weltraum) liegt in presentation/mouse_interaction.hpp.
class PanZoomCamera {
 public:
  // Erlaubter Bereich des Skalierungsfaktors; siehe ComputeZoomLimits.
  struct ScaleLimits {
    float min_scale{0.F};
    float max_scale{std::numeric_limits<float>::max()};
  };

  // Verschiebt die Ansicht um ein Delta im Weltraum.
  void Pan(const glm::vec3& world_delta) { translate_pre_scaled_ += world_delta; }

  // Skaliert um factor (> 1 vergrössert), begrenzt durch die ScaleLimits.
  // Der Weltpunkt world_pos (typisch: der Mauszeiger) bleibt dabei auf
  // derselben Bildposition, indem die durch die Skalierung entstandene
  // Verschiebung zurückkorrigiert wird.
  void ZoomAround(const glm::vec3& world_pos, float factor) {
    const float target_scale = std::clamp(scale_factor_ * factor,
                                          limits_.min_scale, limits_.max_scale);

    const glm::vec3 pre_scale_page_pos = PagePos(world_pos);
    scale_factor_ = target_scale;
    const glm::vec3 post_scale_page_pos = PagePos(world_pos);

    translate_post_scaled_ += post_scale_page_pos - pre_scale_page_pos;
  }

  // Greift erst beim nächsten ZoomAround; ein bereits ausserhalb liegender
  // Zustand wird nicht zurückgeschnappt.
  void SetScaleLimits(const ScaleLimits& limits) { limits_ = limits; }

  [[nodiscard]] float ScaleFactor() const { return scale_factor_; }

  [[nodiscard]] glm::mat4 ViewMatrix() const {
    const auto pre_scaled =
        glm::translate(glm::mat4(1.F), translate_pre_scaled_);
    const auto scaled =
        glm::scale(pre_scaled, glm::vec3(scale_factor_, scale_factor_, 1.F));
    return glm::translate(scaled, translate_post_scaled_);
  }

  // Rechnet einen Weltpunkt in den Seitenraum zurück (inverse View-Matrix).
  [[nodiscard]] glm::vec3 PagePos(const glm::vec3& world_pos) const {
    const auto page_pos = glm::inverse(ViewMatrix()) * glm::vec4(world_pos, 1.F);
    return {page_pos.x, page_pos.y, 0.F};
  }

 private:
  float scale_factor_{1.F};
  glm::vec3 translate_pre_scaled_{0.F};
  glm::vec3 translate_post_scaled_{0.F};
  ScaleLimits limits_;
};

// Leitet die Zoom-Grenzen aus der Ortho-Projektion, der Seitengrösse (mm) und
// der Export-Auflösung ab:
// - max_scale: Hineinzoomen endet, wenn vom Export-Bild (export_dpi) in jeder
//   Richtung mindestens noch 2 Pixel sichtbar sind.
// - min_scale: Herauszoomen endet, wenn in Breite und Höhe je 2 Seiten plus
//   25 % Seitenmass sichtbar sind.
inline PanZoomCamera::ScaleLimits ComputeZoomLimits(
    const glm::mat4& ortho_projection, const glm::vec2& page_size_mm,
    float export_dpi) {
  // glm::ortho legt 2/(right-left) in [0][0] und 2/(top-bottom) in [1][1] ab —
  // daraus die bei Skalierung 1 sichtbare Weltausdehnung zurückrechnen.
  const float visible_width = 2.F / ortho_projection[0][0];
  const float visible_height = 2.F / ortho_projection[1][1];

  constexpr float kMinVisibleExportPixels = 2.F;
  constexpr float kMmPerInch = 25.4F;
  const float min_visible_mm = kMinVisibleExportPixels * kMmPerInch / export_dpi;
  const float max_scale = std::min(visible_width, visible_height) / min_visible_mm;

  constexpr float kMaxVisiblePages = 2.25F;
  const float min_scale =
      std::min(visible_width / (kMaxVisiblePages * page_size_mm.x),
               visible_height / (kMaxVisiblePages * page_size_mm.y));

  // Degenerierte Geometrie (winziges Fenster) darf kein invertiertes
  // Intervall liefern — std::clamp verlangt min <= max.
  return {.min_scale = min_scale, .max_scale = std::max(max_scale, min_scale)};
}

#endif  // PAN_ZOOM_CAMERA_HPP
