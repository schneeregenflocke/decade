#ifndef FRAME_STATS_HPP
#define FRAME_STATS_HPP

#include <chrono>
#include <deque>

// Measures the frame rate of the event-driven rendering: AddFrame registers
// every drawn frame together with its CPU render duration; Fps counts the
// frames in the one-second window ending with the newest frame. The timestamps
// come from outside, so the class stays testable without a clock of its own
// (and without GL).
class FrameStats {
 public:
  using Clock = std::chrono::steady_clock;

  void AddFrame(Clock::time_point now, std::chrono::nanoseconds render_time) {
    frame_times_.push_back(now);
    last_render_time_ = render_time;
    const auto window_start = now - kWindow;
    while (!frame_times_.empty() && frame_times_.front() <= window_start) {
      frame_times_.pop_front();
    }
  }

  // Frames per second in the window ending with the newest frame. Since the app
  // draws event-driven alone, the value carries meaning during an interaction;
  // when idle it stays at its last reading.
  [[nodiscard]] double Fps() const {
    return static_cast<double>(frame_times_.size()) /
           std::chrono::duration<double>(kWindow).count();
  }

  // The CPU duration of the last drawn frame (render plus buffer swap).
  [[nodiscard]] double LastRenderMillis() const {
    return std::chrono::duration<double, std::milli>(last_render_time_).count();
  }

 private:
  static constexpr std::chrono::seconds kWindow{1};

  std::deque<Clock::time_point> frame_times_;
  std::chrono::nanoseconds last_render_time_{0};
};

#endif  // FRAME_STATS_HPP
