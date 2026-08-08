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
  explicit DocumentSetupPanel(QWidget* parent) : QWidget(parent) {
    constexpr int kBorderPx = 5;

    auto* page_setup_panel = MakeOwned<PageSetupPanel>(this);
    page_setup_panel_ = page_setup_panel;
    auto* font_panel = MakeOwned<FontPanel>(this);
    font_panel_ = font_panel;
    auto* title_setup_panel = MakeOwned<TitleSetupPanel>(this);
    title_setup_panel_ = title_setup_panel;

    auto* vertical_layout = MakeOwned<QVBoxLayout>();
    vertical_layout->setContentsMargins(kBorderPx, kBorderPx, kBorderPx,
                                        kBorderPx);
    vertical_layout->addWidget(WrapInGroup("File", CreateFilePathRow()));
    vertical_layout->addWidget(WrapInGroup("Page", page_setup_panel));
    vertical_layout->addWidget(WrapInGroup("Font", font_panel));
    vertical_layout->addWidget(WrapInGroup("Title", title_setup_panel));
    vertical_layout->addStretch(1);
    setLayout(vertical_layout);
  }

  void ReceiveProjectFilePath(const std::string& file_path) {
    file_path_ = file_path;
    const bool has_path = !file_path_.empty();
    const QString shown = has_path ? QString::fromStdString(file_path_)
                                   : QString("unsaved project");
    file_path_label_->SetFullText(shown);
    file_path_label_->setToolTip(has_path ? shown : QString());
    copy_button_->setEnabled(has_path);
  }

  [[nodiscard]] PageSetupPanel* GetPageSetupPanel() const {
    return page_setup_panel_;
  }
  [[nodiscard]] FontPanel* GetFontPanel() const { return font_panel_; }
  [[nodiscard]] TitleSetupPanel* GetTitleSetupPanel() const {
    return title_setup_panel_;
  }

 private:
  // A label instead of an input field: the path never gets typed. Copying runs
  // over the button beside it.
  QLayout* CreateFilePathRow() {
    auto* label = MakeOwned<ElidedPathLabel>(this);
    label->SetFullText("unsaved project");
    file_path_label_ = label;

    auto* copy_button = MakeOwned<QPushButton>("Copy", this);
    copy_button->setEnabled(false);
    connect(copy_button, &QPushButton::clicked, this,
            [this]() { CopyFilePathToClipboard(); });
    copy_button_ = copy_button;

    auto* row_layout = MakeOwned<QHBoxLayout>();
    row_layout->addWidget(label, 1);
    row_layout->addWidget(copy_button);
    return row_layout;
  }

  void CopyFilePathToClipboard() const {
    if (file_path_.empty()) {
      return;
    }
    QGuiApplication::clipboard()->setText(QString::fromStdString(file_path_));
  }

  QGroupBox* WrapInGroup(const QString& label, QWidget* panel) {
    auto* group = MakeOwned<QGroupBox>(label, this);
    auto* group_layout = MakeOwned<QVBoxLayout>();
    group_layout->addWidget(panel);
    group->setLayout(group_layout);
    return group;
  }

  QGroupBox* WrapInGroup(const QString& label, QLayout* content) {
    auto* group = MakeOwned<QGroupBox>(label, this);
    group->setLayout(content);
    return group;
  }

  std::string file_path_;
  QPointer<ElidedPathLabel> file_path_label_;
  QPointer<QPushButton> copy_button_;
  QPointer<PageSetupPanel> page_setup_panel_;
  QPointer<FontPanel> font_panel_;
  QPointer<TitleSetupPanel> title_setup_panel_;
};
#endif  // DOCUMENT_PANEL_HPP
