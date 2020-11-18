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


#pragma once


#include "../page_config.h"



#ifdef WX_PRECOMP
	#include <wx/wxprec.h>
#else 
	#include <wx/wx.h>
#endif

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>

#include <wx/print.h>
#include <wx/printdlg.h>
#include <wx/cmndata.h>
#include <wx/paper.h>

#include <wx/choice.h>
#include <wx/spinctrl.h>

#include <wx/statbox.h>
#include <wx/sizer.h>


#include <sigslot/signal.hpp>



class PageSetupPanel : public wxPanel
{
public:

	PageSetupPanel(wxWindow* parent);

	void SendPageSetup();
	void ReceivePageSetup(const PageSetupConfig& page_setup_config);

	void SendDefaultValues();

	sigslot::signal<const PageSetupConfig&> signal_page_setup_config;

private:

	void UpdateSpinControl()
	{
		const wxPrintData print_data = dialog_data.GetPrintData();
		const wxSize paper_size = dialog_data.GetPaperSize();

		if (print_data.GetOrientation() == wxPORTRAIT)
		{
			page_width_spinctrl->SetValue(paper_size.x);
			page_height_spinctrl->SetValue(paper_size.y);
		}
		if (print_data.GetOrientation() == wxLANDSCAPE)
		{
			page_width_spinctrl->SetValue(paper_size.y);
			page_height_spinctrl->SetValue(paper_size.x);
		}
	}

	void OnButtonClicked(wxCommandEvent& event);
	void OnCheckboxClicked(wxCommandEvent& event);
	void OnSpinControl(wxSpinDoubleEvent& event);

	const int ID_PAGE_WIDTH;
	const int ID_PAGE_HEIGHT;

	wxPageSetupDialogData dialog_data;

	wxSpinCtrlDouble* page_width_spinctrl;
	wxSpinCtrlDouble* page_height_spinctrl;
};
