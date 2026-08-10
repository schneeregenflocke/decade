#include "license_panel.hpp"

#include <QtCore/QString>
#include <QtGui/QTextCursor>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <cstddef>
#include <string>

#include "../common/embedded_resources.hpp"
#include "make_owned.hpp"

LicenseInformationDialog::LicenseInformationDialog(QWidget* parent)
    : QDialog(parent) {
  setWindowTitle("Open Source Licenses Information");
  resize(kDefaultWidth, kDefaultHeight);

  license_select_list_ = MakeOwned<QListWidget>(this);
  license_select_list_->setSelectionMode(QAbstractItemView::SingleSelection);

  text_view_ = MakeOwned<QPlainTextEdit>(this);
  text_view_->setReadOnly(true);

  auto* horizontal_layout = MakeOwned<QHBoxLayout>();
  horizontal_layout->addWidget(license_select_list_);
  horizontal_layout->addWidget(text_view_, 1);

  auto* button_box = MakeOwned<QDialogButtonBox>(QDialogButtonBox::Close, this);

  auto* vertical_layout = MakeOwned<QVBoxLayout>();
  vertical_layout->setContentsMargins(kBorderPx, kBorderPx, kBorderPx,
                                      kBorderPx);
  vertical_layout->addLayout(horizontal_layout, 1);
  vertical_layout->addWidget(button_box);
  setLayout(vertical_layout);

  connect(button_box, &QDialogButtonBox::rejected, this,
          [this]() { reject(); });
  connect(license_select_list_.data(), &QListWidget::currentRowChanged, this,
          [this](int row) { SelectLicenseRow(row); });

  CollectLicenses();
  license_select_list_->setCurrentRow(0);
}

void LicenseInformationDialog::CollectLicenses() {
  collected_licenses_.clear();

  // embed-resource dropped out with the submodule it licensed; tinycolormap
  // joined, which was embedded all along and never shown.
  collected_licenses_.emplace_back("Decade",
                                   std::string(resources::kDecadeLicense));
  collected_licenses_.emplace_back("csv2",
                                   std::string(resources::kCsv2License));
  collected_licenses_.emplace_back("csv2mio",
                                   std::string(resources::kCsv2MioLicense));
  collected_licenses_.emplace_back(
      "tinycolormap", std::string(resources::kTinycolormapLicense));
  collected_licenses_.emplace_back("Bullet Physics",
                                   std::string(resources::kBulletLicense));

  for (const auto& license : collected_licenses_) {
    license_select_list_->addItem(QString::fromStdString(license.first));
  }
}

void LicenseInformationDialog::SelectLicenseRow(int row) {
  if (row < 0 || static_cast<std::size_t>(row) >= collected_licenses_.size()) {
    return;
  }
  text_view_->setPlainText(QString::fromStdString(
      collected_licenses_[static_cast<std::size_t>(row)].second));
  text_view_->moveCursor(QTextCursor::Start);
}
