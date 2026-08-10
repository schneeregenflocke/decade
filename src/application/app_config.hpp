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

MainFrameConfig DefaultMainFrameConfig();

}  // namespace application

#endif  // APP_CONFIG_HPP
