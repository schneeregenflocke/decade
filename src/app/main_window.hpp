#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <wx/event.h>
#include <wx/frame.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/timer.h>
#include <wx/window.h>

#include <memory>
#include <string>

class wxNotebook;

class MainWindow : public wxFrame {
 public:
  MainWindow(wxWindow* parent, const wxString& title, const wxPoint& pos,
             const wxSize& size, bool maximize_on_start = true);
  ~MainWindow() override;
  MainWindow(const MainWindow&) = delete;
  MainWindow& operator=(const MainWindow&) = delete;
  MainWindow(MainWindow&&) = delete;
  MainWindow& operator=(MainWindow&&) = delete;

 private:
  void CreateLayout(bool maximize_on_start);
  void CreatePanels(wxNotebook* notebook);
  void InitializeOpenGL();
  void EstablishConnections();
  void LoadDefaultDates();
  void ConfigureAutoExitTimer();
  void DumpPngIfRequested();
  void InitMenu();

  void CallbackLoadXML(wxCommandEvent& event);
  void CallbackSaveXML(wxCommandEvent& event);
  void CallbackImportCSV(wxCommandEvent& event);
  void CallbackExportCSV(wxCommandEvent& event);
  void CallbackExportPNG(wxCommandEvent& event);
  void CallbackExit(wxCommandEvent& event);
  void OnExitTimer(wxTimerEvent& event);
  void CallbackLicenseInfo(wxCommandEvent& event);

  void LoadXML(const std::string& filepath);
  void SaveXML(const std::string& filepath);

  struct Impl;
  std::unique_ptr<Impl> impl_;

  std::string xml_file_path;
  wxTimer exit_timer;

  const int id_save_xml;
  const int id_save_as_xml;
};

#endif  // MAIN_WINDOW_HPP
