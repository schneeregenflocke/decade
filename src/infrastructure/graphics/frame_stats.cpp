#include "frame_stats.hpp"

#include <chrono>
#include <ratio>

void FrameStats::AddFrame(Clock::time_point now,
                          std::chrono::nanoseconds render_time) {
  frame_times_.push_back(now);
  last_render_time_ = render_time;
  const auto window_start = now - kWindow;
  while (!frame_times_.empty() && frame_times_.front() <= window_start) {
    frame_times_.pop_front();
  }
}

double FrameStats::Fps() const {
  return static_cast<double>(frame_times_.size()) /
         std::chrono::duration<double>(kWindow).count();
}

double FrameStats::LastRenderMillis() const {
  return std::chrono::duration<double, std::milli>(last_render_time_).count();
}
