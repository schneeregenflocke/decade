#ifndef APP_CONFIG_HPP
#define APP_CONFIG_HPP

#include <wx/frame.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

#include <string>

namespace application {
struct MainFrameConfig {
  std::string title;
  wxPoint position;
  wxSize size;
  long style{wxDEFAULT_FRAME_STYLE};
  wxString frame_name{"main_window"};
  bool maximize_on_start{true};
};

inline MainFrameConfig DefaultMainFrameConfig() {
  constexpr int kMainFramePosX = 100;
  constexpr int kMainFramePosY = 100;
  constexpr int kMainFrameWidth = 1280;
  constexpr int kMainFrameHeight = 800;

  return {.title = "Decade",
          .position = wxPoint(kMainFramePosX, kMainFramePosY),
          .size = wxSize(kMainFrameWidth, kMainFrameHeight),
          .style = wxDEFAULT_FRAME_STYLE,
          .frame_name = "main_window",
          .maximize_on_start = false};
}
}  // namespace application

#endif  // APP_CONFIG_HPP
