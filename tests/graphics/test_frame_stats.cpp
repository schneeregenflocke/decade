#include <gtest/gtest.h>

#include <chrono>

#include "graphics/frame_stats.hpp"

using namespace std::chrono_literals;

namespace {

constexpr FrameStats::Clock::time_point kStart{};

}  // namespace

TEST(FrameStatsTest, SteadyCadenceYieldsMatchingFps) {
  FrameStats stats;
  // 2 Sekunden lang alle 10 ms ein Frame → 100 Frames im Sekundenfenster.
  for (int frame = 0; frame < 200; ++frame) {
    stats.AddFrame(kStart + frame * 10ms, 2ms);
  }
  EXPECT_NEAR(stats.Fps(), 100.0, 1e-9);
}

TEST(FrameStatsTest, WindowSlidesWithNewestFrame) {
  FrameStats stats;
  for (int frame = 0; frame < 50; ++frame) {
    stats.AddFrame(kStart + frame * 10ms, 2ms);
  }
  // Langer Stillstand: der nächste Frame ist allein in seinem Fenster.
  stats.AddFrame(kStart + 10s, 2ms);
  EXPECT_NEAR(stats.Fps(), 1.0, 1e-9);
}

TEST(FrameStatsTest, ReportsLastRenderDuration) {
  FrameStats stats;
  stats.AddFrame(kStart, 5ms);
  EXPECT_NEAR(stats.LastRenderMillis(), 5.0, 1e-9);

  stats.AddFrame(kStart + 10ms, 250us);
  EXPECT_NEAR(stats.LastRenderMillis(), 0.25, 1e-9);
}
