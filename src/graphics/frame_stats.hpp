#ifndef FRAME_STATS_HPP
#define FRAME_STATS_HPP

#include <chrono>
#include <deque>

// Misst die Bildrate des ereignisgesteuerten Renderings: AddFrame registriert
// jeden gezeichneten Frame samt seiner CPU-Renderdauer; Fps zählt die Frames
// im Sekundenfenster, das mit dem jüngsten Frame endet. Zeitpunkte kommen von
// aussen, damit die Klasse ohne eigene Uhr (und ohne GL) testbar bleibt.
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

  // Frames pro Sekunde im Fenster, das mit dem jüngsten Frame endet. Da die
  // App nur ereignisgesteuert zeichnet, ist der Wert während einer
  // Interaktion aussagekräftig, im Leerlauf bleibt er beim letzten Stand.
  [[nodiscard]] double Fps() const {
    return static_cast<double>(frame_times_.size()) /
           std::chrono::duration<double>(kWindow).count();
  }

  // CPU-Dauer des zuletzt gezeichneten Frames (Render + Buffer-Swap).
  [[nodiscard]] double LastRenderMillis() const {
    return std::chrono::duration<double, std::milli>(last_render_time_)
        .count();
  }

 private:
  static constexpr std::chrono::seconds kWindow{1};

  std::deque<Clock::time_point> frame_times_;
  std::chrono::nanoseconds last_render_time_{0};
};

#endif  // FRAME_STATS_HPP
