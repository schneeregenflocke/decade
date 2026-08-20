#include "license_panel.hpp"

#include <QtCore/qtypes.h>

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
#include <span>
#include <string_view>

#include "../common/third_party_licenses.hpp"
#include "make_owned.hpp"

namespace {

QString AsQString(std::string_view text) {
  return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

QString AsQString(std::span<const unsigned char> text) {
  return QString::fromUtf8(
      QByteArrayView(text.data(), static_cast<qsizetype>(text.size())));
}

}  // namespace

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

  FillLicenseList();
  license_select_list_->setCurrentRow(0);
}

void LicenseInformationDialog::FillLicenseList() {
  for (const auto& notice : licenses::kNotices) {
    license_select_list_->addItem(AsQString(notice.name));
  }
}

void LicenseInformationDialog::SelectLicenseRow(int row) {
  if (row < 0 || static_cast<std::size_t>(row) >= licenses::kNotices.size()) {
    return;
  }
  text_view_->setPlainText(
      AsQString(licenses::kNotices.at(static_cast<std::size_t>(row)).text));
  text_view_->moveCursor(QTextCursor::Start);
}
