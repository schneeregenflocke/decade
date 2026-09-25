#ifndef SHAPE_CONFIGURATION_HPP
#define SHAPE_CONFIGURATION_HPP

#include <algorithm>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// Pure domain value: the visual configuration of one shape kind. No
// serialization, no signal -> Rule of Zero, copyable.
class ShapeConfiguration {
 public:
  struct OutlineColorValue {
    glm::vec4 value;
  };

  struct FillColorValue {
    glm::vec4 value;
  };

  ShapeConfiguration() = default;

  ShapeConfiguration(std::string key, bool outline_visible, bool fill_visible,
                     float line_width, OutlineColorValue outline_color,
                     FillColorValue fill_color);

  // The identity of this configuration: what a scene builder looks it up by,
  // what a node carries as its style id, what the shapes list shows and what
  // the project file stores.
  [[nodiscard]] const std::string& Key() const;

  void FillVisible(bool value);

  void OutlineVisible(bool value);

  [[nodiscard]] bool FillVisible() const;

  [[nodiscard]] bool OutlineVisible() const;

  void LineWidth(float value);

  void OutlineColor(const glm::vec4& value);

  void FillColor(const glm::vec4& value);

  [[nodiscard]] float LineWidth() const;

  [[nodiscard]] glm::vec4 OutlineColor() const;

  [[nodiscard]] glm::vec4 FillColor() const;

  [[nodiscard]] float LineWidthDisabled() const;

  [[nodiscard]] glm::vec4 OutlineColorDisabled() const;

  [[nodiscard]] glm::vec4 FillColorDisabled() const;

  bool operator==(std::string_view compare) const;

 private:
  std::string key_;
  bool outline_visible_{true};
  bool fill_visible_{false};
  float line_width_{1.0F};
  glm::vec4 outline_color_{0.0F, 0.0F, 0.0F, 1.0F};
  glm::vec4 fill_color_{0.0F, 0.0F, 0.0F, 1.0F};
};

// Pure value object: the shape configurations, split into the fixed ones (page
// margin, labels, the per-year coverage bar, …) and the per-date-category ones.
// The two live in separate containers, so a category configuration is
// identified structurally (its position in the category list), not by parsing
// its key. No signal -> Rule of Zero, copyable.
class ShapeConfigSet {
 public:
  ShapeConfigSet();

  // The keys of the fixed configurations: the identity a scene builder looks a
  // configuration up by, the style id of the node it draws and what a saved
  // project carries on disk. What a person reads is FixedConfigurationLabel().
  // Renaming a key breaks every project written before, because
  // GetShapeConfiguration answers an unknown key with a default-constructed
  // value, which draws nothing.
  static constexpr std::string_view kPageMarginKey = "page_margin";
  static constexpr std::string_view kTitleFrameKey = "title_frame";
  static constexpr std::string_view kCalendarLabelsKey = "calendar_labels";
  static constexpr std::string_view kDayShapesKey = "day_shapes";
  static constexpr std::string_view kSundayShapesKey = "sunday_shapes";
  static constexpr std::string_view kMonthsShapesKey = "months_shapes";
  static constexpr std::string_view kYearsShapesKey = "years_shapes";
  // The per-year bar: styled like a bar category so it sits beside them, but
  // coloured off the palette it is not part of. It aggregates the categories
  // instead of being one, and a colour derived from their count would move
  // under the user every time a category is added. Coverage rather than sum,
  // because the bar measures the marked days of a year and the figure beside
  // it their share of that year — a sum leaves open of what.
  static constexpr std::string_view kAnnualCoverageKey = "annual_coverage";

  // What the shapes list and the legend show for a fixed key; an unknown key
  // comes back as it is. A category configuration reads as its category's
  // name, which this set does not know.
  [[nodiscard]] static std::string_view FixedConfigurationLabel(
      std::string_view key);

  // The configuration under the given key, searched across the fixed and the
  // category configurations (a default-constructed value when absent). The key
  // no longer decides whether a configuration is a category entry — its
  // container does.
  [[nodiscard]] ShapeConfiguration GetShapeConfiguration(
      std::string_view key) const;

  // Replaces the configuration under `config`'s key, in whichever container
  // holds it. Returns false when no such configuration exists.
  bool UpdateConfiguration(const ShapeConfiguration& config);

  // Key of the per-date-category configuration at the given zero-based index;
  // it matches the node style id. Category membership does not depend on it.
  [[nodiscard]] static std::string DynamicConfigurationKey(
      size_t category_index);

  // The configuration for the date category at the given zero-based index (a
  // default-constructed value when out of range).
  [[nodiscard]] ShapeConfiguration GetDynamicConfiguration(
      size_t category_index) const;

  // Reconciles the category configurations with the current date categories:
  // keeps the existing entries (so user customisations survive), drops the
  // entries past `category_count` and synthesises fresh ones from the palette
  // for newly added categories. Every configuration that already exists keeps
  // its colour — an entry is coloured once, when it comes into being, and is
  // the user's from then on.
  void SyncToDateCategories(size_t category_count);

  // Raw access for non-intrusive serialization in the infrastructure layer.
  [[nodiscard]] const std::vector<ShapeConfiguration>& FixedConfigurations()
      const;
  [[nodiscard]] std::vector<ShapeConfiguration>& MutableFixedConfigurations();
  [[nodiscard]] const std::vector<ShapeConfiguration>& CategoryConfigurations()
      const;
  [[nodiscard]] std::vector<ShapeConfiguration>&
  MutableCategoryConfigurations();

 private:
  static constexpr std::string_view kCategoryKeyPrefix = "category_";

  // Locates the configuration under the given key across both containers
  // (fixed first, then category), or nullptr when absent.
  //
  // One body for both constnesses through [deducing this]
  // (https://en.cppreference.com/w/cpp/language/member_functions): a const set
  // hands out a const pointer, a mutable one a mutable pointer. The return type
  // reads that off the iterator rather than off `Self`, so the constness comes
  // from the container the search actually walks. It has to be spelled out
  // rather than deduced, because the callers above stand before this
  // definition. `Self&` and not `Self&&`: the search reads alone, and a
  // forwarding reference would promise a move that never happens.
  //
  // A template, so it stays in the header where every instantiation can see it.
  template <typename Self>
  [[nodiscard]] auto Find(this Self& self, std::string_view key)
      -> std::remove_reference_t<
          decltype(*self.fixed_configurations_.begin())>* {
    using Config =
        std::remove_reference_t<decltype(*self.fixed_configurations_.begin())>;
    for (auto* container :
         {&self.fixed_configurations_, &self.category_configurations_}) {
      const auto found = std::ranges::find_if(
          *container,
          [&](const ShapeConfiguration& config) { return config == key; });
      if (found != container->end()) {
        return static_cast<Config*>(&*found);
      }
    }
    return static_cast<Config*>(nullptr);
  }

  // Builds a bar-styled shape configuration around the given colour: a stronger
  // outline than fill, so the box has a visible border while the fill stays
  // pastel. The single source of that alpha recipe, for the bar categories and
  // for the annual coverage alike.
  static ShapeConfiguration MakeBarStyledConfiguration(std::string key,
                                                       const glm::vec3& color);

  // Default configuration for the dynamic bar category at the given zero-based
  // index, reproducible across sessions because the palette is index-stable.
  static ShapeConfiguration MakeBarCategoryConfiguration(size_t category_index);

  static std::vector<ShapeConfiguration> BuildDefaults();

  std::vector<ShapeConfiguration> fixed_configurations_;
  std::vector<ShapeConfiguration> category_configurations_;
};
#endif  // SHAPE_CONFIGURATION_HPP
