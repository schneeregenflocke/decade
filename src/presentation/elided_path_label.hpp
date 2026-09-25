#ifndef ELIDED_PATH_LABEL_HPP
#define ELIDED_PATH_LABEL_HPP

#include <QtCore/QString>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

// A label that shortens a long path at the front, so the file name stays
// visible. Qt shortens nothing by itself, and without this the path would
// dictate the column width.
class ElidedPathLabel : public QLabel {
 public:
  explicit ElidedPathLabel(QWidget* parent);

  void SetFullText(const QString& text);

 protected:
  void resizeEvent(QResizeEvent* event) override;

 private:
  void RefreshElided();

  QString full_text_;
};
#endif  // ELIDED_PATH_LABEL_HPP
