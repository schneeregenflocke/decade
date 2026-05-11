#ifndef HOME_TITAN99_CODE_DECADE_SRC_GUI_CALENDAR_PANEL_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GUI_CALENDAR_PANEL_HPP

#include <wx/wx.h>

#include <wx/propgrid/propgrid.h>
#include <wx/weakref.h>

#include "../packages/calendar_config.hpp"
#include <memory>
#include <sigslot/signal.hpp>
#include <vector>

class PropertyGridPanel : public wxPropertyGrid {
public:
  PropertyGridPanel(wxWindow *parent)
      : wxPropertyGrid(parent, -1, wxDefaultPosition, wxDefaultSize, wxPG_SPLITTER_AUTO_CENTER),
        box_sizer(std::make_unique<wxBoxSizer>(wxHORIZONTAL).release())
  {
    auto sizer_flags = wxSizerFlags().Proportion(1).Expand();
    parent->GetSizer()->Add(box_sizer, sizer_flags);
    box_sizer->Add(this, sizer_flags);

    SetVerticalSpacing(2);

    Append(std::make_unique<wxPropertyCategory>("Calendar Span", wxPG_LABEL).release());

    gui_auto_span = std::make_unique<wxBoolProperty>("Auto", wxPG_LABEL, false).release();
    gui_lower_limit = std::make_unique<wxIntProperty>("Lower Limit", wxPG_LABEL, 0).release();
    gui_upper_limit = std::make_unique<wxIntProperty>("Upper Limit", wxPG_LABEL, 0).release();

    Append(gui_auto_span);
    Append(gui_lower_limit);
    Append(gui_upper_limit);
    // DisableProperty(gui_lower_limit);
    // DisableProperty(gui_upper_limit);

    Append(std::make_unique<wxPropertyCategory>("Row Spacing Proportions", wxPG_LABEL).release());

    gui_number_spacings =
        std::make_unique<wxIntProperty>("Number Spacings", wxPG_LABEL, 0).release();
    Append(gui_number_spacings);
    DisableProperty(gui_number_spacings);

    RefreshPropertyGrid();
  }

  void RefreshPropertyGrid()
  {
    bool auto_span = GetPropertyValue(gui_auto_span).GetBool();

    if (auto_span) {
      DisableProperty(gui_lower_limit);
      DisableProperty(gui_upper_limit);
    }
    if (!auto_span) {
      EnableProperty(gui_lower_limit);
      EnableProperty(gui_upper_limit);
    }

    const auto number_spacings =
        static_cast<size_t>(GetPropertyValue(gui_number_spacings).GetInteger());

    if (number_spacings > gui_spacings_array.size()) {
      for (size_t index = gui_spacings_array.size(); index < number_spacings; ++index) {
        int index_parity = index % 2;
        std::string label_number_postfix = std::to_string((index - index_parity) / 2 + 1);
        std::string label;

        if (index_parity == 0) {
          label = std::string("Gap ") + label_number_postfix;
        }
        if (index_parity == 1) {
          label = std::string("Subrow ") + label_number_postfix;
        }

        gui_spacings_array.push_back(
            std::make_unique<wxFloatProperty>(label, wxPG_LABEL, 10.0).release());
        Append(gui_spacings_array[index]);
      }
    }

    if (number_spacings < gui_spacings_array.size()) {
      for (size_t index = number_spacings; index < gui_spacings_array.size();) {
        DeleteProperty(gui_spacings_array[index]);
        gui_spacings_array.pop_back();
      }
    }
  }

  wxBoxSizer *box_sizer;

  wxBoolProperty *gui_auto_span;
  wxIntProperty *gui_lower_limit;
  wxIntProperty *gui_upper_limit;
  wxIntProperty *gui_number_spacings;
  std::vector<wxFloatProperty *> gui_spacings_array;
};

class CalendarSetupPanel {
public:
  CalendarSetupPanel(wxWindow *parent) : wx_panel(nullptr), property_grid(nullptr)
  {
    wx_panel = std::make_unique<wxPanel>(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                         wxTAB_TRAVERSAL, wxPanelNameStr)
                   .release();

    wxBoxSizer *vertical_sizer = std::make_unique<wxBoxSizer>(wxVERTICAL).release();
    wx_panel->SetSizer(vertical_sizer);

    property_grid = std::make_unique<PropertyGridPanel>(wx_panel.get()).release();

    wx_panel->Bind(wxEVT_PG_CHANGED, &CalendarSetupPanel::CallbackPropertyGridChanging, this);

    UpdatePropertyGrid();
  }

  wxPanel *PanelPtr() { return wx_panel.get(); }

  void ReceiveCalendarConfigStorage(const CalendarConfigStorage &incoming_calendar_config_storage)
  {
    calendar_config_storage = incoming_calendar_config_storage;
    UpdatePropertyGrid();
  }

  void CallbackPropertyGridChanging(wxPropertyGridEvent & /*event*/)
  {
    property_grid->RefreshPropertyGrid();

    long number_subrows =
        property_grid->GetPropertyValue(property_grid->gui_number_spacings).GetInteger();
    auto &spacing_proportions = calendar_config_storage.MutableSpacingProportions();
    spacing_proportions.resize(static_cast<size_t>(number_subrows));

    for (size_t index = 0; index < static_cast<size_t>(number_subrows); ++index) {
      spacing_proportions[index] = static_cast<float>(
          property_grid->GetPropertyValue(property_grid->gui_spacings_array[index]).GetDouble());
    }

    calendar_config_storage.SetAutoCalendarSpan(
        property_grid->GetPropertyValue(property_grid->gui_auto_span).GetBool());

    long lower_limit = property_grid->GetPropertyValue(property_grid->gui_lower_limit).GetInteger();
    long upper_limit = property_grid->GetPropertyValue(property_grid->gui_upper_limit).GetInteger();

    calendar_config_storage.SetSpan(CalendarSpan::YearSpan{static_cast<int>(lower_limit),
                                                          static_cast<int>(upper_limit)});

    signal_calendar_config_storage(calendar_config_storage);
  }

  void UpdatePropertyGrid()
  {
    property_grid->SetPropertyValue(
        property_grid->gui_number_spacings,
        static_cast<int>(calendar_config_storage.GetSpacingProportions().size()));

    property_grid->RefreshPropertyGrid();

    const auto &spacing_proportions = calendar_config_storage.GetSpacingProportions();
    for (size_t index = 0; index < spacing_proportions.size(); ++index) {
      property_grid->SetPropertyValue(property_grid->gui_spacings_array[index],
                                      spacing_proportions[index]);
    }

    property_grid->SetPropertyValue(property_grid->gui_auto_span,
                                    calendar_config_storage.IsAutoCalendarSpan());
    property_grid->SetPropertyValue(property_grid->gui_lower_limit,
                                    calendar_config_storage.GetSpanLimitsYears()[0]);
    property_grid->SetPropertyValue(property_grid->gui_upper_limit,
                                    calendar_config_storage.GetSpanLimitsYears()[1]);

    property_grid->RefreshPropertyGrid();
  }

  sigslot::signal<const CalendarConfigStorage &> &SignalCalendarConfigStorage()
  {
    return signal_calendar_config_storage;
  }

private:
  wxWeakRef<wxPanel> wx_panel;
  wxWeakRef<PropertyGridPanel> property_grid;
  CalendarConfigStorage calendar_config_storage;
  sigslot::signal<const CalendarConfigStorage &> signal_calendar_config_storage;
};
#endif // HOME_TITAN99_CODE_DECADE_SRC_GUI_CALENDAR_PANEL_HPP
