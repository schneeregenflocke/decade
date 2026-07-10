#ifndef FONT_PANEL_HPP
#define FONT_PANEL_HPP

#include <fontconfig/fontconfig.h>
#include <wx/fontpicker.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <map>
#include <memory>
#include <sigslot/signal.hpp>
#include <string>

#include "../graphics/debug_log.hpp"
#include "wx_owned.hpp"

class FontPanel : public wxPanel {
 public:
  explicit FontPanel(wxWindow* parent)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTAB_TRAVERSAL, wxPanelNameStr),
        fc_config_(FcInitLoadConfigAndFonts()) {
    wxFont const normal_font = *wxNORMAL_FONT;

    wx_font_picker_ = MakeOwned<wxFontPickerCtrl>(
        this, wxID_ANY, normal_font, wxDefaultPosition, wxDefaultSize,
        wxFNTP_FONTDESC_AS_LABEL);

    constexpr int kSizerBorderPx = 5;
    auto* horizontal_sizer = MakeOwned<wxBoxSizer>(wxHORIZONTAL);
    horizontal_sizer->Add(wx_font_picker_, 1, wxALL | wxALIGN_CENTER_VERTICAL,
                          kSizerBorderPx);
    auto* vertical_sizer = MakeOwned<wxBoxSizer>(wxVERTICAL);
    vertical_sizer->Add(horizontal_sizer, 0, wxEXPAND);
    SetSizer(vertical_sizer);

    Bind(wxEVT_FONTPICKER_CHANGED, &FontPanel::CallbackFontChanged, this);
    wx_font_ = wx_font_picker_->GetFont();

    initConvertWxFontWeightToFcWeight();
    initConvertWxFontStyleToFcSlant();

    ProcessFontData();
  }

  const std::string& GetFontFilePath() const { return font_filepath_; }

  [[nodiscard]] auto& SignalFontFilepath() { return signal_font_filepath_; }

 private:
  // Owns the fontconfig configuration for this panel's lifetime. Loading it
  // once (instead of FcInitLoadConfigAndFonts()/FcFini() on every font change)
  // avoids repeated, expensive global re-initialisation of fontconfig. RAII
  // keeps the class at Rule of Zero.
  struct FcConfigDeleter {
    void operator()(FcConfig* config) const { FcConfigDestroy(config); }
  };

  std::unique_ptr<FcConfig, FcConfigDeleter> fc_config_;
  sigslot::signal<const std::string&> signal_font_filepath_;
  void initConvertWxFontWeightToFcWeight() {
    font_weight_map_ = {{wxFONTWEIGHT_THIN, FC_WEIGHT_THIN},
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
    auto it = font_weight_map_.find(wx_font_weight);
    if (it != font_weight_map_.end()) {
      fc_weight = it->second;
    } else {
      throw std::runtime_error("convertWxFontWeightToFcWeight");
    }

    if (decade_debug::LogEnabled()) {
      std::cout << "convertWxFontWeightToFcWeight: " << wx_font_weight
                << " to: " << fc_weight << '\n';
    }

    return fc_weight;
  }

  void initConvertWxFontStyleToFcSlant() {
    font_style_map_ = {{wxFONTSTYLE_NORMAL, FC_SLANT_ROMAN},
                       {wxFONTSTYLE_ITALIC, FC_SLANT_ITALIC},
                       {wxFONTSTYLE_SLANT, FC_SLANT_OBLIQUE},
                       {wxFONTSTYLE_MAX, FC_SLANT_OBLIQUE}};
  }

  int convertWxFontStyleToFcSlant(const wxFontStyle wx_font_style) const {
    int fc_slant = -1;
    auto it = font_style_map_.find(wx_font_style);
    if (it != font_style_map_.end()) {
      fc_slant = it->second;
    } else {
      throw std::runtime_error("convertWxFontStyleToFcSlant");
    }

    if (decade_debug::LogEnabled()) {
      std::cout << "convertWxFontStyleToFcSlant: " << wx_font_style
                << " to: " << fc_slant << '\n';
    }

    return fc_slant;
  }

  void ProcessFontData() {
    wxString const face_name = wx_font_.GetFaceName();
    int const point_size = wx_font_.GetPointSize();
    wxFontWeight const font_weight = wx_font_.GetWeight();
    wxFontStyle const font_style = wx_font_.GetStyle();

    if (decade_debug::LogEnabled()) {
      std::cout << "face_name: " << face_name << "\tpoint_size: " << point_size
                << "\tfont_style: " << font_style
                << "\t font_weight: " << font_weight << '\n';
    }

    FcPattern* pattern = FcPatternCreate();
    if (pattern == nullptr) {
      std::cerr << "ProcessFontData: FcPatternCreate failed\n";
      return;
    }
    FcPatternAddString(
        pattern, FC_FAMILY,
        reinterpret_cast<const FcChar8*>(face_name.utf8_str().data()));

    FcPatternAddInteger(pattern, FC_SIZE, point_size);

    int const fc_weight = convertWxFontWeightToFcWeight(font_weight);
    FcPatternAddInteger(pattern, FC_WEIGHT, fc_weight);

    int const fc_slant = convertWxFontStyleToFcSlant(font_style);
    FcPatternAddInteger(pattern, FC_SLANT, fc_slant);

    FcResult result = FcResultNoMatch;
    FcPattern* match = FcFontMatch(fc_config_.get(), pattern, &result);
    if (match == nullptr) {
      std::cerr << "ProcessFontData: no matching font found\n";
      FcPatternDestroy(pattern);
      return;
    }

    // https://fontconfig.pages.freedesktop.org/fontconfig/fontconfig-devel/fcpatternget.html
    FcChar8* fc_filepath = nullptr;
    FcResult const fc_result =
        FcPatternGetString(match, FC_FILE, 0, &fc_filepath);
    if (fc_result != FcResultMatch || fc_filepath == nullptr) {
      std::cerr << "ProcessFontData: matched font has no file path\n";
      FcPatternDestroy(match);
      FcPatternDestroy(pattern);
      return;
    }

    const size_t len = strlen(reinterpret_cast<const char*>(fc_filepath));
    font_filepath_ = std::string(fc_filepath, fc_filepath + len);
    if (decade_debug::LogEnabled()) {
      std::cout << "font_filepath: " << font_filepath_ << '\n';
    }

    FcPatternDestroy(match);
    FcPatternDestroy(pattern);
  }

  void CallbackFontChanged(wxFontPickerEvent& event) {
    try {
      wx_font_ = event.GetFont();
      ProcessFontData();
      signal_font_filepath_(font_filepath_);
    } catch (...) {
      std::cerr << "Loading Font failed" << '\n';
    }
  }

  wxWeakRef<wxFontPickerCtrl> wx_font_picker_;
  wxFont wx_font_;
  std::string font_filepath_;
  std::map<int, int> font_weight_map_;
  std::map<int, int> font_style_map_;
};
#endif  // FONT_PANEL_HPP
