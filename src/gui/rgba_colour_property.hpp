#ifndef RGBA_COLOUR_PROPERTY_HPP
#define RGBA_COLOUR_PROPERTY_HPP

#include <wx/clrpicker.h>
#include <wx/colour.h>
#include <wx/dialog.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/props.h>
#include <wx/sizer.h>

#include "wx_owned.hpp"

// Presentation: a wxPropertyGrid property that edits an RGBA colour, including
// the alpha channel. The stock wxColourProperty only edits RGB and silently
// drops alpha, so the scene tree's Style colours used a workaround to preserve
// it. This property stores the full RGBA value and opens a colour picker with
// an alpha control on its editor button.
//
// It derives from wxEditorDialogProperty (the base for properties whose value
// is edited through a dialog, like wxFileProperty) and overrides
// DoGetEditorClass() to use the text+button editor. That avoids the
// wxWidgets class-registration macros (which require a separate translation
// unit), keeping this header self-contained per the project's header-only rule.
// The value is stored in the property's variant as a packed RGBA long.
class RgbaColourProperty : public wxEditorDialogProperty {
 public:
  RgbaColourProperty(const wxString& label, const wxString& name,
                     const wxColour& value)
      : wxEditorDialogProperty(label, name) {
    m_dlgTitle = "Select Colour";
    SetValue(wxVariant(ToStored(value)));
  }

  // The current colour (RGBA) as a wxColour, for committing the edit.
  [[nodiscard]] wxColour GetColour() const {
    return FromStored(GetValue().GetLong());
  }

  wxString ValueToString(wxVariant& value, int /*argFlags*/) const override {
    const wxColour colour = FromStored(value.GetLong());
    const int alpha_percent = ((colour.Alpha() * 100) + 127) / 255;
    return wxString::Format("#%02X%02X%02X  α %d%%", colour.Red(),
                            colour.Green(), colour.Blue(), alpha_percent);
  }

  // The colour is only edited through the dialog, so typed text is ignored
  // (returning false means "no change").
  bool StringToValue(wxVariant& /*variant*/, const wxString& /*text*/,
                     int /*argFlags*/) const override {
    return false;
  }

  // Public to match wxPGProperty's declared visibility (an override must not
  // narrow it). The text+button editor is what shows the dialog button.
  const wxPGEditor* DoGetEditorClass() const override {
    return wxPGEditor_TextCtrlAndButton;
  }

 protected:
  // Opens a modal colour picker (with the alpha control enabled) seeded with
  // the current value; on OK writes the picked RGBA back into `value`.
  bool DisplayEditorDialog(wxPropertyGrid* pg, wxVariant& value) override {
    const wxColour current = FromStored(value.GetLong());

    wxDialog dialog(pg, wxID_ANY, m_dlgTitle);
    auto* picker = MakeOwned<wxColourPickerCtrl>(
        &dialog, wxID_ANY, current, wxDefaultPosition, wxDefaultSize,
        wxCLRP_SHOW_ALPHA);

    constexpr int kBorderPx = 10;
    auto* sizer = MakeOwned<wxBoxSizer>(wxVERTICAL);
    sizer->Add(picker, 0, wxALL | wxEXPAND, kBorderPx);
    sizer->Add(dialog.CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0,
               wxALL | wxEXPAND, kBorderPx);
    dialog.SetSizerAndFit(sizer);

    if (dialog.ShowModal() != wxID_OK) {
      return false;
    }
    value = ToStored(picker->GetColour());
    return true;
  }

 private:
  // The colour is kept in the variant as a packed RGBA value (wxColour's own
  // representation), so no variant-data type registration is needed.
  static long ToStored(const wxColour& colour) {
    return static_cast<long>(colour.GetRGBA());
  }
  static wxColour FromStored(long stored) {
    wxColour colour;
    colour.SetRGBA(static_cast<wxUint32>(stored));
    return colour;
  }
};

#endif  // RGBA_COLOUR_PROPERTY_HPP
