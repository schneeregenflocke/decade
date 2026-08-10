#ifndef EMBEDDED_RESOURCES_HPP
#define EMBEDDED_RESOURCES_HPP

#include <array>
#include <string_view>

// The files that travel inside the binary: the GLSL sources the renderer
// compiles, and the licence texts the about panel shows.
//
// [`#embed`](https://en.cppreference.com/cpp/preprocessor/embed) reads them at
// preprocessing time, so there is no generator, no build step and no submodule
// between the file and the constant — which is what this replaced. The paths
// resolve like an `#include "..."`, relative to this header, so nothing has to
// be added to the include path either. A changed file still forces a rebuild:
// the compiler lists it in the dependency file the same way it lists a header.
//
// Written out rather than wrapped in a macro. `#embed` is a directive and
// cannot live inside a function or a template, so a macro is the only way to
// shorten this — and a macro is what AGENTS.md sends us away from.
namespace resources {

namespace detail {

inline constexpr auto kFontVertexShaderData = std::to_array<char>({
#embed "../../shaders/font_vertex_shader.glsl"
});
inline constexpr auto kFontFragmentShaderData = std::to_array<char>({
#embed "../../shaders/font_fragment_shader.glsl"
});
inline constexpr auto kSimpleVertexShaderData = std::to_array<char>({
#embed "../../shaders/simple_vertex_shader.glsl"
});
inline constexpr auto kSimpleFragmentShaderData = std::to_array<char>({
#embed "../../shaders/simple_fragment_shader.glsl"
});
inline constexpr auto kRectanglesVertexShaderData = std::to_array<char>({
#embed "../../shaders/rectangles_vertex_shader.glsl"
});
inline constexpr auto kRectanglesFragmentShaderData = std::to_array<char>({
#embed "../../shaders/rectangles_fragment_shader.glsl"
});

inline constexpr auto kDecadeLicenseData = std::to_array<char>({
#embed "../../LICENSE.txt"
});
inline constexpr auto kCsv2LicenseData = std::to_array<char>({
#embed "../../external/csv2/LICENSE"
});
inline constexpr auto kCsv2MioLicenseData = std::to_array<char>({
#embed "../../external/csv2/LICENSE.mio"
});
inline constexpr auto kTinycolormapLicenseData = std::to_array<char>({
#embed "../../external/tinycolormap/LICENSE"
});
inline constexpr auto kBulletLicenseData = std::to_array<char>({
#embed "../../external/bullet3/LICENSE.txt"
});

}  // namespace detail

inline constexpr std::string_view kFontVertexShader{
    detail::kFontVertexShaderData.data(), detail::kFontVertexShaderData.size()};
inline constexpr std::string_view kFontFragmentShader{
    detail::kFontFragmentShaderData.data(),
    detail::kFontFragmentShaderData.size()};
inline constexpr std::string_view kSimpleVertexShader{
    detail::kSimpleVertexShaderData.data(),
    detail::kSimpleVertexShaderData.size()};
inline constexpr std::string_view kSimpleFragmentShader{
    detail::kSimpleFragmentShaderData.data(),
    detail::kSimpleFragmentShaderData.size()};
inline constexpr std::string_view kRectanglesVertexShader{
    detail::kRectanglesVertexShaderData.data(),
    detail::kRectanglesVertexShaderData.size()};
inline constexpr std::string_view kRectanglesFragmentShader{
    detail::kRectanglesFragmentShaderData.data(),
    detail::kRectanglesFragmentShaderData.size()};

inline constexpr std::string_view kDecadeLicense{
    detail::kDecadeLicenseData.data(), detail::kDecadeLicenseData.size()};
inline constexpr std::string_view kCsv2License{detail::kCsv2LicenseData.data(),
                                               detail::kCsv2LicenseData.size()};
inline constexpr std::string_view kCsv2MioLicense{
    detail::kCsv2MioLicenseData.data(), detail::kCsv2MioLicenseData.size()};
inline constexpr std::string_view kTinycolormapLicense{
    detail::kTinycolormapLicenseData.data(),
    detail::kTinycolormapLicenseData.size()};
inline constexpr std::string_view kBulletLicense{
    detail::kBulletLicenseData.data(), detail::kBulletLicenseData.size()};

}  // namespace resources

#endif  // EMBEDDED_RESOURCES_HPP
