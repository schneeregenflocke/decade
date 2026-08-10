#ifndef FILE_COMMANDS_HPP
#define FILE_COMMANDS_HPP

#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <optional>
#include <string>

#include "../application/project_document.hpp"
#include "main_frame.hpp"

// Carries out the menu commands around files: show the dialogue, fetch the
// path, have the project loaded or written, report errors. The dialogues are
// the reason this sits in presentation — nothing gets computed here.
class FileCommands {
 public:
  FileCommands(MainFrame& frame, application::ProjectDocument& document);

  void Execute(FileCommand command);

 private:
  // The dialogue filter and the suffix that may be missing when saving.
  //
  // `const char*` and not `std::string_view`, against the usual preference:
  // these are string literals in a `static constexpr` member, so they have
  // static storage duration and encode neither ownership nor a dangling risk —
  // and every consumer is a QString, which takes a `const char*` and no
  // string_view. A view would buy nothing and cost a `.data()` at each call,
  // where null termination would hold only by accident.
  struct FileType {
    const char* filter;
    const char* extension;
  };
  static constexpr FileType kXmlFile{.filter = "XML Files (*.xml)",
                                     .extension = "xml"};
  static constexpr FileType kCsvFile{
      .filter = "CSV and TXT files (*.csv *.txt)", .extension = "csv"};
  static constexpr FileType kPngFile{.filter = "PNG files (*.png)",
                                     .extension = "png"};

  void OpenXml();

  // Saving without a known path asks exactly as "save as" does.
  void SaveXml();

  void SaveXmlAs();

  void ImportCsv();

  void ExportCsv();

  void ExportPng();

  // An empty return value means: cancelled.
  [[nodiscard]] std::string AskOpenPath(const QString& title,
                                        const QString& filter);

  // The overwrite question of the save dialogue asks about the typed name
  // alone, so the suffix gets appended afterwards and the question repeated for
  // the completed name.
  [[nodiscard]] std::string AskSavePath(const QString& title,
                                        const FileType& type);

  [[nodiscard]] bool ConfirmOverwrite(const QString& title,
                                      const QString& file_path) const;

  void Report(const QString& title,
              const std::optional<std::string>& error) const;

  MainFrame& frame_;
  application::ProjectDocument& document_;
};

#endif  // FILE_COMMANDS_HPP
