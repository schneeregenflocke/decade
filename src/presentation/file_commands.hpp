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
  FileCommands(MainFrame& frame, application::ProjectDocument& document)
      : frame_(frame), document_(document) {}

  void Execute(FileCommand command) {
    switch (command) {
      case FileCommand::kOpenXml:
        OpenXml();
        break;
      case FileCommand::kSaveXml:
        SaveXml();
        break;
      case FileCommand::kSaveXmlAs:
        SaveXmlAs();
        break;
      case FileCommand::kImportCsv:
        ImportCsv();
        break;
      case FileCommand::kExportCsv:
        ExportCsv();
        break;
      case FileCommand::kExportPng:
        ExportPng();
        break;
    }
  }

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

  void OpenXml() {
    const std::string file_path = AskOpenPath("Open File", kXmlFile.filter);
    if (file_path.empty()) {
      return;
    }
    Report("Open File", document_.LoadXml(file_path));
  }

  // Saving without a known path asks exactly as "save as" does.
  void SaveXml() {
    if (!document_.HasFilePath()) {
      SaveXmlAs();
      return;
    }
    Report("Save File", document_.SaveXml(document_.FilePath()));
  }

  void SaveXmlAs() {
    const std::string file_path = AskSavePath("Save File", kXmlFile);
    if (file_path.empty()) {
      return;
    }
    Report("Save File", document_.SaveXml(file_path));
  }

  void ImportCsv() {
    const std::string file_path = AskOpenPath("Import file", kCsvFile.filter);
    if (file_path.empty()) {
      return;
    }
    document_.ImportCsv(file_path);
  }

  void ExportCsv() {
    const std::string file_path = AskSavePath("Export file", kCsvFile);
    if (file_path.empty()) {
      return;
    }
    Report("Export file", document_.ExportCsv(file_path));
  }

  void ExportPng() {
    const std::string file_path = AskSavePath("Export PNG file", kPngFile);
    if (file_path.empty()) {
      return;
    }
    frame_.Canvas().SavePNG(file_path);
  }

  // An empty return value means: cancelled.
  [[nodiscard]] std::string AskOpenPath(const QString& title,
                                        const QString& filter) {
    return QFileDialog::getOpenFileName(&frame_, title, QString(), filter)
        .toStdString();
  }

  // The overwrite question of the save dialogue asks about the typed name
  // alone, so the suffix gets appended afterwards and the question repeated for
  // the completed name.
  [[nodiscard]] std::string AskSavePath(const QString& title,
                                        const FileType& type) {
    const QString chosen =
        QFileDialog::getSaveFileName(&frame_, title, QString(), type.filter);
    if (chosen.isEmpty()) {
      return {};
    }
    const QFileInfo file_info(chosen);
    if (!file_info.suffix().isEmpty()) {
      return chosen.toStdString();
    }
    const QString with_extension = chosen + "." + type.extension;
    if (QFileInfo::exists(with_extension) &&
        !ConfirmOverwrite(title, with_extension)) {
      return {};
    }
    return with_extension.toStdString();
  }

  [[nodiscard]] bool ConfirmOverwrite(const QString& title,
                                      const QString& file_path) const {
    const QString question =
        file_path + " already exists.\nDo you want to replace it?";
    return QMessageBox::warning(&frame_, title, question,
                                QMessageBox::Yes | QMessageBox::No) ==
           QMessageBox::Yes;
  }

  void Report(const QString& title,
              const std::optional<std::string>& error) const {
    if (error) {
      QMessageBox::critical(&frame_, title, QString::fromStdString(*error));
    }
  }

  MainFrame& frame_;
  application::ProjectDocument& document_;
};

#endif  // FILE_COMMANDS_HPP
