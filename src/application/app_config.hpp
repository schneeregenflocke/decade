#ifndef APP_CONFIG_HPP
#define APP_CONFIG_HPP

#include <wx/frame.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

#include <string>

namespace application {
struct MainWindowConfig {
  std::string title;
  wxPoint position;
  wxSize size;
  long style{wxDEFAULT_FRAME_STYLE};
  wxString frame_name{"main_window"};
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
          .style = wxDEFAULT_FRAME_STYLE,
          .frame_name = "main_window",
          .maximize_on_start = false};
}
}  // namespace application

#endif  // APP_CONFIG_HPP
