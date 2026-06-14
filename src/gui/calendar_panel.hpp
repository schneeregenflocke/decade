#ifndef CALENDAR_PANEL_HPP
#define CALENDAR_PANEL_HPP

#include <wx/propgrid/propgrid.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <cstddef>
#include <sigslot/signal.hpp>
#include <vector>

#include "../packages/calendar_config.hpp"
#include "wx_owned.hpp"

// Property grid that edits a CalendarConfig: the calendar's year span and the
// per-row spacing proportions. It is a pure view — LoadConfig() pushes a config
// into the widgets, ReadConfig() reads the widgets back into a config — so the
// owning panel never reaches into the individual property handles.
//
// Row-spacing order: the rendered layout stacks the proportions along the
// rising y-axis (index 0 is the bottom gap, the last index the top gap). The
// grid therefore lists them top-to-bottom in reverse index order, so the entry
// at the top of the grid is the one drawn at the top of the page.
class CalendarPropertyGrid : public wxPropertyGrid {
 public:
  explicit CalendarPropertyGrid(wxWindow* parent)
      : wxPropertyGrid(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                       wxPG_SPLITTER_AUTO_CENTER),
        auto_span_property_(MakeOwned<wxBoolProperty>("Auto Span", wxPG_LABEL)),
        first_year_property_(
            MakeOwned<wxIntProperty>("First Year", wxPG_LABEL)),
        last_year_property_(MakeOwned<wxIntProperty>("Last Year", wxPG_LABEL)) {
    auto* box_sizer = MakeOwned<wxBoxSizer>(wxHORIZONTAL);
    const auto sizer_flags = wxSizerFlags().Proportion(1).Expand();
    parent->GetSizer()->Add(box_sizer, sizer_flags);
    box_sizer->Add(this, sizer_flags);

    SetVerticalSpacing(2);

    // The Append() order defines the grid's top-to-bottom layout; the dynamic
    // spacing rows are appended below the category by SyncSpacingRows().
    Append(MakeOwned<wxPropertyCategory>("Calendar Span (Years)", wxPG_LABEL));
    Append(auto_span_property_);
    Append(first_year_property_);
    Append(last_year_property_);

    Append(
        MakeOwned<wxPropertyCategory>("Row Spacing Proportions", wxPG_LABEL));

    RefreshSpanLimitsState();
  }

  // Mirrors a config into the grid's widgets.
  void LoadConfig(const CalendarConfig& config) {
    const auto& proportions = config.GetSpacingProportions();

    SyncSpacingRows(proportions.size());
    for (std::size_t index = 0; index < proportions.size(); ++index) {
      SetPropertyValue(spacing_properties_[index],
                       static_cast<double>(proportions[index]));
    }

    SetPropertyValue(auto_span_property_, config.IsAutoCalendarSpan());
    SetPropertyValue(first_year_property_, config.GetSpanLimitsYears()[0]);
    SetPropertyValue(last_year_property_, config.GetSpanLimitsYears()[1]);

    RefreshSpanLimitsState();
  }

  // Reads the grid's widgets back into a fresh config. The number of spacing
  // rows is fixed by the last LoadConfig() — the user only edits values — so
  // this just reads the current widgets back. Not const: it also reconciles the
  // year-limit enabled state with the (possibly just toggled) Auto Span flag.
  [[nodiscard]] CalendarConfig ReadConfig() {
    RefreshSpanLimitsState();

    CalendarConfig config;

    std::vector<float> proportions(spacing_properties_.size());
    for (std::size_t index = 0; index < proportions.size(); ++index) {
      proportions[index] = static_cast<float>(
          GetPropertyValue(spacing_properties_[index]).GetDouble());
    }
    config.SetSpacingProportions(proportions);

    config.SetAutoCalendarSpan(GetPropertyValue(auto_span_property_).GetBool());
    config.SetSpan(CalendarSpan::YearSpan{
        .first_year = static_cast<int>(
            GetPropertyValue(first_year_property_).GetInteger()),
        .last_year = static_cast<int>(
            GetPropertyValue(last_year_property_).GetInteger())});

    return config;
  }

 private:
  static constexpr double kDefaultSpacing = 10.0;

  // Spacings alternate gap / subrow / gap …, so even indices are gaps and odd
  // indices subrows; the ordinal counts each kind from the bottom up.
  [[nodiscard]] static wxString SpacingLabel(std::size_t index) {
    const std::size_t ordinal = (index / 2) + 1;
    const bool is_subrow = (index % 2) == 1;
    return wxString::Format(is_subrow ? "Subrow %zu" : "Gap %zu", ordinal);
  }

  // Enables the explicit year limits only while the span is not automatic.
  void RefreshSpanLimitsState() {
    const bool auto_span = GetPropertyValue(auto_span_property_).GetBool();
    if (auto_span) {
      DisableProperty(first_year_property_);
      DisableProperty(last_year_property_);
    } else {
      EnableProperty(first_year_property_);
      EnableProperty(last_year_property_);
    }
  }

  // Rebuilds the spacing rows when their count changes (only on LoadConfig).
  // Appending from the highest index down lays them out so the grid's top
  // matches the page's top.
  void SyncSpacingRows(std::size_t count) {
    if (count == spacing_properties_.size()) {
      return;
    }

    for (auto* property : spacing_properties_) {
      DeleteProperty(property);
    }
    spacing_properties_.assign(count, nullptr);

    for (std::size_t index = count; index-- > 0;) {
      auto* property = MakeOwned<wxFloatProperty>(SpacingLabel(index),
                                                  wxPG_LABEL, kDefaultSpacing);
      Append(property);
      spacing_properties_[index] = property;
    }
  }

  wxBoolProperty* auto_span_property_;
  wxIntProperty* first_year_property_;
  wxIntProperty* last_year_property_;
  // Indexed by proportion index (rising y-axis), independent of grid order.
  std::vector<wxFloatProperty*> spacing_properties_;
};

class CalendarSetupPanel : public wxPanel {
 public:
  explicit CalendarSetupPanel(wxWindow* parent)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTAB_TRAVERSAL, wxPanelNameStr) {
    SetSizer(MakeOwned<wxBoxSizer>(wxVERTICAL));
    property_grid_ = MakeOwned<CalendarPropertyGrid>(this);

    Bind(wxEVT_PG_CHANGED, &CalendarSetupPanel::CallbackPropertyGridChanged,
         this);

    property_grid_->LoadConfig(calendar_config_);
  }

  void ReceiveCalendarConfig(const CalendarConfig& incoming_calendar_config) {
    calendar_config_ = incoming_calendar_config;
    property_grid_->LoadConfig(calendar_config_);
  }

  sigslot::signal<const CalendarConfig&>& SignalCalendarConfig() {
    return signal_calendar_config_;
  }

 private:
  void CallbackPropertyGridChanged(wxPropertyGridEvent& /*event*/) {
    calendar_config_ = property_grid_->ReadConfig();
    signal_calendar_config_(calendar_config_);
  }

  wxWeakRef<CalendarPropertyGrid> property_grid_;
  CalendarConfig calendar_config_;
  sigslot::signal<const CalendarConfig&> signal_calendar_config_;
};
#endif  // CALENDAR_PANEL_HPP
