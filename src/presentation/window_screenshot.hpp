#ifndef WINDOW_SCREENSHOT_HPP
#define WINDOW_SCREENSHOT_HPP

#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtGui/QImage>
#include <string>

class QWidget;

namespace window_screenshot {

// An image laid over the widget capture, together with its place in the window.
// Needed for the GL surface: the widget capture does not draw it, so its
// content must be delivered separately and mounted in.
struct Overlay {
  QImage image;
  QPoint origin;
  QSize size;
};

// Renders the widget tree of a window into a pixmap, mounts the overlay onto it
// and writes the whole as a PNG. It reports success.
//
// QWidget::grab() renders the widgets rather than reading the screen, so this
// needs no X11 and works under Wayland too — the blit through a device context
// that wx used did not.
[[nodiscard]] bool SaveWindowPng(QWidget& window, const Overlay& overlay,
                                 const std::string& file_path);

}  // namespace window_screenshot

#endif  // WINDOW_SCREENSHOT_HPP
