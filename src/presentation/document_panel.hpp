#ifndef DOCUMENT_PANEL_HPP
#define DOCUMENT_PANEL_HPP

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <string>

#include "elided_path_label.hpp"
#include "font_panel.hpp"
#include "make_owned.hpp"
#include "page_panel.hpp"
#include "title_panel.hpp"

// Presentation: composite tab that groups the page-format, font and title
// settings — all of which configure the overall rendered document — into a
// single notebook page. It owns the three child panels and exposes them so the
// binder can wire each one to its store exactly as before; the child panels and
// their signals are unchanged.
class DocumentSetupPanel : public QWidget {
 public:
  explicit DocumentSetupPanel(QWidget* parent);

  void ReceiveProjectFilePath(const std::string& file_path);

  [[nodiscard]] PageSetupPanel* GetPageSetupPanel() const;
  [[nodiscard]] FontPanel* GetFontPanel() const;
  [[nodiscard]] TitleSetupPanel* GetTitleSetupPanel() const;

 private:
  // A label instead of an input field: the path never gets typed. Copying runs
  // over the button beside it.
  QLayout* CreateFilePathRow();

  void CopyFilePathToClipboard() const;

  QGroupBox* WrapInGroup(const QString& label, QWidget* panel);

  QGroupBox* WrapInGroup(const QString& label, QLayout* content);

  std::string file_path_;
  QPointer<ElidedPathLabel> file_path_label_;
  QPointer<QPushButton> copy_button_;
  QPointer<PageSetupPanel> page_setup_panel_;
  QPointer<FontPanel> font_panel_;
  QPointer<TitleSetupPanel> title_setup_panel_;
};
#endif  // DOCUMENT_PANEL_HPP
