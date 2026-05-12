#ifndef APP_CONFIG_HPP
#define APP_CONFIG_HPP

#include <wx/gdicmn.h>

#include <string>

namespace app {
struct MainWindowConfig {
  std::string title;
  wxPoint position;
  wxSize size;
  bool maximize_on_start{true};
};

inline MainWindowConfig DefaultMainWindowConfig() {
  constexpr int kMainWindowPosX = 100;
  constexpr int kMainWindowPosY = 100;
  constexpr int kMainWindowWidth = 1280;
  constexpr int kMainWindowHeight = 800;

  return {.title = "Decade",
          .position = wxPoint(kMainWindowPosX, kMainWindowPosY),
          .size = wxSize(kMainWindowWidth, kMainWindowHeight),
          .maximize_on_start = true};
}
}  // namespace app

#endif  // APP_CONFIG_HPP
