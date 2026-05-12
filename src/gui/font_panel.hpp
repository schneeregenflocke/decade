#ifndef HOME_TITAN99_CODE_DECADE_SRC_GUI_FONT_PANEL_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GUI_FONT_PANEL_HPP

#include <fontconfig/fontconfig.h>
#include <wx/fontpicker.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <map>
#include <memory>
#include <sigslot/signal.hpp>
#include <string>

class FontPanel : public wxPanel {
 public:
  FontPanel(wxWindow* parent)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTAB_TRAVERSAL, wxPanelNameStr) {
    wxFont normal_font = *wxNORMAL_FONT;
    // auto normal_font_name = normal_font.GetFaceName();

    wx_font_picker = std::make_unique<wxFontPickerCtrl>(
                         this, wxID_ANY, normal_font, wxDefaultPosition,
                         wxDefaultSize, wxFNTP_FONTDESC_AS_LABEL)
                         .release();

    wxBoxSizer* horizontal_sizer =
        std::make_unique<wxBoxSizer>(wxHORIZONTAL).release();
    horizontal_sizer->Add(wx_font_picker, 1, wxALL | wxEXPAND, 5);
    wxBoxSizer* vertical_sizer =
        std::make_unique<wxBoxSizer>(wxVERTICAL).release();
    vertical_sizer->Add(horizontal_sizer, 0, wxEXPAND);
    SetSizer(vertical_sizer);

    Bind(wxEVT_FONTPICKER_CHANGED, &FontPanel::CallbackFontChanged, this);
    wx_font = wx_font_picker->GetFont();

    initConvertWxFontWeightToFcWeight();
    initConvertWxFontStyleToFcSlant();

    ProcessFontData();
  }

  const std::string& GetFontFilePath() const { return font_filepath; }

  // sigslot::signal<const std::vector<unsigned char>&> signal_font_file_path;
  sigslot::signal<const std::string&> signal_font_filepath;

 private:
  void initConvertWxFontWeightToFcWeight() {
    fontWeightMap = {{wxFONTWEIGHT_THIN, FC_WEIGHT_THIN},
                     {wxFONTWEIGHT_EXTRALIGHT, FC_WEIGHT_EXTRALIGHT},
                     //{wxFONTWEIGHT_EXTRALIGHT, FC_WEIGHT_ULTRALIGHT},
                     {wxFONTWEIGHT_LIGHT, FC_WEIGHT_LIGHT},
                     //{wxFONTWEIGHT_LIGHT, FC_WEIGHT_DEMILIGHT},
                     //{wxFONTWEIGHT_LIGHT, FC_WEIGHT_SEMILIGHT},
                     //{wxFONTWEIGHT_NORMAL, FC_WEIGHT_BOOK},
                     //{wxFONTWEIGHT_NORMAL, FC_WEIGHT_REGULAR},
                     {wxFONTWEIGHT_NORMAL, FC_WEIGHT_NORMAL},
                     {wxFONTWEIGHT_MEDIUM, FC_WEIGHT_MEDIUM},
                     {wxFONTWEIGHT_SEMIBOLD, FC_WEIGHT_DEMIBOLD},
                     //{wxFONTWEIGHT_SEMIBOLD, FC_WEIGHT_SEMIBOLD},
                     {wxFONTWEIGHT_BOLD, FC_WEIGHT_BOLD},
                     {wxFONTWEIGHT_EXTRABOLD, FC_WEIGHT_EXTRABOLD},
                     //{wxFONTWEIGHT_EXTRABOLD, FC_WEIGHT_ULTRABOLD},
                     {wxFONTWEIGHT_HEAVY, FC_WEIGHT_BLACK},
                     //{wxFONTWEIGHT_HEAVY, FC_WEIGHT_HEAVY},
                     {wxFONTWEIGHT_EXTRAHEAVY, FC_WEIGHT_EXTRABLACK},
                     //{wxFONTWEIGHT_EXTRAHEAVY, FC_WEIGHT_ULTRABLACK},
                     {wxFONTWEIGHT_MAX, FC_WEIGHT_EXTRABLACK}};
  }

  int convertWxFontWeightToFcWeight(const wxFontWeight wx_font_weight) const {
    if (wx_font_weight == wxFONTWEIGHT_INVALID) {
      throw std::runtime_error("wxFONTWEIGHT_INVALID");
    }

    int fc_weight = -1;
    auto it = fontWeightMap.find(wx_font_weight);
    if (it != fontWeightMap.end()) {
      fc_weight = it->second;
    } else {
      throw std::runtime_error("convertWxFontWeightToFcWeight");
    }

    std::cout << "convertWxFontWeightToFcWeight: " << wx_font_weight
              << " to: " << fc_weight << '\n';

    return fc_weight;
  }

  void initConvertWxFontStyleToFcSlant() {
    fontStyleMap = {{wxFONTSTYLE_NORMAL, FC_SLANT_ROMAN},
                    {wxFONTSTYLE_ITALIC, FC_SLANT_ITALIC},
                    {wxFONTSTYLE_SLANT, FC_SLANT_OBLIQUE},
                    {wxFONTSTYLE_MAX, FC_SLANT_OBLIQUE}};
  }

  int convertWxFontStyleToFcSlant(const wxFontStyle wx_font_style) const {
    int fc_slant = -1;
    auto it = fontStyleMap.find(wx_font_style);
    if (it != fontStyleMap.end()) {
      fc_slant = it->second;
    } else {
      throw std::runtime_error("convertWxFontStyleToFcSlant");
    }

    std::cout << "convertWxFontStyleToFcSlant: " << wx_font_style
              << " to: " << fc_slant << '\n';

    return fc_slant;
  }

  void ProcessFontData() {
    wxString face_name = wx_font.GetFaceName();
    int point_size = wx_font.GetPointSize();
    wxFontWeight font_weight = wx_font.GetWeight();
    wxFontStyle font_style = wx_font.GetStyle();

    std::cout << "face_name: " << face_name << "\tpoint_size: " << point_size
              << "\tfont_style: " << font_style
              << "\t font_weight: " << font_weight << '\n';

    FcConfig* ft_config = FcInitLoadConfigAndFonts();

    FcPattern* pattern = FcPatternCreate();
    FcPatternAddString(
        pattern, FC_FAMILY,
        reinterpret_cast<const FcChar8*>(face_name.utf8_str().data()));

    FcPatternAddInteger(pattern, FC_SIZE, point_size);

    int fc_weight = convertWxFontWeightToFcWeight(font_weight);
    FcPatternAddInteger(pattern, FC_WEIGHT, fc_weight);

    int fc_slant = convertWxFontStyleToFcSlant(font_style);
    FcPatternAddInteger(pattern, FC_SLANT, fc_slant);

    FcResult result;
    FcPattern* match = FcFontMatch(ft_config, pattern, &result);
    FcChar8* name_unparse = FcNameUnparse(match);
    /*int name_unparse_len = strlen(reinterpret_cast<const
    char*>(name_unparse)); std::string name_unparse_string(name_unparse,
    name_unparse + name_unparse_len); std::cout << "font unparse: " <<
    name_unparse_string << '\n';*/

    FcStrFree(name_unparse);

    // https://fontconfig.pages.freedesktop.org/fontconfig/fontconfig-devel/fcpatternget.html
    FcChar8* fc_filepath = nullptr;
    [[maybe_unused]] FcResult fc_result =
        FcPatternGetString(match, FC_FILE, 0, &fc_filepath);

    const size_t len = strlen(reinterpret_cast<const char*>(fc_filepath));
    font_filepath = std::string(fc_filepath, fc_filepath + len);
    std::cout << "font_filepath: " << font_filepath << '\n';

    FcPatternDestroy(match);
    FcPatternDestroy(pattern);
    FcConfigDestroy(ft_config);

    FcFini();
  }

  void CallbackFontChanged(wxFontPickerEvent& event) {
    try {
      wx_font = event.GetFont();
      ProcessFontData();
      signal_font_filepath(font_filepath);
      // font_data.clear();
    } catch (...) {
      std::cout << "Loading Font failed" << '\n';
    }
  }

  wxWeakRef<wxFontPickerCtrl> wx_font_picker;
  wxFont wx_font;
  // std::vector<unsigned char> font_data;
  std::string font_filepath;
  std::map<int, int> fontWeightMap;
  std::map<int, int> fontStyleMap;
};
#endif  // HOME_TITAN99_CODE_DECADE_SRC_GUI_FONT_PANEL_HPP
