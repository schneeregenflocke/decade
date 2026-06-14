#ifndef CALENDAR_PANEL_HPP
#define CALENDAR_PANEL_HPP

#include <wx/propgrid/propgrid.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <memory>
#include <sigslot/signal.hpp>
#include <utility>
#include <vector>

#include "../packages/calendar_config.hpp"

class PropertyGridPanel : public wxPropertyGrid {
 public:
  explicit PropertyGridPanel(wxWindow* parent)
      : wxPropertyGrid(parent, -1, wxDefaultPosition, wxDefaultSize,
                       wxPG_SPLITTER_AUTO_CENTER),
        box_sizer_(std::make_unique<wxBoxSizer>(wxHORIZONTAL).release()) {
    auto sizer_flags = wxSizerFlags().Proportion(1).Expand();
    parent->GetSizer()->Add(box_sizer_, sizer_flags);
    box_sizer_->Add(this, sizer_flags);

    SetVerticalSpacing(2);

    Append(std::make_unique<wxPropertyCategory>("Calendar Span", wxPG_LABEL)
               .release());

    gui_auto_span_ =
        std::make_unique<wxBoolProperty>("Auto", wxPG_LABEL, false).release();
    gui_lower_limit_ =
        std::make_unique<wxIntProperty>("Lower Limit", wxPG_LABEL, 0).release();
    gui_upper_limit_ =
        std::make_unique<wxIntProperty>("Upper Limit", wxPG_LABEL, 0).release();

    Append(gui_auto_span_);
    Append(gui_lower_limit_);
    Append(gui_upper_limit_);

    Append(std::make_unique<wxPropertyCategory>("Row Spacing Proportions",
                                                wxPG_LABEL)
               .release());

    gui_number_spacings_ =
        std::make_unique<wxIntProperty>("Number Spacings", wxPG_LABEL, 0)
            .release();
    Append(gui_number_spacings_);
    DisableProperty(gui_number_spacings_);

    RefreshPropertyGrid();
  }

  void RefreshPropertyGrid() {
    bool const auto_span = GetPropertyValue(gui_auto_span_).GetBool();

    if (auto_span) {
      DisableProperty(gui_lower_limit_);
      DisableProperty(gui_upper_limit_);
    }
    if (!auto_span) {
      EnableProperty(gui_lower_limit_);
      EnableProperty(gui_upper_limit_);
    }

    const auto number_spacings = static_cast<size_t>(
        GetPropertyValue(gui_number_spacings_).GetInteger());

    if (number_spacings > gui_spacings_array_.size()) {
      for (size_t index = gui_spacings_array_.size(); index < number_spacings;
           ++index) {
        const std::size_t index_parity = index % 2;
        std::string const label_number_postfix =
            std::to_string(((index - index_parity) / 2) + 1);
        std::string label;

        if (index_parity == 0) {
          label = std::string("Gap ") + label_number_postfix;
        }
        if (index_parity == 1) {
          label = std::string("Subrow ") + label_number_postfix;
        }

        constexpr double kDefaultSpacing = 10.0;
        gui_spacings_array_.push_back(std::make_unique<wxFloatProperty>(
                                          label, wxPG_LABEL, kDefaultSpacing)
                                          .release());
        Append(gui_spacings_array_[index]);
      }
    }

    if (number_spacings < gui_spacings_array_.size()) {
      for (size_t const index = number_spacings;
           index < gui_spacings_array_.size();) {
        DeleteProperty(gui_spacings_array_[index]);
        gui_spacings_array_.pop_back();
      }
    }
  }

 private:
  // The setup panel drives this grid's widgets directly; granting friendship
  // keeps the widget handles private without a verbose accessor per control.
  friend class CalendarSetupPanel;

  wxBoxSizer* box_sizer_;

  wxBoolProperty* gui_auto_span_;
  wxIntProperty* gui_lower_limit_;
  wxIntProperty* gui_upper_limit_;
  wxIntProperty* gui_number_spacings_;
  std::vector<wxFloatProperty*> gui_spacings_array_;
};

class CalendarSetupPanel : public wxPanel {
 public:
  explicit CalendarSetupPanel(wxWindow* parent)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTAB_TRAVERSAL, wxPanelNameStr),
        property_grid_(nullptr) {
    wxBoxSizer* vertical_sizer =
        std::make_unique<wxBoxSizer>(wxVERTICAL).release();
    SetSizer(vertical_sizer);

    property_grid_ = std::make_unique<PropertyGridPanel>(this).release();

    Bind(wxEVT_PG_CHANGED, &CalendarSetupPanel::CallbackPropertyGridChanging,
         this);

    UpdatePropertyGrid();
  }

  void ReceiveCalendarConfig(const CalendarConfig& incoming_calendar_config) {
    calendar_config_ = incoming_calendar_config;
    UpdatePropertyGrid();
  }

  void CallbackPropertyGridChanging(wxPropertyGridEvent& /*event*/) {
    property_grid_->RefreshPropertyGrid();

    long const number_subrows =
        property_grid_->GetPropertyValue(property_grid_->gui_number_spacings_)
            .GetInteger();
    auto& spacing_proportions = calendar_config_.MutableSpacingProportions();
    spacing_proportions.resize(static_cast<size_t>(number_subrows));

    for (size_t index = 0; std::cmp_less(index, number_subrows); ++index) {
      spacing_proportions[index] = static_cast<float>(
          property_grid_
              ->GetPropertyValue(property_grid_->gui_spacings_array_[index])
              .GetDouble());
    }

    calendar_config_.SetAutoCalendarSpan(
        property_grid_->GetPropertyValue(property_grid_->gui_auto_span_)
            .GetBool());

    long const lower_limit =
        property_grid_->GetPropertyValue(property_grid_->gui_lower_limit_)
            .GetInteger();
    long const upper_limit =
        property_grid_->GetPropertyValue(property_grid_->gui_upper_limit_)
            .GetInteger();

    calendar_config_.SetSpan(
        CalendarSpan::YearSpan{.first_year = static_cast<int>(lower_limit),
                               .last_year = static_cast<int>(upper_limit)});

    signal_calendar_config_(calendar_config_);
  }

  void UpdatePropertyGrid() {
    property_grid_->SetPropertyValue(
        property_grid_->gui_number_spacings_,
        static_cast<int>(calendar_config_.GetSpacingProportions().size()));

    property_grid_->RefreshPropertyGrid();

    const auto& spacing_proportions = calendar_config_.GetSpacingProportions();
    for (size_t index = 0; index < spacing_proportions.size(); ++index) {
      property_grid_->SetPropertyValue(
          property_grid_->gui_spacings_array_[index],
          static_cast<double>(spacing_proportions[index]));
    }

    property_grid_->SetPropertyValue(property_grid_->gui_auto_span_,
                                     calendar_config_.IsAutoCalendarSpan());
    property_grid_->SetPropertyValue(property_grid_->gui_lower_limit_,
                                     calendar_config_.GetSpanLimitsYears()[0]);
    property_grid_->SetPropertyValue(property_grid_->gui_upper_limit_,
                                     calendar_config_.GetSpanLimitsYears()[1]);

    property_grid_->RefreshPropertyGrid();
  }

  sigslot::signal<const CalendarConfig&>& SignalCalendarConfig() {
    return signal_calendar_config_;
  }

 private:
  wxWeakRef<PropertyGridPanel> property_grid_;
  CalendarConfig calendar_config_;
  sigslot::signal<const CalendarConfig&> signal_calendar_config_;
};
#endif  // CALENDAR_PANEL_HPP
