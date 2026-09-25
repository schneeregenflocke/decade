#include "csv_import_options.hpp"

bool CsvImportOptions::TitleFromFileName() const {
  return title_from_file_name_;
}

void CsvImportOptions::SetTitleFromFileName(bool title_from_file_name) {
  title_from_file_name_ = title_from_file_name;
}
