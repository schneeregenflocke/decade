#ifndef DEBUG_LOG_HPP
#define DEBUG_LOG_HPP

#include <glm/glm.hpp>
#include <iostream>

// Laufzeit-Debugausgaben, aktiviert über das CLI-Flag --debug-log; die App
// setzt den Schalter beim Start via SetLogEnabled. Wird sowohl vom
// OpenGL-Canvas als auch von der Maus-Interaktion genutzt, daher in einem
// eigenen Header.
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
