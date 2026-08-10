#ifndef DOCUMENT_PANEL_HPP
#define DOCUMENT_PANEL_HPP

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <string>

#include "font_panel.hpp"
#include "make_owned.hpp"
#include "page_panel.hpp"
#include "title_panel.hpp"

// A label that shortens a long path at the front, so the file name stays
// visible. Qt shortens nothing by itself, and without this the path would
// dictate the column width — the reason wx got wxST_ELLIPSIZE_START here.
class ElidedPathLabel : public QLabel {
 public:
  explicit ElidedPathLabel(QWidget* parent) : QLabel(parent) {
    setMinimumWidth(1);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  }

  void SetFullText(const QString& text) {
    full_text_ = text;
    RefreshElided();
  }

 protected:
  void resizeEvent(QResizeEvent* event) override {
    QLabel::resizeEvent(event);
    RefreshElided();
  }

 private:
  void RefreshElided() {
    setText(fontMetrics().elidedText(full_text_, Qt::ElideLeft, width()));
  }

  QString full_text_;
};

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
