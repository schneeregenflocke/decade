#include "gui_script.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <expected>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

struct CommandShape {
  std::string_view name;
  std::size_t min_arguments;
  std::size_t max_arguments;
};

constexpr std::size_t kUnbounded = 1000;

constexpr std::array<CommandShape, 10> kCommands{{
    {.name = "tab", .min_arguments = 1, .max_arguments = 1},
    {.name = "click", .min_arguments = 1, .max_arguments = 1},
    {.name = "edit-cell", .min_arguments = 3, .max_arguments = 3},
    {.name = "select-rows", .min_arguments = 1, .max_arguments = kUnbounded},
    {.name = "choose", .min_arguments = 2, .max_arguments = 2},
    {.name = "pick-color", .min_arguments = 2, .max_arguments = 2},
    {.name = "menu", .min_arguments = 2, .max_arguments = 2},
    {.name = "file-dialog", .min_arguments = 1, .max_arguments = 1},
    {.name = "wait", .min_arguments = 1, .max_arguments = 1},
    {.name = "quit", .min_arguments = 0, .max_arguments = 0},
}};

std::expected<std::vector<std::string>, std::string> SplitWords(
    std::string_view line) {
  std::vector<std::string> words;
  std::size_t position = 0;
  while (position < line.size()) {
    if (line[position] == ' ' || line[position] == '\t') {
      ++position;
      continue;
    }
    if (line[position] == '"') {
      const std::size_t closing = line.find('"', position + 1);
      if (closing == std::string_view::npos) {
        return std::unexpected("unclosed quote");
      }
      words.emplace_back(line.substr(position + 1, closing - position - 1));
      position = closing + 1;
      continue;
    }
    const std::size_t end = line.find_first_of(" \t", position);
    const std::size_t stop = end == std::string_view::npos ? line.size() : end;
    words.emplace_back(line.substr(position, stop - position));
    position = stop;
  }
  return words;
}

}  // namespace

std::expected<int, std::string> ParseInteger(std::string_view word) {
  int value = 0;
  const auto [end, error] =
      std::from_chars(word.data(), word.data() + word.size(), value);
  if (error != std::errc{} || end != word.data() + word.size()) {
    return std::unexpected("not a number: " + std::string(word));
  }
  return value;
}

std::expected<std::vector<GuiStep>, GuiScriptError> ParseGuiScript(
    std::string_view text) {
  std::vector<GuiStep> steps;
  int line_number = 0;
  std::size_t line_start = 0;
  while (line_start <= text.size()) {
    ++line_number;
    const std::size_t line_end = text.find('\n', line_start);
    const std::size_t stop =
        line_end == std::string_view::npos ? text.size() : line_end;
    std::string_view line = text.substr(line_start, stop - line_start);
    line_start = stop + 1;
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
    }

    const auto words = SplitWords(line);
    if (!words) {
      return std::unexpected(
          GuiScriptError{.line = line_number, .message = words.error()});
    }
    if (words->empty() || words->front().starts_with('#')) {
      continue;
    }

    const std::string& command = words->front();
    const auto* shape =
        std::ranges::find(kCommands, command, &CommandShape::name);
    if (shape == kCommands.end()) {
      return std::unexpected(GuiScriptError{
          .line = line_number, .message = "unknown command: " + command});
    }
    const std::size_t argument_count = words->size() - 1;
    if (argument_count < shape->min_arguments ||
        argument_count > shape->max_arguments) {
      return std::unexpected(GuiScriptError{
          .line = line_number,
          .message = "wrong number of arguments for " + command});
    }
    steps.push_back(GuiStep{.line = line_number,
                            .command = command,
                            .arguments = std::vector<std::string>(
                                words->begin() + 1, words->end())});
  }
  return steps;
}

std::expected<std::vector<int>, std::string> ParseRowList(
    const std::vector<std::string>& words) {
  std::vector<int> rows;
  for (const std::string& word : words) {
    const std::size_t dash = word.find('-');
    if (dash == std::string::npos) {
      const auto row = ParseInteger(word);
      if (!row) {
        return std::unexpected(row.error());
      }
      rows.push_back(*row);
      continue;
    }
    const auto first = ParseInteger(std::string_view(word).substr(0, dash));
    const auto last = ParseInteger(std::string_view(word).substr(dash + 1));
    if (!first || !last || *last < *first) {
      return std::unexpected("not a row range: " + word);
    }
    for (int row = *first; row <= *last; ++row) {
      rows.push_back(row);
    }
  }
  if (std::ranges::any_of(rows, [](int row) { return row < 1; })) {
    return std::unexpected(std::string("rows count from 1"));
  }
  return rows;
}
