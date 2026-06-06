#ifndef HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_DEBUG_LOG_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_DEBUG_LOG_HPP

#include <cstdlib>
#include <glm/glm.hpp>
#include <iostream>

// Laufzeit-Debugausgaben, aktiviert über die Umgebungsvariable
// DECADE_DEBUG_LOG. Wird sowohl vom OpenGL-Canvas als auch von der
// Maus-Interaktion genutzt, daher in einem eigenen Header.
namespace decade_debug {

inline bool LogEnabled() {
  static const bool enabled = std::getenv("DECADE_DEBUG_LOG") != nullptr;
  return enabled;
}

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

#endif  // HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_DEBUG_LOG_HPP
