#ifndef FONT_PANEL_HPP
#define FONT_PANEL_HPP

#include <fontconfig/fontconfig.h>

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtGui/QFont>
#include <QtWidgets/QFontDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include "../common/debug_log.hpp"
#include "../domain/font_config.hpp"
#include "make_owned.hpp"

// Chooses the application-wide font. Qt describes a font through family, size,
// weight and slant; the GL renderer, by contrast, needs a font file —
// fontconfig does the translation between them.
//
// Both get passed on: the file path (what to render with) and the point size
// (how large to render). The size is a pure domain number: Font always rasters
// at kFontPixelHeight and scales the em-normalised metrics, so the font file
// does not hang on it.
//
// Sources:
// - QFont (https://doc.qt.io/qt-6/qfont.html) — family(), pointSizeF(),
//   weight(), style() as the inputs of the mapping.
// - fontconfig font properties
//   (https://fontconfig.pages.freedesktop.org/fontconfig/fontconfig-user.html)
//   — names and types of FC_FAMILY, FC_WEIGHT, FC_SLANT, FC_SIZE.
class FontPanel : public QWidget {
  Q_OBJECT

 public:
  explicit FontPanel(QWidget* parent);

  [[nodiscard]] const FontConfig& GetFontConfig() const;

 signals:
  // The font the user picked. It has no store, so the binder puts it onto the
  // topic itself.
  void FontConfigChosen(const FontConfig& font_config);

 private:
  // Owns the fontconfig configuration for this panel's lifetime. Loading it
  // once (instead of FcInitLoadConfigAndFonts()/FcFini() on every font change)
  // avoids repeated, expensive global re-initialisation of fontconfig. RAII
  // keeps the class at Rule of Zero.
  struct FcConfigDeleter {
    void operator()(FcConfig* config) const { FcConfigDestroy(config); }
  };

  // Qt counts its weights in the OpenType and CSS scale (Thin 100 … Black 900,
  // and any number in between), fontconfig in its own uneven 0…215 one
  // (fontconfig.h, FC_WEIGHT_*). fontconfig converts between them itself, and
  // interpolates — which a table of the named steps could not.
  //
  // Source: FcWeightFromOpenTypeDouble
  // (https://man.archlinux.org/man/FcWeightFromOpenTypeDouble.3)
  static double FcWeightFromQtWeight(QFont::Weight weight);

  // Qt knows three styles, fontconfig the same three slants — the mapping is
  // one to one.
  static int FcSlantFromQtStyle(QFont::Style style);

  void ProcessFontData();

  // Qt carries no inline font picker, only the modal dialogue — so the button
  // shows the current font and opens it, the way wxFontPickerCtrl did.
  void ChooseFont();

  void RefreshButtonLabel();

  std::unique_ptr<FcConfig, FcConfigDeleter> fc_config_;
  QPointer<QPushButton> font_button_;
  QFont font_;
  FontConfig font_config_;
};
#endif  // FONT_PANEL_HPP
