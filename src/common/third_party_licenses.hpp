#ifndef THIRD_PARTY_LICENSES_HPP
#define THIRD_PARTY_LICENSES_HPP

#include <array>
#include <cstddef>
#include <span>
#include <string_view>

// The licence texts the about dialogue shows, embedded in the binary the same
// way the shaders are (see embedded_resources.hpp for how `#embed` carries a
// file into a constant).
//
// Every library the build links appears here, not just the ones vendored under
// `external/`: the notice obligations follow the linker, not the repository
// layout. Whoever adds a dependency to CMakeLists.txt adds its licence text to
// `licenses/` and one line below — and enters it in the SBOM beside the
// `find_package` call it came in with.
//
// The order is the display order: the application first, the notices that no
// licence text carries by itself second, then Qt with the two GNU texts its
// licence refers to, then the rest alphabetically.
namespace licenses {

struct Notice {
  std::string_view name;
  // Bytes rather than characters: a licence text carries UTF-8 above 127, and
  // `char` narrows there — GCC rejects what clang lets through.
  std::span<const unsigned char> text;
};

namespace detail {

inline constexpr auto kDecadeData = std::to_array<unsigned char>({
#embed "../../LICENSE.txt"
});
inline constexpr auto kNoticesData = std::to_array<unsigned char>({
#embed "../../licenses/notices.txt"
});
inline constexpr auto kQtLgplData = std::to_array<unsigned char>({
#embed "../../licenses/lgpl-3.0.txt"
});
inline constexpr auto kGplData = std::to_array<unsigned char>({
#embed "../../licenses/gpl-3.0.txt"
});
inline constexpr auto kBoostData = std::to_array<unsigned char>({
#embed "../../licenses/boost.txt"
});
inline constexpr auto kBulletData = std::to_array<unsigned char>({
#embed "../../external/bullet3/LICENSE.txt"
});
inline constexpr auto kCsv2Data = std::to_array<unsigned char>({
#embed "../../external/csv2/LICENSE"
});
inline constexpr auto kCsv2MioData = std::to_array<unsigned char>({
#embed "../../external/csv2/LICENSE.mio"
});
inline constexpr auto kFontconfigData = std::to_array<unsigned char>({
#embed "../../licenses/fontconfig.txt"
});
inline constexpr auto kFreetypeData = std::to_array<unsigned char>({
#embed "../../licenses/freetype.txt"
});
inline constexpr auto kGlmData = std::to_array<unsigned char>({
#embed "../../licenses/glm.txt"
});
inline constexpr auto kIcuData = std::to_array<unsigned char>({
#embed "../../licenses/icu.txt"
});
inline constexpr auto kLibepoxyData = std::to_array<unsigned char>({
#embed "../../licenses/libepoxy.txt"
});
inline constexpr auto kLibpngData = std::to_array<unsigned char>({
#embed "../../licenses/libpng.txt"
});
inline constexpr auto kMicrosoftGslData = std::to_array<unsigned char>({
#embed "../../licenses/microsoft_gsl.txt"
});
inline constexpr auto kTinycolormapData = std::to_array<unsigned char>({
#embed "../../external/tinycolormap/LICENSE"
});
inline constexpr auto kZlibData = std::to_array<unsigned char>({
#embed "../../licenses/zlib.txt"
});

template <std::size_t Size>
constexpr std::span<const unsigned char> AsText(
    const std::array<unsigned char, Size>& data) {
  return std::span<const unsigned char>{data};
}

}  // namespace detail

inline constexpr std::array kNotices{
    Notice{.name = "Decade", .text = detail::AsText(detail::kDecadeData)},
    Notice{.name = "Third-party notices",
           .text = detail::AsText(detail::kNoticesData)},
    Notice{.name = "Qt 6 (LGPL v3)",
           .text = detail::AsText(detail::kQtLgplData)},
    Notice{.name = "GNU GPL v3", .text = detail::AsText(detail::kGplData)},
    Notice{.name = "Boost", .text = detail::AsText(detail::kBoostData)},
    Notice{.name = "Bullet Physics",
           .text = detail::AsText(detail::kBulletData)},
    Notice{.name = "csv2", .text = detail::AsText(detail::kCsv2Data)},
    Notice{.name = "csv2mio", .text = detail::AsText(detail::kCsv2MioData)},
    Notice{.name = "fontconfig",
           .text = detail::AsText(detail::kFontconfigData)},
    Notice{.name = "FreeType", .text = detail::AsText(detail::kFreetypeData)},
    Notice{.name = "glm", .text = detail::AsText(detail::kGlmData)},
    Notice{.name = "ICU", .text = detail::AsText(detail::kIcuData)},
    Notice{.name = "libepoxy", .text = detail::AsText(detail::kLibepoxyData)},
    Notice{.name = "libpng", .text = detail::AsText(detail::kLibpngData)},
    Notice{.name = "Microsoft GSL",
           .text = detail::AsText(detail::kMicrosoftGslData)},
    Notice{.name = "tinycolormap",
           .text = detail::AsText(detail::kTinycolormapData)},
    Notice{.name = "zlib", .text = detail::AsText(detail::kZlibData)},
};

}  // namespace licenses

#endif  // THIRD_PARTY_LICENSES_HPP
