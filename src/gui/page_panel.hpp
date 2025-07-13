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

#pragma once

#include <wx/wx.h>

#include <wx/printdlg.h>
#include <wx/spinctrl.h>

#include "../packages/page_config.hpp"
#include <sigslot/signal.hpp>

class PageSetupPanel {
public:
  PageSetupPanel(wxWindow *parent)
      : wx_panel(nullptr), ID_PAGE_WIDTH(wxWindow::NewControlId()),
        ID_PAGE_HEIGHT(wxWindow::NewControlId())
  {
    wx_panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL,
                           wxPanelNameStr);

    wxPrintData print_data;
    print_data.SetOrientation(wxPrintOrientation::wxLANDSCAPE);

    dialog_data.SetPrintData(print_data);
    dialog_data.SetPaperId(wxPaperSize::wxPAPER_A4);
    dialog_data.CalculatePaperSizeFromId();
    dialog_data.SetMarginTopLeft(wxPoint(15, 15));
    dialog_data.SetMarginBottomRight(wxPoint(15, 15));

    wxStaticBoxSizer *static_box_sizer = new wxStaticBoxSizer(
        wxVERTICAL, wx_panel,
        L"Paper Format"); // (this, wxID_ANY, L"Paper Format", wxDefaultPosition, wxDefaultSize, 0L,
                          // wxStaticBoxNameStr);

    wxBoxSizer *horizontal_sizer0 = new wxBoxSizer(wxHORIZONTAL);

    wxButton *page_setup_dialog_button = new wxButton(wx_panel, wxID_ANY, L"Page Setup...");

    wxStaticText *page_width_label = new wxStaticText(wx_panel, wxID_ANY, L"Width");
    page_width_label->SetMinSize(wxSize(75, -1));

    wxStaticText *page_height_label = new wxStaticText(wx_panel, wxID_ANY, L"Height");
    page_height_label->SetMinSize(wxSize(75, -1));

    page_width_spinctrl = new wxSpinCtrlDouble(wx_panel, ID_PAGE_WIDTH);
    page_width_spinctrl->SetRange(.0, 2000.);

    page_height_spinctrl = new wxSpinCtrlDouble(wx_panel, ID_PAGE_HEIGHT);
    page_height_spinctrl->SetRange(.0, 2000.);

    horizontal_sizer0->Add(page_setup_dialog_button, 1, wxEXPAND | wxALL,
                           5); //| wxALIGN_CENTER_VERTICAL

    wxBoxSizer *horizontal_sizer1 = new wxBoxSizer(wxHORIZONTAL);
    horizontal_sizer1->Add(page_width_label, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    horizontal_sizer1->Add(page_width_spinctrl, 1, wxEXPAND | wxALL, 5);

    wxBoxSizer *horizontal_sizer2 = new wxBoxSizer(wxHORIZONTAL);
    horizontal_sizer2->Add(page_height_label, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    horizontal_sizer2->Add(page_height_spinctrl, 1, wxEXPAND | wxALL, 5);

    static_box_sizer->Add(horizontal_sizer0, 0, wxEXPAND);
    static_box_sizer->Add(horizontal_sizer1, 0, wxEXPAND);
    static_box_sizer->Add(horizontal_sizer2, 0, wxEXPAND);

    wxBoxSizer *vertical_sizer = new wxBoxSizer(wxVERTICAL);
    // vertical_sizer->Add(static_box_sizer, 0, wxEXPAND | wxALL, 5);
    vertical_sizer->Add(static_box_sizer, 0, wxEXPAND);

    wx_panel->SetSizer(vertical_sizer);

    //////////////////////////////////////////////////

    wx_panel->Bind(wxEVT_BUTTON, &PageSetupPanel::CallbackButtonClicked, this);
    wx_panel->Bind(wxEVT_SPINCTRLDOUBLE, &PageSetupPanel::CallbackSpinControl, this, ID_PAGE_WIDTH);
    wx_panel->Bind(wxEVT_SPINCTRLDOUBLE, &PageSetupPanel::CallbackSpinControl, this,
                   ID_PAGE_HEIGHT);
  }

  wxPanel *PanelPtr() { return wx_panel; }

  void SendPageSetup()
  {
    PageSetupConfig page_setup_config;

    auto dialog_width = static_cast<float>(dialog_data.GetPaperSize().GetWidth());
    auto dialog_height = static_cast<float>(dialog_data.GetPaperSize().GetHeight());

    auto print_data = dialog_data.GetPrintData();
    if (print_data.GetOrientation() == wxPORTRAIT) {
      page_setup_config.orientation = wxPORTRAIT;
      page_setup_config.size[0] = dialog_width;
      page_setup_config.size[1] = dialog_height;
    }
    if (print_data.GetOrientation() == wxLANDSCAPE) {
      page_setup_config.orientation = wxLANDSCAPE;
      page_setup_config.size[0] = dialog_height;
      page_setup_config.size[1] = dialog_width;
    }

    page_setup_config.margins[0] = dialog_data.GetMarginTopLeft().x;
    page_setup_config.margins[1] = dialog_data.GetMarginBottomRight().y;
    page_setup_config.margins[2] = dialog_data.GetMarginBottomRight().x;
    page_setup_config.margins[3] = dialog_data.GetMarginTopLeft().y;

    signal_page_setup_config(page_setup_config);
  }

  void ReceivePageSetup(const PageSetupConfig &page_setup_config)
  {
    wxPrintData print_data = dialog_data.GetPrintData();
    print_data.SetOrientation(page_setup_config.orientation);
    dialog_data.SetPrintData(print_data);

    if (print_data.GetOrientation() == wxPORTRAIT) {
      dialog_data.SetPaperSize(wxSize(page_setup_config.size[0], page_setup_config.size[1]));
    }
    if (print_data.GetOrientation() == wxLANDSCAPE) {
      dialog_data.SetPaperSize(wxSize(page_setup_config.size[1], page_setup_config.size[0]));
    }

    dialog_data.CalculateIdFromPaperSize();

    dialog_data.SetMarginTopLeft(
        wxPoint(page_setup_config.margins[0], page_setup_config.margins[3]));
    dialog_data.SetMarginBottomRight(
        wxPoint(page_setup_config.margins[2], page_setup_config.margins[1]));

    UpdateSpinControl();
  }

  void SendDefaultValues() { SendPageSetup(); }

  sigslot::signal<const PageSetupConfig &> signal_page_setup_config;

private:
  void UpdateSpinControl()
  {
    const wxPrintData print_data = dialog_data.GetPrintData();
    const wxSize paper_size = dialog_data.GetPaperSize();

    if (print_data.GetOrientation() == wxPORTRAIT) {
      page_width_spinctrl->SetValue(paper_size.x);
      page_height_spinctrl->SetValue(paper_size.y);
    }
    if (print_data.GetOrientation() == wxLANDSCAPE) {
      page_width_spinctrl->SetValue(paper_size.y);
      page_height_spinctrl->SetValue(paper_size.x);
    }
  }

  void CallbackButtonClicked(wxCommandEvent &event)
  {
    wxPageSetupDialog page_setup_dialog(nullptr, &dialog_data);

    if (page_setup_dialog.ShowModal() == wxID_OK) {
      dialog_data = page_setup_dialog.GetPageSetupDialogData();
      UpdateSpinControl();
      SendPageSetup();
    }
  }
  void CallbackCheckboxClicked(wxCommandEvent &event)
  {
    page_width_spinctrl->Enable(event.IsChecked());
    page_height_spinctrl->Enable(event.IsChecked());
  }
  void CallbackSpinControl(wxSpinDoubleEvent &event)
  {
    wxSize paper_size;
    paper_size.x = static_cast<float>(page_width_spinctrl->GetValue());
    paper_size.y = static_cast<float>(page_height_spinctrl->GetValue());

    const wxPrintData print_data = dialog_data.GetPrintData();

    if (print_data.GetOrientation() == wxPORTRAIT) {
      dialog_data.SetPaperSize(wxSize(paper_size.x, paper_size.y));
    }
    if (print_data.GetOrientation() == wxLANDSCAPE) {
      dialog_data.SetPaperSize(wxSize(paper_size.y, paper_size.x));
    }

    dialog_data.CalculateIdFromPaperSize();
    SendPageSetup();
  }

  wxPanel *wx_panel;

  const int ID_PAGE_WIDTH;
  const int ID_PAGE_HEIGHT;

  wxPageSetupDialogData dialog_data;

  wxSpinCtrlDouble *page_width_spinctrl;
  wxSpinCtrlDouble *page_height_spinctrl;
};
