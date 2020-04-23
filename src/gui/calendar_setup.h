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

#include "../calendar_config.h"

#ifdef WX_PRECOMP
#include <wx/wxprec.h>
#else 
#include <wx/wx.h>
#endif

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>

#include <array>
#include <vector>

#include <memory>
#include <functional>

#include <sigslot/signal.hpp>

#include <pugixml.hpp>


class CalendarSetupPanel : public wxPanel
{
public:

	CalendarSetupPanel(wxWindow* parent);

	/*template<typename T, typename U>
	void ConnectSignalCalendarConfig(T memfunptr, U objectptr);*/
	
	void SendDefaultValues();

	void SaveToXML(pugi::xml_node* node);
	void LoadFromXML(const pugi::xml_node& node);

	sigslot::signal<const CalendarConfig&> signal_calendar_config;

private:

	void SlotWeightCtrls(wxSpinDoubleEvent& event);
	void SlotSpanSpinCtrls(wxSpinEvent& event);
	void SlotAutoSpanCheckBox(wxCommandEvent& event);

	void UpdateControls();

	wxSpinCtrlDouble* weight0_spin_ctrl;
	wxSpinCtrlDouble* weight1_spin_ctrl;
	wxSpinCtrlDouble* weight2_spin_ctrl;
	wxSpinCtrlDouble* gap_factor_spin_ctrl;
	wxSpinCtrl* calendar_span_from_spin_ctrl;
	wxSpinCtrl* calendar_span_to_spin_ctrl;
	wxCheckBox* auto_span_check_box;

	CalendarConfig calendar_config;
	

	const int ID_WEIGHT0;
	const int ID_WEIGHT1;
	const int ID_WEIGHT2;
	const int ID_GAP_FACTOR;

	const int ID_SPAN_FROM;
	const int ID_SPAN_TO;

	const int ID_AUTO_SPAN;
};

/*template<typename T, typename U>
void CalendarSetupPanel::ConnectSignalCalendarConfig(T memfunptr, U objectptr)
{
	c*alendar_config_signal.connect(std::bind(memfunptr, objectptr, std::placeholders::_1));
}*/