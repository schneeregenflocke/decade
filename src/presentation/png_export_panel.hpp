#ifndef PNG_EXPORT_PANEL_HPP
#define PNG_EXPORT_PANEL_HPP

#include <QtCore/QPointer>
#include <QtCore/QTime>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <array>
#include <string>

#include "../domain/page_setup_config.hpp"
#include "../domain/png_export_config.hpp"

// The PNG export tab: target, resolution and multisampling, what the page
// comes out at in pixels, and the button that exports. The export itself runs
// in FileCommands, so the menu and the button share one path.
class PngExportPanel : public QWidget {
  Q_OBJECT

 public:
  explicit PngExportPanel(QWidget* parent);

  void ReceivePngExportConfig(const PngExportConfig& config);

  void ReceivePageSetup(const PageSetupConfig& page_setup_config);

  void ReceiveProjectFilePath(const std::string& file_path);

  // The button stays locked until the export ends, so a second one cannot
  // start beside it.
  void ShowExportStarted();

  void ShowProgress(int percent);

  void ShowSaved(const std::string& file_path, const QTime& time);

  void ShowExportFailed();

  void ClearSaved();

 signals:
  void PngExportConfigEdited(const PngExportConfig& config);

  void ExportRequested();

  void BrowseRequested();

 private:
  void CommitDpi();

  void CommitMsaaSamples();

  void CommitFilePath();

  void UpdateDimensions();

  PngExportConfig config_;
  std::array<float, 2> page_size_mm_{};
  std::string project_file_path_;

  QPointer<QLineEdit> file_path_;
  QPointer<QComboBox> dpi_;
  QPointer<QComboBox> msaa_samples_;
  QPointer<QLabel> pixel_size_;
  QPointer<QLabel> millimetre_size_;
  QPointer<QLabel> effective_dpi_;
  QPointer<QPushButton> export_button_;
  QPointer<QProgressBar> progress_;
  QPointer<QLabel> saved_;
};
#endif  // PNG_EXPORT_PANEL_HPP
