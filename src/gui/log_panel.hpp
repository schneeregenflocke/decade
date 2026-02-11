#ifndef HOME_TITAN99_CODE_DECADE_SRC_GUI_LOG_PANEL_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GUI_LOG_PANEL_HPP

#include <memory>
#include <wx/weakref.h>
#include <wx/wx.h>

class LogPanel {
public:
  LogPanel(wxWindow *parent)
      : wx_panel(std::make_unique<wxPanel>(parent, wxID_ANY).release())
  {
    wxFont log_font = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, wxString("Consolas"));

    text_control =
        std::make_unique<wxTextCtrl>(wx_panel.get(), wxID_ANY, wxEmptyString, wxDefaultPosition,
                                     wxDefaultSize,
                                     wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxHSCROLL)
            .release();
    text_control->SetBackgroundColour(*wxBLACK);
    text_control->SetDefaultStyle(wxTextAttr(*wxWHITE, *wxBLACK, log_font));

    wxBoxSizer *sizer = std::make_unique<wxBoxSizer>(wxVERTICAL).release();
    sizer->Add(text_control, 1, wxEXPAND, 5);
    wx_panel->SetSizer(sizer);
  }

  wxPanel *PanelPtr() const { return wx_panel.get(); }

  wxTextCtrl *GetTextCtrlPtr() const { return text_control.get(); }

private:
  wxWeakRef<wxPanel> wx_panel;
  wxWeakRef<wxTextCtrl> text_control;
};
#endif // HOME_TITAN99_CODE_DECADE_SRC_GUI_LOG_PANEL_HPP
