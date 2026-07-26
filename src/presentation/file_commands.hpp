#ifndef FILE_COMMANDS_HPP
#define FILE_COMMANDS_HPP

#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/string.h>

#include <string>

#include "../application/project_document.hpp"
#include "main_frame.hpp"

// Führt die Menübefehle rund um Dateien aus: Dialog zeigen, Pfad holen, das
// Projekt laden oder schreiben lassen, Fehler melden. Die Dialoge sind der
// Grund, warum das in der Presentation liegt — gerechnet wird nichts hier.
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
  // Filter des Dialogs und die Endung, die beim Speichern fehlen darf.
  struct FileType {
    const char* wildcard;
    const char* extension;
  };
  static constexpr FileType kXmlFile{.wildcard = "XML Files (*.xml)|*.xml",
                                     .extension = "xml"};
  static constexpr FileType kCsvFile{
      .wildcard = "CSV and TXT files (*.csv;*.txt)|*.csv;*.txt",
      .extension = "csv"};
  static constexpr FileType kPngFile{.wildcard = "PNG files (*.png)|*.png",
                                     .extension = "png"};

  void OpenXml() {
    const std::string file_path = AskOpenPath("Open File", kXmlFile.wildcard);
    if (file_path.empty()) {
      return;
    }
    Report("Open File", document_.LoadXml(file_path));
  }

  // Speichern ohne bekannten Pfad fragt genau wie «Speichern unter».
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
    const std::string file_path = AskOpenPath("Import file", kCsvFile.wildcard);
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

  // Leerer Rückgabewert heisst: abgebrochen.
  [[nodiscard]] std::string AskOpenPath(const wxString& title,
                                        const wxString& wildcard) {
    wxFileDialog dialog(&frame_, title, wxEmptyString, wxEmptyString, wildcard,
                        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dialog.ShowModal() != wxID_OK) {
      return {};
    }
    return dialog.GetPath().ToStdString();
  }

  // wxGTK hängt die Endung des Filters nie an (wxWidgets #9917) — das holen wir
  // nach, samt der Überschreibfrage, die der Dialog nur zum getippten Namen
  // stellt.
  [[nodiscard]] std::string AskSavePath(const wxString& title,
                                        const FileType& type) {
    wxFileDialog dialog(&frame_, title, wxEmptyString, wxEmptyString,
                        type.wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_OK) {
      return {};
    }
    wxFileName file_path(dialog.GetPath());
    if (file_path.HasExt()) {
      return file_path.GetFullPath().ToStdString();
    }
    file_path.SetExt(type.extension);
    if (file_path.FileExists() && !ConfirmOverwrite(title, file_path)) {
      return {};
    }
    return file_path.GetFullPath().ToStdString();
  }

  [[nodiscard]] bool ConfirmOverwrite(const wxString& title,
                                      const wxFileName& file_path) const {
    const wxString question = file_path.GetFullPath() +
                              " already exists.\nDo you want to replace it?";
    return wxMessageBox(question, title, wxYES_NO | wxICON_WARNING, &frame_) ==
           wxYES;
  }

  void Report(const wxString& title,
              const std::optional<std::string>& error) const {
    if (error) {
      wxMessageBox(*error, title, wxOK | wxICON_ERROR, &frame_);
    }
  }

  MainFrame& frame_;
  application::ProjectDocument& document_;
};

#endif  // FILE_COMMANDS_HPP
