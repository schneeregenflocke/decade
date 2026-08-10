#include "value_serialization.hpp"

#include <array>
#include <charconv>
#include <cstddef>
#include <format>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <system_error>

#include "../../domain/date.hpp"

namespace persistence::serialization_detail {

std::string DateToIsoString(const Date& date) {
  if (!date.IsValid()) {
    return {};
  }
  return std::format("{:04}-{:02}-{:02}", date.Year(), date.Month(),
                     date.Day());
}

Date DateFromIsoString(const std::string& text) {
  // Field positions inside "YYYY-MM-DD".
  constexpr std::size_t kIsoLength = 10;
  constexpr std::size_t kYearEnd = 4;
  constexpr std::size_t kMonthBegin = 5;
  constexpr std::size_t kMonthEnd = 7;
  constexpr std::size_t kDayBegin = 8;
  if (text.size() != kIsoLength || text[kYearEnd] != '-' ||
      text[kMonthEnd] != '-') {
    return {};
  }
  const auto parse_int = [](const char* first, const char* last, int& out) {
    return std::from_chars(first, last, out).ec == std::errc{};
  };
  int year = 0;
  int month = 0;
  int day = 0;
  const char* data = text.data();
  if (!parse_int(data, data + kYearEnd, year) ||
      !parse_int(data + kMonthBegin, data + kMonthEnd, month) ||
      !parse_int(data + kDayBegin, data + kIsoLength, day)) {
    return {};
  }
  return Date::FromYmd(year, month, day);
}

std::array<float, 4> ColorToArray(const glm::vec4& color) {
  return {color[0], color[1], color[2], color[3]};
}

glm::vec4 ColorFromArray(const std::array<float, 4>& array) {
  return {array[0], array[1], array[2], array[3]};
}

}  // namespace persistence::serialization_detail
