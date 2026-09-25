#ifndef CSV_IMPORT_PANEL_HPP
#define CSV_IMPORT_PANEL_HPP

#include <QtCore/QPointer>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QWidget>

#include "../domain/csv_import_options.hpp"

class CsvImportPanel : public QWidget {
  Q_OBJECT

 public:
  explicit CsvImportPanel(QWidget* parent);

 signals:
  void CsvImportOptionsEdited(const CsvImportOptions& options);

 private:
  [[nodiscard]] CsvImportOptions ReadOptions() const;

  QPointer<QCheckBox> title_from_file_name_;
};
#endif  // CSV_IMPORT_PANEL_HPP
