#ifndef SHAPE_CONFIGURATION_HPP
#define SHAPE_CONFIGURATION_HPP

#include <algorithm>
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

  ShapeConfiguration(std::string name, bool outline_visible, bool fill_visible,
                     float line_width, OutlineColorValue outline_color,
                     FillColorValue fill_color);

  [[nodiscard]] const std::string& Name() const;

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
  std::string name_;
  bool outline_visible_{true};
  bool fill_visible_{false};
  float line_width_{1.0F};
  glm::vec4 outline_color_{0.0F, 0.0F, 0.0F, 1.0F};
  glm::vec4 fill_color_{0.0F, 0.0F, 0.0F, 1.0F};
};

// Pure value object: the shape configurations, split into the fixed ones (page
// margin, labels, the per-year "Annual Sum", …) and the per-date-group ones.
// The two live in separate containers, so a group configuration is identified
// structurally (its position in the group list), not by parsing its name. No
// signal -> Rule of Zero, copyable.
class ShapeConfigSet {
 public:
  ShapeConfigSet();

  // The names of the fixed configurations. They are the contract between this
  // set and the scene builders that ask for them, and a mistyped one does not
  // fail loudly: GetShapeConfiguration answers with a default-constructed
  // value, which draws an invisible shape. So the string exists once, here,
  // beside the defaults that carry it.
  static constexpr std::string_view kPageMargin = "Page Margin";
  static constexpr std::string_view kTitleFrame = "Title Frame";
  static constexpr std::string_view kCalendarLabels = "Calendar Labels";
  static constexpr std::string_view kDayShapes = "Day Shapes";
  static constexpr std::string_view kSundayShapes = "Sunday Shapes";
  static constexpr std::string_view kMonthsShapes = "Months Shapes";
  static constexpr std::string_view kYearsShapes = "Years Shapes";
  // The per-year total, styled like a bar group so it sits beside them.
  static constexpr std::string_view kYearsTotals = "Years Totals";

  // The configuration with the given name, searched across the fixed and the
  // group configurations (a default-constructed value when absent). The name
  // remains a stable per-configuration label (and a node's style id); it is
  // just no longer what decides whether a configuration is a group entry.
  [[nodiscard]] ShapeConfiguration GetShapeConfiguration(
      std::string_view name) const;

  // Replaces the configuration that shares `config`'s name, in whichever
  // container holds it. Returns false when no such configuration exists.
  bool UpdateConfiguration(const ShapeConfiguration& config);

  // Display name of the per-date-group configuration at the given zero-based
  // index. Still used to label the group configurations (and as the matching
  // node style id); group membership no longer depends on it.
  [[nodiscard]] static std::string DynamicConfigurationName(size_t group_index);

  // The configuration for the date group at the given zero-based index (a
  // default-constructed value when out of range).
  [[nodiscard]] ShapeConfiguration GetDynamicConfiguration(
      size_t group_index) const;

  // Reconciles the group configurations with the current date groups: keeps the
  // existing entries (so user customisations survive), drops the entries past
  // `group_count` and synthesises fresh ones from the palette for newly added
  // groups. The "Annual Sum" entry is re-derived from the palette only when the
  // group count actually changed, so a rename of a group keeps its colour.
  void SyncToDateGroups(size_t group_count);

  // Raw access for non-intrusive serialization in the infrastructure layer.
  [[nodiscard]] const std::vector<ShapeConfiguration>& FixedConfigurations()
      const;
  [[nodiscard]] std::vector<ShapeConfiguration>& MutableFixedConfigurations();
  [[nodiscard]] const std::vector<ShapeConfiguration>& GroupConfigurations()
      const;
  [[nodiscard]] std::vector<ShapeConfiguration>& MutableGroupConfigurations();

 private:
  // Name prefix shared by every per-date-group configuration; used only to
  // label them, no longer to decide group membership.
  static constexpr std::string_view kDynamicNamePrefix = "Bar Group ";

  // Locates the configuration with the given name across both containers
  // (fixed first, then group), or nullptr when absent.
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
  [[nodiscard]] auto Find(this Self& self, std::string_view name)
      -> std::remove_reference_t<
          decltype(*self.fixed_configurations_.begin())>* {
    using Config =
        std::remove_reference_t<decltype(*self.fixed_configurations_.begin())>;
    for (auto* container :
         {&self.fixed_configurations_, &self.group_configurations_}) {
      const auto found = std::ranges::find_if(
          *container,
          [&](const ShapeConfiguration& config) { return config == name; });
      if (found != container->end()) {
        return static_cast<Config*>(&*found);
      }
    }
    return static_cast<Config*>(nullptr);
  }

  // Builds a categorical shape configuration: the colour comes from the shared
  // palette at the given index, with a stronger outline than fill so the box
  // has a visible border while the fill stays pastel. The single source of the
  // alpha recipe for every palette-driven entry (bar groups and the annual
  // sum).
  static ShapeConfiguration MakeCategoricalConfiguration(std::string name,
                                                         size_t palette_index);

  // Default configuration for the dynamic bar group at the given zero-based
  // index, reproducible across sessions because the palette is index-stable.
  static ShapeConfiguration MakeBarGroupConfiguration(size_t group_index);

  // Re-derives the "Annual Sum" configuration from the palette at the index
  // right past the last group, so it is coloured and styled exactly like a bar
  // group and stays consistent if the palette is ever changed.
  void RefreshAnnualSumConfiguration(size_t group_count);

  static std::vector<ShapeConfiguration> BuildDefaults();

  std::vector<ShapeConfiguration> fixed_configurations_;
  std::vector<ShapeConfiguration> group_configurations_;
};
#endif  // SHAPE_CONFIGURATION_HPP
