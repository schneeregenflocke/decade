#include "page_panel.hpp"

#include <QtCore/qtmetamacros.h>

#include <QtCore/QMarginsF>
#include <QtCore/QPointer>
#include <QtCore/QSizeF>
#include <QtCore/QtTypes>
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

#include "../domain/page_setup_config.hpp"
#include "make_owned.hpp"

PageSetupPanel::PageSetupPanel(QWidget* parent) : QWidget(parent) {
  constexpr int kBorderPx = 5;

  auto* page_setup_button = MakeOwned<QPushButton>("Page Setup...", this);

  page_width_spin_ = MakeOwned<QDoubleSpinBox>(this);
  page_width_spin_->setRange(0.0, kMaxPageSizeMm);
  page_height_spin_ = MakeOwned<QDoubleSpinBox>(this);
  page_height_spin_->setRange(0.0, kMaxPageSizeMm);

  auto* form_layout = MakeOwned<QFormLayout>();
  form_layout->addRow("Width", page_width_spin_.data());
  form_layout->addRow("Height", page_height_spin_.data());

  auto* vertical_layout = MakeOwned<QVBoxLayout>();
  vertical_layout->setContentsMargins(kBorderPx, kBorderPx, kBorderPx,
                                      kBorderPx);
  vertical_layout->addWidget(page_setup_button);
  vertical_layout->addLayout(form_layout);
  setLayout(vertical_layout);

  UpdateSpinControls();

  connect(page_setup_button, &QPushButton::clicked, this,
          [this]() { OpenPageSetupDialog(); });
  connect(page_width_spin_.data(), &QDoubleSpinBox::valueChanged, this,
          [this](double) { OnSpinChanged(); });
  connect(page_height_spin_.data(), &QDoubleSpinBox::valueChanged, this,
          [this](double) { OnSpinChanged(); });
}

void PageSetupPanel::SendPageSetup() {
  PageSetupConfig page_setup_config;

  const QSizeF paper = PaperSizeMillimetres();
  const bool landscape = IsLandscape();

  page_setup_config.SetOrientation(landscape ? kLandscape : kPortrait);
  page_setup_config.SetSize(
      landscape ? std::array<float, 2>{static_cast<float>(paper.height()),
                                       static_cast<float>(paper.width())}
                : std::array<float, 2>{static_cast<float>(paper.width()),
                                       static_cast<float>(paper.height())});

  const QMarginsF margins = page_layout_.margins(QPageLayout::Millimeter);
  page_setup_config.SetMargins(
      {static_cast<float>(margins.left()), static_cast<float>(margins.bottom()),
       static_cast<float>(margins.right()), static_cast<float>(margins.top())});

  emit PageSetupEdited(page_setup_config);
}

void PageSetupPanel::ReceivePageSetup(
    const PageSetupConfig& page_setup_config) {
  page_layout_.setOrientation(page_setup_config.Orientation() == kLandscape
                                  ? QPageLayout::Landscape
                                  : QPageLayout::Portrait);

  const std::array<float, 2>& size = page_setup_config.Size();
  SetPaperSizeMillimetres(
      IsLandscape()
          ? QSizeF(static_cast<qreal>(size[1]), static_cast<qreal>(size[0]))
          : QSizeF(static_cast<qreal>(size[0]), static_cast<qreal>(size[1])));

  const std::array<float, 4>& margins = page_setup_config.Margins();
  page_layout_.setMargins(QMarginsF(
      static_cast<qreal>(margins[0]), static_cast<qreal>(margins[3]),
      static_cast<qreal>(margins[2]), static_cast<qreal>(margins[1])));

  UpdateSpinControls();
}

void PageSetupPanel::SendDefaultValues() { SendPageSetup(); }

bool PageSetupPanel::IsLandscape() const {
  return page_layout_.orientation() == QPageLayout::Landscape;
}

QSizeF PageSetupPanel::PaperSizeMillimetres() const {
  return page_layout_.pageSize().size(QPageSize::Millimeter);
}

void PageSetupPanel::SetPaperSizeMillimetres(const QSizeF& size_millimetres) {
  page_layout_.setPageSize(QPageSize(size_millimetres, QPageSize::Millimeter));
}

void PageSetupPanel::UpdateSpinControls() {
  const QSizeF paper = PaperSizeMillimetres();
  const bool landscape = IsLandscape();

  updating_ = true;
  page_width_spin_->setValue(landscape ? paper.height() : paper.width());
  page_height_spin_->setValue(landscape ? paper.width() : paper.height());
  updating_ = false;
}

void PageSetupPanel::OpenPageSetupDialog() {
  // The printer exists for the dialogue alone: QPageSetupDialog takes one,
  // while the panel's state is the page layout. Constructing it on demand
  // keeps the printing subsystem out of the startup path.
  QPrinter printer(QPrinter::HighResolution);
  printer.setPageLayout(page_layout_);

  QPageSetupDialog dialog(&printer, this);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }
  page_layout_ = printer.pageLayout();
  UpdateSpinControls();
  SendPageSetup();
}

void PageSetupPanel::OnSpinChanged() {
  if (updating_) {
    return;
  }
  const double width = page_width_spin_->value();
  const double height = page_height_spin_->value();
  SetPaperSizeMillimetres(IsLandscape() ? QSizeF(height, width)
                                        : QSizeF(width, height));
  SendPageSetup();
}
