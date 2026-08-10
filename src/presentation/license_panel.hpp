#ifndef LICENSE_PANEL_HPP
#define LICENSE_PANEL_HPP

#include <QtCore/QPointer>
#include <QtWidgets/QDialog>
#include <string>
#include <utility>
#include <vector>

class QListWidget;
class QPlainTextEdit;
class QWidget;

class LicenseInformationDialog : public QDialog {
 public:
  explicit LicenseInformationDialog(QWidget* parent);

 private:
  static constexpr int kDefaultWidth = 800;
  static constexpr int kDefaultHeight = 600;
  static constexpr int kBorderPx = 10;

  void CollectLicenses();

  // The row indexes the same vector the list was filled from, so no name
  // lookup is needed to find the text.
  void SelectLicenseRow(int row);

  QPointer<QListWidget> license_select_list_;
  QPointer<QPlainTextEdit> text_view_;

  std::vector<std::pair<std::string, std::string>> collected_licenses_;
};
#endif  // LICENSE_PANEL_HPP
