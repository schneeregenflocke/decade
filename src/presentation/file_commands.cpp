#include "file_commands.hpp"

#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QTime>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <optional>
#include <string>

#include "../application/project_document.hpp"
#include "../domain/png_export_config.hpp"
#include "../domain/png_export_config_store.hpp"
#include "graphics/render_to_png.hpp"
#include "main_window.hpp"

FileCommands::FileCommands(MainWindow& window,
                           application::ProjectDocument& document)
    : window_(window), document_(document) {}

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
    case FileCommand::kChoosePngFile:
      ChoosePngFile();
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
  const QString title = "Export PNG file";
  PngExportConfigStore& store = document_.PngExportConfiguration();
  PngExportConfig config = store.Get();
  std::string file_path = config.ResolvedFilePath(document_.FilePath());
  if (file_path.empty()) {
    file_path =
        AskSavePath(title, kPngFile, QString::fromStdString(config.FilePath()));
    if (file_path.empty()) {
      return;
    }
    config.SetFilePath(file_path);
    store.ReceivePngExportConfig(config);
  } else if (QFileInfo::exists(QString::fromStdString(file_path)) &&
             !ConfirmOverwrite(title, QString::fromStdString(file_path))) {
    return;
  }
  const auto image =
      window_.Canvas().RenderPng(config.Dpi(), config.MsaaSamples());
  const std::optional<std::string> error =
      image ? WritePng(file_path, *image) : image.error();
  Report(title, error);
  if (error) {
    window_.PngExport().ClearSaved();
  } else {
    window_.PngExport().ShowSaved(QFileInfo(QString::fromStdString(file_path))
                                      .absoluteFilePath()
                                      .toStdString(),
                                  QTime::currentTime());
  }
}

void FileCommands::ChoosePngFile() {
  PngExportConfigStore& store = document_.PngExportConfiguration();
  PngExportConfig config = store.Get();
  const std::string file_path = AskSavePath(
      "PNG export file", kPngFile,
      QString::fromStdString(config.ResolvedFilePath(document_.FilePath())),
      QFileDialog::DontConfirmOverwrite);
  if (file_path.empty()) {
    return;
  }
  config.SetFilePath(file_path);
  store.ReceivePngExportConfig(config);
}

std::string FileCommands::AskOpenPath(const QString& title,
                                      const QString& filter) {
  return QFileDialog::getOpenFileName(&window_, title, QString(), filter)
      .toStdString();
}

std::string FileCommands::AskSavePath(const QString& title,
                                      const FileType& type,
                                      const QString& start,
                                      QFileDialog::Options options) {
  const QString chosen = QFileDialog::getSaveFileName(
      &window_, title, start, type.filter, nullptr, options);
  if (chosen.isEmpty()) {
    return {};
  }
  const QFileInfo file_info(chosen);
  if (!file_info.suffix().isEmpty()) {
    return chosen.toStdString();
  }
  const QString with_extension = chosen + "." + type.extension;
  if (!options.testFlag(QFileDialog::DontConfirmOverwrite) &&
      QFileInfo::exists(with_extension) &&
      !ConfirmOverwrite(title, with_extension)) {
    return {};
  }
  return with_extension.toStdString();
}

bool FileCommands::ConfirmOverwrite(const QString& title,
                                    const QString& file_path) const {
  const QString question =
      file_path + " already exists.\nDo you want to replace it?";
  return QMessageBox::warning(&window_, title, question,
                              QMessageBox::Yes | QMessageBox::No) ==
         QMessageBox::Yes;
}

void FileCommands::Report(const QString& title,
                          const std::optional<std::string>& error) const {
  if (error) {
    QMessageBox::critical(&window_, title, QString::fromStdString(*error));
  }
}
