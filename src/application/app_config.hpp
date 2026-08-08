#ifndef APP_CONFIG_HPP
#define APP_CONFIG_HPP

#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtCore/Qt>
#include <string>

namespace application {
struct MainFrameConfig {
  std::string title;
  QPoint position;
  QSize size;
  Qt::WindowFlags flags{Qt::Window};
  std::string object_name{"main_window"};
  bool maximize_on_start{true};
};

inline MainFrameConfig DefaultMainFrameConfig() {
  constexpr int kMainFramePosX = 100;
  constexpr int kMainFramePosY = 100;
  constexpr int kMainFrameWidth = 1280;
  constexpr int kMainFrameHeight = 800;

  return {.title = "Decade",
          .position = QPoint(kMainFramePosX, kMainFramePosY),
          .size = QSize(kMainFrameWidth, kMainFrameHeight),
          .flags = Qt::Window,
          .object_name = "main_window",
          .maximize_on_start = false};
}
}  // namespace application

#endif  // APP_CONFIG_HPP
