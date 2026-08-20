#ifndef EMBEDDED_RESOURCES_HPP
#define EMBEDDED_RESOURCES_HPP

#include <array>
#include <string_view>

// The GLSL sources the renderer compiles, carried inside the binary. The
// licence texts travel the same way, in third_party_licenses.hpp.
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

}  // namespace resources

#endif  // EMBEDDED_RESOURCES_HPP
