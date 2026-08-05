#ifndef DEBUG_LOG_HPP
#define DEBUG_LOG_HPP

#include <glm/glm.hpp>
#include <iostream>

// Runtime debug output, switched on through the CLI flag --debug-log; the app
// sets the switch at start through SetLogEnabled. Both the OpenGL canvas and the
// mouse interaction use it, hence a header of its own.
namespace decade_debug {

namespace debug_log_detail {
inline bool& LogEnabledFlag() {
  static bool enabled = false;
  return enabled;
}
}  // namespace debug_log_detail

inline void SetLogEnabled(bool enabled) {
  debug_log_detail::LogEnabledFlag() = enabled;
}

inline bool LogEnabled() { return debug_log_detail::LogEnabledFlag(); }

inline void LogMat4(const char* tag, const glm::mat4& matrix) {
  if (!LogEnabled()) {
    return;
  }
  std::cout << tag << ": diag(" << matrix[0][0] << "," << matrix[1][1] << ","
            << matrix[2][2] << ") trans(" << matrix[3][0] << "," << matrix[3][1]
            << "," << matrix[3][2] << ")\n";
}

inline void LogVec3(const char* tag, const glm::vec3& vector) {
  if (!LogEnabled()) {
    return;
  }
  std::cout << tag << ": (" << vector.x << "," << vector.y << "," << vector.z
            << ")\n";
}

}  // namespace decade_debug

#endif  // DEBUG_LOG_HPP
