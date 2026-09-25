#ifndef GUI_SCRIPT_RUNNER_HPP
#define GUI_SCRIPT_RUNNER_HPP

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtWidgets/QMainWindow>
#include <cstddef>
#include <expected>
#include <string>
#include <vector>

#include "gui_script.hpp"

// Plays a GUI script against the running window through QTest, so every step
// takes the route a person's input takes: hit test, focus, the widget's own
// signals. Widgets are found by what a person reads — a tab label, a button
// text, a column header, a label beside a combo box.
//
// The next step is armed before the current one runs. A step that opens a
// modal dialogue (a file dialogue behind File -> Save As) returns only once the
// dialogue closes; the armed timer fires inside the dialogue's event loop, so
// the following step can fill the dialogue in.
//
// A failing step names its line on stderr and ends the application with exit
// code 1. With --debug-log every step is echoed before it runs.
class GuiScriptRunner : public QObject {
 public:
  GuiScriptRunner(std::vector<GuiStep> steps, QMainWindow& window);

  void Start();

 private:
  static constexpr int kStepDelayMs = 150;

  void RunNext();

  [[nodiscard]] std::expected<void, std::string> Execute(const GuiStep& step);

  void Fail(const GuiStep& step, const std::string& message);

  std::vector<GuiStep> steps_;
  QPointer<QMainWindow> window_;
  std::size_t next_step_{0};
  bool stopped_{false};
};
#endif  // GUI_SCRIPT_RUNNER_HPP
