/*
Decade
Copyright (c) 2019-2022 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

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
