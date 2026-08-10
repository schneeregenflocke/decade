#ifndef DEBUG_LOG_HPP
#define DEBUG_LOG_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

// Runtime debug output, switched on through the CLI flag --debug-log; the app
// sets the switch at start through SetLogEnabled. Both the OpenGL canvas and
// the mouse interaction use it, hence a header of its own.
namespace decade_debug {

void SetLogEnabled(bool enabled);

bool LogEnabled();

void LogMat4(const char* tag, const glm::mat4& matrix);

void LogVec3(const char* tag, const glm::vec3& vector);

}  // namespace decade_debug

#endif  // DEBUG_LOG_HPP
