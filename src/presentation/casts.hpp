#ifndef CASTS_HPP
#define CASTS_HPP

#include <QtGui/QColor>
#include <glm/vec4.hpp>

glm::vec4 ToGlmVec4(const QColor& color);

QColor ToQColor(const glm::vec4& color);

#endif  // CASTS_HPP
