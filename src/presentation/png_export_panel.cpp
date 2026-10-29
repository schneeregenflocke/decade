#include "png_export_panel.hpp"

#include <QtCore/qnamespace.h>
#include <QtCore/qtmetamacros.h>

#include <QtCore/QPointer>
#include <QtCore/QSignalBlocker>
#include <QtCore/QString>
#include <QtCore/QTime>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <string>

#include "../domain/page_setup_config.hpp"
#include "../domain/png_export_config.hpp"
#include "graphics/export_size.hpp"
#include "make_owned.hpp"
#include "restoring_int_validator.hpp"

namespace {

constexpr int kEffectiveDpiDecimals = 3;

QLabel* MakeValueLabel(const char* name, QWidget* parent) {
  auto* label = MakeOwned<QLabel>(parent);
  label->setObjectName(name);
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  return label;
}

}  // namespace

PngExportPanel::PngExportPanel(QWidget* parent)
    : QWidget(parent),
      pixel_size_(MakeValueLabel("pixel_size", this)),
      millimetre_size_(MakeValueLabel("millimetre_size", this)),
      effective_dpi_(MakeValueLabel("effective_dpi", this)),
      saved_(MakeValueLabel("saved", this)) {
  file_path_ = MakeOwned<QLineEdit>(this);
  file_path_->setObjectName("file_path");
  auto* browse = MakeOwned<QPushButton>("...", this);
  browse->setObjectName("browse");
  auto* file_row = MakeOwned<QHBoxLayout>();
  file_row->addWidget(file_path_.data(), 1);
  file_row->addWidget(browse);

  dpi_ = MakeOwned<QComboBox>(this);
  dpi_->setObjectName("dpi");
  dpi_->setEditable(true);
  dpi_->setInsertPolicy(QComboBox::NoInsert);
  dpi_->addItems({"72", "150", "300", "600"});
  // An editable combo box completes inline from its items, which would turn a
  // typed 30 into 300.
  dpi_->setCompleter(nullptr);
  dpi_->setValidator(MakeOwned<RestoringIntValidator>(
      PngExportConfig::kMinDpi, PngExportConfig::kMaxDpi,
      [this] { return config_.Dpi(); }, dpi_.data()));

  msaa_samples_ = MakeOwned<QComboBox>(this);
  msaa_samples_->setObjectName("msaa_samples");
  msaa_samples_->addItems({"1", "2", "4", "8", "16"});

  auto* export_button = MakeOwned<QPushButton>("Export", this);
  export_button->setObjectName("export");

  auto* form = MakeOwned<QFormLayout>();
  form->addRow("File", file_row);
  form->addRow("Resolution (dpi)", dpi_.data());
  form->addRow("Multisampling", msaa_samples_.data());
  form->addRow("Pixels", pixel_size_.data());
  form->addRow("Page", millimetre_size_.data());
  form->addRow("Effective resolution", effective_dpi_.data());

  auto* vertical_layout = MakeOwned<QVBoxLayout>();
  vertical_layout->addLayout(form);
  vertical_layout->addWidget(export_button);
  vertical_layout->addWidget(saved_.data());
  vertical_layout->addStretch(1);
  setLayout(vertical_layout);

  ReceivePngExportConfig(config_);

  connect(dpi_->lineEdit(), &QLineEdit::editingFinished, this,
          [this] { CommitDpi(); });
  connect(dpi_.data(), &QComboBox::activated, this,
          [this](int) { CommitDpi(); });
  connect(msaa_samples_.data(), &QComboBox::currentIndexChanged, this,
          [this](int) { CommitMsaaSamples(); });
  connect(file_path_.data(), &QLineEdit::editingFinished, this,
          [this] { CommitFilePath(); });
  connect(browse, &QPushButton::clicked, this,
          [this] { emit BrowseRequested(); });
  connect(export_button, &QPushButton::clicked, this,
          [this] { emit ExportRequested(); });
}

void PngExportPanel::ReceivePngExportConfig(const PngExportConfig& config) {
  config_ = config;
  {
    const QSignalBlocker block_dpi(dpi_.data());
    const QSignalBlocker block_dpi_text(dpi_->lineEdit());
    const QSignalBlocker block_samples(msaa_samples_.data());
    const QSignalBlocker block_path(file_path_.data());
    dpi_->setEditText(QString::number(config_.Dpi()));
    msaa_samples_->setCurrentIndex(
        msaa_samples_->findText(QString::number(config_.MsaaSamples())));
    file_path_->setText(QString::fromStdString(config_.FilePath()));
  }
  UpdateDimensions();
}

void PngExportPanel::ReceivePageSetup(
    const PageSetupConfig& page_setup_config) {
  page_size_mm_ = page_setup_config.Size();
  UpdateDimensions();
}

void PngExportPanel::ReceiveProjectFilePath(const std::string& file_path) {
  project_file_path_ = file_path;
  file_path_->setPlaceholderText(
      QString::fromStdString(DefaultPngFilePath(file_path)));
}

void PngExportPanel::ShowSaved(const std::string& file_path,
                               const QTime& time) {
  saved_->setText(
      QString("Saved: %1, %2")
          .arg(QString::fromStdString(file_path), time.toString("HH:mm:ss")));
}

void PngExportPanel::CommitDpi() {
  bool parsed = false;
  const int dpi = dpi_->currentText().toInt(&parsed);
  if (!parsed || dpi < PngExportConfig::kMinDpi ||
      dpi > PngExportConfig::kMaxDpi || dpi == config_.Dpi()) {
    return;
  }
  config_.SetDpi(dpi);
  UpdateDimensions();
  emit PngExportConfigEdited(config_);
}

void PngExportPanel::CommitMsaaSamples() {
  const int samples = msaa_samples_->currentText().toInt();
  if (samples == config_.MsaaSamples()) {
    return;
  }
  config_.SetMsaaSamples(samples);
  emit PngExportConfigEdited(config_);
}

void PngExportPanel::CommitFilePath() {
  const std::string file_path = file_path_->text().toStdString();
  if (file_path == config_.FilePath()) {
    return;
  }
  config_.SetFilePath(file_path);
  emit PngExportConfigEdited(config_);
}

void PngExportPanel::UpdateDimensions() {
  const auto [width_mm, height_mm] = page_size_mm_;
  millimetre_size_->setText(
      QString("%1 × %2 mm").arg(width_mm, 0, 'f', 1).arg(height_mm, 0, 'f', 1));
  const auto size =
      ExportPixelSize(width_mm, height_mm, static_cast<float>(config_.Dpi()));
  if (!size) {
    pixel_size_->setText("–");
    effective_dpi_->setText("–");
    return;
  }
  pixel_size_->setText(
      QString("%1 × %2 px").arg(size->width).arg(size->height));
  effective_dpi_->setText(QString("%1 × %2 dpi")
                              .arg(EffectiveDpi(size->width, width_mm), 0, 'f',
                                   kEffectiveDpiDecimals)
                              .arg(EffectiveDpi(size->height, height_mm), 0,
                                   'f', kEffectiveDpiDecimals));
}
