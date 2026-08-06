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

#include "../common/debug_log.hpp"
#include "../domain/font_config.hpp"
#include "wx_owned.hpp"

// Chooses the application-wide font. wx describes a font through family, size,
// weight and slant; the GL renderer, by contrast, needs a font file —
// fontconfig does the translation between them.
//
// Both get passed on: the file path (what to render with) and the point size
// (how large to render). The size is a pure domain number: Font always rasters
// at kFontPixelHeight and scales the em-normalised metrics, so the font file
// does not hang on it.
//
// Sources:
// - wxFont (https://docs.wxwidgets.org/3.2/classwx_font.html) — GetPointSize(),
//   GetFaceName(), GetWeight(), GetStyle() as the inputs of the mapping.
// - fontconfig font properties
//   (https://fontconfig.pages.freedesktop.org/fontconfig/fontconfig-user.html)
//   — names and types of FC_FAMILY, FC_WEIGHT, FC_SLANT, FC_SIZE.
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
    SetSizerAndFit(vertical_sizer);

    Bind(wxEVT_FONTPICKER_CHANGED, &FontPanel::CallbackFontChanged, this);
    wx_font_ = wx_font_picker_->GetFont();

    BuildSlantTable();

    ProcessFontData();
  }

  [[nodiscard]] const FontConfig& GetFontConfig() const { return font_config_; }

  [[nodiscard]] auto& SignalFontConfig() { return signal_font_config_; }

 private:
  // Owns the fontconfig configuration for this panel's lifetime. Loading it
  // once (instead of FcInitLoadConfigAndFonts()/FcFini() on every font change)
  // avoids repeated, expensive global re-initialisation of fontconfig. RAII
  // keeps the class at Rule of Zero.
  struct FcConfigDeleter {
    void operator()(FcConfig* config) const { FcConfigDestroy(config); }
  };

  std::unique_ptr<FcConfig, FcConfigDeleter> fc_config_;
  sigslot::signal<const FontConfig&> signal_font_config_;
  // wx counts its weights in the OpenType and CSS scale (THIN 100 …
  // EXTRAHEAVY 1000), fontconfig in its own uneven 0…215 one (fontconfig.h,
  // FC_WEIGHT_*). fontconfig converts between them itself.
  //
  // This was a hand-written table of the eleven enum steps. The call is not
  // merely shorter: it interpolates, where the table snapped onto its keys and
  // threw on everything else — and a wx weight is not confined to the enum,
  // `wxFont::SetNumericWeight` takes any number in between.
  //
  // Sources:
  // - wxFontWeight (https://docs.wxwidgets.org/3.2/font_8h.html)
  // - FcWeightFromOpenTypeDouble
  //   (https://man.archlinux.org/man/FcWeightFromOpenTypeDouble.3)
  static double FcWeightFromWxWeight(const wxFontWeight wx_font_weight) {
    if (wx_font_weight == wxFONTWEIGHT_INVALID) {
      throw std::runtime_error("wxFONTWEIGHT_INVALID");
    }

    const double fc_weight =
        FcWeightFromOpenTypeDouble(static_cast<double>(wx_font_weight));

    if (decade_debug::LogEnabled()) {
      std::cout << "FcWeightFromWxWeight: " << wx_font_weight
                << " to: " << fc_weight << '\n';
    }

    return fc_weight;
  }

  // wx knows three slants, fontconfig the same three (FC_SLANT_ROMAN, ITALIC,
  // OBLIQUE) — the mapping is one to one here. wxFONTSTYLE_MAX is merely the
  // count marker behind SLANT and lands on the same OBLIQUE.
  void BuildSlantTable() {
    font_style_map_ = {{wxFONTSTYLE_NORMAL, FC_SLANT_ROMAN},
                       {wxFONTSTYLE_ITALIC, FC_SLANT_ITALIC},
                       {wxFONTSTYLE_SLANT, FC_SLANT_OBLIQUE},
                       {wxFONTSTYLE_MAX, FC_SLANT_OBLIQUE}};
  }

  int FcSlantFromWxStyle(const wxFontStyle wx_font_style) const {
    int fc_slant = -1;
    auto it = font_style_map_.find(wx_font_style);
    if (it != font_style_map_.end()) {
      fc_slant = it->second;
    } else {
      throw std::runtime_error("FcSlantFromWxStyle");
    }

    if (decade_debug::LogEnabled()) {
      std::cout << "FcSlantFromWxStyle: " << wx_font_style
                << " to: " << fc_slant << '\n';
    }

    return fc_slant;
  }

  void ProcessFontData() {
    wxString const face_name = wx_font_.GetFaceName();
    double const point_size = wx_font_.GetFractionalPointSize();
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

    // FC_SIZE is a double according to fontconfig.h (the point size); an
    // integer here would be a type break that fontconfig matches worse in
    // silence. For scalable fonts the size chooses no other file anyway — it
    // stands in the pattern so that size-specific cuts (bitmap fonts, optical
    // grades) resolve correctly.
    FcPatternAddDouble(pattern, FC_SIZE, point_size);

    // As a double, so an interpolated value survives instead of being cut
    // back onto a step.
    const double fc_weight = FcWeightFromWxWeight(font_weight);
    FcPatternAddDouble(pattern, FC_WEIGHT, fc_weight);

    int const fc_slant = FcSlantFromWxStyle(font_style);
    FcPatternAddInteger(pattern, FC_SLANT, fc_slant);

    // The mandatory prelude to FcFontMatch: FcConfigSubstitute applies the
    // rules of the system configuration (aliases such as "sans-serif"),
    // FcDefaultSubstitute fills in the underspecified and converts the point
    // size into a pixel size. Without both, fontconfig matches wrongly
    // according to the manual. https://man.archlinux.org/man/FcFontMatch.3.en
    FcConfigSubstitute(fc_config_.get(), pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

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
    font_config_.SetFilePath(std::string(fc_filepath, fc_filepath + len));
    font_config_.SetSizePoints(static_cast<float>(point_size));
    if (decade_debug::LogEnabled()) {
      std::cout << "font_filepath: " << font_config_.FilePath() << '\n';
    }

    FcPatternDestroy(match);
    FcPatternDestroy(pattern);
  }

  void CallbackFontChanged(wxFontPickerEvent& event) {
    try {
      wx_font_ = event.GetFont();
      ProcessFontData();
      signal_font_config_(font_config_);
    } catch (...) {
      std::cerr << "Loading Font failed" << '\n';
    }
  }

  wxWeakRef<wxFontPickerCtrl> wx_font_picker_;
  wxFont wx_font_;
  FontConfig font_config_;
  std::map<int, int> font_style_map_;
};
#endif  // FONT_PANEL_HPP
