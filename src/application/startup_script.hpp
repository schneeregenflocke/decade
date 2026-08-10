#ifndef STARTUP_SCRIPT_HPP
#define STARTUP_SCRIPT_HPP

#include "../presentation/main_frame.hpp"
#include "calendar/calendar_page.hpp"
#include "calendar/title_text_editor.hpp"
#include "project_document.hpp"
#include "runtime_options.hpp"

namespace application {

// Turns the command line into actions — load the startup file, preselect a tab,
// write an image, close after N milliseconds. These steps used to sit scattered
// across the main window as six methods; here they stand together, and the
// window knows nothing of runtime options any more.
//
// Two moments, because OpenGL stands ready with a delay: whatever works without
// a context runs at once, the rest afterwards.
class StartupScript {
 public:
  StartupScript(const RuntimeOptions& options, ProjectDocument& document);

  void RunBeforeGraphics(MainFrame& frame) const;

  void RunAfterGraphics(MainFrame& frame, CalendarPage& calendar_page,
                        TitleTextEditor& title_text_editor) const;

 private:
  void SelectStartupTab(MainFrame& frame) const;

  // Opt-in: what came as a positional argument gets loaded, and nothing else.
  // Without one an empty project starts; a default path relative to the working
  // directory deliberately does not exist.
  void LoadStartupFile() const;

  void ApplyDebugHighlights(MainFrame& frame, CalendarPage& calendar_page,
                            TitleTextEditor& title_text_editor) const;

  void WriteRequestedImages(MainFrame& frame) const;

  const RuntimeOptions& options_;
  ProjectDocument& document_;
};

}  // namespace application

#endif  // STARTUP_SCRIPT_HPP
