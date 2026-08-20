#include <gtest/gtest.h>

#include <algorithm>
#include <span>
#include <string_view>

#include "common/third_party_licenses.hpp"

namespace {

// The notices carry bytes, so the needle gets compared byte-wise rather than
// through std::string_view::find.
bool Carries(std::span<const unsigned char> text, std::string_view needle) {
  const auto found =
      std::ranges::search(text, needle, [](unsigned char left, char right) {
        return left == static_cast<unsigned char>(right);
      });
  return !found.empty();
}

bool Lists(std::string_view name) {
  return std::ranges::any_of(licenses::kNotices, [name](const auto& notice) {
    return notice.name == name;
  });
}

}  // namespace

// An `#embed` of a missing or emptied file compiles and yields nothing, so the
// dialogue would show a blank page where a licence has to stand.
TEST(ThirdPartyLicensesTest, EveryNoticeCarriesText) {
  for (const auto& notice : licenses::kNotices) {
    EXPECT_FALSE(notice.name.empty());
    EXPECT_FALSE(notice.text.empty()) << notice.name;
  }
}

// The obligations follow the linker: every library CMakeLists.txt links has to
// appear, and dropping one silently is what this test exists to prevent.
TEST(ThirdPartyLicensesTest, ListsEveryLinkedLibrary) {
  for (const std::string_view name :
       {"Decade", "Third-party notices", "Qt 6 (LGPL v3)", "GNU GPL v3",
        "Boost", "Bullet Physics", "csv2", "csv2mio", "fontconfig", "FreeType",
        "glm", "ICU", "libepoxy", "libpng", "Microsoft GSL", "tinycolormap",
        "zlib"}) {
    EXPECT_TRUE(Lists(name)) << name;
  }
}

// Qt is the only copyleft in the tree: LGPLv3 asks for its own text and for the
// GPLv3 text it refers to, both reachable from the running application.
TEST(ThirdPartyLicensesTest, CarriesBothGnuTextsForQt) {
  const auto text_of = [](std::string_view name) {
    const auto found =
        std::ranges::find(licenses::kNotices, name, &licenses::Notice::name);
    return found == licenses::kNotices.end() ? std::span<const unsigned char>{}
                                             : found->text;
  };
  EXPECT_TRUE(Carries(text_of("Qt 6 (LGPL v3)"), "GNU LESSER GENERAL PUBLIC"));
  EXPECT_TRUE(Carries(text_of("GNU GPL v3"), "GNU GENERAL PUBLIC LICENSE"));
  EXPECT_TRUE(Carries(text_of("Third-party notices"), "FreeType Team"));
}
