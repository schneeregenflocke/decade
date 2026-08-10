#include "file_commands.hpp"

#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <optional>
#include <string>

#include "../application/project_document.hpp"
#include "main_frame.hpp"

FileCommands::FileCommands(MainFrame& frame,
                           application::ProjectDocument& document)
    : frame_(frame), document_(document) {}

void FileCommands::Execute(FileCommand command) {
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

void FileCommands::OpenXml() {
  const std::string file_path = AskOpenPath("Open File", kXmlFile.filter);
  if (file_path.empty()) {
    return;
  }
  Report("Open File", document_.LoadXml(file_path));
}

void FileCommands::SaveXml() {
  if (!document_.HasFilePath()) {
    SaveXmlAs();
    return;
  }
  Report("Save File", document_.SaveXml(document_.FilePath()));
}

void FileCommands::SaveXmlAs() {
  const std::string file_path = AskSavePath("Save File", kXmlFile);
  if (file_path.empty()) {
    return;
  }
  Report("Save File", document_.SaveXml(file_path));
}

void FileCommands::ImportCsv() {
  const std::string file_path = AskOpenPath("Import file", kCsvFile.filter);
  if (file_path.empty()) {
    return;
  }
  document_.ImportCsv(file_path);
}

void FileCommands::ExportCsv() {
  const std::string file_path = AskSavePath("Export file", kCsvFile);
  if (file_path.empty()) {
    return;
  }
  Report("Export file", document_.ExportCsv(file_path));
}

void FileCommands::ExportPng() {
  const std::string file_path = AskSavePath("Export PNG file", kPngFile);
  if (file_path.empty()) {
    return;
  }
  frame_.Canvas().SavePNG(file_path);
}

std::string FileCommands::AskOpenPath(const QString& title,
                                      const QString& filter) {
  return QFileDialog::getOpenFileName(&frame_, title, QString(), filter)
      .toStdString();
}

std::string FileCommands::AskSavePath(const QString& title,
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

bool FileCommands::ConfirmOverwrite(const QString& title,
                                    const QString& file_path) const {
  const QString question =
      file_path + " already exists.\nDo you want to replace it?";
  return QMessageBox::warning(&frame_, title, question,
                              QMessageBox::Yes | QMessageBox::No) ==
         QMessageBox::Yes;
}

void FileCommands::Report(const QString& title,
                          const std::optional<std::string>& error) const {
  if (error) {
    QMessageBox::critical(&frame_, title, QString::fromStdString(*error));
  }
}
