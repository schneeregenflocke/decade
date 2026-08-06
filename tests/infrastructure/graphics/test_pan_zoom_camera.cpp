#include <gtest/gtest.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "infrastructure/graphics/pan_zoom_camera.hpp"

namespace {

// Ortho as in Projection::OrthoMatrix: a visible world extent width × height,
// centred on the origin.
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

// A4 portrait in mm; the view shows 1.1 times the page, as GLCanvas::RefreshMVP
// does.
constexpr float kPageWidth = 210.0F;
constexpr float kPageHeight = 297.0F;
constexpr float kViewScale = 1.1F;

}  // namespace

TEST(PanZoomCameraTest, PanShiftsWorldOrigin) {
  PanZoomCamera camera;
  camera.Pan({5.0F, -3.0F, 0.0F});

  const auto shifted = camera.ViewMatrix() * glm::vec4(0.0F, 0.0F, 0.0F, 1.0F);
  ExpectNearVec3({shifted.x, shifted.y, shifted.z}, {5.0F, -3.0F, 0.0F}, 1e-5F);
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

  // At the stop, zooming in further changes nothing.
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

  // 2 pixels at 200 dpi = 0.254 mm; the smaller view measure decides (the
  // width, 231 mm): 231 / 0.254.
  EXPECT_NEAR(limits.max_scale, 231.0F / 0.254F, 0.5F);

  // At the stop the smaller view measure is exactly 2 export pixels wide.
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

  // A finer export resolution → smaller pixels → one may go 3x further in.
  EXPECT_NEAR(limits_600.max_scale / limits_200.max_scale, 3.0F, 1e-3F);
  EXPECT_NEAR(limits_600.min_scale, limits_200.min_scale, 1e-6F);
}

TEST(ComputeZoomLimitsTest, MinScaleShowsTwoPagesPlusQuarter) {
  const auto projection =
      OrthoFor(kPageWidth * kViewScale, kPageHeight * kViewScale);
  const auto limits =
      ComputeZoomLimits(projection, {kPageWidth, kPageHeight}, 200.0F);

  // The view extent is 1.1 pages; 2.25 pages visible → 1.1 / 2.25.
  EXPECT_NEAR(limits.min_scale, kViewScale / 2.25F, 1e-5F);

  const float visible_pages_wide =
      kPageWidth * kViewScale / limits.min_scale / kPageWidth;
  EXPECT_NEAR(visible_pages_wide, 2.25F, 1e-4F);
}

TEST(ComputeZoomLimitsTest, LetterboxedViewportStopsWhenBothAxesShowTwoPages) {
  // A wide window: the visible width is markedly larger than the page width.
  const auto projection = OrthoFor(500.0F, kPageHeight * kViewScale);
  const auto limits =
      ComputeZoomLimits(projection, {kPageWidth, kPageHeight}, 200.0F);

  // The height is the tighter axis; zooming out stops once it too shows 2.25
  // pages.
  EXPECT_NEAR(limits.min_scale,
              kPageHeight * kViewScale / (2.25F * kPageHeight), 1e-5F);

  const float visible_pages_high =
      kPageHeight * kViewScale / limits.min_scale / kPageHeight;
  EXPECT_NEAR(visible_pages_high, 2.25F, 1e-4F);
}
