#include "app_config.hpp"

#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtCore/Qt>

namespace application {

MainFrameConfig DefaultMainFrameConfig() {
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
