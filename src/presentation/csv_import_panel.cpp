#include "csv_import_panel.hpp"

#include <QtCore/qtmetamacros.h>

#include <QtCore/QPointer>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "../domain/csv_import_options.hpp"
#include "make_owned.hpp"

CsvImportPanel::CsvImportPanel(QWidget* parent) : QWidget(parent) {
  const CsvImportOptions defaults;
  title_from_file_name_ = MakeOwned<QCheckBox>(this);
  title_from_file_name_->setChecked(defaults.TitleFromFileName());
  title_from_file_name_->setToolTip(
      "Sets the calendar title to the imported file's name, without its "
      "extension.");

  auto* form = MakeOwned<QFormLayout>();
  form->addRow("Use file name as title", title_from_file_name_.data());

  auto* vertical_layout = MakeOwned<QVBoxLayout>();
  vertical_layout->addLayout(form);
  vertical_layout->addStretch(1);
  setLayout(vertical_layout);

  connect(title_from_file_name_.data(), &QCheckBox::toggled, this,
          [this](bool) { emit CsvImportOptionsEdited(ReadOptions()); });
}

CsvImportOptions CsvImportPanel::ReadOptions() const {
  CsvImportOptions options;
  options.SetTitleFromFileName(title_from_file_name_->isChecked());
  return options;
}
