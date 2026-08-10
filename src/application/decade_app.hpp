#ifndef DECADE_APP_HPP
#define DECADE_APP_HPP

namespace application {

// Builds the application in the order Qt prescribes and runs its event loop.
// The surface format has to stand before the QApplication: Qt honours a
// per-widget format on some platforms alone, and the canvas would otherwise
// quietly get a compatibility context.
// https://doc.qt.io/qt-6/qopenglwidget.html#details
int RunDecadeApp(int& argument_count, char** arguments);

}  // namespace application

#endif  // DECADE_APP_HPP
