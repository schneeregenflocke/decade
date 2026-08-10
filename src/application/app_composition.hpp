#ifndef APP_COMPOSITION_HPP
#define APP_COMPOSITION_HPP

#include <QtCore/QObject>
#include <memory>
#include <optional>
#include <string>

#include "../domain/date_format.hpp"
#include "../presentation/file_commands.hpp"
#include "../presentation/main_frame.hpp"
#include "app_binder.hpp"
#include "app_config.hpp"
#include "calendar/calendar_page.hpp"
#include "calendar/interaction_controller.hpp"
#include "calendar/title_text_editor.hpp"
#include "event_bus.hpp"
#include "project_document.hpp"
#include "runtime_options.hpp"
#include "startup_script.hpp"

namespace application {

// The composition root (AGENTS.md, layer rule 3). Two parts come into being
// later, because OpenGL stands ready with a delay: the rendering adapter and
// the wiring, both in an `optional`. The adapter needs that order for its GL
// objects, which want a current context; the wiring needs it for the callbacks
// it planted in canvas, controller and editor, which capture the adapter and
// are no Qt connections that would release themselves.
class AppComposition {
 public:
  AppComposition(LocaleDateFormatter& locale_date_formatter,
                 RuntimeOptions options);

  ~AppComposition();
  AppComposition(const AppComposition&) = delete;
  AppComposition& operator=(const AppComposition&) = delete;
  AppComposition(AppComposition&&) = delete;
  AppComposition& operator=(AppComposition&&) = delete;

  [[nodiscard]] MainFrame& Frame();

 private:
  // Runs as soon as the GL context stands — only here may GL state be touched,
  // and only here is there anything to wire.
  //
  // A scene that cannot be built lands in the same place as a context that
  // never came up: the window would stand there operable and die on the first
  // interaction. Building it needs GL resources that exist by then or not at
  // all — a missing shader, for instance — so the failure is final and gets
  // reported rather than retried. Whatever came into being before the throw
  // gets dissolved first, so no half-built page survives.
  void OnGraphicsReady();

  // Without a context the ready path never runs: no wiring, no initial values.
  // An operable window in that state crashes on the first click — hence report
  // and close.
  //
  // Queued on the event loop, because this can arrive during the first paint:
  // a close() from there fizzles out, and a modal dialogue would stand in front
  // of the loop.
  void OnGraphicsFailed(const std::string& message);

  [[nodiscard]] AppComponents Components(CalendarPage& calendar_page);

  // Idempotent: it dissolves the wiring while both ends are still alive.
  //
  // The calendar page owns GL objects, and a buffer deleted without a current
  // context is a call into nothing — Qt makes the context current for the three
  // rendering callbacks alone, and this runs from the close event.
  void ReleaseGraphics();

  // Declared first, so destroyed last: every producer publishes over the bus,
  // and something can still fire while clearing away.
  EventBus bus_;

  RuntimeOptions runtime_options_;
  ProjectDocument document_;
  InteractionController interaction_controller_;
  TitleTextEditor title_text_editor_;
  StartupScript startup_script_;

  std::unique_ptr<MainFrame> frame_;
  std::optional<FileCommands> file_commands_;
  std::optional<CalendarPage> calendar_page_;
  std::optional<AppWiring> wiring_;

  // The context object of the two connections onto the window. Declared last it
  // dies first, so no menu command and no close event reaches a member already
  // gone.
  QObject connection_scope_;
};

}  // namespace application

#endif  // APP_COMPOSITION_HPP
