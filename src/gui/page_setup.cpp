/*
Decade
Copyright (c) 2019-2020 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/gpl-3.0.txt>.
*/


#include "page_setup.h"

PageSetupPanel::PageSetupPanel(wxWindow* parent) : 
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxPanelNameStr), 
	ID_PAGE_SETUP(wxNewId()), 
	ID_CUSTOM_SIZE(wxNewId()), 
	ID_PAGE_WIDTH(wxNewId()), 
	ID_PAGE_HEIGHT(wxNewId())
{

	wxPrintData print_data;
	print_data.SetOrientation(wxPrintOrientation::wxLANDSCAPE);

	page_setup_dialog_data = std::make_unique<wxPageSetupDialogData>(print_data);

	page_setup_dialog_data->SetPaperId(wxPaperSize::wxPAPER_A4);
	page_setup_dialog_data->CalculatePaperSizeFromId();

	page_setup_dialog_data->SetMarginTopLeft(wxPoint(10, 10));
	page_setup_dialog_data->SetMarginBottomRight(wxPoint(10, 10));

	//////////////////////////////////////////////////

	wxStaticBoxSizer* static_box_sizer = new wxStaticBoxSizer(wxVERTICAL, this, L"Paper Format"); // (this, wxID_ANY, L"Paper Format", wxDefaultPosition, wxDefaultSize, 0L, wxStaticBoxNameStr);

	wxBoxSizer* horizontal_sizer0 = new wxBoxSizer(wxHORIZONTAL);
	
	wxButton* page_setup_dialog_button = new wxButton(this, ID_PAGE_SETUP, L"Page Setup...");
	
	wxStaticText* page_width_label = new wxStaticText(this, wxID_ANY, L"Width");
	page_width_label->SetMinSize(wxSize(75, -1));

	wxStaticText* page_height_label = new wxStaticText(this, wxID_ANY, L"Height");
	page_height_label->SetMinSize(wxSize(75, -1));

	page_width_spinctrl = new wxSpinCtrlDouble(this, ID_PAGE_WIDTH);
	page_width_spinctrl->SetRange(.0, 2000.);
	
	page_height_spinctrl = new wxSpinCtrlDouble(this, ID_PAGE_HEIGHT);
	page_height_spinctrl->SetRange(.0, 2000.);

	horizontal_sizer0->Add(page_setup_dialog_button, 1, wxEXPAND | wxALL, 5); //| wxALIGN_CENTER_VERTICAL

	wxBoxSizer* horizontal_sizer1 = new wxBoxSizer(wxHORIZONTAL);
	horizontal_sizer1->Add(page_width_label, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	horizontal_sizer1->Add(page_width_spinctrl, 1, wxEXPAND | wxALL, 5);
	
	wxBoxSizer* horizontal_sizer2 = new wxBoxSizer(wxHORIZONTAL);
	horizontal_sizer2->Add(page_height_label, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	horizontal_sizer2->Add(page_height_spinctrl, 1, wxEXPAND | wxALL, 5);

	
	static_box_sizer->Add(horizontal_sizer0, 0, wxEXPAND);
	static_box_sizer->Add(horizontal_sizer1, 0, wxEXPAND);
	static_box_sizer->Add(horizontal_sizer2, 0, wxEXPAND);

	wxBoxSizer* vertical_sizer = new wxBoxSizer(wxVERTICAL);
	//vertical_sizer->Add(static_box_sizer, 0, wxEXPAND | wxALL, 5);
	vertical_sizer->Add(static_box_sizer, 0, wxEXPAND);

	SetSizer(vertical_sizer);

	//////////////////////////////////////////////////

	Bind(wxEVT_BUTTON, &PageSetupPanel::OnButtonClicked, this);

	Bind(wxEVT_SPINCTRLDOUBLE, &PageSetupPanel::OnSpinControl, this, ID_PAGE_WIDTH);
	Bind(wxEVT_SPINCTRLDOUBLE, &PageSetupPanel::OnSpinControl, this, ID_PAGE_HEIGHT);

	ScanPageSetupDialogData(*page_setup_dialog_data);
}

void PageSetupPanel::SendDefaultValues()
{
	ScanPageSetupDialogData(*page_setup_dialog_data);
}

std::array<float, 2> PageSetupPanel::GetPageSize()
{
	return page_size;
}

std::array<float, 4> PageSetupPanel::GetPageMargins()
{
	return page_margins;
}

void PageSetupPanel::SetPageSize(const std::array<float, 2>& value)
{
	page_size = value;
	ScanData();
}

void PageSetupPanel::SetPageMargins(const std::array<float, 4>& value)
{
	page_margins = value;
	ScanData();
}

void PageSetupPanel::ScanPageSetupDialogData(const wxPageSetupDialogData& dialog_data)
{
	auto print_data = dialog_data.GetPrintData();

	auto dialog_width = static_cast<float>(dialog_data.GetPaperSize().GetWidth());
	auto dialog_height = static_cast<float>(dialog_data.GetPaperSize().GetHeight());

	if (print_data.GetOrientation() == wxPORTRAIT)
	{
		page_size[0] = dialog_width;
		page_size[1] = dialog_height;
	}
	if (print_data.GetOrientation() == wxLANDSCAPE)
	{
		page_size[0] = dialog_height;
		page_size[1] = dialog_width;
	}

	page_width_spinctrl->SetValue(page_size[0]);
	page_height_spinctrl->SetValue(page_size[1]);

	page_margins[0] = dialog_data.GetMarginTopLeft().x;
	page_margins[1] = dialog_data.GetMarginBottomRight().y;
	page_margins[2] = dialog_data.GetMarginBottomRight().x;
	page_margins[3] = dialog_data.GetMarginTopLeft().y;

	signal_page_size(page_size[0], page_size[1]);
	signal_page_margins(page_margins[0], page_margins[1], page_margins[2], page_margins[3]);
}

void PageSetupPanel::ScanData()
{
	auto print_data = page_setup_dialog_data->GetPrintData();

	if (print_data.GetOrientation() == wxPORTRAIT)
	{
		page_setup_dialog_data->SetPaperSize(wxSize(page_size[0], page_size[1]));
	}
	if (print_data.GetOrientation() == wxLANDSCAPE)
	{
		page_setup_dialog_data->SetPaperSize(wxSize(page_size[1], page_size[0]));
	}

	page_setup_dialog_data->CalculateIdFromPaperSize();

	page_setup_dialog_data->SetMarginTopLeft(wxPoint(page_margins[0], page_margins[3]));
	page_setup_dialog_data->SetMarginBottomRight(wxPoint(page_margins[2], page_margins[1]));

	signal_page_size(page_size[0], page_size[1]);
	signal_page_margins(page_margins[0], page_margins[1], page_margins[2], page_margins[3]);
}

void PageSetupPanel::OnButtonClicked(wxCommandEvent& event)
{
	if (event.GetId() == ID_PAGE_SETUP)
	{
		wxPageSetupDialog page_setup_dialog(nullptr, page_setup_dialog_data.get());

		if (page_setup_dialog.ShowModal() == wxID_OK)
		{
			*page_setup_dialog_data = page_setup_dialog.GetPageSetupDialogData();
			
			ScanPageSetupDialogData(page_setup_dialog.GetPageSetupDialogData());	
		}		
	}
}

void PageSetupPanel::OnCheckboxClicked(wxCommandEvent& event)
{
	if (event.GetId() == ID_CUSTOM_SIZE)
	{
		page_width_spinctrl->Enable(event.IsChecked());
		page_height_spinctrl->Enable(event.IsChecked());
	}
}

void PageSetupPanel::OnSpinControl(wxSpinDoubleEvent& event)
{
	page_size[0] = static_cast<float>(page_width_spinctrl->GetValue());
	page_size[1] = static_cast<float>(page_height_spinctrl->GetValue());

	ScanData();
}
