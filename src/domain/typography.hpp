#ifndef TYPOGRAPHY_HPP
#define TYPOGRAPHY_HPP

// Points and millimetres, the only place carrying the conversion factor.
//
// The page computes in millimetres (the paper size arrives in mm). The user
// thinks in points: 1 pt = 1/72 inch. A point size is the height of the em —
// exactly the measure the font scales its glyphs by (Font normalises every
// metric to em = 1), so the conversion is a pure factor and no rendering
// matter.
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
