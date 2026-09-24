#include "app_config.hpp"

#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtCore/Qt>

namespace application {

MainWindowConfig DefaultMainWindowConfig() {
  constexpr int kMainWindowPosX = 100;
  constexpr int kMainWindowPosY = 100;
  constexpr int kMainWindowWidth = 1280;
  constexpr int kMainWindowHeight = 800;

  return {.title = "Decade",
          .position = QPoint(kMainWindowPosX, kMainWindowPosY),
          .size = QSize(kMainWindowWidth, kMainWindowHeight),
          .flags = Qt::Window,
          .object_name = "main_window",
          .maximize_on_start = false};
}
}  // namespace application
