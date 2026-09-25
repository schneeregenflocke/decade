#ifndef GUI_SCRIPT_HPP
#define GUI_SCRIPT_HPP

#include <expected>
#include <string>
#include <string_view>
#include <vector>

// One step of a GUI script: a command and its arguments, and the line it came
// from, so every report can point at it. The vocabulary and what each command
// does stand in operations.md; GuiScriptRunner carries them out.
struct GuiStep {
  int line{0};
  std::string command;
  std::vector<std::string> arguments;
};

struct GuiScriptError {
  int line{0};
  std::string message;
};

// Splits a script into steps: one per line, words split at blanks, a double-
// quoted word may hold blanks, `#` starts a comment line. An unknown command or
// a wrong number of arguments fails the whole script before anything runs, so
// a typo cannot leave the application half driven.
[[nodiscard]] std::expected<std::vector<GuiStep>, GuiScriptError>
ParseGuiScript(std::string_view text);

[[nodiscard]] std::expected<int, std::string> ParseInteger(
    std::string_view word);

// "3 7-9" -> {3, 7, 8, 9}; the numbers are table rows counted from 1.
[[nodiscard]] std::expected<std::vector<int>, std::string> ParseRowList(
    const std::vector<std::string>& words);
#endif  // GUI_SCRIPT_HPP
