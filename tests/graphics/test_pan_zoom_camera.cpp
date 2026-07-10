#include <gtest/gtest.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics/pan_zoom_camera.hpp"

namespace {

// Ortho wie in Projection::OrthoMatrix: um den Ursprung zentrierte, sichtbare
// Weltausdehnung width × height.
glm::mat4 OrthoFor(float width, float height) {
  constexpr float kHalf = 0.5F;
  return glm::ortho(-width * kHalf, width * kHalf, -height * kHalf,
                    height * kHalf);
}

void ExpectNearVec3(const glm::vec3& actual, const glm::vec3& expected,
                    float tolerance) {
  EXPECT_NEAR(actual.x, expected.x, tolerance);
  EXPECT_NEAR(actual.y, expected.y, tolerance);
  EXPECT_NEAR(actual.z, expected.z, tolerance);
}

// A4 hochkant in mm; die Ansicht zeigt wie in GLCanvas::RefreshMVP das
// 1.1-fache der Seite.
constexpr float kPageWidth = 210.0F;
constexpr float kPageHeight = 297.0F;
constexpr float kViewScale = 1.1F;

}  // namespace

TEST(PanZoomCameraTest, PanShiftsWorldOrigin) {
  PanZoomCamera camera;
  camera.Pan({5.0F, -3.0F, 0.0F});

  const auto shifted = camera.ViewMatrix() * glm::vec4(0.0F, 0.0F, 0.0F, 1.0F);
  ExpectNearVec3({shifted.x, shifted.y, shifted.z}, {5.0F, -3.0F, 0.0F},
                 1e-5F);
}

TEST(PanZoomCameraTest, ZoomAroundMultipliesScaleFactor) {
  PanZoomCamera camera;
  camera.ZoomAround({0.0F, 0.0F, 0.0F}, 2.0F);
  EXPECT_NEAR(camera.ScaleFactor(), 2.0F, 1e-6F);

  camera.ZoomAround({0.0F, 0.0F, 0.0F}, 0.5F);
  EXPECT_NEAR(camera.ScaleFactor(), 1.0F, 1e-6F);
}

TEST(PanZoomCameraTest, ZoomAroundKeepsAnchorPagePosition) {
  PanZoomCamera camera;
  camera.Pan({12.0F, 7.0F, 0.0F});

  const glm::vec3 anchor{10.0F, -4.0F, 0.0F};
  const glm::vec3 page_before = camera.PagePos(anchor);
  camera.ZoomAround(anchor, 2.5F);
  const glm::vec3 page_after = camera.PagePos(anchor);

  ExpectNearVec3(page_after, page_before, 1e-4F);
}

TEST(PanZoomCameraTest, ZoomClampsAtMaxAndKeepsAnchor) {
  PanZoomCamera camera;
  camera.SetScaleLimits({.min_scale = 0.5F, .max_scale = 4.0F});

  const glm::vec3 anchor{3.0F, 8.0F, 0.0F};
  const glm::vec3 page_before = camera.PagePos(anchor);
  camera.ZoomAround(anchor, 100.0F);

  EXPECT_NEAR(camera.ScaleFactor(), 4.0F, 1e-6F);
  ExpectNearVec3(camera.PagePos(anchor), page_before, 1e-4F);

  // Am Anschlag ändert weiteres Hineinzoomen nichts mehr.
  camera.ZoomAround(anchor, 2.0F);
  EXPECT_NEAR(camera.ScaleFactor(), 4.0F, 1e-6F);
}

TEST(PanZoomCameraTest, ZoomClampsAtMin) {
  PanZoomCamera camera;
  camera.SetScaleLimits({.min_scale = 0.5F, .max_scale = 4.0F});

  camera.ZoomAround({0.0F, 0.0F, 0.0F}, 1e-6F);
  EXPECT_NEAR(camera.ScaleFactor(), 0.5F, 1e-6F);
}

TEST(ComputeZoomLimitsTest, MaxScaleLeavesTwoExportPixelsVisibleAt200Dpi) {
  const auto projection =
      OrthoFor(kPageWidth * kViewScale, kPageHeight * kViewScale);
  const auto limits =
      ComputeZoomLimits(projection, {kPageWidth, kPageHeight}, 200.0F);

  // 2 Pixel bei 200 dpi = 0.254 mm; massgeblich ist das kleinere Sichtmass
  // (die Breite, 231 mm): 231 / 0.254.
  EXPECT_NEAR(limits.max_scale, 231.0F / 0.254F, 0.5F);

  // Am Anschlag ist das kleinere Sichtmass genau 2 Export-Pixel gross.
  const float visible_width_at_max = kPageWidth * kViewScale / limits.max_scale;
  EXPECT_NEAR(visible_width_at_max, 0.254F, 1e-4F);
}

TEST(ComputeZoomLimitsTest, MaxScaleAt600DpiIsThreeTimesTighter) {
  const auto projection =
      OrthoFor(kPageWidth * kViewScale, kPageHeight * kViewScale);
  const auto limits_200 =
      ComputeZoomLimits(projection, {kPageWidth, kPageHeight}, 200.0F);
  const auto limits_600 =
      ComputeZoomLimits(projection, {kPageWidth, kPageHeight}, 600.0F);

  // Feinere Export-Auflösung → kleinere Pixel → man darf 3x weiter hinein.
  EXPECT_NEAR(limits_600.max_scale / limits_200.max_scale, 3.0F, 1e-3F);
  EXPECT_NEAR(limits_600.min_scale, limits_200.min_scale, 1e-6F);
}

TEST(ComputeZoomLimitsTest, MinScaleShowsTwoPagesPlusQuarter) {
  const auto projection =
      OrthoFor(kPageWidth * kViewScale, kPageHeight * kViewScale);
  const auto limits =
      ComputeZoomLimits(projection, {kPageWidth, kPageHeight}, 200.0F);

  // Sichtausdehnung ist 1.1 Seiten; 2.25 Seiten sichtbar → 1.1 / 2.25.
  EXPECT_NEAR(limits.min_scale, kViewScale / 2.25F, 1e-5F);

  const float visible_pages_wide =
      kPageWidth * kViewScale / limits.min_scale / kPageWidth;
  EXPECT_NEAR(visible_pages_wide, 2.25F, 1e-4F);
}

TEST(ComputeZoomLimitsTest, LetterboxedViewportStopsWhenBothAxesShowTwoPages) {
  // Breites Fenster: sichtbare Breite deutlich grösser als die Seitenbreite.
  const auto projection = OrthoFor(500.0F, kPageHeight * kViewScale);
  const auto limits =
      ComputeZoomLimits(projection, {kPageWidth, kPageHeight}, 200.0F);

  // Die Höhe ist die knappere Achse; erst wenn auch sie 2.25 Seiten zeigt,
  // stoppt das Herauszoomen.
  EXPECT_NEAR(limits.min_scale, kPageHeight * kViewScale / (2.25F * kPageHeight),
              1e-5F);

  const float visible_pages_high =
      kPageHeight * kViewScale / limits.min_scale / kPageHeight;
  EXPECT_NEAR(visible_pages_high, 2.25F, 1e-4F);
}
