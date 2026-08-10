#include "window_screenshot.hpp"

#include <QtCore/QPoint>
#include <QtCore/QRect>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtWidgets/QWidget>
#include <string>

namespace window_screenshot {

bool SaveWindowPng(QWidget& window, const Overlay& overlay,
                   const std::string& file_path) {
  if (window.width() <= 0 || window.height() <= 0) {
    return false;
  }

  const QPixmap grabbed = window.grab();
  if (grabbed.isNull()) {
    return false;
  }

  // The capture gets flattened onto an opaque ground instead of being saved as
  // it comes: the format of a QPixmap follows the window surface, and where
  // that carries an alpha channel (Wayland does, X11 here does not) parts of
  // the capture are translucent. Saving that directly writes a PNG whose
  // colours a viewer divides by an alpha that means nothing to a screenshot.
  // Drawing it over a filled image composites that alpha away — the one
  // operation that is correct for both kinds of surface — and everything below
  // works in one known, opaque format.
  QImage canvas(grabbed.size(), QImage::Format_RGB32);
  canvas.setDevicePixelRatio(grabbed.devicePixelRatio());
  canvas.fill(Qt::white);
  {
    QPainter painter(&canvas);
    painter.drawPixmap(QPoint(0, 0), grabbed);
  }

  if (!overlay.image.isNull()) {
    // Into the rectangle rather than at a point: the widget capture counts in
    // logical pixels, the GL read-back in device pixels, and drawing into a
    // rectangle scales between the two instead of leaving the mismatch to
    // chance on a HiDPI display.
    QPainter painter(&canvas);
    painter.drawImage(QRect(overlay.origin, overlay.size), overlay.image);
  }

  return canvas.save(QString::fromStdString(file_path), "PNG");
}
}  // namespace window_screenshot
