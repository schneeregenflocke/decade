#ifndef FILE_COMMANDS_HPP
#define FILE_COMMANDS_HPP

#include <wx/filedlg.h>
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
  static constexpr const char* kXmlWildcard = "XML Files (*.xml)|*.xml";
  static constexpr const char* kCsvWildcard =
      "CSV and TXT files (*.csv;*.txt)|*.csv;*.txt";
  static constexpr const char* kPngWildcard = "PNG files (*.png)|*.png";

  void OpenXml() {
    const std::string file_path = AskOpenPath("Open File", kXmlWildcard);
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
    const std::string file_path = AskSavePath("Save File", kXmlWildcard);
    if (file_path.empty()) {
      return;
    }
    Report("Save File", document_.SaveXml(file_path));
  }

  void ImportCsv() {
    const std::string file_path = AskOpenPath("Import file", kCsvWildcard);
    if (file_path.empty()) {
      return;
    }
    document_.ImportCsv(file_path);
  }

  void ExportCsv() {
    const std::string file_path = AskSavePath("Export file", kCsvWildcard);
    if (file_path.empty()) {
      return;
    }
    Report("Export file", document_.ExportCsv(file_path));
  }

  void ExportPng() {
    const std::string file_path = AskSavePath("Export PNG file", kPngWildcard);
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

  [[nodiscard]] std::string AskSavePath(const wxString& title,
                                        const wxString& wildcard) {
    wxFileDialog dialog(&frame_, title, wxEmptyString, wxEmptyString, wildcard,
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_OK) {
      return {};
    }
    return dialog.GetPath().ToStdString();
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
