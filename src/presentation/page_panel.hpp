#ifndef PAGE_PANEL_HPP
#define PAGE_PANEL_HPP

#include <QtCore/QMarginsF>
#include <QtCore/QPointer>
#include <QtCore/QSizeF>
#include <QtGui/QPageLayout>
#include <QtGui/QPageSize>
#include <QtPrintSupport/QPageSetupDialog>
#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <array>
#include <sigslot/signal.hpp>

#include "../domain/page_setup_config.hpp"
#include "make_owned.hpp"

class PageSetupPanel : public QWidget {
 public:
  explicit PageSetupPanel(QWidget* parent);

  void SendPageSetup();

  void ReceivePageSetup(const PageSetupConfig& page_setup_config);

  void SendDefaultValues();

  // Defined here, and this member therefore stays in the header: a deduced
  // return type has to be visible where it is called.
  [[nodiscard]] auto& SignalPageSetupConfig() {
    return signal_page_setup_config_;
  }

 private:
  // The orientation travels through the domain as a plain int and lands in the
  // project file that way (value_serialization.hpp). These two are the whole
  // vocabulary, taken from Qt's enum so there is no second numbering to keep in
  // step. Reading is total: anything that is not landscape counts as portrait,
  // which also makes a default-constructed PageSetupConfig portrait.
  static constexpr int kPortrait = static_cast<int>(QPageLayout::Portrait);
  static constexpr int kLandscape = static_cast<int>(QPageLayout::Landscape);

  static constexpr double kMaxPageSizeMm = 2000.0;
  static constexpr qreal kPageMarginMm = 15.0;

  [[nodiscard]] bool IsLandscape() const;

  // QPageSize always reports the paper in portrait; the orientation is a
  // separate flag. Every swap in this file follows from that.
  [[nodiscard]] QSizeF PaperSizeMillimetres() const;

  // QPageSize matches a free size back onto a standard paper name where one
  // fits, which is what keeps "A4" showing in the dialogue after a round trip
  // through the two spin boxes.
  void SetPaperSizeMillimetres(const QSizeF& size_millimetres);

  void UpdateSpinControls();

  void OpenPageSetupDialog();

  void OnSpinChanged();

  QPageLayout page_layout_{
      QPageSize(QPageSize::A4), QPageLayout::Landscape,
      QMarginsF(kPageMarginMm, kPageMarginMm, kPageMarginMm, kPageMarginMm),
      QPageLayout::Millimeter};

  QPointer<QDoubleSpinBox> page_width_spin_;
  QPointer<QDoubleSpinBox> page_height_spin_;

  sigslot::signal<const PageSetupConfig&> signal_page_setup_config_;

  bool updating_{false};
};
#endif  // PAGE_PANEL_HPP
