#include "debug_log.hpp"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <iostream>

namespace decade_debug {
namespace {

bool& LogEnabledFlag() {
  static bool enabled = false;
  return enabled;
}

}  // namespace

void SetLogEnabled(bool enabled) { LogEnabledFlag() = enabled; }

bool LogEnabled() { return LogEnabledFlag(); }

void LogMat4(const char* tag, const glm::mat4& matrix) {
  if (!LogEnabled()) {
    return;
  }
  std::cout << tag << ": diag(" << matrix[0][0] << "," << matrix[1][1] << ","
            << matrix[2][2] << ") trans(" << matrix[3][0] << "," << matrix[3][1]
            << "," << matrix[3][2] << ")\n";
}

void LogVec3(const char* tag, const glm::vec3& vector) {
  if (!LogEnabled()) {
    return;
  }
  std::cout << tag << ": (" << vector.x << "," << vector.y << "," << vector.z
            << ")\n";
}

}  // namespace decade_debug
