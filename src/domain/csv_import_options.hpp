#ifndef CSV_IMPORT_OPTIONS_HPP
#define CSV_IMPORT_OPTIONS_HPP

// How a CSV import treats the project around the entries it reads. The options
// belong to the session, not to the project file.
class CsvImportOptions {
 public:
  [[nodiscard]] bool TitleFromFileName() const;
  void SetTitleFromFileName(bool title_from_file_name);

 private:
  bool title_from_file_name_{true};
};
#endif  // CSV_IMPORT_OPTIONS_HPP
