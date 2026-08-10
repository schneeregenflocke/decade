#include "utf8_codec.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "unicode.hpp"

void AppendCodePoints(const icu::UnicodeString& text, std::int32_t begin,
                      std::int32_t end, std::vector<char32_t>& out) {
  constexpr UChar32 kBmpMax = 0xFFFF;
  for (std::int32_t index = begin; index < end;) {
    const UChar32 code_point = text.char32At(index);
    index += (code_point > kBmpMax) ? 2 : 1;
    out.push_back(static_cast<char32_t>(code_point));
  }
}

std::vector<char32_t> DecodeUtf8(const std::string& text) {
  std::vector<char32_t> code_points;
  if (text.empty()) {
    return code_points;
  }

  const icu::UnicodeString utf16 = icu::UnicodeString::fromUTF8(
      icu::StringPiece(text.data(), static_cast<std::int32_t>(text.size())));

  UErrorCode status = U_ZERO_ERROR;
  const icu::Normalizer2* nfc = icu::Normalizer2::getNFCInstance(status);
  // ICU's U_SUCCESS / U_FAILURE expand to UBool (signed char); compare against
  // 0 to get a real bool rather than relying on an implicit conversion.
  icu::UnicodeString normalized =
      (U_SUCCESS(status) != 0) ? nfc->normalize(utf16, status) : utf16;
  if (U_FAILURE(status) != 0) {
    normalized = utf16;  // fall back to the unnormalised text
  }

  code_points.reserve(static_cast<std::size_t>(normalized.countChar32()));

  status = U_ZERO_ERROR;
  const std::unique_ptr<icu::BreakIterator> grapheme_breaks(
      icu::BreakIterator::createCharacterInstance(icu::Locale::getRoot(),
                                                  status));
  if (U_FAILURE(status) != 0 || !grapheme_breaks) {
    AppendCodePoints(normalized, 0, normalized.length(), code_points);
    return code_points;
  }

  grapheme_breaks->setText(normalized);
  std::int32_t start = grapheme_breaks->first();
  for (std::int32_t end = grapheme_breaks->next();
       end != icu::BreakIterator::DONE;
       start = end, end = grapheme_breaks->next()) {
    AppendCodePoints(normalized, start, end, code_points);
  }

  return code_points;
}

std::string EncodeUtf8(const std::u32string& code_points) {
  icu::UnicodeString utf16;
  for (const char32_t code_point : code_points) {
    utf16.append(static_cast<UChar32>(code_point));
  }
  std::string text;
  utf16.toUTF8String(text);
  return text;
}
