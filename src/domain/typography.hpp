#ifndef TYPOGRAPHY_HPP
#define TYPOGRAPHY_HPP

// Punkt und Millimeter, die einzige Stelle mit dem Umrechnungsfaktor.
//
// Die Seite rechnet in Millimetern (die Papiergrösse kommt in mm herein). Der
// Nutzer denkt in Punkt: 1 pt = 1/72 Zoll. Eine Punktgrösse ist die Höhe des
// Gevierts — genau das Mass, in dem die Schrift ihre Glyphen skaliert (Font
// normiert alle Metriken auf Geviert = 1), also ist die Umrechnung ein reiner
// Faktor und kein Rendering-Thema.
namespace domain {

inline constexpr float kMillimetresPerPoint = 25.4F / 72.0F;

[[nodiscard]] constexpr float MillimetresFromPoints(float points) {
  return points * kMillimetresPerPoint;
}

[[nodiscard]] constexpr float PointsFromMillimetres(float millimetres) {
  return millimetres / kMillimetresPerPoint;
}

}  // namespace domain

#endif  // TYPOGRAPHY_HPP
