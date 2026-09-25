#ifndef COLOR_BUTTON_DELEGATE_HPP
#define COLOR_BUTTON_DELEGATE_HPP

#include <QtCore/QAbstractItemModel>
#include <QtCore/QEvent>
#include <QtCore/QModelIndex>
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOptionViewItem>
#include <QtWidgets/QStyledItemDelegate>

// Draws a cell as a push button carrying a colour swatch and reports a click
// on it. The colour comes from the cell's Qt::UserRole; what a click leads to
// is the owner's business, so the delegate opens no dialogue itself.
//
// A delegate rather than a widget per cell: Qt reserves setIndexWidget for
// static content and names QStyledItemDelegate for anything interactive
// (https://doc.qt.io/qt-6/qabstractitemview.html#setIndexWidget).
class ColorButtonDelegate : public QStyledItemDelegate {
  Q_OBJECT

 public:
  explicit ColorButtonDelegate(QObject* parent);

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;

  // As wide as the swatch plus the button's padding, so the column stays
  // compact under a resize-to-contents header.
  [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem& option,
                               const QModelIndex& index) const override;

 signals:
  void Clicked(const QModelIndex& index);

 protected:
  bool editorEvent(QEvent* event, QAbstractItemModel* model,
                   const QStyleOptionViewItem& option,
                   const QModelIndex& index) override;

 private:
  static constexpr int kButtonMarginPx = 2;
  static constexpr int kSwatchWidthPx = 32;
  static constexpr int kSwatchHeightPx = 12;
  static constexpr int kButtonPaddingPx = 12;
};
#endif  // COLOR_BUTTON_DELEGATE_HPP
