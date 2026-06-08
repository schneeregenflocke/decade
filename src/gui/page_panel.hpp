#ifndef PAGE_PANEL_HPP
#define PAGE_PANEL_HPP

#include <wx/printdlg.h>
#include <wx/spinctrl.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <memory>
#include <sigslot/signal.hpp>

#include "../packages/page_setup_config.hpp"

class PageSetupPanel : public wxPanel {
 public:
  explicit PageSetupPanel(wxWindow* parent)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTAB_TRAVERSAL, wxPanelNameStr),
        id_page_width_(wxWindow::NewControlId()),
        id_page_height_(wxWindow::NewControlId()) {
    wxPrintData print_data;
    print_data.SetOrientation(wxPrintOrientation::wxLANDSCAPE);

    dialog_data_.SetPrintData(print_data);
    dialog_data_.SetPaperId(wxPaperSize::wxPAPER_A4);
    dialog_data_.CalculatePaperSizeFromId();

    constexpr int kPageMarginMm = 15;
    constexpr int kLabelMinWidthPx = 75;
    constexpr double kMaxPageSizeMm = 2000.0;
    constexpr int kSizerBorderPx = 5;

    dialog_data_.SetMarginTopLeft(wxPoint(kPageMarginMm, kPageMarginMm));
    dialog_data_.SetMarginBottomRight(wxPoint(kPageMarginMm, kPageMarginMm));

    wxBoxSizer* vertical_sizer =
        std::make_unique<wxBoxSizer>(wxVERTICAL).release();

    wxBoxSizer* horizontal_sizer0 =
        std::make_unique<wxBoxSizer>(wxHORIZONTAL).release();

    wxButton* page_setup_dialog_button =
        std::make_unique<wxButton>(this, wxID_ANY, L"Page Setup...").release();

    wxStaticText* page_width_label =
        std::make_unique<wxStaticText>(this, wxID_ANY, L"Width").release();
    page_width_label->SetMinSize(wxSize(kLabelMinWidthPx, -1));

    wxStaticText* page_height_label =
        std::make_unique<wxStaticText>(this, wxID_ANY, L"Height").release();
    page_height_label->SetMinSize(wxSize(kLabelMinWidthPx, -1));

    page_width_spinctrl_ =
        std::make_unique<wxSpinCtrlDouble>(this, id_page_width_).release();
    page_width_spinctrl_->SetRange(.0, kMaxPageSizeMm);

    page_height_spinctrl_ =
        std::make_unique<wxSpinCtrlDouble>(this, id_page_height_).release();
    page_height_spinctrl_->SetRange(.0, kMaxPageSizeMm);

    horizontal_sizer0->Add(page_setup_dialog_button, 1, wxEXPAND | wxALL,
                           kSizerBorderPx);  //| wxALIGN_CENTER_VERTICAL

    wxBoxSizer* horizontal_sizer1 =
        std::make_unique<wxBoxSizer>(wxHORIZONTAL).release();
    horizontal_sizer1->Add(page_width_label, 0, wxALL | wxALIGN_CENTER_VERTICAL,
                           kSizerBorderPx);
    horizontal_sizer1->Add(page_width_spinctrl_, 1, wxEXPAND | wxALL,
                           kSizerBorderPx);

    wxBoxSizer* horizontal_sizer2 =
        std::make_unique<wxBoxSizer>(wxHORIZONTAL).release();
    horizontal_sizer2->Add(page_height_label, 0,
                           wxALL | wxALIGN_CENTER_VERTICAL, kSizerBorderPx);
    horizontal_sizer2->Add(page_height_spinctrl_, 1, wxEXPAND | wxALL,
                           kSizerBorderPx);

    vertical_sizer->Add(horizontal_sizer0, 0, wxEXPAND);
    vertical_sizer->Add(horizontal_sizer1, 0, wxEXPAND);
    vertical_sizer->Add(horizontal_sizer2, 0, wxEXPAND);

    SetSizer(vertical_sizer);

    //////////////////////////////////////////////////

    Bind(wxEVT_BUTTON, &PageSetupPanel::CallbackButtonClicked, this);
    Bind(wxEVT_SPINCTRLDOUBLE, &PageSetupPanel::CallbackSpinControl, this,
         id_page_width_);
    Bind(wxEVT_SPINCTRLDOUBLE, &PageSetupPanel::CallbackSpinControl, this,
         id_page_height_);
  }

  void SendPageSetup() {
    PageSetupConfig page_setup_config;

    auto dialog_width =
        static_cast<float>(dialog_data_.GetPaperSize().GetWidth());
    auto dialog_height =
        static_cast<float>(dialog_data_.GetPaperSize().GetHeight());

    auto print_data = dialog_data_.GetPrintData();
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

    page_setup_config.margins[0] =
        static_cast<float>(dialog_data_.GetMarginTopLeft().x);
    page_setup_config.margins[1] =
        static_cast<float>(dialog_data_.GetMarginBottomRight().y);
    page_setup_config.margins[2] =
        static_cast<float>(dialog_data_.GetMarginBottomRight().x);
    page_setup_config.margins[3] =
        static_cast<float>(dialog_data_.GetMarginTopLeft().y);

    signal_page_setup_config_(page_setup_config);
  }

  void ReceivePageSetup(const PageSetupConfig& page_setup_config) {
    wxPrintData print_data = dialog_data_.GetPrintData();
    print_data.SetOrientation(
        static_cast<wxPrintOrientation>(page_setup_config.orientation));
    dialog_data_.SetPrintData(print_data);

    if (print_data.GetOrientation() == wxPORTRAIT) {
      dialog_data_.SetPaperSize(
          wxSize(static_cast<int>(page_setup_config.size[0]),
                 static_cast<int>(page_setup_config.size[1])));
    }
    if (print_data.GetOrientation() == wxLANDSCAPE) {
      dialog_data_.SetPaperSize(
          wxSize(static_cast<int>(page_setup_config.size[1]),
                 static_cast<int>(page_setup_config.size[0])));
    }

    dialog_data_.CalculateIdFromPaperSize();

    dialog_data_.SetMarginTopLeft(
        wxPoint(static_cast<int>(page_setup_config.margins[0]),
                static_cast<int>(page_setup_config.margins[3])));
    dialog_data_.SetMarginBottomRight(
        wxPoint(static_cast<int>(page_setup_config.margins[2]),
                static_cast<int>(page_setup_config.margins[1])));

    UpdateSpinControl();
  }

  void SendDefaultValues() { SendPageSetup(); }

  [[nodiscard]] auto& SignalPageSetupConfig() {
    return signal_page_setup_config_;
  }

 private:
  sigslot::signal<const PageSetupConfig&> signal_page_setup_config_;
  void UpdateSpinControl() {
    const wxPrintData print_data = dialog_data_.GetPrintData();
    const wxSize paper_size = dialog_data_.GetPaperSize();

    if (print_data.GetOrientation() == wxPORTRAIT) {
      page_width_spinctrl_->SetValue(paper_size.x);
      page_height_spinctrl_->SetValue(paper_size.y);
    }
    if (print_data.GetOrientation() == wxLANDSCAPE) {
      page_width_spinctrl_->SetValue(paper_size.y);
      page_height_spinctrl_->SetValue(paper_size.x);
    }
  }

  void CallbackButtonClicked(wxCommandEvent& /*event*/) {
    wxPageSetupDialog page_setup_dialog(nullptr, &dialog_data_);

    if (page_setup_dialog.ShowModal() == wxID_OK) {
      dialog_data_ = page_setup_dialog.GetPageSetupDialogData();
      UpdateSpinControl();
      SendPageSetup();
    }
  }
  void CallbackCheckboxClicked(wxCommandEvent& event) {
    page_width_spinctrl_->Enable(event.IsChecked());
    page_height_spinctrl_->Enable(event.IsChecked());
  }
  void CallbackSpinControl(wxSpinDoubleEvent& /*event*/) {
    wxSize paper_size;
    paper_size.x = static_cast<int>(page_width_spinctrl_->GetValue());
    paper_size.y = static_cast<int>(page_height_spinctrl_->GetValue());

    const wxPrintData print_data = dialog_data_.GetPrintData();

    if (print_data.GetOrientation() == wxPORTRAIT) {
      dialog_data_.SetPaperSize(wxSize(paper_size.x, paper_size.y));
    }
    if (print_data.GetOrientation() == wxLANDSCAPE) {
      dialog_data_.SetPaperSize(wxSize(paper_size.y, paper_size.x));
    }

    dialog_data_.CalculateIdFromPaperSize();
    SendPageSetup();
  }

  const int id_page_width_;
  const int id_page_height_;

  wxPageSetupDialogData dialog_data_;

  wxWeakRef<wxSpinCtrlDouble> page_width_spinctrl_;
  wxWeakRef<wxSpinCtrlDouble> page_height_spinctrl_;
};
#endif  // PAGE_PANEL_HPP
