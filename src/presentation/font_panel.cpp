#include "font_panel.hpp"

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
#include <sigslot/signal.hpp>
#include <string>

#include "../common/debug_log.hpp"
#include "../domain/font_config.hpp"
#include "make_owned.hpp"

FontPanel::FontPanel(QWidget* parent)
    : QWidget(parent), fc_config_(FcInitLoadConfigAndFonts()) {
  constexpr int kBorderPx = 5;

  font_button_ = MakeOwned<QPushButton>(this);
  auto* horizontal_layout = MakeOwned<QHBoxLayout>();
  horizontal_layout->setContentsMargins(kBorderPx, kBorderPx, kBorderPx,
                                        kBorderPx);
  horizontal_layout->addWidget(font_button_);
  setLayout(horizontal_layout);

  RefreshButtonLabel();
  connect(font_button_.data(), &QPushButton::clicked, this,
          [this]() { ChooseFont(); });

  ProcessFontData();
}

const FontConfig& FontPanel::GetFontConfig() const { return font_config_; }

double FontPanel::FcWeightFromQtWeight(QFont::Weight weight) {
  const double fc_weight =
      FcWeightFromOpenTypeDouble(static_cast<double>(weight));

  if (decade_debug::LogEnabled()) {
    std::cout << "FcWeightFromQtWeight: " << static_cast<int>(weight)
              << " to: " << fc_weight << '\n';
  }

  return fc_weight;
}

int FontPanel::FcSlantFromQtStyle(QFont::Style style) {
  switch (style) {
    case QFont::StyleItalic:
      return FC_SLANT_ITALIC;
    case QFont::StyleOblique:
      return FC_SLANT_OBLIQUE;
    case QFont::StyleNormal:
      break;
  }
  return FC_SLANT_ROMAN;
}

void FontPanel::ProcessFontData() {
  const QString face_name = font_.family();
  const double point_size = font_.pointSizeF();

  if (decade_debug::LogEnabled()) {
    std::cout << "face_name: " << face_name.toStdString()
              << "\tpoint_size: " << point_size
              << "\tfont_style: " << static_cast<int>(font_.style())
              << "\t font_weight: " << static_cast<int>(font_.weight()) << '\n';
  }

  FcPattern* pattern = FcPatternCreate();
  if (pattern == nullptr) {
    std::cerr << "ProcessFontData: FcPatternCreate failed\n";
    return;
  }
  const std::string face_name_utf8 = face_name.toStdString();
  FcPatternAddString(pattern, FC_FAMILY,
                     reinterpret_cast<const FcChar8*>(face_name_utf8.c_str()));

  // FC_SIZE is a double according to fontconfig.h (the point size); an
  // integer here would be a type break that fontconfig matches worse in
  // silence. For scalable fonts the size chooses no other file anyway — it
  // stands in the pattern so that size-specific cuts (bitmap fonts, optical
  // grades) resolve correctly.
  FcPatternAddDouble(pattern, FC_SIZE, point_size);

  // As a double, so an interpolated value survives instead of being cut
  // back onto a step.
  FcPatternAddDouble(pattern, FC_WEIGHT, FcWeightFromQtWeight(font_.weight()));
  FcPatternAddInteger(pattern, FC_SLANT, FcSlantFromQtStyle(font_.style()));

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

  const size_t length = strlen(reinterpret_cast<const char*>(fc_filepath));
  font_config_.SetFilePath(std::string(fc_filepath, fc_filepath + length));
  font_config_.SetSizePoints(static_cast<float>(point_size));
  if (decade_debug::LogEnabled()) {
    std::cout << "font_filepath: " << font_config_.FilePath() << '\n';
  }

  FcPatternDestroy(match);
  FcPatternDestroy(pattern);
}

void FontPanel::ChooseFont() {
  bool accepted = false;
  const QFont chosen =
      QFontDialog::getFont(&accepted, font_, this, "Choose Font");
  if (!accepted) {
    return;
  }
  font_ = chosen;
  RefreshButtonLabel();
  ProcessFontData();
  signal_font_config_(font_config_);
}

void FontPanel::RefreshButtonLabel() {
  font_button_->setText(
      QString("%1 %2").arg(font_.family()).arg(font_.pointSizeF()));
}
