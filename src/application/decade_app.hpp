#ifndef DECADE_APP_HPP
#define DECADE_APP_HPP

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtCore/QtMessageHandler>
#include <QtGui/QSurfaceFormat>
#include <QtWidgets/QApplication>
#include <exception>
#include <iostream>
#include <memory>

#include "../common/debug_log.hpp"
#include "../presentation/gl_canvas.hpp"
#include "app_composition.hpp"
#include "locale_services.hpp"
#include "runtime_info.hpp"
#include "runtime_options.hpp"

namespace application {

namespace decade_app_detail {

// Holds the two channels of operations.md: a run without --debug-log says
// nothing on stdout and puts on stderr only what the user has to act on. Qt
// speaks up through qDebug and qInfo about matters that are diagnosis, not
// news, so those stay silent unless the switch is on. A warning is never
// swallowed — it is the category that means "act".
inline void MessageHandler(QtMsgType type, const QMessageLogContext& /*unused*/,
                           const QString& message) {
  if ((type == QtDebugMsg || type == QtInfoMsg) &&
      !decade_debug::LogEnabled()) {
    return;
  }
  std::cerr << message.toStdString() << '\n';
}

}  // namespace decade_app_detail

// Builds the application in the order Qt prescribes and runs its event loop.
// The surface format has to stand before the QApplication: Qt honours a
// per-widget format on some platforms alone, and the canvas would otherwise
// quietly get a compatibility context.
// https://doc.qt.io/qt-6/qopenglwidget.html#details
inline int RunDecadeApp(int& argument_count, char** arguments) {
  QSurfaceFormat::setDefaultFormat(GLCanvas::SurfaceFormat());

  const QApplication app(argument_count, arguments);
  QCoreApplication::setApplicationName("decade");

  QCommandLineParser parser;
  parser.setApplicationDescription(
      "A calendar and timeline for periods across several years.");
  parser.addHelpOption();
  AddRuntimeOptions(parser);
  parser.process(app);

  const RuntimeOptions runtime_options = RuntimeOptionsFromParser(parser);
  decade_debug::SetLogEnabled(runtime_options.debug_log);
  qInstallMessageHandler(decade_app_detail::MessageHandler);

  std::unique_ptr<LocaleServices> locale_services;
  try {
    locale_services = std::make_unique<LocaleServices>();
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return 1;
  }

  if (decade_debug::LogEnabled()) {
    PrintRuntimeInfo(std::cout);
  }

  const AppComposition composition(locale_services->date_formatter(),
                                   runtime_options);
  return QApplication::exec();
}

}  // namespace application

#endif  // DECADE_APP_HPP
